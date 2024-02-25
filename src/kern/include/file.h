#ifndef _FILE_H_
#define _FILE_H_

#include <types.h>
#include <limits.h>
#include <thread.h>
#include <synch.h>
#include <vnode.h>

typedef struct file file;
typedef struct file_table file_table;

// A file cannot have descriptor 0,1,2 as these are reserved
// for console I/O operattions.
#define MIN_FD 3

#define FLOATING_FILE -1

#define ONE_FILE_OWNER 2

struct file {
  struct vnode * file_vnode;
  struct lock * file_lock;
  int seek_pos;
  char * file_path;
  int master_fd;// fd in global table or floating if not in global.
};

/*
* Creates a new struct. Returns file or NULL on failure.
* NO VNODE IS CREATED, YOU MUST USE sys_open() TO ENSURE PROPER ACCESS
*/
file * file_create(char * path);

/*
* Destroys a file.
* Removes from global table there is only one instance of it.
*/
int file_destroy(file * file);

/*
* Copies a file into file_t.
* oldfd is index in global table.
* newfd is new index in local table.
* Returns -1 on failure.
*/
int file_copy(file_table * file_t, int oldfd, int newfd);

struct file_table {
  struct file * file_arr[OPEN_MAX];
  struct lock * file_table_lock;
  int num_files;
};

/*
* Creates and returns a new file table, or NULL on failure.
*/
file_table * file_table_create(void);

/*
* Destroys a file table and all included files w/ file_destroy().
*/
int file_table_destroy(file_table * file_t);

/*
* Adds a given file to the master file table. Increments ref count,
*   - CANNOT BE CALLED IF ALREADY IN GLOBAL table
*   - DOES NOT ADD TO LOCAL
*   - MUST BE CALLED AFTER A FILE IS CREATED.
* Returns file descriptor on success and -1 on failure.
*/
int file_table_add(file_table * file_t, file * file);

/*
* Returns the file in the local table w/ descriptor fd.
* Returns null on error.
*/
file * file_table_lookup(file_table * file_t, int fd);

/*
* Duplicates a file table and returns the copy.
* Returns null on error. Increments REFCOUNT.
*/
file_table * file_table_copy(file_table * file_t);

#endif /* _FILE_H_ */
