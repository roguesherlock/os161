/*
 * definition for waitpid syscall
 *
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/wait.h>
#include <limits.h>
#include <proc.h>
#include <proc_table.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>
#include <copyinout.h>
#include <wchan.h>

int
sys_waitpid (pid_t pid, int *status, int options, int32_t *retval)
{
    bool no_one_is_waiting;
    int result, p_status;
    struct proc *p, *cp;

    p = NULL;
    cp = curthread->t_proc;
    no_one_is_waiting = false;

    result = 0;
    *retval = -1;

    result = get_proc(pid, &p);
    if (result)
        return result;

    if (cp != p->parent)
        return ECHILD;

    /* filter out supported options */
    switch (options) {
        case 0:
        case WNOHANG:
        // case WUNTRACED:
        break;
        default:
            return EINVAL;
    }

    if ((options == WNOHANG) && (p->p_state == PS_ACTIVE)) {
        *retval = 0;
        return result;
    }

    /* check if status ptr is valid before sleeping */
    result = check_userptr ((const_userptr_t) status);
    if (result)
        if (status != NULL)     /* NULL ptr is allowed */
            return result;

    spinlock_acquire(&p->p_lock);
    while (p->p_state == PS_ACTIVE)
        wchan_sleep(p->p_wait, &p->p_lock);
    spinlock_release(&p->p_lock);

    p_status = p->exit_status;

    if (status) {
        result = copyout((const void *) &p_status, (userptr_t) status, sizeof(int));
        if (result)
            return result;
    }

    *retval = pid;

    spinlock_acquire(&p->p_lock);
    no_one_is_waiting = wchan_isempty(p->p_wait, &p->p_lock);
    spinlock_release(&p->p_lock);
    if (no_one_is_waiting && status && p->rogue)
        mark_proc_for_deletion(p);

    return 0;
}