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
#include <elf.h>


#define COREMAP_DEBUG 0

/* Coremap */
static struct coremap cmap;
static struct spinlock cmap_lock = SPINLOCK_INITIALIZER;

/*
 *  coremap_init - Initialize coremap
 *
 */
void
coremap_init ()
{
    unsigned long i, npages, size, extra_size;

    spinlock_init(&cmap_lock);

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
        cmap.pages[i].flags = 0;
        cmap.pages[i].dirty = 0;
        cmap.pages[i].id = 0;
    }
}


size_t
get_ram_size ()
{
    return (size_t) cmap.ram_size;
}

size_t
get_avail_memory ()
{
    return (size_t) (cmap.nfreepages * PAGE_SIZE);
}



#if COREMAP_DEBUG

static
void
print_coremap_entry (struct page *p)
{
    KASSERT (p != NULL);
    kprintf("\n");
    if (p->refcount == 0)
        kprintf("Free\n");
    else
        kprintf("Being Used\n");
    kprintf("id: %p\n", (void *) p->id);
    kprintf("refcount: %u\n", p->refcount);
    kprintf("dirty: %u\n", p->dirty);
    kprintf("flags: %u\n", p->flags);
    kprintf("\n");
}


static
void
print_coremap (void)
{
    unsigned long i;

    spinlock_acquire(&cmap_lock);
    for (i = 0; i < cmap.npages; ++i) {
        kprintf("----------------\n");
        kprintf("index: %lu\n", i);
        print_coremap_entry(&cmap.pages[i]);
    }
    spinlock_release(&cmap_lock);
}

#endif

/*
 *  get_vaddr - get virtual addres for the given index of a page
 *
 */
vaddr_t
get_vaddr (unsigned long i)
{
    paddr_t paddr;
    vaddr_t vaddr;

    KASSERT(spinlock_do_i_hold(&cmap_lock));
    KASSERT(i < cmap.npages);

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

    if (i >= 0 && i < NUM_TLB)
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
    vaddr_t tmp_addr;
	int spl, result, flags;
	struct addrspace *as;
    struct page_table_entry *pte;

	if (curproc == NULL) {
        return EFAULT;
    }

	as = proc_getas();
	if (as == NULL) {
		/*
		 * No address space set up. This is probably also a
		 * kernel fault early in boot.
		 */
		return EFAULT;
	}

	switch (faulttype) {
        /* do not allow write to readonly segments */
	    case VM_FAULT_READONLY:
        return EPERM;
        break;
	    case VM_FAULT_READ:
        break;
	    case VM_FAULT_WRITE:
        /* we've a write. make sure each process has it's own unique addrspace now */
        if (as->as_copied == 0) {
            result = as_actually_copy(as);
            if (result)
                return result;
        }
		break;
	    default:
		return EINVAL;
	}

    if (!as_is_addr_valid(as, faultaddress))
        return EFAULT;

    tmp_addr = faultaddress;
    faultaddress &= PAGE_FRAME;

    pte = as_get_pte(as, faultaddress);
    if (pte == NULL) {
        flags = as_get_flags(as, tmp_addr);
        result = as_add_new_pte(as, faultaddress, flags, &pte);
        if (result)
            return result;
    }

    paddr = pte->pte_paddr;
    flags = pte->pte_flags;

	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while probing the TLB. */
	spl = splhigh();

    if (faulttype == VM_FAULT_READ) {
        if (flags & PF_W)
            set_tlb_entry(faultaddress, paddr | TLBLO_DIRTY);
        else
            set_tlb_entry(faultaddress, paddr);
    } else if (faulttype == VM_FAULT_WRITE) {
        /* TODO: should executable segment have write permission? */
        if ((flags & PF_X) || (flags & PF_W)) {
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
    unsigned long i, j, k;
    bool found_it, tried_again;
    vaddr_t pages;

    found_it = tried_again = false;

#if COREMAP_DEBUG
    if ((cmap.npages - cmap.nfreepages) > 200) {
        print_coremap();
        panic("shit!\n");
    }
#endif

    spinlock_acquire(&cmap_lock);
    if (npages > cmap.nfreepages)
        goto done;

    if (cmap.next_free_page >= cmap.npages)
        cmap.next_free_page = 0;

again:
    for (i = cmap.next_free_page; i < cmap.npages; ++i) {
        if (cmap.pages[i].refcount == 0) {
            found_it = true;
            for (k = 0, j = i; k < npages; ++j, ++k) {
                if ((j >= cmap.npages) || cmap.pages[j].refcount != 0) {
                    found_it = false;
                    break;
                }
            }
            if (found_it)
                break;
        }
    }
    if (!found_it && cmap.nfreepages >= npages && !tried_again) {
        cmap.next_free_page = 0;
        tried_again = true;
        goto again;
    }

    if (found_it) {
        pages = get_vaddr(i);
        for (k = 0, j = i; k < npages; ++j, ++k) {
            spinlock_acquire(&cmap.pages[j].p_lock);
            cmap.pages[j].id = pages;
            cmap.pages[j].refcount++;
            bzero((void *) get_vaddr(j), PAGE_SIZE);
            spinlock_release(&cmap.pages[j].p_lock);
            cmap.next_free_page++;
            cmap.nfreepages--;
        }
        spinlock_release(&cmap_lock);
        return pages;
    }

done:
    spinlock_release(&cmap_lock);
    return (vaddr_t) 0;
}


/*
 *  free_kpages - free pages starting at given physical addr
 *
 */
void
free_kpages (vaddr_t addr)
{
    unsigned i, j; //, k;


    /* Get the index of the page. Note: both work. */
    /* i = ((addr & PAGE_FRAME) - MIPS_KSEG0 - cmap.firstfre) / PAGE_SIZE; */
    i = j = (addr - PADDR_TO_KVADDR(cmap.firstfree)) / PAGE_SIZE;

    KASSERT (cmap.pages[i].id == addr);

    spinlock_acquire(&cmap_lock);
    for ( ; cmap.pages[i].id == addr; ++i) {
        spinlock_acquire(&cmap.pages[i].p_lock);
        cmap.pages[i].refcount--;
        cmap.pages[i].id = 0;
        spinlock_release(&cmap.pages[i].p_lock);
        cmap.nfreepages++;
    }
    if (cmap.next_free_page >= cmap.npages)
        cmap.next_free_page = j;
    spinlock_release(&cmap_lock);
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