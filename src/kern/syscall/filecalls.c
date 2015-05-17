#include <types.h>
#include <kern/filecalls.h>
#include <kern/processcalls.h>
#include <syscall.h>
#include <lib.h>
#include <vfs.h>
#include <vnode.h>
#include <stdarg.h>
#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <limits.h>
#include <uio.h>
#include <thread.h>
#include <kern/iovec.h>
#include <synch.h>
#include <current.h>
#include <copyinout.h>
#include <kern/seek.h>
#include <kern/stat.h>
#define true 1
#define false 0

int fdentry(int fd,int flag, off_t offset,int ref_count, struct vnode* vn,char* lockname)
{
	//kprintf("fdentry:1\n");
	curthread->filetable[fd] = kmalloc(sizeof(struct file_descriptor));
	//kprintf("fdentry:2\n");
	//if(curthread->filetable[fd] ==NULL)
	//	return -1;
	curthread->filetable[fd]->vn=vn;
	//kprintf("fdentry:3\n");
	curthread->filetable[fd]->flag=flag;
	//kprintf("fdentry:4\n");
	curthread->filetable[fd]->lock=lock_create(lockname);
	//kprintf("fdentry:5\n");
	curthread->filetable[fd]->ref_count=ref_count;
	//kprintf("fdentry:6\n");
	curthread->filetable[fd]->offset=offset;
	return 0;
	
}

int filetable_init()
{
	struct vnode *in;
	struct vnode *out;
	struct vnode *error;
	char *conin;
	char *conout;
	char *conerr;
	int q=0;
	conin = kstrdup("con:");
	conout = kstrdup("con:");
	conerr = kstrdup("con:");

	q=vfs_open(conin, O_RDONLY, 0, &in);
	q=fdentry(0,O_RDONLY, 0,1, in,conin);
	if(q!=0)
				return EBADF;
	q=vfs_open(conout, O_WRONLY, 0, &out);
	q=fdentry(1,O_WRONLY, 0,1, out,conout);
	if(q!=0)
				return EBADF;
	q=vfs_open(conerr, O_WRONLY, 0, &error);
	q=fdentry(2,O_WRONLY, 0,1, error,conerr);
	if(q!=0)
				return EBADF;

	return 0;
}

int validfd_check(int fd)
{
	if(fd>(OPEN_MAX-1) || fd <0 )
	{
		// given fd is not valid fd
		return EBADF;
	}

	if(curthread->filetable[fd]==NULL)
	{
		//given fd is not valid
		return EBADF;
	}
	return 0;
}

/*************************************************************** open call  *****************************************************************/
int sys_open (const_userptr_t pathname, int flags,mode_t mode,int *retval)
{
		int index=3; 
		int flg=0;
		bool appnedTrue=false;
		char kbuffer[PATH_MAX];
		size_t length;
		if(pathname==(userptr_t)NULL)
			return EFAULT;
		if(pathname==(userptr_t)0x40000000)
			return EFAULT;
		if(pathname==(userptr_t)0x80000000)
			return EFAULT;
		
		
		flg=copyinstr((const_userptr_t) pathname,kbuffer, PATH_MAX,&length);
		if(length==0)
			return EINVAL;
		if(flg)
		{
			return flg;
		}
		for(index=3;index<OPEN_MAX;index++)
		{
			if(curthread->filetable[index]==NULL)
				break;
			if(index==(OPEN_MAX-1))
				{
			  		return ENFILE;
				}
		}
		switch(flags) {
		case O_RDONLY: 
			break;
		case O_WRONLY: 
			break;
		case O_RDWR: 
			break;
		case O_RDWR|O_CREAT|O_TRUNC: 
				break;
		case O_RDWR|O_APPEND: appnedTrue = true;
				break;
		case O_WRONLY|O_APPEND: 
				 appnedTrue= true;
				 break;
		case O_RDONLY|O_CREAT: 
			break;
		case O_WRONLY|O_CREAT: 
			break;
		case O_WRONLY|O_CREAT|O_TRUNC: 
			break;
		case O_RDWR|O_CREAT: 
				break;
		default: flg = EINVAL;
			return flg;
	}
		struct vnode *vn_open;
		flg= vfs_open(kbuffer, flags, mode, &vn_open);
		if(flg)
		{
			return flg;
		}
		int temp;
		if (appnedTrue)
		 {  
			struct stat buffer;
			VOP_STAT(vn_open, &buffer);
			int bytes = buffer.st_size;
			*retval = index;
			temp=fdentry(index,flags, bytes,1, vn_open,kbuffer);	
		}
		else
			{
				//////kprintf("cool\n");
				*retval=index;
				temp=fdentry(index,flags, 0,1, vn_open,kbuffer);	
			}
		
		return 0;

}


/***************************************************************   read call **************************************************************** */

int sys_read(int fd, void *buf, size_t buflen,int *retval)
{	
	   //kprintf("read fd:%d\n",fd);
	////kprintf("read happening %d\n",fd);
	int r=validfd_check(fd);
	if(r!=0)
	{
		return EBADF;
	}
	if(curthread->filetable[fd]==NULL)
		return EBADF;
	if(curthread->filetable[fd]->flag ==O_WRONLY)
	{
		return EBADF;
	}
	if (buf == NULL) {
		return EFAULT;
	}
	if((userptr_t)buf==(userptr_t)0x40000000)
		return EFAULT;
	if((userptr_t)buf==(userptr_t)0x80000000)
		return EFAULT;

	struct iovec iov;
	struct uio u;
	struct file_descriptor *temp;
	temp=curthread->filetable[fd];
	int flg=0;
	uio_uinit(&iov,&u,buf,buflen,temp->offset, UIO_READ);
	flg = VOP_READ(temp->vn,&u);
	if (flg) 
	{
		////kprintf("VOP_read failed,%d\n",fd);
		return flg;
	}
	*retval = buflen - u.uio_resid;
	temp->offset  =  temp->offset +*retval;
	return 0;

}
/**************************************************************   write call ********************************************************/

int sys_write(int fd, void *buf, size_t nbytes,int *retval)
{
	////kprintf("fd:%d\n",fd);
	int r=validfd_check(fd);
	if(r!=0)
	{
		return EBADF;
	}
	
	if(curthread->filetable[fd]->flag ==O_RDONLY)
	{
		////kprintf("permission failed,%d\n",fd);
		return EBADF;
	}
	int flg=0;
	if (buf == NULL) {
		return EFAULT;
	}
	if((userptr_t)buf==(userptr_t)0x40000000)
		return EFAULT;
	if((userptr_t)buf==(userptr_t)0x80000000)
		return EFAULT;
	struct iovec iov;
	struct uio u;
	struct file_descriptor *temp;
	temp=curthread->filetable[fd];
	uio_uinit(&iov,&u,buf,nbytes,temp->offset, UIO_WRITE);
	flg = VOP_WRITE(temp->vn,&u);
	if (flg) 
	{
		////kprintf("VOP_write failed,%d\n",fd);
		return flg;
	}
	*retval = nbytes - u.uio_resid;
	temp->offset =temp->offset +  *retval;
	return 0;

}


/*************************************************************  chdir call ****************************************************************/

int sys_chdir(const char *pathname,int *retval) {
	int flg=0;
	size_t length;
	char *kbuffer;
	if((userptr_t)pathname==(userptr_t)NULL)
			return EFAULT;
	if((userptr_t)pathname==(userptr_t)0x40000000)
		return EFAULT;
	if((userptr_t)pathname==(userptr_t)0x80000000)
		return EFAULT;
	kbuffer=(char*) kmalloc (sizeof(char) * PATH_MAX);
	flg=copyinstr((const_userptr_t) pathname,kbuffer, PATH_MAX, &length);
	if(length==0)
			return EINVAL;
	if(flg){
		kfree(kbuffer);
		return EFAULT;
	}
	flg = vfs_chdir(kbuffer);
	if(flg) {
		kfree(kbuffer);
		return flg;
	}
	kfree(kbuffer);
	*retval = 0;
	return 0;
}

/************************************************************  close call  *******************************************************************/
int sys_close(int fd,int *retval)
{
			
	int r=validfd_check(fd);
	if(r!=0)
	{	
		//*retval=-1;
		return EBADF;
	}
	//kprintf("close 1 \n");
	if(curthread->filetable[fd]->ref_count==1)
	{
		
		lock_destroy(curthread->filetable[fd]->lock);
		vfs_close(curthread->filetable[fd]->vn);
		curthread->filetable[fd]=NULL;
		kfree(curthread->filetable[fd]);
		*retval=0;
		//kprintf("close in 1 \n");
		return 0;
	}
	else if(curthread->filetable[fd]->ref_count>1)
	{
		curthread->filetable[fd]->ref_count--;	
		//curthread->filetable[fd]=NULL;
		//kprintf("close in 2 \n");
	}
	//kprintf("close 2 \n");
	*retval=0;
	return 0;
}


/*****************************************************************  dup2 call  ****************************************************************/

int sys_dup2(int oldfd, int newfd,int *retval)
{
	
	int r=validfd_check(oldfd);
	//kprintf("1\n");
	//char* dup2lock;
	if(r!=0)
	{
		return EBADF;
	}
	//kprintf("2\n");
	if(newfd>(OPEN_MAX-1) || newfd <2 )
	{
		return EBADF;
	}
	//kprintf("3\n");
	if(newfd==oldfd)
	{
		*retval = newfd;
		return 0;
	}
	//kprintf("4\n");
	if(curthread->filetable[newfd]!=NULL)
	{
		int q,*p;
		q=sys_close(newfd,p);
		if(q)
		 {
			return EBADF;
		}
	}	
	//kprintf("5\n");
		//kprintf("dup2:1\n");
		lock_acquire(curthread->filetable[oldfd]->lock);
		//kprintf("dup2:2\n");
		//kfree(curthread->filetable[newfd]);
		curthread->filetable[oldfd]->ref_count++;
		int temp;
		//kprintf("dup2:3\n");
		char dup2lock[7]="Duplock";
		temp=fdentry(newfd,curthread->filetable[oldfd]->flag, curthread->filetable[oldfd]->offset,( curthread->filetable[oldfd]->ref_count)+1, curthread->filetable[oldfd]->vn,dup2lock);
		//kprintf("dup2:4\n");
		lock_release(curthread->filetable[oldfd]->lock);
		//kprintf("dup2:5\n");
		*retval = newfd;
		return 0;

}

/*******************************************************************  getcwd call  **************************************************************/

int sys___getcwd(char *buf, size_t buflen,int *retval)
{
	if(buf == NULL) {
		return EFAULT;
	}

	if((userptr_t)buf==(userptr_t)0x40000000)
		return EFAULT;
	if((userptr_t)buf==(userptr_t)0x80000000)
		return EFAULT;
	
	int flg=0;
	struct iovec iov;
	struct uio u;
	uio_uinit(&iov,&u,buf,buflen, 0, UIO_READ);
	flg = vfs_getcwd(&u);
	if(flg)
	{
		return flg;
	}
	*retval = buflen- u.uio_resid;
	return 0;
}

/*******************************************************************  lseek call  ***************************************************************/
int sys_lseek(int fd, off_t pos, int whence, off_t *retval1){	
	int r=validfd_check(fd);
	if(r!=0)
	{
		return EBADF;
	}
	struct file_descriptor *temp;
	temp=curthread->filetable[fd];
	int flg=0;
	off_t cur_ptr=0;
	off_t seek_ptr=0;
	struct stat buf;
	switch(whence) {
		case SEEK_SET: 
				if (pos < 0)
					 return EINVAL;	
				seek_ptr = pos;
				if ((flg = VOP_TRYSEEK(temp->vn, seek_ptr)) != 0) {
					//kprintf("problem\n");
					return ESPIPE;	
				}
				temp->offset = seek_ptr;
				*retval1 =seek_ptr;
				break;
			
		case SEEK_CUR:  
				cur_ptr = temp->offset;
				seek_ptr = cur_ptr + pos;
				if (seek_ptr < 0)
					 return EINVAL;
				if ((flg = VOP_TRYSEEK(temp->vn, seek_ptr)) != 0) 
				{
					return ESPIPE;
				}
				temp->offset = seek_ptr;
				*retval1 = seek_ptr;
				break;

		case SEEK_END:  
				VOP_STAT(temp->vn, &buf);
				off_t file_size = buf.st_size;
				seek_ptr = file_size + pos;
				if (seek_ptr < 0) 
					return EINVAL;
				if ((flg = VOP_TRYSEEK(temp->vn, seek_ptr)) != 0) {
					return ESPIPE;
				}
				temp->offset = seek_ptr;
				*retval1 = seek_ptr;
				break;
		default: 
			return EINVAL; 
	}
	
	return 0;

}

