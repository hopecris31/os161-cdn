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
#include <kern/unistd.h>
#include <endian.h>

off_t
sys_lseek(int fd, off_t offset, int whence)
{
    struct stat stat;
    struct file *file;
    struct stat sbuf;
    off_t new_pos;
    int result;
    off_t cur_pos;

    join32to64((uint32_t) fd, (uint32_t) offset, (uint64_t *) &cur_pos); // combines 2 32 bit values into a single 64 bit value. Thanks jack!

    if (fd < 0 || fd >= __OPEN_MAX) {
        return -EBADF;
    }

    if (fd == STDIN_FILENO){
        return -EINVAL;
    }
    if (fd == STDOUT_FILENO){
        return -EINVAL;
    }
    if (fd == STDERR_FILENO){
        return -EINVAL;
    }

    // get open file
    file = file_table_lookup(curproc->proc_ft, fd);
    if (file == NULL) {
        return -EBADF;
    }


    if (!VOP_ISSEEKABLE(file->file_vnode)) {
        return -ESPIPE; 
    }

    //get file size
    result = VOP_STAT(file->file_vnode, &sbuf);
    if (result) {
         lock_release(file->file_lock);
        return -result;
    }


    if(whence < 0){
        return -EINVAL;
    }
    if(offset + whence < 0){
        return -EINVAL;
    }


    lock_acquire(file->file_lock);
    // seek according to whence
     if(whence == SEEK_SET){
        new_pos = cur_pos;
    } else if(whence == SEEK_CUR){
        new_pos = file -> seek_pos + cur_pos;
    } else if(whence == SEEK_END){
        result = VOP_STAT(file -> file_vnode, &stat);
        if(result){
            lock_release(file -> file_lock);
            return -result;
        }
        new_pos = cur_pos + stat.st_size;
    } else{
        lock_release(file -> file_lock);
        return -EINVAL;
    }


    file->seek_pos = new_pos;

    off_t ret = file->seek_pos;

    lock_release(file->file_lock);
    return ret;
}
