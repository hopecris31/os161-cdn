#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <copyinout.h>

/*
 * helper function to kfree an entire array of strings (char ptrs) at once
 * takes an array
 * does not return anything
 * results in each string in the array being deallocated.
 * DOES NOT deallocate the actual array, just its contents.
 */
static void kfree_array(char **array){
        int j;
        for (j = 0; array[j] != NULL; j++){
                kfree(array[j]);
        }
}

int sys_execv(const char * program, char **args)
{
	(void)args;
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
	int argc;
	char **copied_args;
	char *kernel_path;

	// check *program is valid
	

	// find the size of the null terminated args array by iterating through it until we find a null value
	for(argc = 0; args[argc] != NULL; argc++) {}

	// if argc > ARG_MAX, there are too many arguments! error E2BIG!
	if (argc > ARG_MAX){
		return -E2BIG;
	}

	// create array to store copied arguments
	copied_args = (char **) kmalloc(sizeof(char **) * (argc+1));
	if (copied_args == NULL){
		// allocation failed! error ENOMEM
		return -ENOMEM;
	}
	
	// populate copied_args
	int i;
	for (i = 0; i < argc; i++){
		copied_args[i] = kmalloc(strlen(args[i]) + 1);  // +1 gives space for the null terminator
		copied_args[i + 1] = NULL;  // set next arg as NULL so we can check if the next kmalloc works + to null terminate the array
		if (copied_args[i] == NULL) {
			// if the current array value is null, it wasn't allocated. ENOMEM failure.
			kfree_array(copied_args);  // kfree the args we've already copied
			kfree(copied_args);        // and then free the array itself
			return -ENOMEM;
		}
		// copy in the arg from userspace
		result = copyinstr((const_userptr_t) args[i], copied_args[i], strlen(args[i]) +1, NULL);
		if (result) {
			// free in same manner as above
			kfree_array(copied_args);
			kfree(copied_args);
			return result;
		}
	}
	copied_args[argc] = NULL;
	
	// allocate the kernel path
	kernel_path = (char *)kmalloc(strlen(program) + 1);
	if (kernel_path == NULL){
		// if kmalloc didn't work, return apropriate error
		kfree_array(copied_args);
		kfree(copied_args);
		return -ENOMEM;
	}
	// copy the program path to the kernel path
	result = copyin((const_userptr_t)program, kernel_path, strlen(program) + 1);
	if (result){
		kfree_array(copied_args);
		kfree(copied_args);
		kfree(kernel_path);
		return result;
	}

	// destroy current addrspace so we can replace it
	as_destroy(curproc->p_addrspace);

	// create a new addrspace
	as = as_create();
	if (as == NULL) {
		return -ENOMEM;
	}

	// destroy the old per proc file table
	file_table_destroy(curproc->proc_ft);

	// create a new per proc file table
	curproc->proc_ft = file_table_create();

	/* Open the file. */
	result = vfs_open((char*)program, O_RDONLY, 0, &v);
	if (result) {
		as_destroy(curproc->p_addrspace);
		kfree_array(copied_args);
		kfree(copied_args);
		return result;
	}

	// switch to our new addrspace & activate it
	proc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

	curproc->p_name = kernel_path;

	/* Warp to user mode. */
	enter_new_process(argc, (userptr_t) stackptr /*userspace addr of argv*/,
			NULL /*userspace addr of environment*/,
			stackptr, entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return -EINVAL;
}

