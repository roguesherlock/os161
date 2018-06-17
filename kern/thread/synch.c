/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(*sem));
	if (sem == NULL) {
		return NULL;
	}

	sem->sem_name = kstrdup(name);
	if (sem->sem_name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
	sem->sem_count = initial_count;

	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
	kfree(sem->sem_name);
	kfree(sem);
}

void
P(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
	while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(sem->sem_wchan, &sem->sem_lock);
	}
	KASSERT(sem->sem_count > 0);
	sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

	sem->sem_count++;
	KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(*lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->lk_name = kstrdup(name);
	if (lock->lk_name == NULL) {
		kfree(lock);
		return NULL;
	}

	HANGMAN_LOCKABLEINIT(&lock->lk_hangman, lock->lk_name);

	lock->lk_wchan = wchan_create(lock->lk_name);
	if (lock->lk_wchan == NULL) {
		kfree(lock->lk_name);
		kfree(lock);
		return NULL;
	}

	spinlock_init(&lock->splk_lock);
	lock->lk_lock = 0;
	lock->lk_holder = NULL;
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	KASSERT(lock != NULL);
	// make sure no one's holding it
	KASSERT(lock->lk_lock == 0);
	KASSERT(lock->lk_holder == NULL);

	spinlock_cleanup(&lock->splk_lock);
	wchan_destroy(lock->lk_wchan);
	kfree(lock->lk_name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	KASSERT(lock != NULL);
	KASSERT(curthread->t_in_interrupt == false);

	spinlock_acquire(&lock->splk_lock);
	if (lock->lk_holder == curthread) {
		panic("Deadlock on lock %p\n", lock);
	}
	/* Call this (atomically) before waiting for a lock */
	HANGMAN_WAIT(&curthread->t_hangman, &lock->lk_hangman);

	while (lock->lk_lock == 1) {
		wchan_sleep(lock->lk_wchan, &lock->splk_lock);
	}
	KASSERT(lock->lk_lock == 0);
	lock->lk_lock++;
	lock->lk_holder = curthread;

	/* Call this (atomically) once the lock is acquired */
	HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);
	spinlock_release(&lock->splk_lock);
}

void
lock_release(struct lock *lock)
{
	KASSERT(lock != NULL);
	KASSERT(lock->lk_holder == curthread);

	spinlock_acquire(&lock->splk_lock);

	lock->lk_lock--;
	KASSERT(lock->lk_lock == 0);

	lock->lk_holder = NULL;

	wchan_wakeone(lock->lk_wchan, &lock->splk_lock);

	/* Call this (atomically) when the lock is released */
	HANGMAN_RELEASE(&curthread->t_hangman, &lock->lk_hangman);

	spinlock_release(&lock->splk_lock);
}

bool
lock_do_i_hold(struct lock *lock)
{
	KASSERT(lock != NULL);

	return (lock->lk_holder == curthread);
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(*cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->cv_name = kstrdup(name);
	if (cv->cv_name==NULL) {
		kfree(cv);
		return NULL;
	}

	cv->cv_wchan = wchan_create(cv->cv_name);
	if (cv->cv_wchan == NULL) {
		kfree(cv->cv_name);
		kfree(cv);
		return NULL;
	}

	spinlock_init(&cv->splk_lock);

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	KASSERT(cv != NULL);

	wchan_destroy(cv->cv_wchan);
	spinlock_cleanup(&cv->splk_lock);
	kfree(cv->cv_name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
	KASSERT(lock_do_i_hold(lock));
	spinlock_acquire(&cv->splk_lock);
	lock_release(lock);

	wchan_sleep(cv->cv_wchan, &cv->splk_lock);

	spinlock_release(&cv->splk_lock);
	lock_acquire(lock);
	KASSERT(lock_do_i_hold(lock));
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
	KASSERT(lock_do_i_hold(lock));
	spinlock_acquire(&cv->splk_lock);

	wchan_wakeone(cv->cv_wchan, &cv->splk_lock);

	spinlock_release(&cv->splk_lock);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	KASSERT(cv != NULL);
	KASSERT(lock != NULL);
	KASSERT(lock_do_i_hold(lock));
	spinlock_acquire(&cv->splk_lock);

	wchan_wakeall(cv->cv_wchan, &cv->splk_lock);

	spinlock_release(&cv->splk_lock);
}


////////////////////////////////////////////////////////////
//
// Reader Writer Locks


struct rwlock *
rwlock_create(const char *name)
{
	KASSERT(name != NULL);

	struct rwlock *rwlock;
	rwlock = kmalloc(sizeof(*rwlock));
	if (rwlock == NULL)
		return NULL;

	rwlock->name = kstrdup(name);
	if (rwlock->name == NULL) {
		kfree(rwlock);
		return NULL;
	}

	rwlock->reader_wchan = wchan_create(rwlock->name);
	if (rwlock->reader_wchan == NULL) {
		kfree(rwlock->name);
		kfree(rwlock);
		return NULL;
	}

	rwlock->writer_wchan = wchan_create(rwlock->name);
	if (rwlock->writer_wchan == NULL) {
		wchan_destroy(rwlock->reader_wchan);
		kfree(rwlock->name);
		kfree(rwlock);
		return NULL;
	}

	spinlock_init(&rwlock->lock);

	rwlock->reader_count = 0;
	rwlock->readers_in = 0;
	rwlock->writer_in = false;
	rwlock->writer_turn = false;

	return rwlock;
}

void
rwlock_destroy(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);

	wchan_destroy(rwlock->writer_wchan);
	wchan_destroy(rwlock->reader_wchan);
	spinlock_cleanup(&rwlock->lock);
	kfree(rwlock->name);
	kfree(rwlock);
}

void
rwlock_acquire_read(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);

	spinlock_acquire(&rwlock->lock);

	if (rwlock->writer_in ||
		(!wchan_isempty(rwlock->reader_wchan, &rwlock->lock)) ||
		(rwlock->writer_turn && (!wchan_isempty(rwlock->writer_wchan, &rwlock->lock))))
		wchan_sleep(rwlock->reader_wchan, &rwlock->lock);

	rwlock->reader_count++;
	rwlock->readers_in++;
	if ((rwlock->reader_count % MAX_COUNT_RWLOCK) == 0)
		rwlock->writer_turn = true;

	spinlock_release(&rwlock->lock);
}

void
rwlock_release_read(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	KASSERT(!rwlock->writer_in);
	KASSERT(rwlock->readers_in > 0);

	spinlock_acquire(&rwlock->lock);

	rwlock->readers_in--;
	if (!rwlock->writer_turn && !wchan_isempty(rwlock->reader_wchan, &rwlock->lock))
		wchan_wakeone(rwlock->reader_wchan, &rwlock->lock);

	else if ((rwlock->readers_in == 0)
		 && !wchan_isempty(rwlock->writer_wchan, &rwlock->lock)) {
		wchan_wakeone(rwlock->writer_wchan, &rwlock->lock);
		rwlock->writer_in = true;
	}

	spinlock_release(&rwlock->lock);
}

void
rwlock_acquire_write(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);

	spinlock_acquire(&rwlock->lock);

	if (rwlock->writer_in ||
		rwlock->readers_in != 0 ||
		(!rwlock->writer_turn && !wchan_isempty(rwlock->reader_wchan, &rwlock->lock)))
		wchan_sleep(rwlock->writer_wchan, &rwlock->lock);
	rwlock->writer_in = true;

	spinlock_release(&rwlock->lock);
}

void
rwlock_release_write(struct rwlock *rwlock)
{
	KASSERT(rwlock != NULL);
	KASSERT(rwlock->writer_in);

	spinlock_acquire(&rwlock->lock);

	rwlock->writer_in = false;
	rwlock->writer_turn = false;

	if (!wchan_isempty(rwlock->reader_wchan, &rwlock->lock))
		wchan_wakeone(rwlock->reader_wchan, &rwlock->lock);

	else if (!wchan_isempty(rwlock->writer_wchan, &rwlock->lock)) {
		wchan_wakeone(rwlock->writer_wchan, &rwlock->lock);
		rwlock->writer_in = true;
	}

	spinlock_release(&rwlock->lock);
}