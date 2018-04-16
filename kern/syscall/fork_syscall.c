#include <types.h>
#include <kern/errno.h>
#include <proc.h>
#include <proc_table.h>
#include <thread.h>
#include <mips/trapframe.h>
#include <current.h>
#include <syscall.h>
#include <vnode.h>
#include <addrspace.h>
#include <files_table.h>


int
sys_fork(struct trapframe *tf, int32_t *ret)
{
    int result;
    struct proc *src, *newp;
    struct trapframe *ntf;

    KASSERT(tf != NULL);

    src = curproc;

    *ret = -1;

    ntf = kmalloc(sizeof(*ntf));
    if (ntf == NULL)
        return ENOMEM;

    memcpy(ntf, tf, sizeof(*ntf));

    newp = NULL;
    result = proc_copy(src, &newp);
    if (result) {
        kfree(ntf);
        return result;
    }
    *ret = newp->pid;
    set_proc(newp->pid, newp);

    thread_fork(src->p_name, newp,
                help_enter_forked_process,
                (void *) ntf, (unsigned long) 1);

    return 0;
}

