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
 * You'll probably want to add stuff here.
 */


#include <machine/vm.h>

/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/
#define S_FREE 0		/* free for allocation */
#define S_DIRTY 1	/* not free, and there is no fresh copy of this on the disk */
#define S_CLEAN 2	/* not free, and there is an exact copy of this page stored on the disk */
#define S_FIXED 3	/* used by the kernel, do not touch these. */
#include <../../../user/include/sys/null.h>
#define PAGE_SIZE  4096        /* size of VM page */
#define PAGE_FRAME 0xfffff000 
#define PADDR_TO_KVADDR(paddr) ((paddr)+MIPS_KSEG0)
#define KVADDR_TO_PADDR(vaddr) ((vaddr)-MIPS_KSEG0)
#define DUMBVM_STACKPAGES 9
#include <limits.h>
#include <types.h>
#include <../../arch/mips/include/trapframe.h>
/*
  Structure to manage a physica page entry.
  vaddr: will be used in swapping
  page_state: DIRTY | CLEAN | FREE | FIXED
  chunk_size : the contiguous number of pages that were allocated.
  parent_addr: to go to the first page of the chunk (and then free)
  owner: Will be used in swapping (owner process)
  fifo related flags will be added later in swapping
*/

struct coremap_entry{
	vaddr_t va;
	int page_state;
	size_t chunk_size;
	paddr_t pa;
	// pid_t owner;
};
int vm_fault(int faulttype, vaddr_t faultaddress);
void vm_tlbshootdown_all(void);
void vm_tlbshootdown(const struct tlbshootdown *ts);
void free_kpages(vaddr_t va);
int getindex(paddr_t addr);
paddr_t getaddr(int index);
void vm_bootstrap(void);
paddr_t getppages(int npages);
void free_upages(paddr_t pa);
vaddr_t alloc_kpages(int npages);


#endif /* _VM_H_ */
