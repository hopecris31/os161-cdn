#include <types.h>
#include <lib.h>
#include <file.h>
#include <vnode.h>
#include <vfs.h>
#include <stat.h>
#include <current.h>
#include <kern/fcntl.h>
#include <kern/seek.h>
#include <kern/errno.h>
#include <kern/machine/types.h>
#include <syscall.h>
#include <limits.h>
#include <stat.h>  
#include <proc.h>
#include <spl.h>
#include <current.h>
#include <addrspace.h>
#include <linked_list.h>
#include <proctable.h>
#include <mips/trapframe.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <uio.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <copyinout.h>

int sys_getcwd(char *buf, size_t buflen) {
	if (buf == NULL) {
        return -EFAULT;
    }

    char test;
    int result = copyin((userptr_t) buf, &test, sizeof(char));
    if (result != 0) {
        return -EFAULT;
    }

    if (buflen < 1) {
        return -EINVAL;
    }
	if(curproc -> p_cwd == NULL){
        return -ENOENT;
    }
	
	char * tmp_buf = kmalloc(buflen);
	if(tmp_buf == NULL){
        return -EFAULT;
	}
    
    struct iovec iov;
    struct uio ku;
	uio_kinit(&iov, &ku, buf, buflen, 0, UIO_READ);
	
	result = vfs_getcwd(&ku);
    if(result){
		kfree(tmp_buf);
		return -result;
    }

    result = copyoutstr((const void *) tmp_buf, (userptr_t) buf, buflen, (size_t *) ku.uio_resid);
    
	if(result){
		kfree(tmp_buf);
		return -EFAULT;
    }

    int ret = buflen - ku.uio_resid;
	kfree(tmp_buf);
    return ret;
}
