#ifndef _PROCTABLE_H_
#define _PROCTABLE_H_

#include <types.h>
#include <proc.h>
#include <synch.h>
#include <spinlock.h>
#include <limits.h>



/**
* proctable: a table that contains processes.  Each process
*  node contains a pointer to the proc and its PID. slay
* FOR PROCTABLE EXTERN AND ASSIGN PID, MIMIC KPROC
* figure out how to make proctable exist globally
* Fork, _exit, cwd and proctable
* how to allocate filetable/make sure functions work
*/

struct proctable {
   struct proc *procs[PID_MAX];
   struct cv * exit_cvs[PID_MAX];
   struct lock * exit_locks[PID_MAX];
   struct spinlock pt_lock;
   int active_procs;
};

/* This is the process structure for the kernel and for kernel-only threads. */
extern struct proctable * proc_table;

void create_proctable(void);

void destroy_proctable(void);

void add_proc(struct proc *proc, pid_t pid);

void remove_proc(pid_t pid);

struct proc* find_process(pid_t pid);

pid_t assign_pid(void);

int proc_count(void);

#endif /* _PROCTABLE_H_ */
