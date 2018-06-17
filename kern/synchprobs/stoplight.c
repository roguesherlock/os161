/*
 * Copyright (c) 2001, 2002, 2009
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
 * Driver code is in kern/tests/synchprobs.c We will replace that file. This
 * file is yours to modify as you see fit.
 *
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is, of
 * course, stable under rotation)
 *
 *   |0 |
 * -     --
 *    01  1
 * 3  32
 * --    --
 *   | 2|
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X first.
 * The semantics of the problem are that once a car enters any quadrant it has
 * to be somewhere in the intersection until it call leaveIntersection(),
 * which it should call while in the final quadrant.
 *
 * As an example, let's say a car approaches the intersection and needs to
 * pass through quadrants 0, 3 and 2. Once you call inQuadrant(0), the car is
 * considered in quadrant 0 until you call inQuadrant(3). After you call
 * inQuadrant(2), the car is considered in quadrant 2 until you call
 * leaveIntersection().
 *
 * You will probably want to write some helper functions to assist with the
 * mappings. Modular arithmetic can help, e.g. a car passing straight through
 * the intersection entering from direction X will leave to direction (X + 2)
 * % 4 and pass through quadrants X and (X + 3) % 4.  Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in synchprobs.c to record their progress.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

#define NQUADRANTS 4
static bool is_available[NQUADRANTS];
static struct semaphore *quadrantsem[NQUADRANTS];
static struct spinlock quadrantspn;
static struct cv *wftcv;
static struct lock *wftlk;

static bool check_and_acquire(int [], size_t);
static void make_available(int);

/*
 * Called by the driver during initialization.
 */

void
stoplight_init() {
	int i;

	for (i=0; i<NQUADRANTS; ++i)  {
		is_available[i] = true;
		quadrantsem[i] = sem_create("quadrantsem", 1);
		KASSERT(quadrantsem[i] != NULL);
	}

	wftcv = cv_create("wftcv");
	wftlk = lock_create("wftcv");

	KASSERT(wftcv != NULL);
	KASSERT(wftlk != NULL);

	spinlock_init(&quadrantspn);
}

/*
 * Called by the driver during teardown.
 */

void stoplight_cleanup() {
	int i;

	for (i=0; i<NQUADRANTS; ++i)  {
		KASSERT(quadrantsem[i] != NULL);
		sem_destroy(quadrantsem[i]);
		quadrantsem[i] = NULL;
	}

	KASSERT(wftcv != NULL);
	KASSERT(wftlk != NULL);

	cv_destroy(wftcv);
	lock_destroy(wftlk);

	wftcv = NULL;
	wftlk = NULL;

	spinlock_cleanup(&quadrantspn);
}

void
turnright(uint32_t direction, uint32_t index)
{
	int cq[1];

	cq[0] = (int) direction;

	while (!check_and_acquire(cq, 1)) {
		lock_acquire(wftlk);
		cv_wait(wftcv, wftlk);
		lock_release(wftlk);
	}

	inQuadrant(cq[0], index);
	leaveIntersection(index);

	make_available(cq[0]);
}

void
gostraight(uint32_t direction, uint32_t index)
{
	int passqs[2];

	passqs[0] = (int) direction;
	passqs[1] = (passqs[0] + 3) % 4;

	while (!check_and_acquire(passqs, 2)) {
		lock_acquire(wftlk);
		cv_wait(wftcv, wftlk);
		lock_release(wftlk);
	}

	inQuadrant(passqs[0], index);
	inQuadrant(passqs[1], index);
	make_available(passqs[0]);
	leaveIntersection(index);
	make_available(passqs[1]);
}

void
turnleft(uint32_t direction, uint32_t index)
{
	int i, cq, passqs[3];

	passqs[0] = (int) direction;
	passqs[1] = (passqs[0] + 3) % 4;
	passqs[2] = (passqs[0] + 2) % 4;

	while (!check_and_acquire(passqs, 3)) {
		lock_acquire(wftlk);
		cv_wait(wftcv, wftlk);
		lock_release(wftlk);
	}

	for (i=0; i<3; ++i) {
		cq = passqs[i];

		inQuadrant(cq, index);
		if (i == 2)
			leaveIntersection(index);
		if (i > 0)
			make_available(passqs[i-1]);
	}
	make_available(cq);
}

bool
check_and_acquire(int qs[], size_t s)
{
	bool all_available = true;
	int i, cq, n;

	n = (int) s;
	spinlock_acquire(&quadrantspn);
	for (i=0; i<n; ++i) {
		cq = qs[i];
		if (!is_available[cq])
			all_available = false;
	}

	if (all_available) {
		for (i=0; i<n; ++i) {
			cq = qs[i];
			is_available[cq] = false;
			P(quadrantsem[cq]);
		}
	}
	spinlock_release(&quadrantspn);

	return all_available;
}

void
make_available (int q)
{
	spinlock_acquire(&quadrantspn);
	V(quadrantsem[q]);
	is_available[q] = true;
	spinlock_release(&quadrantspn);

	lock_acquire(wftlk);
	cv_signal(wftcv, wftlk);
	lock_release(wftlk);
}