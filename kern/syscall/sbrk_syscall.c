/*
 *  sbrk_syscall.c - definiton for sys_sbrk
 *
 */

#include <types.h>
#include <spl.h>
#include <kern/errno.h>
#include <proc.h>
#include <proc_table.h>
#include <current.h>
#include <syscall.h>
#include <addrspace.h>


/*
 *  sys_sbrk -
 *
 */
int
sys_sbrk (intptr_t amount, int32_t *retval)
{
    int i, npages;
    struct addrspace *as;
    size_t sg_end, avail_mem;

    KASSERT (curproc != NULL);
    KASSERT (curproc->p_addrspace != NULL);

    as = curproc->p_addrspace;
    avail_mem = get_avail_memory();

    sg_end = as->heap.sg_vbase + as->heap.sg_size;

    *retval = (int32_t) ((void *)-1);

    if (amount == 0) {
        *retval = as->heap.sg_vbase + as->heap.sg_size;
        return 0;
    }

    if ((sg_end + amount) % PAGE_SIZE != 0)
        return EINVAL;

    if (amount > 0) {
        avail_mem *= 20;   /* Only Temporary until swaping implemented */
        if (amount > (intptr_t) avail_mem ||
            (sg_end + amount) >= (as->stack.sg_vbase - as->stack.sg_size)) {
            return ENOMEM;
        }
    }
    else if (amount < 0) {
        if ((size_t) -amount > as->heap.sg_size)
            return EINVAL;

        npages = (-amount + PAGE_SIZE - 1) / PAGE_SIZE;
        sg_end += amount;
        for (i = 0; i < npages; ++i, sg_end += PAGE_SIZE)
            as_rm_pte(as, (vaddr_t) sg_end);
    }

    *retval = as->heap.sg_vbase + as->heap.sg_size;
    as->heap.sg_size += amount;

    return 0;
}
