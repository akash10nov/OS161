#ifndef FILECALLS_H_
#define FILECALLS_H_

#include <types.h>
#include <limits.h>

/* File Descriptor Structure */
struct file_descriptor{
	int flag;						  // to get permission details.
	off_t offset;					  // file off value.
	int ref_count;					  //to keep tabs on number of child processes.
	struct lock* lock;				  // for synchronization and perform other atomic or concurrent tasks
	struct vnode* vn; 				  //to keep (abstract)) file data after vfs_open call.

};
int sys_open( const_userptr_t  pathname, int flags,mode_t mode,int *retval);
int sys_read( int fd, void *buf, size_t buflen,int *retval);
int sys_write( int fd, void *buf, size_t nbytes,int *retval);
int sys_close( int fd,int *retval);
int sys_dup2( int oldfd, int newfd,int *retval);
int sys_chdir( const char *pathname,int *retval);
int sys___getcwd( char *buf, size_t buflen,int *retval);
int filetable_init(void);
int validfd_check(int fd);
int fdentry(int fd,int flag, off_t offset,int ref_count, struct vnode* vn, char* lockname);
int sys_lseek( int fd, off_t pos, int whence,off_t *retval1);

#endif /* FILECALLS_H_ */



