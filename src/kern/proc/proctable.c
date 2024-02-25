#include <types.h>
#include <proctable.h>
#include <proc.h>
#include <synch.h>
#include <limits.h>
#include <spinlock.h>


struct proctable *proc_table;


void create_proctable() {
    proc_table = kmalloc(sizeof(struct proctable));
    KASSERT(proc_table != NULL);
    spinlock_init(&((proc_table) -> pt_lock));

    for (int i = 0; i < PID_MAX; i++) {
        proc_table->procs[i] = NULL;
	    proc_table->exit_locks[i] = NULL;
	    proc_table->exit_cvs[i] = NULL;
    }
    proc_table->active_procs = 0;
}

void destroy_proctable() {
    spinlock_acquire(&proc_table->pt_lock);
    for (int i=0; i < PID_MAX; i++) {
        if (proc_table->procs[i] != NULL) {
	    // destroy the proc
        proc_destroy(proc_table->procs[i]);
	    // destroy the lock
	    lock_destroy(proc_table->exit_locks[i]);
	    // destroy the cv
	    cv_destroy(proc_table->exit_cvs[i]);
	    // update the vals in the respective tables
        proc_table->procs[i] = NULL;
	    proc_table->exit_locks[i] = NULL;
	    proc_table->exit_cvs[i] = NULL;
        }
    }

    spinlock_release(&proc_table -> pt_lock);
    spinlock_cleanup(&proc_table->pt_lock);
    kfree(proc_table);
    proc_table = NULL;
}


void add_proc(struct proc *new_proc, pid_t pid) {
    KASSERT(proc_table != NULL);

    spinlock_acquire(&proc_table->pt_lock);

    // put the proc in the proper place in the procs[] array and update proc count
    proc_table->procs[pid] = new_proc;
    proc_table->active_procs++;

    // give it an exit lock and an exit cv
    proc_table->exit_locks[pid] = lock_create("pt_lock_#");
    proc_table->exit_cvs[pid] = cv_create("pt_cv_#");
    // ideally should figure out how to concatenate the pid# into the lock and cv names. low priority issue.

    spinlock_release(&proc_table->pt_lock);
}

void remove_proc(pid_t pid) {
   spinlock_acquire(&proc_table->pt_lock);

    KASSERT (proc_table != NULL);
    KASSERT(proc_table->procs!=NULL);
    KASSERT (proc_table->procs[pid] != NULL); 

    // take proc out of table and decrement proc counter
    proc_table->procs[pid] = NULL;
    proc_table->active_procs--;

    // destroy the associated exit lock and exit cv
    lock_destroy(proc_table->exit_locks[pid]);
    proc_table->exit_locks[pid] = NULL;
    cv_destroy(proc_table->exit_cvs[pid]);
    proc_table->exit_cvs[pid] = NULL;

    spinlock_release(&proc_table->pt_lock);
}

struct proc* find_process(pid_t pid) {
   spinlock_acquire(&proc_table->pt_lock);
   struct proc* proc = NULL;
   if (proc_table->procs[pid] != NULL) {
       proc = proc_table->procs[pid];
   }
   spinlock_release(&proc_table->pt_lock);
   return proc;
}

pid_t assign_pid() {
   spinlock_acquire(&proc_table->pt_lock);
   for (pid_t i = 2; i < PID_MAX; i++) {
       if (proc_table->procs[i] == NULL) {
            spinlock_release(&proc_table->pt_lock);
            return i;
       }
   }
   spinlock_release(&proc_table->pt_lock);
   return -1; // No free PIDs
}

int proc_count() {
   spinlock_acquire(&proc_table->pt_lock);
   return proc_table->active_procs;
   spinlock_release(&proc_table->pt_lock);
}

