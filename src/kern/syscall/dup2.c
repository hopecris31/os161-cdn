#include <types.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>
#include <kern/unistd.h>
#include <file.h>
#include <uio.h>
#include <vfs.h>
#include <copyinout.h>
#include <proc.h>

int sys_dup2(int oldfd, int newfd){
	newfd = file_copy(curproc->proc_ft, oldfd, newfd);
	if(newfd < MIN_FD){
		return -EBADF;
	}
	return newfd;
}
