
#include <kern/processcalls.h>
#include <kern/filecalls.h>
#include <thread.h>
#include <../include/synch.h>
#include <kern/wait.h>
#include <copyinout.h>
#include <lib.h>
#include <test.h>
#include <kern/fcntl.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <limits.h>
#include <types.h>
#include <current.h>
#include <kern/errno.h>
#include <kern/wait.h>
#include <../include/spl.h>
#include <synch.h>
#include <wchan.h>
#include <kern/filecalls.h>


#define FALSE 0
#define TRUE 1
#define HEAP_MAX 0x40000000

void entrypoint(void* data1, unsigned long data2)
 { 
	struct addrspace *fork_addr;
	fork_addr=(struct addrspace*) data2;
	struct trapframe *fork_frame;
	fork_frame=(struct trapframe*) data1;
	fork_frame->tf_a3 = 0;
	fork_frame->tf_v0 = 0;
	fork_frame->tf_epc =fork_frame->tf_epc+4;
	curthread->t_addrspace = fork_addr;
	as_activate(fork_addr);
	struct trapframe tf; // tf will be allocated on child's kernel stack
	tf = *fork_frame ;
	mips_usermode(&tf);
}

/******************************************************************** assign_pid call **************************************************************/
pid_t assign_pid()
{
	////kprintf("assign_pid:I am being called\n");

	int i=1;
	int j=0;
	int flag=0;
	for(i=2;i<PID_MAX;i++)
	{	
		if(process_table[i]==NULL)
		{
			process_table[i]= (struct process *) kmalloc (sizeof(struct process));
			if(flag==0)
				{
					j=i;
					flag=1;
					break;
				}
		}
	}
	////kprintf("assign pid %3d\n",j);
	return  j;
}

void process_init(struct thread *t, pid_t id)
{		
		process_table[t->pid]->ppid= -1;
		process_table[t->pid]->pid=t->pid;
		process_table[t->pid]->exitflag = FALSE;
		process_table[t->pid]->exitcode = 0;
		if(t->pid==id)
			process_table[id]->procthread=t;
		else	
			process_table[t->pid]->procthread=t;
		process_table[t->pid]->exitlock=lock_create("lock");
}


/*********************************************************************  process_destroy call ************************************************************************/

void process_destroy(pid_t pid)
{
	process_table[pid]=NULL;
	//lock_destroy(process_table[pid]->exitlock);
	//thread_destroy(process_table[pid]->procthread);
	kfree(process_table[pid]);
	////kprintf("destroyed\n");
}


/********************************************************************* getpid call ************************************************************************/

pid_t sys_getpid(pid_t  *retval)
{
	*retval=(curthread->pid);
	////kprintf("pid:%d\n",curthread->pid);
	return curthread->pid;
}


/********************************************************************* exit call ************************************************************************/

void sys__exit(int exitcode)
{

	if (curthread->pid == 11) {
		////kprintf ("This is 11\n");
	}
	//kprintf("exit\n");
	if(process_table[curthread->pid]->ppid<1)
	{
	}
	else
	{
			lock_acquire(process_table[curthread->pid]->exitlock);
			process_table[curthread->pid]->exitcode=_MKWAIT_EXIT(exitcode);
			process_table[curthread->pid]->exitflag=TRUE;
			//process_table[process_table[curthread->pid]->ppid]->procthread->filetable[2]->ref_count=process_table[process_table[curthread->pid]->ppid]->procthread->filetable[2]->ref_count-1;
	
		////kprintf("C: ppid & ref_count:%3d  %3d   %3d\n",curthread->pid,process_table[curthread->pid]->ppid,process_table[process_table[curthread->pid]->ppid]->procthread->filetable[2]->ref_count);
		lock_release(process_table[curthread->pid]->exitlock);
	}
	//process_table[curthread->pid]->exitflag=TRUE;
	////kprintf("C:%3d\n",curthread->pid);
	////kprintf("P: %3d\n",process_table[curthread->pid]->ppid);
	//kprintf ("Exiting: %3d\n", curthread->pid);
	thread_exit();
	//process_destroy(curthread->pid);
	//RIGHT THREAD EXIT OR NOT

}

/********************************************************************* wait call ************************************************************************/

pid_t sys_waitpid(pid_t pid, userptr_t status, int options, int *retval) {
	
	//kprintf("pid:waitpid:%d\n",pid);
	if (options != 0) 
	{
		return EINVAL;
	}
	if (status == NULL ) 
	{
		////kprintf("status\n");
		return EFAULT;
	}
	if (status == (userptr_t)0X40000000)
	{
		//kprintf("in val\n");
		return EFAULT;
	}
	if (status == (userptr_t)0X80000000)
	{
		//kprintf("in val\n");
		return EFAULT;
	}
	if (process_table[pid] == NULL ) {
		return ESRCH;
	}
	if (pid==curthread->pid) 
	{
		//kprintf("ok\n");
		return ECHILD;
	}
	/*if(curthread->pid!=process_table[pid]->ppid)
	{
		//kprintf("!=ok\n");
		return ECHILD;
	}*/
	if(pid ==process_table[curthread->pid]->ppid)
	{
		//kprintf("pid== process ppid true\n");
		return ECHILD;
	}
	if (pid < PID_MIN) 
	{
		return EINVAL;
	}
	if (pid > PID_MAX) 
	{
		return ESRCH;
	}
	
	if (process_table[pid]->exitflag == FALSE && process_table[pid]->ppid==curthread->pid) 
	{
		lock_acquire(process_table[pid]->exitlock);
	
		while(process_table[pid]->exitflag == FALSE)
		{
			
			lock_release(process_table[pid]->exitlock);
			wchan_lock(process_table[pid]->exitlock->lk_wchan);
			wchan_sleep(process_table[pid]->exitlock->lk_wchan);
                 	lock_acquire(process_table[pid]->exitlock);
			//kprintf("wait pid:%d.\n",pid);	
		}
		
		lock_release(process_table[pid]->exitlock);
	}
	//kprintf("W:check:%3d\n",pid);
	int flg;
	struct process* child;
	child=process_table[pid];
	flg = copyout((const void *) (&process_table[pid]->exitcode), status,sizeof(int));
	*retval = pid;
	if(process_table[pid]->ppid==curthread->pid)
		process_destroy(pid);
	return 0;
}

/********************************************************************* fork call ************************************************************************/


pid_t sys_fork(struct trapframe *tf, pid_t *retval)
{
	struct addrspace *forkc_addrspace;
	int flg;
	//kprintf("");
	if(curthread->t_addrspace==NULL)
	{
		forkc_addrspace=(struct addrspace*)kmalloc(sizeof(struct addrspace));
		if(forkc_addrspace==NULL)
		{
			return EFAULT;
		}
	}
	else{
		flg = as_copy(curthread->t_addrspace, &forkc_addrspace);
		if(flg)
		{
			return ENOMEM;
		}
	}
	struct trapframe *tf_fork= (struct trapframe*)kmalloc(sizeof(struct trapframe));
	if(tf_fork==NULL)
	{
		return ENOMEM;
	}
	if (tf == NULL ) {
		////kprintf("parent have a problem\n");
		return ENOMEM;
	}
	if (tf_fork == NULL ) {
		////kprintf("problem in fork\n");
		////kprintf("FORK: kmalloc for trapframe failed\n");
		return ENOMEM;
	}
	*tf_fork =* tf;
	
	struct thread * fork_thread;
	////kprintf("fork P:%3d\n",curthread->pid);
	flg = thread_fork("fork", (void*)entrypoint,(struct trapframe *) tf_fork,(unsigned long) forkc_addrspace,&fork_thread);
	////kprintf("fork P:%3d fork C:%3d\n",curthread->pid,flg);
	if (flg)
	 {
		//kprintf("enomem problem\n");
		return ENOMEM;
	 }
	//kfree(tf_fork);
	//kfree(forkc_addrspace);
	*retval = fork_thread->pid;
	return 0;
}

/**                                     ***/
int sys_execv(const char *programname,char **uargs)
{
    char* buffer;

    int flg;
    size_t length=0;
    vaddr_t entrypoint, stackptr;
	//lock_acquire(lock_exec);
    
       if(programname==NULL || ((userptr_t)programname<=(userptr_t)0X40000000 &&(userptr_t)programname>=(userptr_t)0X20000000 )|| (userptr_t)programname>=(userptr_t)0X80000000)
       {
        //kprintf("Null programname \n");
            return EFAULT;
        }
	
        if(uargs==NULL || (userptr_t)uargs==(userptr_t)0X40000000 || (userptr_t)uargs>=(userptr_t)0X80000000 )
        {
       // kprintf("uargs\n");
            return EFAULT;
        }
	if(uargs[0]==NULL)
		return EINVAL;
     	 buffer=(char*) kmalloc (sizeof(char)*PATH_MAX);
   	 flg=copyinstr((const_userptr_t)programname,buffer,PATH_MAX,&length);
	//kprintf("length %d:\n",length);
   	 if(length==1)
		return EINVAL;
    int i=0;
    if(length==0)
	return EINVAL;
    char** args=(char**)kmalloc(sizeof(char**));
    flg=copyin((const_userptr_t)uargs,args,sizeof(char**));
    while (uargs[i] != NULL ) {
	if((userptr_t)uargs[i]==(userptr_t)0X40000000 || (userptr_t)uargs[i]>=(userptr_t)0X80000000 )
      		{
      		    
	            return EFAULT;
        	}
        args[i] = (char *) kmalloc(sizeof(char) * PATH_MAX);
        int result;
        result = copyinstr((const_userptr_t) uargs[i], args[i], PATH_MAX,&length);
	if(length==1)
		return EINVAL;
        i++;
    }
	//lock_acquire(lock_exec);
    args[i] = NULL;
    int p=i;
    i=0;
    struct vnode *vn;
    flg = vfs_open(buffer, O_RDONLY, 0, &vn);
    //kprintf("execv:%2d\n",4);
    struct addrspace *temp;
    temp =curthread->t_addrspace;
    if(curthread->t_addrspace != NULL)
    {
		as_destroy(curthread->t_addrspace);
		curthread->t_addrspace = NULL;
     }

    KASSERT(curthread->t_addrspace == NULL);
    curthread->t_addrspace = as_create();
    if(curthread->t_addrspace==NULL)
	return EFAULT;
    as_activate(curthread->t_addrspace);
    //kprintf("execv:%2d\n",5);
    flg = load_elf(vn, &entrypoint);
    vfs_close(vn);
    flg = as_define_stack(curthread->t_addrspace, &stackptr); 
    i=0;
    while(args[i]!=NULL)
    {
        size_t len;
        size_t padded;
        len=strlen(args[i])+1;
        int k;
        char *for_stack;
        k=(len)%4;
        if(k==0)
        {
            padded=len+4;
        }
        else
        {
            padded=len+4-k;
        }
        int q;
        q=(int)(padded-len+1);
        for_stack=kmalloc(sizeof(char)*(padded));
        for_stack=kstrdup(args[i]);
        for(int j=0;j<(int)padded;j++)
            {
                if (j >= (int)len)
                    for_stack[j] = '\0';
                else
                    for_stack[j] = args[i][j];
                
            }
        stackptr= stackptr-padded;
        flg = copyout((const void *) for_stack, (userptr_t) stackptr,(size_t)len);
        kfree(for_stack);
        args[i] = (char *) stackptr;
        i++;
    }
         if(args[i]==NULL){
        stackptr =stackptr-( 4 * sizeof(char));
    }
    for(int j=(i-1); j>= 0;j--) {
        stackptr=stackptr-sizeof(char*);
        flg = copyout((const void *)(args+j), (userptr_t)stackptr, (sizeof(char *)));
        }
	//lock_release(lock_exec);
	kfree(buffer);
	kfree(args);
   enter_new_process(p /*argc*/,(userptr_t) stackptr /*userspace addr of argv*/, stackptr,entrypoint);
    
    /* enter_new_process does not return. */
    panic("enter_new_process returned\n");
    return EINVAL;
}






int sys_sbrk(intptr_t amount,int *retval )
{
	struct addrspace *as = curthread->t_addrspace;
	if(as==NULL){
		panic("can't perform sbrk on null addrspace.");
		}

	KASSERT(as->h_end!=(vaddr_t)NULL);
	if(amount%4!=0)
		return EINVAL;
	if(amount>=1073741824)
		return ENOMEM;
	if(amount<=-1073741824)
		return EINVAL;

	if(amount==0){
		*retval= as->h_end;
		return 0;
		// Return current location of heap end
		// Now have to make sure that inital heap end is not null;
		}
	if(amount>0){
		// If more than allowed total, return error ENOMEM
		// Increase the pointer and return the old pointer
		// Check Alignment
		// How to deal with non aligned request?
		if( as->h_end + amount < HEAP_MAX && as->h_end+amount < USERSTACK - DUMBVM_STACKPAGES*PAGE_SIZE)
		{
			// Valid case
			vaddr_t temp = as->h_end;
			as->h_end= as->h_end + amount;
			*retval = temp;
			return 0;
		}
		else
		{
			// Either the requested memory is overlapping the stack region
			// Or it is more than the maximum allowable request
			*retval=-1;
			return ENOMEM;
		} 
	}
	if(amount<0){
		// Check that the amount is < heap start?
		// Remember that amount is negative
		// TODO: Freeing pages. Right now this implementation will leak memory.
		// Will implement if required.
		if(as->h_end + amount >= as->h_start)
		{	
	          	// Else decrease and return the old pointer
			// Valid case
			//vaddr_t temp = as->h_end;
                        as->h_end= as->h_end + amount;
                        *retval = as->h_end;
			return 0;

		}
		else 
		{
		// If not, error EINVAL
		*retval = -1;
		return EINVAL;
		}

	}
	return 0;
}

