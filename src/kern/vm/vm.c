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
//#include <coremap.h>


//#define DIVROUNDUP(a,b) (((a)+(b)+1)/b)
// Already defined #define PAGE_SIZE = 4096
//#define ROUNDUP(a,b) (DIVROUNDUP(a,b)*b)

struct coremap_entry *coremap;
//int base; // all adresses before this shouldnt be into our coremap.
static int total_page_num;
paddr_t firstaddr;
paddr_t freeaddr;
paddr_t lastaddr;
int kernel_indices;

static struct spinlock vm_lock = SPINLOCK_INITIALIZER;
static bool has_vm_booted = false;
int swap;

void vm_bootstrap(void){

// Probably create a coremap_entry array[i.e. the coremap ::table],
// Get the number of pages(occupied mem/page size) that are already used by the kernel,
// create page entries for that number of pages and fill it with "FIXED" (So that we wo'nt ever toch them)
// Mark rest of the entries as "FREE"


	ram_getsize(&firstaddr, &lastaddr);
//  	base = firstaddr/PAGE_SIZE; 
 	total_page_num = (lastaddr-firstaddr) / PAGE_SIZE;
	paddr_t paddr;
	swap=0;
	paddr=getaddr(total_page_num-1);
	freeaddr = firstaddr + (total_page_num)*(sizeof(struct coremap_entry));
	freeaddr = ROUNDUP(freeaddr,PAGE_SIZE);
  	coremap = (struct coremap_entry*)PADDR_TO_KVADDR(firstaddr);
  	kernel_indices = ROUNDUP(freeaddr-firstaddr,PAGE_SIZE)/PAGE_SIZE;
	
	int i;

// Now we will fill the pages as FREE (We have started the index from )
	for(i=0;i<total_page_num;i++){
		if(i<=kernel_indices) {
			coremap[i].page_state=S_FIXED;
		}
		else{
		coremap[i].page_state = S_FREE;
		coremap[i].va=(vaddr_t)NULL;
		coremap[i].chunk_size=1;
		coremap[i].pa=(paddr_t)NULL;
		} // No parent right now, we can also set this value as own adress.
}
has_vm_booted = true;
// TODO: Something according to jshshi.me blog. Nore sure what.

}

paddr_t getaddr(int index){
  return ((index*PAGE_SIZE) + firstaddr);
}
int getindex(paddr_t addr){
  return (addr-firstaddr)/PAGE_SIZE;
}

/*
acquire lock first!
The function will return the address of the first page for the newly assigned npages number of pages.

Without swapping:
  1. Locate a page entry in the coremap array which has "S_FREE" page
  2. See if there are contiguous npage number of free pages.
  3  If yes, mark all the pages' page_state as "S_DIRTY" and add the first page's address in the pa and add chunk_size as npage and return the pa
  4. If not, move on to find another page.
*/
vaddr_t alloc_kpages(int npages){
if(has_vm_booted){
	spinlock_acquire(&vm_lock);
	int i; 
	//vm_bootstrap();
	int flag=0;
	 if(npages==1){
		for(i=0;i<total_page_num;i++){
			if(coremap[i].page_state==S_FREE){
				// Assign the page
				flag=1;
				
				coremap[i].page_state=S_CLEAN;
				coremap[i].chunk_size=1;
				coremap[i].va=PADDR_TO_KVADDR(getaddr(i));
				coremap[i].pa=getaddr(i);
				spinlock_release(&vm_lock);
				return  coremap[i].va;
			}
		}
		panic("Not a single free page found!");
  	}
	else if(npages>1){
		i=0;
		int flag3=0;
		while(i<total_page_num)
		{
			if(coremap[i].page_state==S_FREE)
			{
				// Now we have to see if there nare contigous npages free..
				int j;
				int flag2=0;
				for(j=i;j<i+npages;j++)
				{
					if(j>total_page_num)
							{
								flag2=1;
								break;
							}
					if(coremap[j].page_state!=S_FREE) flag2=1;
				}
				if(flag2==0)
				{
					// It means there are total npages contiguous free pages. ALOCATE THEM ALL
					paddr_t temp_2 = coremap[i].pa;
					for(j=i;j<i+npages;j++)
					{
						
						coremap[j].page_state=S_CLEAN;
						coremap[j].chunk_size=npages;
						coremap[j].va=PADDR_TO_KVADDR(temp_2);
						coremap[j].pa=temp_2;
					}
					flag3=1;
					spinlock_release(&vm_lock);
					return  PADDR_TO_KVADDR(temp_2);
				} 
				else{
				flag2=0;
				i=j;
				// move on to find more pages	
				}
			}
			i++;
		}
	
	if(flag3==0){
	spinlock_release(&vm_lock); // That means no contiguos npages found.	
	panic("No contiguous npages found");
	return (vaddr_t) NULL;
	}
	}
	else
	{
		spinlock_release(&vm_lock);
		panic("requesting bad number of npages");}
	
		return (vaddr_t)NULL;
	}
else{
spinlock_acquire(&vm_lock);
paddr_t temp3 = ram_stealmem(npages);
spinlock_release(&vm_lock);
return PADDR_TO_KVADDR(temp3);
}
	
}

/*
Acquire lock first!
Find the index for that address
if the address is not pa itself then will go to pa
now from that index to index+chunk_size pages will be markd free. Done! */

void free_kpages(vaddr_t va){
	if(has_vm_booted){
	spinlock_acquire(&vm_lock);
	vaddr_t address = (va);
	int i=0;
	for(i=0;i<total_page_num;i++)
	{
		if(coremap[i].va==address)
		{
			size_t j=0;
			size_t s=coremap[i].chunk_size;
			for(j=0;j<s;j++)
			{
				coremap[i+j].va=(vaddr_t) NULL;
				coremap[i+j].page_state=S_FREE;
				coremap[i+j].pa=(paddr_t) NULL;
				coremap[i+j].chunk_size=0;
			}
			break;
		}
	}
	spinlock_release(&vm_lock);
	}
	else
	{
		// Do nothing.
	}
}
paddr_t getppages(int npages)
{
	if(has_vm_booted){
	spinlock_acquire(&vm_lock);
	int i; 
	int flag=0;
	 if(npages==1){
		for(i=0;i<total_page_num;i++){
			if(coremap[i].page_state==S_FREE){
				// Assign the page
				flag=1;
				if(swap==0)
					swap=i;
				coremap[i].page_state=S_CLEAN;
				coremap[i].chunk_size=1;
				//coremap[i].va=PADDR_TO_KVADDR(getaddr(i));
				coremap[i].pa=getaddr(i);
				spinlock_release(&vm_lock);
				return  coremap[i].pa;
			}
		}
		coremap[4+(swap%(total_page_num-5)].page_state=S_CLEAN;
		coremap[4+(swap%(total_page_num-5)].chunk_size=1;
		//coremap[i].va=PADDR_TO_KVADDR(getaddr(i));
		coremap[4+((swap++)%(total_page_num-5)].pa=getaddr(i);
		spinlock_release(&vm_lock);
		return  coremap[4+(swap%(total_page_num-5)].pa;
		panic("Not a single free page found!");
  	}
	else if(npages>1){
		i=0;
		int flag3=0;
		while(i<total_page_num)
		{
			if(coremap[i].page_state==S_FREE)
			{
				if((i+npages)>=total_page_num)
				{
					flag3=0;
					break;
				}
				// Now we have to see if there nare contigous npages free..
				int j;
				int flag2=0;
				for(j=i;j<i+npages;j++){
					if(coremap[j].page_state!=S_FREE) flag2=1;
				}
				if(flag2==0)
				{
					// It means there are total npages contiguous free pages. ALLOCATE THEM ALL
					coremap[i].pa=getaddr(i);
					paddr_t temp_2 = coremap[i].pa;
					for(j=i;j<i+npages;j++)
					{
						coremap[j].page_state=S_CLEAN;
						coremap[j].chunk_size=npages;
						coremap[j].va=PADDR_TO_KVADDR(temp_2);
						coremap[j].pa=temp_2;
					}
					flag3=1;
					spinlock_release(&vm_lock);
					return  temp_2;
				} 
				else{
				flag2=0;
				i=j;
				// move on to find more pages	
				}
			}
			i++;
		}
	
	if(flag3==0){
	for(i=swap;i<total_page_num;i++)
	{
		coremap[j].page_state=S_CLEAN;
		coremap[j].chunk_size=npages;
		coremap[j].va=PADDR_TO_KVADDR(temp_2);
		coremap[j].pa=temp_2;
	}
	spinlock_release(&vm_lock); // That means no contiguos npages found.	
	panic("No contiguous npages found");
	return (paddr_t) NULL;
	}
	}
	else {
		spinlock_release(&vm_lock);
		panic("requesting bad number of npages");}
	
	return (vaddr_t)NULL;
}

else{
spinlock_acquire(&vm_lock);
paddr_t temp3 = ram_stealmem(npages);
spinlock_release(&vm_lock);
return (temp3);
}
	
}




void free_upages(paddr_t pa)
{
	spinlock_acquire(&vm_lock);
	int i=0;
	for(i=0;i<total_page_num;i++)
	{
		if(coremap[i].pa==pa)
		{
		
			size_t j=0;
			size_t s=coremap[i].chunk_size;
			for(j=0;j<s;j++)
			{
				coremap[i+j].va=(vaddr_t) NULL;
				coremap[i+j].page_state=S_FREE;
				coremap[i+j].pa=(paddr_t) NULL;
				coremap[i+j].chunk_size=0;
			}
		}
	}
	spinlock_release(&vm_lock);
}



void vm_tlbshootdown_all(void){
panic("vm_tlbshootdown_all is not implemented yet");
}

void vm_tlbshootdown(const struct tlbshootdown *ts){
(void)ts;
panic("vm_tlbshootdown is not implemented yet");
}


int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	vaddr_t stackbase, stacktop;
	paddr_t paddr;
	int i;
	uint32_t ehi, elo;
	struct addrspace *as;
	int spl;
	//KASSERT(faultaddress<MIPS_KSEG0);
	
	faultaddress &= PAGE_FRAME;

	DEBUG(DB_VM, "dumbvm: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	}
	if(faultaddress<10000)
	{	
		i=0;
	}

	as = curthread->t_addrspace;
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		return EFAULT;
	}
	//as->pages->va &=PAGE_FRAME;
	/* Assert that the address space has been set up properly. */
	KASSERT(as->pages != NULL);
	KASSERT(as->stack != NULL);
	KASSERT(as->heap != NULL);
	KASSERT(as->h_start != 0);
	KASSERT(as->h_end != 0);
	KASSERT(as->regions != NULL);
	KASSERT((as->pages->va & PAGE_FRAME) == as->pages->va);
	//KASSERT((as->as_pbase1 & PAGE_FRAME) == as->as_pbase1);
	struct pte *itr;
	int p_flag=0;
	stackbase = USERSTACK - DUMBVM_STACKPAGES * PAGE_SIZE;
	stacktop = USERSTACK;
	itr=as->stack;
	struct pte *itr_page=as->pages;
	KASSERT(itr!=NULL);
	KASSERT(itr_page!=NULL);
	int flag=0;
	if (faultaddress >= stackbase && faultaddress < stacktop) {
		while(itr->next!=NULL)
		{
			if(faultaddress >= itr->va && faultaddress < ((itr->va)+PAGE_SIZE))
			{
				paddr = (faultaddress - itr->va) + itr->pa;
				flag=1;
				break;
			}
			itr=itr->next;
		}
	}
	else
	{
		while(itr_page->next!=NULL)
		{
			if(faultaddress >= itr_page->va && faultaddress < ((itr_page->va)+PAGE_SIZE))
			{
				paddr = (faultaddress - itr_page->va) + itr_page->pa;
				flag=1;
				break;
			}
			p_flag++;
			itr_page=itr_page->next;
			if(itr_page->next==NULL)
			{
				
				if(faultaddress >= itr_page->va && faultaddress < ((itr_page->va)+PAGE_SIZE))
				{
					paddr = (faultaddress - itr_page->va) + itr_page->pa;
					flag=1;
					break;
				}
			}
		}
		
	}
	if(flag!=1)
	{
		itr_page->next=(struct pte *)kmalloc(sizeof(struct pte));
		 paddr=getppages(1);
		itr_page->next->pa=paddr;
		itr_page->next->va=itr_page->va+PAGE_SIZE;
		//as->h_end=
		itr_page->next->read=itr_page->read;
		itr_page->next->write=itr_page->write;
		itr_page->next->exec=itr_page->exec;
		itr_page->next->next=NULL;	
	}
	//paddr_t pa_vm=getppages(1);
	
	/* make sure it's page-aligned */
	KASSERT((paddr & PAGE_FRAME) == paddr);

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		tlb_write(ehi, elo, i);
		splx(spl);
		return 0;
	}

	ehi = faultaddress;
	elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
	DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
	tlb_random(ehi, elo);
	splx(spl);
	return 0;
}



