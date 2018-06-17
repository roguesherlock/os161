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
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>
#include <elf.h>
#include <current.h>
#include <spl.h>
#include <machine/tlb.h>


/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL)
		return NULL;

	as->as_copied = 1;

	as->as_page_table = NULL;
	as->as_segments = NULL;

	return as;
}


int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	int spl;
	struct addrspace *newas;
	struct segment *sg_ptr;
	struct page_table_entry *pte_ptr;


	KASSERT (old != NULL);
	KASSERT (ret != NULL);
	KASSERT (old->as_copied == 1);

	newas = kmalloc (sizeof(*newas));
	if (newas == NULL)
		return ENOMEM;

	newas->as_copied = 0;

	spl = splhigh();

	for(sg_ptr = old->as_segments; sg_ptr != NULL; sg_ptr = sg_ptr->next)
		sg_ptr->sg_refcount++;

	for (pte_ptr = old->as_page_table; pte_ptr != NULL; pte_ptr = pte_ptr->next)
		pte_ptr->pte_refcount++;

	newas->stack.sg_vbase = old->stack.sg_vbase;
	newas->stack.sg_size = old->stack.sg_size;
	newas->stack.sg_flags = old->stack.sg_flags;
	newas->stack.sg_refcount = 1;
	newas->stack.next = NULL;

	newas->heap.sg_vbase = old->heap.sg_vbase;
	newas->heap.sg_size = old->heap.sg_size;
	newas->heap.sg_flags = old->heap.sg_flags;
	newas->heap.sg_refcount = 1;
	newas->heap.next = NULL;

	splx(spl);

	newas->as_segments = old->as_segments;
	newas->as_page_table = old->as_page_table;

	*ret = newas;

	return 0;
}


int
as_actually_copy(struct addrspace *as)
{
	int spl, result;
	struct segment *sg_ptr, *new_sg;
	struct page_table_entry *pte_ptr, *new_pte;

	KASSERT (as != NULL);
	KASSERT (as->as_copied == 0);

	as->as_copied = 1;

	sg_ptr = as->as_segments;
	as->as_segments = NULL;		/* to make sure it now points to its own segments */
	for (; sg_ptr != NULL; sg_ptr = sg_ptr->next) {
		new_sg = kmalloc (sizeof(*new_sg));
		if (new_sg == NULL)
			return ENOMEM;

		new_sg->sg_vbase = sg_ptr->sg_vbase;
		new_sg->sg_size = sg_ptr->sg_size;
		new_sg->sg_flags = sg_ptr->sg_flags;
		new_sg->sg_refcount = 1;
		new_sg->next = as->as_segments;
		as->as_segments = new_sg;

		spl = splhigh();
		sg_ptr->sg_refcount--;
		splx(spl); /* lol! don't do this on real systems */
	}

	pte_ptr = as->as_page_table;
	as->as_page_table = NULL;	/* to make sure it now points to its own pte's */
	for (; pte_ptr != NULL; pte_ptr = pte_ptr->next) {
		result = as_add_new_pte(as, pte_ptr->pte_vaddr, pte_ptr->pte_flags, &new_pte);
		if (result)
			return result;

		memcpy((void *) PADDR_TO_KVADDR(new_pte->pte_paddr), (const void *) PADDR_TO_KVADDR(pte_ptr->pte_paddr), PAGE_SIZE);
		spl = splhigh();
		pte_ptr->pte_refcount--;
		splx(spl); 	/* Seriously! don't do this on real systems*/
	}

	return 0;
}


void
as_destroy(struct addrspace *as)
{
	struct segment *sg_ptr, *sg_tmp;
	struct page_table_entry *pte_ptr, *pte_tmp;

	KASSERT(as != NULL);

	sg_ptr = as->as_segments;
	while (sg_ptr != NULL) {
		sg_tmp = sg_ptr;
		sg_ptr = sg_ptr->next;
		if (--sg_tmp->sg_refcount == 0)
			kfree(sg_tmp);
	}

	pte_ptr = as->as_page_table;
	while (pte_ptr != NULL) {
		pte_tmp = pte_ptr;
		pte_ptr = pte_ptr->next;
		if (--pte_tmp->pte_refcount == 0) {
			/* as we explicitly used alloc_kpages when creating */
			free_kpages(PADDR_TO_KVADDR(pte_tmp->pte_paddr));
			kfree(pte_tmp);
		}
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
	KASSERT (as != NULL);

	struct segment *new_sg;

	new_sg = kmalloc(sizeof(*new_sg));
	if (new_sg == NULL)
		return ENOMEM;

	/* Align the region. First, the base... */
	memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;


	new_sg->sg_vbase = vaddr;
	new_sg->sg_size = memsize;
	new_sg->sg_flags = readable | writeable | executable;
	new_sg->sg_refcount = 1;
	new_sg->next = as->as_segments;

	as->as_segments = new_sg;

	return 0;
}

int
as_prepare_load(struct addrspace *as)
{
	vaddr_t heap_base;
	struct segment *sg_ptr;

	KASSERT (as != NULL);
	KASSERT (as->as_segments != NULL);

	heap_base = 0;

	/* determine heap base */
	for (sg_ptr = as->as_segments; sg_ptr != NULL; sg_ptr = sg_ptr->next)
		if (sg_ptr->sg_vbase > heap_base)
			heap_base = sg_ptr->sg_vbase + sg_ptr->sg_size;

	heap_base += PAGE_SIZE + (((random() % HEAP_OFFSET) * PAGE_SIZE) & PAGE_FRAME);

	as->heap.sg_vbase = heap_base;
	as->heap.sg_size = 0;
	as->heap.sg_flags = PF_R | PF_W;
	as->heap.sg_refcount = 1;
	as->heap.next = NULL;

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void) as;

	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	KASSERT (as != NULL);

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	as->stack.sg_vbase = USERSTACK;
	as->stack.sg_size = STACK_CHUNK * PAGE_SIZE;
	as->stack.sg_flags = PF_R | PF_W;
	as->stack.sg_refcount = 1;
	as->stack.next = NULL;

	return 0;
}


bool
as_is_addr_valid (struct addrspace *as, vaddr_t vaddr)
{
	vaddr_t sg_end;
	struct segment *sg_ptr;

	KASSERT (as != NULL);

	for (sg_ptr = as->as_segments; sg_ptr != NULL; sg_ptr = sg_ptr->next) {
		sg_end = sg_ptr->sg_vbase + sg_ptr->sg_size;
		if (vaddr >= sg_ptr->sg_vbase && vaddr < sg_end)
			return true;
	}

	sg_end = as->stack.sg_vbase - as->stack.sg_size; // - (STACK_CHUNK * PAGE_SIZE);
	if (vaddr <= as->stack.sg_vbase && vaddr >= sg_end)
		return true;

	sg_end = as->heap.sg_vbase + as->heap.sg_size;
	if (vaddr >= as->heap.sg_vbase && vaddr < sg_end)
		return true;

	return false;

}


struct page_table_entry *
as_get_pte(struct addrspace *as, vaddr_t vaddr)
{
	struct page_table_entry *pte_ptr;

	KASSERT (as != NULL);
	KASSERT ((vaddr & PAGE_FRAME) == vaddr);

	for (pte_ptr = as->as_page_table; pte_ptr != NULL; pte_ptr = pte_ptr->next)
		if (pte_ptr->pte_vaddr == vaddr)
			return pte_ptr;

	return NULL;
}



unsigned
as_get_flags (struct addrspace *as, vaddr_t vaddr)
{
	vaddr_t sg_end;
	struct segment *sg_ptr;

	KASSERT (as != NULL);
	// KASSERT ((vaddr & PAGE_FRAME) == vaddr);


	for (sg_ptr = as->as_segments; sg_ptr != NULL; sg_ptr = sg_ptr->next) {
		sg_end = sg_ptr->sg_vbase + sg_ptr->sg_size;
		if (vaddr >= sg_ptr->sg_vbase && vaddr < sg_end)
			return sg_ptr->sg_flags;
	}

	sg_end = as->stack.sg_vbase - as->stack.sg_size;// - (STACK_CHUNK * PAGE_SIZE);
	if (vaddr <= as->stack.sg_vbase && vaddr >= sg_end)
		return as->stack.sg_flags;

	sg_end = as->heap.sg_vbase + as->heap.sg_size;
	if (vaddr >= as->heap.sg_vbase && vaddr < sg_end)
		return as->heap.sg_flags;

	return 0;
}



int
as_add_new_pte (struct addrspace *as, vaddr_t vaddr, unsigned flags, struct page_table_entry **pte)
{
	vaddr_t addr;
	struct page_table_entry *new_pte;

	KASSERT (as != NULL);
	KASSERT ((vaddr & PAGE_FRAME) == vaddr);

	addr = alloc_kpages(1);
	if (addr == (vaddr_t) NULL)
		return ENOMEM;

	new_pte = kmalloc(sizeof(*new_pte));
	if (new_pte == NULL) {
		return ENOMEM;
	}

	// bzero((void *) addr, PAGE_SIZE);

	new_pte->pte_vaddr = vaddr;
	new_pte->pte_paddr = GET_PHYSICAL_ADDR(addr);
	new_pte->pte_flags = flags;
	new_pte->pte_refcount = 1;
	new_pte->pte_refrenced = 1;
	new_pte->pte_in_memory = 1;
	new_pte->next = as->as_page_table;

	as->as_page_table = new_pte;

	if (pte)
		*pte = new_pte;

	return 0;
}


void
as_rm_pte (struct addrspace *as, vaddr_t vaddr)
{
	struct page_table_entry *pte_ptr, *prev_pte_ptr;

	KASSERT (as != NULL);
	KASSERT (curproc->p_addrspace == as);

	prev_pte_ptr = pte_ptr = as->as_page_table;

	while ((pte_ptr) && pte_ptr->pte_vaddr != vaddr) {
		prev_pte_ptr = pte_ptr;
		pte_ptr = pte_ptr->next;
	}

	if ((pte_ptr) && pte_ptr->pte_vaddr == vaddr) {
		if (pte_ptr == prev_pte_ptr) /* head */
			as->as_page_table = pte_ptr->next;
		else
			prev_pte_ptr->next = pte_ptr->next;
		free_kpages(PADDR_TO_KVADDR(pte_ptr->pte_paddr));
		kfree(pte_ptr);
	}
}