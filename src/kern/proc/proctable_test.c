#include <types.h>
#include <test.h>
#include <proc.h>
#include <proctable.h>
#include <lib.h>

int run_proctable_tests(int nargs, char **args) {
    (void) nargs;
    (void) args;

    kprintf("expected active_procs: 1, Actual: %d\n", proc_table->active_procs);

    struct proc * test_proc = proc_create("test proc");
    kprintf("expected first available PID: 3, Actual: %d\n", test_proc->p_pid);

    remove_proc(test_proc->p_pid);
    kprintf("total active procs after proc removal should be 1, Actual: %d\n", proc_table->active_procs);

    return 0;
}