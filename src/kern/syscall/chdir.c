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
#include <vfs.h>

extern int errno;

int sys_chdir(const char * pathname){
	
	KASSERT(curthread != NULL);
	KASSERT(curproc != NULL);
	
	struct vnode * directory = NULL;


	int error = vfs_lookup((char*)pathname, &directory);

	if (error) {
		// errno = error;
		return -1;
	}

	error = vfs_chdir((char*)pathname);

	if (error) {
		// errno = error
		return -1;
	}

	return 0;
}
