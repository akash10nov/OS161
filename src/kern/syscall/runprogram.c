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

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/filecalls.h>
#include <kern/processcalls.h>
#include <lib.h>
#include <thread.h>
#include <limits.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <synch.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <copyinout.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char** uargs)
{
    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result;
    if (progname == NULL || uargs == NULL ) {
        //kprintf("EXECV- Argument is a null pointer\n");
        return EFAULT;
    }
    /* Open the file. */
    if(curthread->filetable[0]==NULL)
    {
        int q;
        q=filetable_init();
    }
    int index=0;
    //lock_exec=lock_create("exec");
    while(uargs[index] != NULL) {
        index++;
    }
    result = vfs_open(progname, O_RDONLY, 0, &v);
    if (result) {
       // kprintf("problem is here\n");
        return result;
    }

    /* We should be a new thread. */
    KASSERT(curthread->t_addrspace == NULL);

    /* Create a new address space. */
    curthread->t_addrspace = as_create();
    if (curthread->t_addrspace==NULL) {
        vfs_close(v);
        return ENOMEM;
    }

    /* Activate it. */
    as_activate(curthread->t_addrspace);

    /* Load the executable. */
    result = load_elf(v, &entrypoint);
    if (result) {
        /* thread_exit destroys curthread->t_addrspace */
        vfs_close(v);
        return result;
    }

    /* Done with the file now. */
    vfs_close(v);

    /* Define the user stack in the address space */
    result = as_define_stack(curthread->t_addrspace, &stackptr);
    if (result) {
        /* thread_exit destroys curthread->t_addrspace */
        return result;
    }
    
    int flg;
    int i=0;
    
    char ** arguments=(char**)kmalloc(sizeof(char*)*index);
    while(uargs[i]!=NULL)
    {
        size_t len;
        size_t padded;
        len=strlen(uargs[i])+1;
        int k;
        char *for_stack;
        k=(len)%4;
        if(k==0)
        {
            padded=len;
        }
        else
        {
            padded=len+4-k;
        }
        int q;
        q=(int)padded;
        for_stack=kmalloc(sizeof(char)*(padded));
        for_stack=kstrdup(uargs[i]);
        for(int j=0;j<q;j++)
		{
			if(j>=(int)len)
					for_stack[j]='\0';
				else
					for_stack[j]=uargs[i][j];
		}
        stackptr= stackptr-padded;
        flg = copyout((const void *) for_stack, (userptr_t) stackptr,
                (size_t)padded);
        if (flg) 
        {
                    return flg;
            }
        kfree(for_stack);
    arguments[i]=(char *)stackptr;
        i++;
    }
     if(uargs[i]==NULL){
        stackptr =stackptr-( 4 * sizeof(char));
    }
	
    for(int j=(i-1); j>= 0;j--) {
        stackptr=stackptr-sizeof(char*);
        flg = copyout((const void *)(arguments+j), (userptr_t)stackptr, (sizeof(char *)));
        if(flg) {
            kprintf("RunProgram copyout error\n");
            return flg;
        }
    }


	
    /* Warp to user mode. */
    enter_new_process(i/*argc*/, (userptr_t) stackptr /*userspace addr of argv*/,
              stackptr, entrypoint);
    /* enter_new_process does not return. */
    panic("enter_new_process returned\n");
    return EINVAL;
}

