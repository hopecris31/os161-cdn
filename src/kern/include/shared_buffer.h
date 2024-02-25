#ifndef _SHARED_BUFFER_H_
#define _SHARED_BUFFER_H_

#include <types.h>
#include <thread.h>
#include <spinlock.h>
#include <linked_list.h>

typedef struct Shared_Buffer Shared_Buffer;

struct Shared_Buffer {
	//char* buffer;
	Linked_List* buffer_ll;
	int count;  // number of items in buffer
	int BUFFER_SIZE;  // max size of buffer
	struct lock* sb_lock;  // mutex lock for exclusivity
	struct cv* producer_cv;
	struct cv* consumer_cv;
};

/*
 * creates and returns a new buffer with maximum size BUFFER_SIZE.
 */
Shared_Buffer* shared_buffer_create(int BUFFER_SIZE);

/*
 * destroys the passed buffer.
 */
void shared_buffer_destroy(Shared_Buffer* buffer);

/*
 * adds a character to the buffer. spin waits if there is no space in the buffer.
 */
void shared_buffer_produce(Shared_Buffer* buffer, void* c);

/*
 * consumes a character from the buffer and returns it. spin waits if there are no characters in the buffer.
 */
void* shared_buffer_consume(Shared_Buffer* buffer);

/*
 * Prints the internal list.  Adds the int 'which' to the front of the output,
 * to aid in debugging.
 */
void shared_buffer_printbuffer(Shared_Buffer* buffer, int which);

#endif
