#include <types.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>
#include <copyinout.h>
#include <proctable.h>

pid_t
sys_waitpid(pid_t pid, int *status, int options){
	
		
	// check EINVAL
	if (options != 0){
		// return with EINVAL as error code
		return -EINVAL;
	}
	
	// check ESRCH
        struct proc * target_proc = find_process(pid);
        if (target_proc == NULL) {
                // return with ESRCH as error code
                return -ESRCH;
        }

	lock_acquire(proc_table->exit_locks[pid]);

	// check ECHILD
	bool found = false;
	Linked_List_Node * target_proc_pid = curproc->p_children_ll->first;
	while (!found) {
		if (target_proc_pid == NULL) {
			// return with ECHILD as error code
			lock_release(proc_table->exit_locks[pid]);
			return -ECHILD;
       		}
		if (*(pid_t*)target_proc_pid->data == pid) {
			found = true;
		}
		target_proc_pid = target_proc_pid->next;
	}
	
	// if (status is invalid pointer):
	// 	error val is EFAULT
	// 	return -1

	// wait on the target proc to signal its own exit_cv
	// once that happens, the target proc is exiting
	cv_wait(proc_table->exit_cvs[pid], proc_table->exit_locks[pid]);

	if (status != NULL) {
		status = &target_proc->exitcode;
	}
	(void)status;

	lock_release(proc_table->exit_locks[pid]);
	
	return pid;
}
