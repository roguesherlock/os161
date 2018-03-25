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
    struct proc *p;

    p = curthread->t_proc;

    KASSERT (p != NULL);

    /* set process exit code and status */
    spinlock_acquire(&p->p_lock);
    p->p_state = PS_INACTIVE;
    p->exit_status = _MKWAIT_EXIT(exitcode);
    /* notify parent */
    wchan_wakeall(p->p_wait, &p->p_lock);
    spinlock_release(&p->p_lock);

    /* notify parent */
    // lock_acquire(p->p_wait_lock);
    // cv_broadcast(p->p_wait, p->p_wait_lock);
    // lock_release(p->p_wait_lock);

    /* exit thread */
    thread_exit();
}
