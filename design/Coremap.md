# Coremap

    A coremap is a data structure that keeps track of the state of all physical page frames (pages).


## Proposal

-------------------------------------------------------------------------------------
| Coremap                                                                           |
-------------------------------------------------------------------------------------
|    ram_size:         Total Ram Size                                               |
|    mem_managing:     Total Ram we're managing                                     |
|    firstfree:        physical address of the first free memory                    |
|    npages:           Total number of pages in physical memory                     |
|    nfreepages:       Total number of free pages in physical memeory               |
|    c_lock:           lock for next free page cache                                |
|    next_free_page:   cache for next free page index                               |
|    pages:            pointer to array of pages that represent physical memeory    |
-------------------------------------------------------------------------------------



<br/>



## Functions on Coremap

-   #### coremap_init
    Initialization function for coremap

    `void coremap_init(void);`


-   #### get_vaddr
    get virtual addres for the given index of a page

    `vaddr_t get_vaddr(unsigned long i);`


-   #### alloc_kpages
    allocate n contiguous physical pages.
    returns 0 if can't find n contiguous physical pages.

    `vaddr_t alloc_kpages(unsigned npages);`


-   #### free_kpages
    free pages starting at given physical addr

    `void free_kpages(vaddr_t addr);`


-   #### coremap_used_bytes
    Return amount of memory (in bytes) used by allocated coremap pages.
    If there are ongoing allocations, this value could change after it
    is returned to the caller. But it should have been correct at some point in time.

    `unsigned int coremap_used_bytes(void);`



<br/>



### Source

-   [vm.h][1]
-   [vm.c][2]



[1]:../kern/include/vm.h
[2]:../kern/vm/vm.c