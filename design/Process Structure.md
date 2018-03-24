# Process Structure


## Proposal

---------------------------------------------------------
|Process Structure                                      |
|-------------------------------------------------------|
|   Reference to process thread                         |
|   Reference to process's parent                       |
|   File Table: maintains refrence to file handles.     |
|   PID: Process ID                                     |
|   PPID: Parent Process ID                             |
|   Exit Status: Did the process exit or not?           |
|   Exit Code: If process exited, what is exit code?    |
|   CVs: for waitpid                                    |
---------------------------------------------------------


## Functions of Proccess


-   #### kproc
    This is the process structure for the kernel and for kernel-only threads.

    `extern struct proc *kproc;`


-   #### proc_bootstrap
    Call once during system startup to allocate data structures.

    `void proc_bootstrap(void);`

-   #### proc_create_runprogram
    Create a fresh process for use by runprogram().

    `struct proc *proc_create_runprogram(const char *name);`

-   #### proc_destroy
    Destroy a process.

    `void proc_destroy(struct proc *proc);`

-   #### proc_addthread
    Attach a thread to a process. Must not already have a process.

    `int proc_addthread(struct proc *proc, struct thread *t);`

-   #### proc_remthread
    Detach a thread from its process.

    `void proc_remthread(struct thread *t);`

-   #### proc_getas
    Fetch the address space of the current process.

    `struct addrspace *proc_getas(void);`

-   #### proc_setas
    Change the address space of the current process, and return the old one.

    `struct addrspace *proc_setas(struct addrspace *);`

-   #### proc_copy
    copy src process to dst process. dst process has new unique pid.

    `int proc_copy(struct proc *src, struct proc **dst);`


