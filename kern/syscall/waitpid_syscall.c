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
#include <addrspace.h>
#include <wchan.h>

int
sys_waitpid (pid_t pid, int *status, int options, int32_t *retval)
{
    int result, p_status;
    struct proc *p_child;

    KASSERT (curproc != NULL);

    result = 0;
    *retval = -1;

    result = get_proc(pid, &p_child);
    if (result)
        return result;

    if (curproc != p_child->parent)
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

    if ((options == WNOHANG) && !p_child->exited) {
        *retval = 0;
        return result;
    }

    if (status != NULL && (!as_is_addr_valid(curproc->p_addrspace, (vaddr_t) status)))
        return EFAULT;

    if (!p_child->exited)
        P(p_child->exit_sem);

    KASSERT (p_child->exited);

    p_status = p_child->exit_status;

    if (status) {
        result = copyout((const void *) &p_status, (userptr_t) status, sizeof(int));
        if (result)
            return result;
    }

    proc_destroy(p_child);

    *retval = pid;

    return 0;
}