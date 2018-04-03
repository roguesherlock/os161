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
 *  vm_fault - handle vm faults
 *
 */
int
vm_fault (int faulttype, vaddr_t faultaddress)
{
    (void) faulttype;
    (void) faultaddress;

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
    bool found_it;
    vaddr_t pages;

    found_it = false;

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
    if (!found_it && cmap.nfreepages > npages) {
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