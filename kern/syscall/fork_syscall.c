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


// void
// copy_tf(struct trapframe *src, struct trapframe *dst)
// {
//     KASSERT(src != NULL);
//     KASSERT(dst != NULL);

//     int i, len;
//     // size_t len;

//     /* utilizing the fact that every element in tf is uint32_t */
//     uint32_t *s = (uint32_t *) src;
//     uint32_t *d = (uint32_t *) dst;

//     len = sizeof(*src) / sizeof(uint32_t);

//     for (i = 0; i < len; ++i)
//         d[i] = s[i];

// }


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
    // copy_tf(tf, ntf);

    newp = NULL;
    result = proc_copy(src, &newp);
    if (result) {
        kfree(ntf);
        return result;
    }
    *ret = newp->pid;

    thread_fork(src->p_name, newp,
                help_enter_forked_process,
                (void *) ntf, (unsigned long) 1);

    return 0;
}

