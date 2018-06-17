# Process Table

    ### A Process Table is a data structure that maps a PID to a process structure.

    *Requirements:*
    -   When creating new process,
        a.  It should be able to allocate a new process structure.
        b.  It should be able to allocate new PID.
    -   Given PID, it should be able to retrieve  the process structure.
    -   When a process exits,
        a.  It should be able to destroy the process structure.
        b.  It should be able to deallocate the assigned PID.

## Proposal

    I decided eventually to go with a simple array as os161 wouln't have lots of processes (I'm assuming).


-------------------------------------------------------------------------------------
| Process Table                                                                     |
-------------------------------------------------------------------------------------
|	pt_lock                     /* for synchronization */                           |
|	pnum			            /* number of active processess */                   |
|	next_pid		            /* cache for next pid */                            |
|   p_array[MAX_ACTIVE_PROC];   /* Process Array that maps pids to processess */    |
-------------------------------------------------------------------------------------



## Functions on Process Table

-   #### proc_table_create
    create a new process table.

    `void proc_table_create(struct proc_table **pt);`

-   #### proc_table_destroy
    destroy process table.
    also call _exit on every process. except kproc.
    very destructive. hence, only kproc is allowed to call it.

    `void proc_table_destroy(struct proc_table *pt);`

-   #### proc_table_is_empty
    checks weather proc_table is empty or not

    `bool proc_table_is_empty(void);`


-   #### set_proc
    map a process to a given pid.

    return proper error code on error.

    `int set_proc(pid_t pid, struct proc *p);`

-   #### get_proc
    get a process for a given pid.

    return proper error code on error.

    `int get_proc(pid_t pid, struct proc **p);`

-   #### get_pid
    get a new valid pid.
    on sucess pid contains valid pid.

    return proper error code on error.

    `int get_pid(pid_t *pid);`

-   #### rel_pid
    release given pid.

    return process associated with given pid, if any.

    `struct proc *rel_pid(pid_t pid);`



<br/>



## Source

-   [proc_table.h][1]
-   [proc_table.c][2]


[1]:../kern/include/proc_table.h
[2]:../kern/proc/proc_table.c