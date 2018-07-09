/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _VM_H_
#define _VM_H_

/*
 * VM system-related definitions.
 *
 */

#include <spinlock.h>
#include <machine/vm.h>

/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/




/* if dumbvm isn't loaded, use our vm */
#if !OPT_DUMBVM

#define STACK_CHUNK     1024  /* initial stack size in chunks */
#define HEAP_OFFSET     50    /* Heap offset for heap base*/

/* Page Flags */
#define PAGE_READ        0    /* A Page is used as read only */
#define PAGE_WRITE       1    /* A Page is used as write only */
#define PAGE_RDWR        3    /* A Page is being used  as read, write */


/*
 *  struct page: A page is the basic unit of memory management
 *              page represents the physical memory and describes
 *              whatever we are using it for.
 *
 */
struct page {
    struct spinlock p_lock; /* lock for refrence count */
    unsigned refcount:8;      /* how many threads are refrenceing this page */
    unsigned flags:7;       /* access permissions for the page */
    unsigned dirty:1;       /* does the page contents need to be written back to disk? */
    vaddr_t id;             /* page id. allows to detect contiguous pages */
};


/*
 *  struct coremap: Coremap is the data structure that keeps track
 *                  of all the physical pages in memory
 *
 */
struct coremap {
    // struct spinlock c_lock;         /* lock for next free page cache */

    paddr_t ram_size;               /* Total Ram Size */
    paddr_t mem_managing;           /* Total Ram we're managing */
    paddr_t firstfree;              /* physical address of the first free memory */

    unsigned long npages;           /* Total number of pages in physical memory */
    unsigned long nfreepages;       /* Total number of free pages in physical memeory */
    unsigned long next_free_page;   /* cache for next free page index */

    struct page *pages;            /* pointer to array of pages that represent physical memeory */
};



/*
 *  page_table_entry - Represents an mapping of virtual page to a physical page
 *                      of a single process. Also, it's permissions
 *
 */
struct page_table_entry {
    vaddr_t pte_vaddr;           /* Virtual Page Number */
    paddr_t pte_paddr;           /* Physical Page NUmber */

    unsigned pte_flags:8;           /* Page flags */
    unsigned pte_refrenced:8;       /* Is page refrenced recently? */
    unsigned pte_in_memory:8;       /* Is page in memory? */

    struct page_table_entry *next;  /* next page_table_entry */
};



/* Initialization function for coremap */
void coremap_init(void);

vaddr_t get_vaddr(unsigned long i);

size_t get_ram_size(void);
size_t get_avail_memory(void);
#endif




/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(unsigned npages);
void free_kpages(vaddr_t addr);

/*
 * Return amount of memory (in bytes) used by allocated coremap pages.  If
 * there are ongoing allocations, this value could change after it is returned
 * to the caller. But it should have been correct at some point in time.
 */
unsigned int coremap_used_bytes(void);

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown(const struct tlbshootdown *);


#endif /* _VM_H_ */
