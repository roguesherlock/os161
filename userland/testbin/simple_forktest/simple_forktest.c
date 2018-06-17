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
 * readtest.c
 *
 * 	Tests whether read syscalls works
 * 	This should run correctly when read is
 * 	implemented correctly.
 *
 * NOTE: While checking, this test only checks the first 31 characters.
 */
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <fcntl.h>
#include <test161/test161.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


int
main(int argc, char **argv)
{

	// 23 Mar 2012 : GWA : Assume argument passing is *not* supported.

	(void) argc;
	(void) argv;

	const char * data1 = "Hello from the child process!\n";
	const char * data2 = "Hello from the parent process!\n";
	const char * data3 = "I don't understand what's wrong here\n";
	const char * data4 = "Anyone, please help me here\n";

	size_t len1 = strlen(data1);
	size_t len2 = strlen(data2);
	size_t len3 = strlen(data3);
	size_t len4 = strlen(data4);

   	pid_t pid = fork();
   	if (pid == -1) {
      	err(1, "fork failed");
   	} else if (pid == 0) {
		write(STDOUT_FILENO, data1, len1);
		write(STDOUT_FILENO, data3, len3);
      	// printf("Hello from the child process!\n");
		// printf("I don't understand what's wrong here\n");
		// printf("I really don't\n");
   	} else {
		   write(STDOUT_FILENO, data2, len2);
		   write(STDOUT_FILENO, data4, len4);
		// printf("Hello from the parent process!\n");
		// printf("Got Pid: %d\n", (int) pid);
		// printf("Anyone, please help me here\n");
		// int status;
      	// (void)waitpid(pid, &status, 0);
   	}

	success(TEST161_SUCCESS, SECRET, "/testbin/simple_forktest");
	return 0;
}
