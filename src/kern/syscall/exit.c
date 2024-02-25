#include <types.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>
#include <copyinout.h>
#include <current.h>
#include <proc.h>
#include <spinlock.h>
#include <proctable.h>
#include <synch.h>
#include <thread.h>
#include <kern/wait.h>


int sys_exit(int exitcode)
{
	struct proc * cur_proc = curproc;
	if (cur_proc == NULL) {
		panic("cannot call _exit: curproc is NULL\n");
	}
	
	pid_t pid = cur_proc->p_pid;

	lock_acquire(proc_table->exit_locks[pid]);

	cv_broadcast(proc_table->exit_cvs[pid], proc_table->exit_locks[pid]); //need to get from wpid, handles lock_release within this

	lock_release(proc_table->exit_locks[pid]);

	cur_proc->exitcode = _MKWAIT_EXIT(cur_proc->exitcode); //this is needed? is this right

	if (cur_proc->parent_proc != NULL) {
		cur_proc->parent_proc->exitcode = cur_proc->exitcode;
	}

  	thread_exit();
	
	KASSERT(cur_proc != NULL);
	proc_destroy(cur_proc);


	panic("should not get here");
	return exitcode;
}


