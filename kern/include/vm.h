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

/* Page Flags */
#define PAGE_READ        0    /* A Page is used as read only */
#define PAGE_WRITE       1    /* A Page is used as write only */
#define PAGE_RDWR        3    /* A Page is being used  as read, write */


/* PAGE_TABLE_SIZE */
#define PAGE_TABLE_SIZE 1024

/*
 * macros and masks to split virtual page address into 2 parts
 * this is because we use two level page tables
 * A virtual page address is divided into two parts
 * page location: 20 bits
 * offset into page: 12 bits
 * We split 20 bits page location into two equal parts
 * Upper half and Lower half
 * Upper half is index into top level page table (which every process has)
 * Lower half is index into second level page table which points to
 * actual page tale entry
 */
#define UPPER_HALF_PAGE_ADDRESS 0xffc00000
#define LOWER_HALF_PAGE_ADDRESS 0x003ff000
#define GET_UPPER_HALF_PAGE_ADDRESS(X) ((X) &  UPPER_HALF_PAGE_ADDRESS)
#define GET_LOWER_HALF_PAGE_ADDRESS(X) ((X) & LOWER_HALF_PAGE_ADDRESS)
#define GET_PAGE_ADDRESS(X) (((X) - MIPS_KSEG0) & PAGE_FRAME)


/*
 *  struct page: A page is the basic unit of memory management
 *              page represents the physical memory and describes
 *              whatever we are using it for.
 *
 */
struct page {
    struct spinlock p_lock; /* lock for refrence count */
    unsigned refcount;      /* how many threads are refrenceing this page */
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
    paddr_t ram_size;               /* Total Ram Size */
    paddr_t mem_managing;           /* Total Ram we're managing */
    paddr_t firstfree;              /* physical address of the first free memory */
    unsigned long npages;           /* Total number of pages in physical memory */
    unsigned long nfreepages;       /* Total number of free pages in physical memeory */

    struct spinlock c_lock;         /* lock for next free page cache */
    unsigned long next_free_page;   /* cache for next free page index */

    struct page *pages;            /* pointer to array of pages that represent physical memeory */
};


/*
 *  struct page_table_entry
 */
struct page_table_entry {
    paddr_t paddr;       /* physical page number */
    unsigned flags:3;       /* read, write, execute */
    unsigned valid:1;       /* is page located in memory */
    unsigned refrenced:1;   /* has page been read/written to recently? */
    unsigned refcount:11;    /* don't need this much space. just to fill. */
};


/*
 * struct page_table
 */
struct page_table {
    unsigned refcount;
    struct page_table_entry *page_table_entries[PAGE_TABLE_SIZE];
};

/* Initialization function for coremap */
void coremap_init(void);

vaddr_t get_vaddr(unsigned long i);

struct addrspace;

bool get_sg_flags_if_valid (struct addrspace *as, vaddr_t faultaddress, int *flags);

void vm_can_sleep(void);

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
