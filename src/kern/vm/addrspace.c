#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <thread.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */


/*
 * Wrap rma_stealmem in a spinlock.
 */
//static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;
struct addrspace *
as_create(void)
{
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	as->h_start =(vaddr_t) NULL;
	as->h_end   =(vaddr_t) NULL;
	as->pages= NULL;
	as->stack= NULL;
	as->heap = NULL;
	as->regions=NULL;
	return as;
}

void as_destroy(struct addrspace *as)
{
	if(as==NULL)
		{
		}
	else
	{
		struct pte *temp_pte=as->pages;
		struct pte *temp_store_pte;
		struct region *temp=as->regions;
		struct region *temp_store;
		
		while(temp!=NULL)
		{
			temp_store=temp;
			temp=temp->next;
			kfree(temp_store);
		}
		
		while(temp_pte!=NULL)
		{
			temp_store_pte=temp_pte;
			temp_pte=temp_pte->next;
			//paddr_t stackbase;
			free_upages(temp_store_pte->pa);
			kfree(temp_store_pte);
		
		}
		kfree(as);
	}
}


int as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,int readable, int writeable, int executable)
{
	size_t npages; 

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;
	if(as->regions==NULL)
	{
		as->regions=(struct region *)kmalloc(sizeof(struct region));
		as->regions->read=readable;
		as->regions->write=writeable;
		as->regions->exec=executable;
		as->regions->v_base=vaddr;
		as->regions->p_base=(paddr_t)NULL;
		as->regions->page_num=npages;
		as->regions->next=NULL;
		return 0;
	}
	struct region *new_region=as->regions;
	
	while(new_region->next!=NULL)
	{
		new_region=new_region->next;
	}
	new_region->next=(struct region *)kmalloc(sizeof(struct region));	
	new_region->next->read=readable;
	new_region->next->write=writeable;
	new_region->next->exec=executable;
	new_region->next->v_base=vaddr;
	new_region->next->p_base=(paddr_t)NULL;
	new_region->next->page_num=npages;
	//paddr_t pa_new=getppages(npages);
	//new_region->next->p_base=pa_new;
	new_region->next->next=NULL;
	return 0;
}

void as_activate(struct addrspace *as)
{
	int i, spl;

	(void)as;

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

int as_prepare_load(struct addrspace *as)
{
	struct region *region_temp=as->regions;
	//struct pte *temp_stack=as->stack;
	
	struct pte *temp=as->pages;
	struct pte *temp1=as->pages;
	vaddr_t heap_start;
	while(region_temp!=NULL)
	{
		int flg=0;
		vaddr_t v_addr=region_temp->v_base;
		//paddr_t region_pa;
		/* copy pointer of all the pages */
		size_t i=0;
		for(i=0;i<region_temp->page_num;i++)
		{
			if(temp1==NULL)
			{
				temp1=(struct pte *)kmalloc(sizeof(struct pte));				
				temp1->read=region_temp->read;
				temp1->va=v_addr;
				v_addr=v_addr+PAGE_SIZE;
				
				temp1->exec=region_temp->exec;
				paddr_t addr=getppages(1);
				//int p;
				KASSERT((addr & PAGE_FRAME)==addr);
				KASSERT((temp1->va & PAGE_FRAME) == temp1->va);
				KASSERT((v_addr & PAGE_FRAME)== v_addr);

				//p=tlb_entry(temp1->va, addr);
				if(i==0)
				region_temp->p_base=addr;
				temp1->write=region_temp->write;
				as->pages=temp1;
				temp=as->pages;
				as->pages->next=NULL;
				flg=1;
				if(addr!=0)
				{
					temp1->pa=addr;
					temp->pa=addr;
				}
				else
					return ENOMEM;
			}
			else{
				while(temp->next!=NULL)
				{
					temp=temp->next;
				}
				temp->next=(struct pte *)kmalloc(sizeof(struct pte));
				temp->next->read=region_temp->read;
				//temp->next->va=v_addr;
				paddr_t addr=getppages(1);
				temp->next->va=v_addr;
				//int p;
				temp->next->pa=addr;
				KASSERT((addr & PAGE_FRAME)==addr);
				KASSERT((temp->next->va & PAGE_FRAME) == temp->next->va);
				KASSERT((v_addr & PAGE_FRAME)== v_addr);
				//p=tlb_entry(temp->next->va, addr);
				v_addr=v_addr+PAGE_SIZE;
				if(i==0)
				region_temp->p_base=addr;
				temp->next->exec=region_temp->exec;
				temp->next->write=region_temp->write;
				temp->next->next=NULL;
				
			}
		}
		//flg=0;
		heap_start=v_addr;
		region_temp=region_temp->next;
	}
	
	vaddr_t stackbase=USERSTACK - (DUMBVM_STACKPAGES * PAGE_SIZE);
	//stacktop = USERSTACK;
	struct pte *temp_stack;
	temp_stack=as->pages;
    while(temp_stack->next!=NULL)
    {
        temp_stack=temp_stack->next;
    }
    int i=0;
    while(i<DUMBVM_STACKPAGES)
    {
        struct pte *for_stack=(struct pte *)kmalloc(sizeof(struct pte));
        temp_stack->next=for_stack;
        for_stack->va=stackbase+(i*PAGE_SIZE);
        //int qq;
	for_stack->next=NULL;
        paddr_t addr=getppages(1);
        //qq=tlb_entry(for_stack->va, addr);
         
        if(i==0)
            as->stack=for_stack;
        if(addr!=0)
            for_stack->pa=addr;
        else
            return ENOMEM;
        temp_stack=temp_stack->next;
        // just for safety, when next kmalloc on struct is called, it will make sure of NULL beforehand.
        i++;
    }
	//struct pte *for_stack=(struct pte *)kmalloc(sizeof(struct pte));
	//temp_stack->next=for_stack;
	//for_stack->va=stackbase+(PAGE_SIZE);
	//stackbase=stackbase+(PAGE_SIZE);
	//int qq;
	//paddr_t addr=getppages(1);
	//qq=tlb_entry(for_stack->va, addr);
	//for_stack->next=NULL; 
	
	struct pte *temp_heap=(struct pte *)kmalloc((sizeof(struct pte)));
	temp_stack->next = temp_heap;
	

	paddr_t padd = getppages(1);
	//int ss;
	//ss=tlb_entry(v_addr, padd);
	if(padd!=0){
		temp_heap->pa = padd;
		temp_heap->va = heap_start;;
		as->heap = temp_heap;
		as->heap->next = NULL;
	}
	else
	{
		return ENOMEM;
	}
	as->h_start =heap_start;
	as->h_end = heap_start;
	KASSERT(as->h_start != 0 && as->h_end !=0);
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}

int as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	//KASSERT(as->as_stackpbase != 0);
	//check
	if(as!=NULL){
	*stackptr = USERSTACK;
	return 0;
	}
	else
	{
	return -2;
	}
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}
	struct region *old_t;
	old_t=old->regions;
	struct region *new_t;
	if(new->regions==NULL)
	new->regions=(struct region *)kmalloc(sizeof(struct region));
	new_t=new->regions;
	int i=0;
	while(old_t!=NULL)
	{
		if(i==0)
		{
			
			//new_t=(struct region *)kmalloc(sizeof(struct region));
			new_t->p_base=old_t->p_base;
			new_t->v_base=old_t->v_base;
			new_t->page_num=old_t->page_num;
			new_t->read=old_t->read;
			new_t->write=old_t->write;
			new_t->exec=old_t->exec;
			new_t->next=NULL;
			old_t=old_t->next;
			i++;
		}
		else
		{
			while(new_t->next!=NULL)
			{
				new_t=new_t->next;
			}
			new_t->next=(struct region *)kmalloc(sizeof(struct region));
			new_t->next->p_base=old_t->p_base;
			new_t->next->v_base=old_t->v_base;
			new_t->next->page_num=old_t->page_num;
			new_t->next->read=old_t->read;
			new_t->next->write=old_t->write;
			new_t->next->exec=old_t->exec;
			new_t->next->next=NULL;
			old_t=old_t->next;
		}
	}
	/* (Mis)use as_prepare_load to allocate some physical memory. */
	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}
	struct pte *old_pte;
	struct pte *new_pte;
	struct pte *old_pte_stack;
	struct pte *new_pte_stack;
	new_pte_stack=new->stack;
	old_pte_stack=old->stack;
	int j=0;
	while(j<DUMBVM_STACKPAGES)
	{
		
		memmove((void *)PADDR_TO_KVADDR(new_pte_stack->pa),(const void *)PADDR_TO_KVADDR(old_pte_stack->pa),PAGE_SIZE);
		new_pte_stack=new_pte_stack->next;
		old_pte_stack=old_pte_stack->next;
		j++;
	}
	new_pte=new->pages;
	old_pte=old->pages;
	int count=0;
	while(old_pte!=NULL)
	{
		memmove((void *)PADDR_TO_KVADDR(new_pte->pa),(const void *)PADDR_TO_KVADDR(old_pte->pa),PAGE_SIZE);
		count++;
		new_pte=new_pte->next;
		old_pte=old_pte->next;
		
	}
	*ret = new;
	return 0;
}
int tlb_entry(vaddr_t va,paddr_t pa)
{
int i=0;
int spl;
spl=splhigh();
uint32_t ehi, elo;
for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) 
		{
			continue;
		}
		ehi = va;
		elo = pa | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", va, pa);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}
	return 0;
}

