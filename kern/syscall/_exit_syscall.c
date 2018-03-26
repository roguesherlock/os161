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
    if (wchan_isempty(p->p_wait, &p->p_lock) && !(p->rogue)) {
        /*
         * can't have spinlocks when deleting. Why?
         * dumbvm can sleep!
         * 
         */
        spinlock_release(&p->p_lock);
        mark_proc_for_deletion(p);
    } else {
        /* notify parent */
        wchan_wakeall(p->p_wait, &p->p_lock);
        spinlock_release(&p->p_lock);
    }


    /* exit thread */
    thread_exit();
}
