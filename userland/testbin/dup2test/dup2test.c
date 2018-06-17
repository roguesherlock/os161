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
 * dup2test.c
 *
 * 	Tests whether dup2 syscalls works
 * 	This should run correctly when read, write, open is
 * 	implemented correctly.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <test161/test161.h>


#define FILENAME "dup2test.dat"

static const char *MAGIC = "h4xa0rRq0Vgbc35tiYJ^!#nXzZSAKPO";


int
main(int argc, char **argv)
{

	// 23 Mar 2012 : GWA : Assume argument passing is *not* supported.

	(void) argc;
	(void) argv;

	int fd, fd2, len, len2;
	int expected_len = strlen(MAGIC);
	int max_len = expected_len + 1;
	char buf[max_len], buf2[max_len];

	fd = open(FILENAME, O_RDWR | O_CREAT | O_TRUNC);
	if(fd < 0) {
		err(1, "Failed to open file.\n");
	}
	nprintf(".");

	len = write(fd, MAGIC, expected_len);
	if(len != expected_len) {
		err(1, "writetest expected to write %d bytes to readwritetest.dat."
			" Syscall reports that it wrote %d bytes.\n"
			"Is your write syscall returning the right value?\n",
			expected_len, len);
	}
	close(fd);

	fd = open(FILENAME, O_RDONLY);
	if (fd < 0)
		err(1, "couldn't open file. Is your open syscall working right?\n");

	fd2 = open(FILENAME, O_RDONLY);
	if (fd2 < 0)
		err(1, "couldn't open file. Is your open syscall working right?\n");

	fd2 = dup2(fd, fd2);
	if (fd < 0)
		err(1, "couldn't clone file handle. Is your dup2 syscall working right?\n");


	len = read(fd, buf, expected_len);
	len2 = read(fd2, buf2, expected_len);

	if (len != len2)
		err(1, "cloned file's length do not match. Is your dup2 syscall working right?\n");

	if (strcmp(buf, buf2) != 0)
		err(1, "file contents do not match. Is your dup2 syscall working righ?\n");


	nprintf(".");
	nprintf("\n");

	success(TEST161_SUCCESS, SECRET, "/testbin/dup2test");
	return 0;
}
