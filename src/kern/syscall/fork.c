#include <types.h>
#include <syscall.h>
#include <spl.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vnode.h>
#include <linked_list.h>
#include <proctable.h>
#include <file.h>
#include <mips/trapframe.h>
#include <kern/errno.h>



int sys_fork(struct trapframe *parent_tf, pid_t *retval) {
    //add lock acquire
    if (curproc->p_addrspace == NULL) {
        //kprintf("sys_fork: curproc->addrspace is NULL");
        return -EINVAL;
    }

    //create string for child proc name
    const char *child_name = "child_proc";
    if (child_name == NULL) {
        return -ENOMEM;
    }

    //create a child process
    struct proc *child = proc_create(child_name);
    child->p_ppid = curproc->p_pid;
    //verify proc_create works as intended/no bugs
    if (child == NULL) {
        return -ENOMEM;
    }
    
    add_proc(child, child->p_pid);

    // copy addrspace
    int addrspace_result = as_copy(curproc->p_addrspace, &child->p_addrspace);
    if (addrspace_result != 0) {
        proc_destroy(child);
        return addrspace_result;
    }
    
    child->proc_ft = file_table_copy(curproc->proc_ft);
    if (child->proc_ft == NULL) {
        as_destroy(child->p_addrspace);
        proc_destroy(child);
        return -ENOMEM;
    }

    struct trapframe *child_tf = kmalloc(sizeof(struct trapframe));
    if (child_tf == NULL) {
        as_destroy(child->p_addrspace);
        proc_destroy(child);
        return -ENOMEM;
    }
    memcpy(child_tf, parent_tf, sizeof(struct trapframe));

    int fork_result = thread_fork(child_name, child, entrypoint, child_tf, (unsigned long)child->p_addrspace);
    if (fork_result != 0) {
        as_destroy(child->p_addrspace);
        kfree(child_tf);
        proc_destroy(child);
        return -ENOMEM;
    }

    *retval = child->p_pid;
    kprintf("%d\n", *retval);
    return 0;
}

void entrypoint(void * data1, unsigned long data2){
    struct trapframe child_tf = * (struct trapframe *)data1;
    struct addrspace *child_addresses = (struct addrspace *)data2;

    curproc->p_addrspace = child_addresses;
    as_activate();

    child_tf.tf_v0 = 0;
    child_tf.tf_a3 = 0;
    child_tf.tf_epc += 4;

    mips_usermode(&child_tf);
}


