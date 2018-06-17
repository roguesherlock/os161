/*
 * definition for _exit syscall
 *
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/wait.h>
#include <proc.h>
#include <proc_table.h>
#include <thread.h>
#include <wchan.h>
#include <synch.h>
#include <current.h>
#include <syscall.h>

void
sys__exit (int exitcode)
{
    KASSERT (curproc != NULL);

    curproc->exited = true;
    curproc->exit_status = _MKWAIT_EXIT(exitcode);
    V(curproc->exit_sem);
    thread_exit();
}
