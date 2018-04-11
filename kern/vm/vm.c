/*
 *  vm - defintions for virtual memeory.
 *
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>


/* Coremap */
static struct coremap cmap;


/*
 *  coremap_init - Initialize coremap
 *
 */
void
coremap_init ()
{
    unsigned long i, npages, size, extra_size;

    spinlock_init(&cmap.c_lock);

    /* set the size of ram */
    cmap.ram_size = ram_getsize();

    /*
     * get the addresss of firstfree physical memory.
     * Note: now, ram_getfirstfree and all the ram_* functions are
     * useless
     */
    cmap.firstfree = ram_getfirstfree();

    /* set the start of memeory to manage physical pages */
    cmap.pages = (struct page *) PADDR_TO_KVADDR(cmap.firstfree);

    /*
     *
     *  Physical Memory
     *   ----------------------------------------------------------------
     *  |           |                                                   |
     *  |           |                                                   |
     *  |  kernel   |                                                   |
     *  |           |                                                   |
     *  |           |                                                   |
     *  -----------------------------------------------------------------
     *             ^
     *         firstfree
     *      coremap->pages
     *             ^                                                    ^
     *             |------------------- npages -------------------------|
     */

    /* Total number of pages we've. Round up to a whole number of pages. */
    npages = ((cmap.ram_size - cmap.firstfree) + PAGE_SIZE - 1) / PAGE_SIZE;

    /* size required to manage the physical pages. */
    size = (npages * sizeof(struct page)) + sizeof(struct page);

   /*
     *  npages in the size calculation above includes the size of
     *  memory we'll be using for coremap pages. so we subtract the
     *  page size acquired by size. Only if size acquired is greater
     *  than atleast 1 page size
     *
     *  Physical Memory
     *   ----------------------------------------------------------------
     *  |           |    size   |                                       |
     *  |           |  used by  |                                       |
     *  |  kernel   |  coremap  |                                       |
     *  |           |   pages   |                                       |
     *  |           |           |                                       |
     *  -----------------------------------------------------------------
     *             ^
     *         firstfree
     *      coremap->pages
     *             ^                                                   ^
     *             |------------------- npages ------------------------|
     */
    extra_size = size - ((size/PAGE_SIZE) > 1) ? ((size/PAGE_SIZE) * sizeof(struct page)) : 0;

    size -= extra_size;

    /* page allign size */
    size = (size % PAGE_SIZE == 0) ? size : size + (PAGE_SIZE - (size % PAGE_SIZE));

    /* increase the start address of physical memeory we're suppose to manage */
    cmap.firstfree += size;

    /* Now get the accurate size of physical memeory we'll actually manage */
    cmap.mem_managing = cmap.ram_size - cmap.firstfree;

    /* Round up to a whole number of pages. */
    cmap.npages = cmap.nfreepages = (cmap.mem_managing + PAGE_SIZE - 1) / PAGE_SIZE;

   /*
     *  Physical Memory
     *   ----------------------------------------------------------------
     *  |           |    size   |                                       |
     *  |           |  used by  |                                       |
     *  |  kernel   |  coremap  |                                       |
     *  |           |   pages   |                                       |
     *  |           |           |                                       |
     *  -----------------------------------------------------------------
     *             ^            ^
     *      coremap->pages  firstfree
     *
     *                          ^                                        ^
     *                          |---- npages/nfreepages/mem_managing ----|
     */



    cmap.next_free_page = 0;


    for (i = 0; i < cmap.npages; ++i) {
        spinlock_init(&(cmap.pages[i].p_lock));
        cmap.pages[i].refcount = 0;
        cmap.pages[i].refcount = 0;
        cmap.pages[i].dirty = 0;
        cmap.pages[i].id = 0;
    }
}


/*
 *  get_vaddr - get virtual addres for the given index of a page
 *
 */
vaddr_t
get_vaddr (unsigned long i)
{
    paddr_t paddr;
    vaddr_t vaddr;

    KASSERT(spinlock_do_i_hold(&cmap.c_lock));
    KASSERT(i <= cmap.npages);

    paddr = cmap.firstfree + (i * PAGE_SIZE);
    vaddr = PADDR_TO_KVADDR(paddr);

    return vaddr;
}


/*
 *  vm_bootstrap - Initialize vm
 *
 */
void
vm_bootstrap ()
{

}


/*
 * Check if we're in a context that can sleep. While most of the
 * operations in vm don't in fact sleep, in a real VM system many
 * of them would. In those, assert that sleeping is ok. This helps
 * avoid the situation where syscall-layer code that works ok with
 * vm starts blowing up during the VM assignment.
 */
void
vm_can_sleep(void)
{
	if (CURCPU_EXISTS()) {
		/* must not hold spinlocks */
		KASSERT(curcpu->c_spinlocks == 0);

		/* must not be in an interrupt handler */
		KASSERT(curthread->t_in_interrupt == 0);
	}
}


/*
 *  get_sg_flags_if_valid - checks if the given address is valid for the given addrspace
 *                          and sets segments flags.
 *                          returns validity
 *
 */
bool
get_sg_flags_if_valid (struct addrspace *as, vaddr_t faultaddress, int *flags)
{
    int i;
    vaddr_t sg_end;

    if (as == NULL || faultaddress == (vaddr_t) NULL)
        return false;

    for (i = 0; i < INITIAL_SEGMENTS; ++i) {
        /* stack */
        if (as->as_segments[i].sg_dynamic) {
            if (((as->as_segments[i].sg_vbase == USERSTACK) &&
                (faultaddress <= as->as_segments[i].sg_vbase)) ||
                (faultaddress >= as->as_segments[i].sg_vbase)){
                if (flags)
                    *flags = as->as_segments[i].sg_flags;
                return true;
            }
            return false;
        }

        sg_end = as->as_segments[i].sg_vbase + as->as_segments[i].sg_size;
        if ((faultaddress >= as->as_segments[i].sg_vbase) &&
            (faultaddress < sg_end)) {
                if (flags)
                    *flags = as->as_segments[i].sg_flags;
                return true;
            }
    }

    /* if we're here, we need to look into extra_segments */
    if (as->as_extra_segments == NULL)
        return false;

    /* declared here as it is very unlikely we'll reach here */
    struct segment_list *sg_ptr;

    sg_ptr = as->as_extra_segments;
    while (sg_ptr != NULL) {
        if (sg_ptr->segment.sg_dynamic &&
            (faultaddress >= sg_ptr->segment.sg_vbase)) {
                if (flags)
                    *flags = sg_ptr->segment.sg_flags;
                return true;
            }

        sg_end = sg_ptr->segment.sg_vbase + sg_ptr->segment.sg_size;
        if ((faultaddress >= sg_ptr->segment.sg_vbase) &&
            (faultaddress < sg_end)) {
                if (flags)
                    *flags = sg_ptr->segment.sg_flags;
                return true;
            }

        ++i;
        sg_ptr = sg_ptr->next;
    }

    return false;
}


/*
 *  get_paddr - get physical page address of the given virtual address
 *              for the given process
 *              Note: it assumes that the given address is valid
 *
 */
static
int
get_paddr (struct proc *proc, vaddr_t vaddr, paddr_t *paddr)
{
    paddr_t addr;
    struct page_table *pt;
    struct page_table_entry *pte;
    uint32_t hi, lo;

    vm_can_sleep();

    if (proc == NULL)
        return EFAULT;

    hi = GET_UPPER_HALF_PAGE_ADDRESS(vaddr);
    lo = GET_LOWER_HALF_PAGE_ADDRESS(vaddr);
    hi >>= 22;
    lo >>= 12;

    pt = proc->p_addrspace->as_page_table[hi];
    if (pt == NULL) {
        pt = kmalloc(sizeof(*pt));
        if (pt == NULL) {
            return ENOMEM;
        }
        proc->p_addrspace->as_page_table[hi] = pt;
        proc->p_addrspace->as_page_table[hi]->refcount = 1;
    }

    pte = pt->page_table_entries[lo];
    if (pte == NULL) {
        pte = kmalloc(sizeof(*pte));
        if (pte == NULL)
            return ENOMEM;

        addr = alloc_kpages(1);
        if (addr == (paddr_t) NULL) {
            kfree(pte);
            return ENOMEM;
        }
        /* discard residual page data */
        bzero((void *) addr, PAGE_SIZE);

        /* alloc_kpages returns directly mapped kseg addressess */
        // addr -= MIPS_KSEG0;

        pte->paddr = GET_PAGE_ADDRESS(addr);
        pte->refrenced = 1;
        pte->valid = 1;
        pte->flags = 0;
        pte->refcount = 1;
        pt->page_table_entries[lo] = pte;
    }

    // if (!pte->valid) {

    // }

    *paddr = pte->paddr;

    return 0;
}


/*
 *  find_tlb_entry - finds the invalid tlb entry
 *                  returns index of tlb entry
 *                  -1 if unable to find one
 *
 */
static
int
find_tlb_entry ()
{
    int i, spl;
    uint32_t ehi, elo;

    spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if ((elo & TLBLO_VALID)) {
			continue;
		}
		splx(spl);
		return i;
	}

    splx(spl);
    return -1;
}


/*
 *  set_tlb_entry - sets the tlb entry
 *                  tries to find an empty tlb entry or invalid one
 *                  otherwise overwrites random tlb entry
 *
 */
static
void
set_tlb_entry (uint32_t entryhi, uint32_t entrylo)
{
    int i;

    i = find_tlb_entry();
    entrylo |= TLBLO_VALID;

    if (i >= 0)
        tlb_write(entryhi, entrylo, i);
    else
        tlb_random(entryhi, entrylo);
}


/*
 *  vm_fault - handle vm faults
 *
 */
int
vm_fault (int faulttype, vaddr_t faultaddress)
{
	paddr_t paddr;
	int spl, result, flags;
    bool is_valid;
	struct addrspace *as;

    is_valid = false;

	if (curproc == NULL) {
		/*
		 * No process. This is probably a kernel fault early
		 * in boot. Return EFAULT so as to panic instead of
		 * getting into an infinite faulting loop.
		 */
		return EFAULT;
	}
    struct proc *p = curproc;
    (void) p;

	switch (faulttype) {
        /* do not allow write to readonly segments */
	    case VM_FAULT_READONLY:
        return EPERM;
        break;
	    case VM_FAULT_READ:
        break;
	    case VM_FAULT_WRITE:
        /* we've a write. make sure each process has it's own unique addrspace now */
        if (curproc->p_addrspace->as_copied == 0) {
            result = as_actually_copy(curproc->p_addrspace);
            if (result)
                return result;
        }
		break;
	    default:
		return EINVAL;
	}

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}


    is_valid = get_sg_flags_if_valid(as, faultaddress, &flags);
    if (!is_valid)
        return EFAULT;

    faultaddress &= PAGE_FRAME;

    result = get_paddr(curproc, faultaddress, &paddr);
    if (result)
        return result;

	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while probing the TLB. */
	spl = splhigh();


    if (faulttype == VM_FAULT_READ) {
        if (flags & SG_WRITE)
            set_tlb_entry(faultaddress, paddr | TLBLO_DIRTY);
        else
            set_tlb_entry(faultaddress, paddr);
    } else if (faulttype == VM_FAULT_WRITE) {
        /* TODO: should executable segment have write permission? */
        if ((flags & SG_EXECUTE) || (flags & SG_WRITE)) {
            set_tlb_entry(faultaddress, paddr | TLBLO_DIRTY);
        } else {
            return EPERM;
        }
    }


	splx(spl);
    return 0;
}


/*
 *  alloc_kpages - allocate n contiguous physical pages
 *
 */
vaddr_t
alloc_kpages (unsigned npages)
{
    unsigned i, j, k;
    unsigned long pi;
    bool found_it, one_more_try;
    vaddr_t pages;

    found_it = one_more_try = false;

    spinlock_acquire(&cmap.c_lock);
    if (npages > cmap.nfreepages)
        goto done;

    pi = cmap.npages;
    if (cmap.next_free_page >= cmap.npages)
        cmap.next_free_page = 0;

again:
    for (i = cmap.next_free_page; i < cmap.npages; ++i) {
        if (cmap.pages[i].refcount == 0) {
            pi = i;
            found_it = true;
            for (k = 0, j = i; k < npages; ++j, ++k) {
                if ((j >= cmap.npages) || cmap.pages[j].refcount > 0) {
                    found_it = false;
                    break;
                }
            }
            if (found_it)
                break;
        }
    }
    if (!found_it && cmap.nfreepages > npages && !one_more_try) {
        one_more_try = true;
        cmap.next_free_page = 0;
        goto again;
    }

    if (found_it) {
        pages = get_vaddr(pi);
        for (k = 0, j = i; k < npages; ++j, ++k) {
            spinlock_acquire(&cmap.pages[j].p_lock);
            cmap.pages[j].id = pages;
            cmap.pages[j].refcount++;
            spinlock_release(&cmap.pages[j].p_lock);
        }
        cmap.next_free_page += npages;
        cmap.nfreepages -= npages;
        spinlock_release(&cmap.c_lock);
        return pages;
    }

done:
    spinlock_release(&cmap.c_lock);
    return (vaddr_t) 0;
}


/*
 *  free_kpages - free pages starting at given physical addr
 *
 */
void
free_kpages (vaddr_t addr)
{
    unsigned i, j, k;

    /* Get the index of the page. Note: both work. */
    /* i = ((addr & PAGE_FRAME) - MIPS_KSEG0 - cmap.firstfre) / PAGE_SIZE; */
    i = (addr - PADDR_TO_KVADDR(cmap.firstfree)) / PAGE_SIZE;

    for (j = k = 0; i < cmap.npages; ++i) {
        if (cmap.pages[i].id == addr) {
            k = i;
            j = 0;
            while (cmap.pages[i].id == addr) {
                spinlock_acquire(&cmap.pages[i].p_lock);
                cmap.pages[i].refcount--;
                cmap.pages[i].id = 0;
                spinlock_release(&cmap.pages[i].p_lock);
                ++j;
                ++i;
            }
            break;
        }
    }
    if (j != 0) {
        spinlock_acquire(&cmap.c_lock);
        cmap.nfreepages += j;
        if (cmap.next_free_page >= cmap.npages)
            cmap.next_free_page = k;
        spinlock_release(&cmap.c_lock);
    }
}


/*
 *  coremap_used_bytes - Return amount of memory (in bytes) used by allocated coremap pages.  If
 *                      there are ongoing allocations, this value could change after it is returned
 *                      to the caller. But it should have been correct at some point in time.
 */
unsigned int
coremap_used_bytes ()
{
    return (unsigned int) ((cmap.npages - cmap.nfreepages) * PAGE_SIZE);
}


/*
 *  vm_tlbshootdown - Remove an entry from another CPU’s TLB address mapping
 *
 */
void
vm_tlbshootdown (const struct tlbshootdown *tlb)
{
    (void) tlb;
}