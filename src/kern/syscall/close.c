#include <types.h>
#include <limits.h>
#include <lib.h>
#include <current.h>
#include <proc.h>
#include <file.h>
#include <vnode.h>
#include <vfs.h>
#include <syscall.h>
#include <kern/errno.h>
#include <stdarg.h>
#include <copyinout.h>
#include <synch.h>

int sys_close(int fd)
{
    if(fd < MIN_FD || fd >= OPEN_MAX){
      return -EBADF;
    }
    file * f = file_table_lookup(curproc->proc_ft,fd);
    if(f == NULL){
      return -EBADF;
    }
    lock_acquire(curproc->proc_ft->file_table_lock);
    curproc->proc_ft->file_arr[fd] = NULL;
    lock_release(curproc->proc_ft->file_table_lock);
    int result = file_destroy(f);
    return -result;
}
