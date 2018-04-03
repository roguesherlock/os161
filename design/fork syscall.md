# Fork
fork duplicates the currently running process. The two copies are identical, except that one (the "new" one, or "child"), has a new, unique process id, and in the other (the "parent") the process id is unchanged.

The process id must be greater than 0.

The two processes do not share memory or open file tables; this state is copied into the new process, and subsequent modification in one process does not affect the other.

However, the file handle objects the file tables point to are shared, so, for instance, calls to lseek in one process can affect the other.



## Synopsis

``` pid_t fork(void);```

### On kernel side:
```int sys_fork(struct trapframe *tf, int32_t *ret)```


## Return Values
On success, fork returns twice, once in the parent process and once in the child process. In the child process, 0 is returned. In the parent process, the process id of the new child process is returned.

On error, no new process is created. fork, only returns once, returning -1, and errno is set according to the error encountered.

### On kernel side:
return 0 on success or error code on errors. Acctual return values are put in ret pointer



## Errors
The following error codes should be returned under the conditions given. Other [error][1] codes may be returned for other cases not mentioned here.

-   **EMPROC**  The current user already has too many processes.
-   **ENPROC**  There are already too many processes on the system.
-   **ENOMEM**  Sufficient virtual memory for the new process was not available.



<br/>



## Source

-   [fork_syscall.c][2]



[1]:../kern/include/kern/errno.h
[2]:../kern/syscall/fork_syscall.c