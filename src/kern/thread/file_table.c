#include <types.h>
#include <lib.h>
#include <kern/limits.h>
#include <thread.h>
#include <synch.h>
#include <file.h>
#include <vfs.h>
#include <proc.h>

/////////////////////////////////////////////////
//                FILE METHODS                 //
/////////////////////////////////////////////////

/*
* Constructor for a file.
*/
file * file_create(char * path)
{
  if(path == NULL){
    return NULL;
  }
  struct file * tmp = kmalloc(sizeof(file));
  if(tmp == NULL){
    return NULL;
  }
  tmp->file_path = kstrdup(path);
  if(tmp->file_path == NULL){
    kfree(tmp);
    return NULL;
  }
  tmp->file_lock = lock_create("file_lock");
  if(tmp->file_lock == NULL){
    tmp->file_path = NULL;
    kfree(tmp);
    return NULL;
  }
  tmp->file_vnode = NULL;
  tmp->seek_pos = 0;
  tmp->master_fd = FLOATING_FILE;
  return tmp;
}

/*
* Descructor for a file.
*/
int file_destroy(file * file)
{
  int result = 0;
  if(file == NULL){
    return 0;
  }
  lock_acquire(file->file_lock);
  if(file->file_vnode->vn_refcount == ONE_FILE_OWNER)
  {
    if(file->file_vnode != NULL){
      VOP_DECREF(file->file_vnode);
      kfree(file->file_path);
    }
    file->file_vnode = NULL;
    file->seek_pos = 0;
    if(file->master_fd != FLOATING_FILE){
      lock_acquire(kproc->proc_ft->file_table_lock);
      kproc->proc_ft->file_arr[file->master_fd] = NULL;
      lock_release(kproc->proc_ft->file_table_lock);
    }
    file->master_fd = 0;
    lock_release(file->file_lock);
    lock_destroy(file->file_lock);
    kfree(file);
  }else{
    VOP_DECREF(file->file_vnode);
    lock_release(file->file_lock);
  }
  return result;
}

/*
* File copy constuctor.
* Returns the integer file descriptor of the new file.
* If there were too many files, returns -1.
*/
int file_copy(file_table * file_t, int oldfd, int newfd)
{
  if(  oldfd < MIN_FD
    || newfd < MIN_FD
    || oldfd >= OPEN_MAX
    || newfd >= OPEN_MAX
    || file_t == NULL
    || kproc->proc_ft->file_arr[oldfd] == NULL)
  {
    return -1;
  }
  lock_acquire(file_t->file_table_lock);
  lock_acquire(file_t->file_arr[oldfd]->file_lock);
  lock_acquire(kproc->proc_ft->file_table_lock);

  if(file_t->file_arr[newfd] != NULL){
    file_destroy(file_t->file_arr[newfd]);
  }

  file_t->file_arr[newfd] = kproc->proc_ft->file_arr[oldfd];
  lock_release(kproc->proc_ft->file_table_lock);
  lock_release(file_t->file_arr[oldfd]->file_lock);
  lock_release(file_t->file_table_lock);
  return 0;
}

/////////////////////////////////////////////////
//            FILE TABLE METHODS               //
/////////////////////////////////////////////////

/*
* File table constructor.
*/
file_table * file_table_create()
{
  struct file_table * tmp;
  tmp = kmalloc(sizeof(file_table));
  if(tmp == NULL){
    return NULL;
  }
  if(tmp->file_arr == NULL){
    kfree(tmp);
    return NULL;
  }
  tmp->file_table_lock = lock_create("file_table_lock");
  if(tmp->file_table_lock == NULL){
    kfree(tmp->file_arr);
    kfree(tmp);
    return NULL;
  }
  tmp->num_files = 0;
  for(int i = 0; i < OPEN_MAX; i++){
    tmp->file_arr[i] = NULL;
  }
  return tmp;
}

/*
* File table destructor.
*/
int file_table_destroy(file_table * file_t)
{
  lock_acquire(file_t->file_table_lock);
  for(int i = 0; i < OPEN_MAX; i++){
    file_destroy(file_t->file_arr[i]);
  }
  file_t->num_files = 0;
  kfree(file_t->file_arr);
  lock_release(file_t->file_table_lock);
  lock_destroy(file_t->file_table_lock);
  kfree(file_t);

  return 0;
}

/*
* Adds a file to the master file table.
*/
int file_table_add(file_table * file_t, file * file)
{
  if(kproc->proc_ft->num_files == OPEN_MAX
    || file == NULL
    || file->file_vnode == NULL)
  {
    return -1;
  }
  lock_acquire(file->file_lock);
  lock_acquire(file_t->file_table_lock);
  lock_acquire(kproc->proc_ft->file_table_lock);
  int fd = FLOATING_FILE;
  for(int i = MIN_FD; i < OPEN_MAX; i++)
  {
    if(kproc->proc_ft->file_arr[i] == NULL && file->master_fd == FLOATING_FILE)
    {
      //We have found a global home for the file.
      kproc->proc_ft->file_arr[i] = file;
      file->master_fd = i;
    }if(file_t->file_arr[i] == NULL && fd < MIN_FD)
    {
      //We have found a local home for the file.
      file_t->file_arr[i] = file;
      VOP_INCREF(file->file_vnode);
      fd = i;
    }if(fd >= MIN_FD && file->master_fd != FLOATING_FILE)
    {
      lock_release(kproc->proc_ft->file_table_lock);
      lock_release(file_t->file_table_lock);
      lock_release(file->file_lock);
      return fd;
    }
  }
  if(fd >= MIN_FD){
    file_t->file_arr[fd] = NULL;
    VOP_DECREF(file->file_vnode);
  }
  if(file->master_fd != FLOATING_FILE){
    kproc->proc_ft->file_arr[file->master_fd] = NULL;
    file->master_fd = FLOATING_FILE;
  }
  lock_release(kproc->proc_ft->file_table_lock);
  lock_release(file_t->file_table_lock);
  lock_release(file->file_lock);
  return -1;
}

/*
* File table lookup in local table.
*/
file * file_table_lookup(file_table * file_t, int fd)
{
  if(file_t==NULL || fd < 3)
  {
    return NULL;
  }
  lock_acquire(file_t->file_table_lock);
  file * f = file_t->file_arr[fd];
  lock_release(file_t->file_table_lock);
  return f;
}

/*
* Copies a file table.
*/
file_table * file_table_copy(file_table * file_t){
  if(file_t == NULL)
  {
    return NULL;
  }
  lock_acquire(file_t->file_table_lock);
  struct file_table * tmp = file_table_create();
  if(tmp == NULL)
  {
    return NULL;
  }
  tmp->num_files = file_t->num_files;
  for(int i = MIN_FD; i < OPEN_MAX; i++)
  {
    if(file_t->file_arr[i] != NULL)
    {
        lock_acquire(file_t->file_arr[i]->file_lock);
        tmp->file_arr[i] = file_t->file_arr[i];
        VOP_INCREF(tmp->file_arr[i]->file_vnode);
        lock_release(file_t->file_arr[i]->file_lock);
    }
  }
  lock_release(file_t->file_table_lock);
  return tmp;
}
