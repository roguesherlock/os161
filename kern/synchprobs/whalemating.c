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
 * Driver code is in kern/tests/synchprobs.c We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

static unsigned long males_available;
static unsigned long females_available;
static unsigned long matchmakers_available;
static struct lock *lk_male;
static struct lock *lk_female;
static struct lock *lk_matchmaker;
static struct cv *cv_male;
static struct cv *cv_female;
static struct cv *cv_matchmaker;

/*
 * Called by the driver during initialization.
 */

void whalemating_init() {
	males_available = 0;
	females_available = 0;
	matchmakers_available = 0;
	lk_male = lock_create("lk_male");
	lk_female = lock_create("lk_female");
	lk_matchmaker = lock_create("lk_matchmaker");
	cv_male = cv_create("cv_male");
	cv_female = cv_create("cv_female");
	cv_matchmaker = cv_create("cv_matchmaker");
}

/*
 * Called by the driver during teardown.
 */

void
whalemating_cleanup() {
	KASSERT(lk_male != NULL);
	KASSERT(lk_female != NULL);
	KASSERT(lk_matchmaker != NULL);
	KASSERT(cv_male != NULL);
	KASSERT(cv_female != NULL);
	KASSERT(cv_matchmaker != NULL);

	lock_destroy(lk_male);
	lock_destroy(lk_female);
	lock_destroy(lk_matchmaker);
	cv_destroy(cv_male);
	cv_destroy(cv_female);
	cv_destroy(cv_matchmaker);

	lk_male = lk_female = lk_matchmaker = NULL;
	cv_male = cv_female = cv_matchmaker = NULL;
}

void
male(uint32_t index)
{
	KASSERT(lk_male != NULL);
	KASSERT(lk_female != NULL);
	KASSERT(lk_matchmaker != NULL);
	KASSERT(cv_male != NULL);
	KASSERT(cv_female != NULL);
	KASSERT(cv_matchmaker != NULL);

	male_start(index);
	lock_acquire(lk_male);

	lock_acquire(lk_female);
	lock_acquire(lk_matchmaker);
	unsigned long female_status = females_available;
	unsigned long matchmaker_status = matchmakers_available;
	cv_signal(cv_matchmaker, lk_matchmaker);
	lock_release(lk_female);
	lock_release(lk_matchmaker);

	++males_available;
	if (female_status == 0 || matchmaker_status == 0) {
		cv_wait(cv_male, lk_male);
	}

	lock_release(lk_male);
	male_end(index);
}

void
female(uint32_t index)
{
	KASSERT(lk_male != NULL);
	KASSERT(lk_female != NULL);
	KASSERT(lk_matchmaker != NULL);
	KASSERT(cv_male != NULL);
	KASSERT(cv_female != NULL);
	KASSERT(cv_matchmaker != NULL);

	female_start(index);
	lock_acquire(lk_female);

	lock_acquire(lk_male);
	lock_acquire(lk_matchmaker);
	unsigned long male_status = males_available;
	unsigned long matchmaker_status = matchmakers_available;
	cv_signal(cv_matchmaker, lk_matchmaker);
	lock_release(lk_male);
	lock_release(lk_matchmaker);

	++females_available;
	if (male_status == 0 || matchmaker_status == 0) {
		cv_wait(cv_female, lk_female);
	}

	lock_release(lk_female);
	female_end(index);
}

void
matchmaker(uint32_t index)
{
	KASSERT(lk_male != NULL);
	KASSERT(lk_female != NULL);
	KASSERT(lk_matchmaker != NULL);
	KASSERT(cv_male != NULL);
	KASSERT(cv_female != NULL);
	KASSERT(cv_matchmaker != NULL);

	matchmaker_start(index);

	lock_acquire(lk_male);
	lock_acquire(lk_female);
	lock_acquire(lk_matchmaker);
	++matchmakers_available;
	while (males_available == 0 || females_available == 0) {
		lock_release(lk_male);
		lock_release(lk_female);
		cv_wait(cv_matchmaker, lk_matchmaker);
		lock_acquire(lk_male);
		lock_acquire(lk_female);
	}


	--matchmakers_available;
	--males_available;
	--females_available;
	cv_signal(cv_male, lk_male);
	cv_signal(cv_female, lk_female);
	lock_release(lk_male);
	lock_release(lk_female);
	lock_release(lk_matchmaker);
	matchmaker_end(index);

	lock_acquire(lk_matchmaker);
	cv_broadcast(cv_matchmaker, lk_matchmaker);
	lock_release(lk_matchmaker);
}
