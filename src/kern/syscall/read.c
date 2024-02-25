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


int sys_read(int fd, void *buf, size_t buflen)
{
  int result;
  if(fd < 0 || fd >= OPEN_MAX){
      return -EBADF;
  }else if(buf == NULL){
    return -EFAULT;
  }else if(fd == STDIN_FILENO){
    //CASE: STDIN call, cannot read usr input on write. Produce error.
    size_t i;
    char * my_buf = kmalloc(buflen);
    for(i = 0; i < buflen; i++){
      int ch_buf = getch();
      if(ch_buf < 0){
        return ch_buf;
      }
      my_buf[i] = ch_buf;
      if(ch_buf == '\n' || ch_buf == '\r'){
        break;
      }
    }
    result = copyout((const void *)my_buf,buf,i);
    if(result != 0){
      return -EFAULT;
    }
    return i;
  }else if(fd == STDOUT_FILENO){
    //CASE: Write to console
    return -EBADF;
  }else if(fd == STDERR_FILENO){
    //CASE: Write error call. Write the current ERRNO data to console
    return -EBADF;
  }else{
    //CASE: Write to file fd in the file table if it exists
    int result;
    file * f = file_table_lookup(curproc->proc_ft,fd);
    if(f == NULL){
      return -EBADF;
    }
    lock_acquire(f->file_lock);
    struct iovec iov;
    struct uio myuio;
    uio_kinit(&iov, &myuio, buf, buflen, f->seek_pos, UIO_READ);
    result = VOP_READ(f->file_vnode, &myuio);
    if(result != 0){
      return -result;
    }
    f->seek_pos += (buflen - myuio.uio_resid);
    lock_release(f->file_lock);
    return buflen-myuio.uio_resid;
  }
  return -EIO;
}
