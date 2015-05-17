#ifndef PROCESSCALLS_H_
#define PROCESSCALLS_H_
#include <../../../user/include/sys/null.h>
#include <limits.h>
#include <types.h>
#include <../../arch/mips/include/trapframe.h>

struct process {
	pid_t ppid;
	pid_t pid;
	struct lock* exitlock;
	bool exitflag:1;
	int exitcode;
	struct thread* procthread;
};
struct lock* lock_exec;
struct process *process_table[PID_MAX];
pid_t assign_pid(void);
void process_init(struct thread *t, pid_t id);
void process_destroy(pid_t pid);
pid_t sys_getpid(pid_t *retval);
pid_t sys_waitpid1(pid_t pid);
pid_t sys_waitpid(pid_t pid,userptr_t  status, int options, int *retval);
void sys__exit(int exitcode);
pid_t sys_fork(struct trapframe *tf, pid_t *retval);
int sys_execv(const char *programname, char **args);
void entrypoint(void* data1, unsigned long data2) ;
int sys_sbrk(intptr_t amount,int *retval);
#endif /* PROCESSCALLS_H_ */
