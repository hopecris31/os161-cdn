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

int sys_open(const char *filename, int flags)
{
  int result;
  struct file * f;
  struct vnode * v;
  if(filename == NULL){
    return -EFAULT;
  }
  char * my_buf = (char*)kmalloc(PATH_MAX);
  if(my_buf == NULL){
    return -EIO;
  }
  result = copyin((const_userptr_t)filename,my_buf,PATH_MAX);
  if(result != 0){
    kfree(my_buf);
    return -result;
  }
  result = vfs_open(my_buf, flags, 0, &v);
  if(result != 0){
    kfree(my_buf);
    vnode_cleanup(v);
    return -result;
  }
  if(v == NULL){
    kfree(my_buf);
    return -ENODEV;
  }
  f = file_create(my_buf);
  if(f == NULL){
    kfree(my_buf);
    vnode_cleanup(v);
    return -ENOSPC;
  }
  f->file_vnode = v;
  //Result is negative if either table did not have room.
  result = file_table_add(curproc->proc_ft,f);
  if(result < MIN_FD){
    kfree(my_buf);
    file_destroy(f);
    return -ENFILE;
  }
  kfree(my_buf);
  return result;
}
