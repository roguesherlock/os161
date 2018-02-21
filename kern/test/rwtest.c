/*
 * All the contents of this file are overwritten during automated
 * testing. Please consider this before changing anything in this file.
 */

#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <kern/test161.h>
#include <spinlock.h>

#define NLOOPS 40
#define CREATELOOPS 8
#define NREADERTHREADS 50
#define NWRITERTHREADS 50

struct spinlock status_lock;
static bool test_status = TEST161_FAIL;
static volatile unsigned long testval1;
static volatile unsigned long testval2;
static volatile unsigned long testval3;
static struct semaphore *donesem;
static struct rwlock *rwtestlock;

static
bool
failif(bool condition) {
	if (condition) {
		spinlock_acquire(&status_lock);
		test_status = TEST161_FAIL;
		spinlock_release(&status_lock);
	}
	return condition;
}

static
void
readertestthread(void *junk1, unsigned long junk2)
{
	(void)junk1;
	(void)junk2;

	unsigned i;

	random_yielder(4);

	rwlock_acquire_read(rwtestlock);
	for (i=0; i<NLOOPS; i++) {
		// rwlock_acquire_read(rwtestlock);
		kprintf_t(".");

		failif(testval2 != testval1*testval1);
		random_yielder(4);

		failif(testval2%3 != (testval3*testval3)%3);
		random_yielder(4);

		failif(testval3 != testval1%3);
		random_yielder(4);

		// rwlock_release_read(rwtestlock);
	}

	rwlock_release_read(rwtestlock);
	V(donesem);
	return;
}

static
void
writertestthread(void *junk1, unsigned long num)
{
	(void)junk1;

	unsigned i;

	random_yielder(4);

	rwlock_acquire_write(rwtestlock);
	kprintf_t("writer in: %u\n", num);
	for (i=0; i<NLOOPS; i++) {
		// rwlock_acquire_write(rwtestlock);
		kprintf_t(".");
		testval1 = num;
		testval2 = num*num;
		testval3 = num%3;

		random_yielder(4);
		failif(rwtestlock->readers_in != 0);

		random_yielder(4);
		failif(testval1 != num);

		testval1 = num*num;
		random_yielder(4);
		failif(testval1 != testval2);

		testval1 = num%3;
		random_yielder(4);
		failif(testval1 != testval3);

		testval1 = num;
		random_yielder(4);
		failif(testval1 != num);

		random_yielder(4);
		failif(rwtestlock->readers_in != 0);
		// rwlock_release_write(rwtestlock);
	}

	rwlock_release_write(rwtestlock);
	kprintf_t("writer out: %u\n", num);
	V(donesem);
}


int
rwtest(int nargs, char **args)
{
	(void)nargs;
	(void)args;

	int i, result;
	unsigned long num = 9;

	testval1 = num;
	testval2 = num*num;
	testval3 = num%3;

	kprintf_n("Starting rwt1...\n");
	for (i=0; i<CREATELOOPS; i++) {
		kprintf_t(".");
		rwtestlock = rwlock_create("rwtestlock");
		if (rwtestlock == NULL) {
			panic("rwt1: rwlock_create failed\n");
		}
		donesem = sem_create("donesem", 0);
		if (donesem == NULL) {
			panic("rwt1: sem_create failed\n");
		}
		if (i != CREATELOOPS - 1) {
			rwlock_destroy(rwtestlock);
			sem_destroy(donesem);
		}
	}
	spinlock_init(&status_lock);
	test_status = TEST161_SUCCESS;

	for (i=0; i<NREADERTHREADS; ++i) {
		kprintf_t(".");
		result = thread_fork("reader", NULL, readertestthread, NULL, i);
		if (result)
			panic("rwt1: thread_fork failed: %s\n", strerror(result));
	}
	for (i=0; i<NWRITERTHREADS; ++i) {
		kprintf_t(".");
		result = thread_fork("writer", NULL, writertestthread, NULL, i);
		if (result)
			panic("rwt1: thread_fork failed: %s\n", strerror(result));
	}

	for (i=0; i<(NREADERTHREADS + NWRITERTHREADS); ++i) {
		kprintf_t(".");
		P(donesem);
	}

	rwlock_destroy(rwtestlock);
	rwtestlock = NULL;
	sem_destroy(donesem);
	donesem = NULL;

	kprintf_t("\n");
	success(test_status, SECRET, "rwt1");

	return 0;
}

int rwtest2(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt2 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt2");

	return 0;
}

int rwtest3(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt3 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt3");

	return 0;
}

int rwtest4(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt4 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt4");

	return 0;
}

int rwtest5(int nargs, char **args) {
	(void)nargs;
	(void)args;

	kprintf_n("rwt5 unimplemented\n");
	success(TEST161_FAIL, SECRET, "rwt5");

	return 0;
}
