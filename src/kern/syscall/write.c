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
#include <synch.h>

int sys_write(int fd, const void *buf, int buflen)
{
  if(fd < 0 || fd >= OPEN_MAX){
    return -EBADF;
  }if(buf == NULL){
    return -EFAULT;
  }
  int i;
  int result;
  char * my_buf = (char*) kmalloc(buflen);
  if(my_buf == NULL){
    return -EIO;
  }
  result = copyin((const_userptr_t)buf,my_buf,buflen);
  if(result != 0){
    return -result;
  }
  if(fd == STDIN_FILENO){
    //CASE: STDIN call, cannot read usr input on write.
    kfree(my_buf);
    return -EBADF;
  }else if(fd == STDOUT_FILENO){
    //CASE: Write to console
    for(i = 0; i < buflen; i++){
      putch(my_buf[i]);
    }
    kfree(my_buf);
    return i;
  }else if(fd == STDERR_FILENO){
    //CASE: Write error call. Write the current ERRNO data to console
    //Not sure if this is right..
    for(i = 0; i < buflen; i++){
      putch(my_buf[i]);
    }
    kfree(my_buf);
    return i;
  }else{
    //CASE: Write to file fd in the file table if it exists
    file * f = file_table_lookup(curproc->proc_ft,fd);
    if(f == NULL){
      return -EBADF;
    }
    lock_acquire(f->file_lock);
    struct iovec iov;
    struct uio myuio;
    uio_kinit(&iov, &myuio, my_buf, buflen, f->seek_pos, UIO_WRITE);
    result = VOP_WRITE(f->file_vnode, &myuio);
    lock_release(f->file_lock);
    if(result != 0){
      return -result;
    }
    kfree(my_buf);
    return buflen-myuio.uio_resid;
  }
  return -EIO;
}
