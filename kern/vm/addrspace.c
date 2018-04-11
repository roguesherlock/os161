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

#include <types.h>
#include <current.h>
#include <spl.h>
#include <mips/tlb.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	int i;
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	for (i = 0; i < INITIAL_SEGMENTS; ++i) {
		as->as_segments[i].sg_vbase = 0;
		as->as_segments[i].sg_size = 0;
		as->as_segments[i].sg_flags = 0;
		as->as_segments[i].sg_inuse = 0;
		as->as_segments[i].sg_dynamic = 0;
	}

	/* initialize page table */
	for (i = 0; i < PAGE_TABLE_SIZE; ++i)
		as->as_page_table[i] = NULL;

	as->as_extra_segments = NULL;
	as->as_copied = 1;

	return as;
}


static
void
free_as_extra_segments (struct segment_list *head)
{
	struct segment_list *ptr, *fptr;

	ptr = head;
	while (ptr != NULL) {
		fptr = ptr;
		ptr = ptr->next;
		kfree(fptr);
	}
}



int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	int i, j; //, spl;
	struct page_table *pt;
	struct page_table_entry *pte;
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

	/* don't copy the whole addrspace just yet. */
	newas->as_copied = 0;

	for (i = 0; i < INITIAL_SEGMENTS; ++i) {
		newas->as_segments[i].sg_vbase = old->as_segments[i].sg_vbase;
		newas->as_segments[i].sg_size = old->as_segments[i].sg_size;
		newas->as_segments[i].sg_flags = old->as_segments[i].sg_flags;
		newas->as_segments[i].sg_inuse = old->as_segments[i].sg_inuse;
		newas->as_segments[i].sg_dynamic = old->as_segments[i].sg_dynamic;
	}
	if (old->as_extra_segments != NULL) {
		/* declaring here as it is unlikey we'll ever reach here */
		struct segment_list *sg_ptr, *head, *new_sg_ptr;

		new_sg_ptr = head = NULL;
		sg_ptr = old->as_extra_segments;
		while (sg_ptr != NULL) {
			new_sg_ptr = kmalloc(sizeof(*new_sg_ptr));
			if (new_sg_ptr == NULL) {
				free_as_extra_segments(head);
				return ENOMEM;
			}
			new_sg_ptr->segment.sg_vbase = sg_ptr->segment.sg_vbase;
			new_sg_ptr->segment.sg_size = sg_ptr->segment.sg_size;
			new_sg_ptr->segment.sg_flags = sg_ptr->segment.sg_flags;
			new_sg_ptr->segment.sg_inuse = sg_ptr->segment.sg_inuse;
			new_sg_ptr->segment.sg_dynamic = sg_ptr->segment.sg_dynamic;
			new_sg_ptr->next = head;
			head = new_sg_ptr;

			sg_ptr = sg_ptr->next;
		}
		newas->as_extra_segments = head;
	}

	// spl = splhigh();
	/*
	 * just point to the current addrspace's page table
	 * we don't want to modify addrspace page table until process
	 * tries to access. That's because, the process may just call execv
	 * and all of our hardwork would be for nothing.
	 */
	for (i = 0; i < PAGE_TABLE_SIZE; ++i) {
		pt = old->as_page_table[i];
		if (pt == NULL)
			continue;
		pt->refcount++;
		for (j = 0; j <PAGE_TABLE_SIZE; ++j) {
			pte = pt->page_table_entries[j];
			if (pte == NULL)
				continue;
			pte->refcount++;
		}
		newas->as_page_table[i] = pt;
	}
	// splx(spl);

	*ret = newas;
	return 0;
}


int
as_actually_copy(struct addrspace *as)
{
	int i, j; //, spl;
	paddr_t addr, paddr;
	// const paddr_t *paddr;
	struct page_table *pt, *npt;
	struct page_table_entry *pte, *npte;

	KASSERT(as->as_copied == 0);

	vm_can_sleep();

	// spl = splhigh();

	as->as_copied = 1;

	for (i = 0; i < PAGE_TABLE_SIZE; ++i) {
		pt = as->as_page_table[i];
		if (pt == NULL)
			continue;

		/* TODO: Free page tables and page table entries */
		npt = kmalloc(sizeof(*npt));
		if (npt == NULL) {
			return ENOMEM;
		}
		npt->refcount = 1;
		for (j = 0; j < PAGE_TABLE_SIZE; ++j) {
			pte = pt->page_table_entries[j];
			if (pte == NULL)
				continue;
			npte = kmalloc(sizeof(*npte));
			if (npte == NULL) {
				return ENOMEM;
			}
			addr = alloc_kpages(1);
			if (addr == (paddr_t) NULL) {
				return ENOMEM;
			}
			paddr = pte->paddr;
			memmove((void *) addr, (const void *) (paddr + MIPS_KSEG0), PAGE_SIZE);
			npte->paddr = GET_PAGE_ADDRESS(addr);
			npte->refrenced = 1;
			npte->valid = 1;
			npte->flags = pte->flags;
			npte->refcount = 1;
			npt->page_table_entries[j] = npte;

			--pte->refcount;
		}
		--pt->refcount;

		as->as_page_table[i] = npt;
	}

	// splx(spl);

	return 0;
}



void
as_destroy(struct addrspace *as)
{

	int i, j;
	struct page_table *pt;
	struct page_table_entry *pte;

	for (i = 0; i < PAGE_TABLE_SIZE; ++i) {
		pt = as->as_page_table[i];
		if (pt == NULL)
			continue;
		for (j = 0; j <PAGE_TABLE_SIZE; ++j) {
			pte = pt->page_table_entries[j];
			if (pte == NULL)
				continue;
			if (--pte->refcount == 0)
				kfree(pte);
		}
		if (--pt->refcount == 0)
			kfree(pt);
	}
	kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable)
{
	int i;
	bool found_seg;

	found_seg = false;

	for (i = 0; i < INITIAL_SEGMENTS; ++i) {
		if (!as->as_segments[i].sg_inuse) {
			found_seg = true;
			as->as_segments[i].sg_inuse = 1;
			as->as_segments[i].sg_vbase = vaddr;
			as->as_segments[i].sg_size = memsize;
			as->as_segments[i].sg_dynamic = (memsize) ? 0 : 1;
			as->as_segments[i].sg_flags |= readable ? SG_READ: 0;
			as->as_segments[i].sg_flags |= writeable ? SG_WRITE : 0;
			as->as_segments[i].sg_flags |= executable ? SG_EXECUTE : 0;
			break;
		}
	}

	if (!found_seg) {
		/*
		 * Note: normally I don't like to declare variables so far into the function
		 * But, I don't like wasting memory either. So,  ¯\_(ツ)_/¯
		 */
		struct segment_list *extra_seg, *prev_seg;

		extra_seg = kmalloc(sizeof(*extra_seg));
		if (extra_seg == NULL)
			return ENOMEM;

		extra_seg->segment.sg_inuse = 1;
		extra_seg->segment.sg_vbase = vaddr;
		extra_seg->segment.sg_size = memsize;
		as->as_segments[i].sg_dynamic = (memsize) ? 0 : 1;
		extra_seg->segment.sg_flags |= readable ? SG_READ: 0;
		extra_seg->segment.sg_flags |= writeable ? SG_WRITE : 0;
		extra_seg->segment.sg_flags |= executable ? SG_EXECUTE : 0;

		prev_seg = (as->as_extra_segments == NULL) ? NULL : as->as_extra_segments;
		extra_seg->next = prev_seg;
		as->as_extra_segments = extra_seg;
	}

	return 0;
}


int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	*stackptr = USERSTACK;

	return as_define_region(as, USERSTACK, 0, SG_READ, SG_WRITE, 0);
}

