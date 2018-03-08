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
 * writetest.c
 *
 * 	Tests whether open and write syscalls works
 * 	This should run correctly when open and write are
 * 	implemented correctly.
 *
 * NOTE: While checking, this test only checks the first 31 characters.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <test161/test161.h>

#define FILENAME "writetest.dat"

static const char *MAGIC = "h4sd0rRq0Vgbc96tiYJ^!#nXzZSAKPO";


int
main(int argc, char **argv)
{

	// 23 Mar 2012 : GWA : Assume argument passing is *not* supported.

	(void) argc;
	(void) argv;

	int fd, len;
	int expected_len = strlen(MAGIC);

	fd = open(FILENAME, O_WRONLY | O_CREAT | O_TRUNC);
	if(fd < 0) {
		err(1, "Failed to open file.\n");
	}
	nprintf(".");

	len = write(fd, MAGIC, expected_len);
	if(len != expected_len) {
		err(1, "writetest expected to write %d bytes to writetest.dat."
			" Syscall reports that it wrote %d bytes.\n"
			"Is your write syscall returning the right value?\n",
			expected_len, len);
	}

	nprintf("\n");

	success(TEST161_SUCCESS, SECRET, "/testbin/writetest");
	return 0;
}
