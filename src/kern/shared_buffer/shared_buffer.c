#include <types.h>
#include <shared_buffer.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>
#include <linked_list.h>

Shared_Buffer* shared_buffer_create(int BUFFER_SIZE){
	// allocate space for shared buffer
	Shared_Buffer* buffer = kmalloc(sizeof(Shared_Buffer));
	if (buffer == NULL){
		// if allocation fails, return null
		return NULL;
	}

	// allocate space for linked list 
	buffer->buffer_ll = linked_list_create();
	if (buffer->buffer_ll == NULL){
		// if allocation fails, destroy buffer and return null
		shared_buffer_destroy(buffer);
		return NULL;
	}

	// allocate space for producer's condition variable
	buffer->producer_cv = cv_create("producer_cv");
	if (buffer->producer_cv == NULL){
		// if alloc fails, destroy everything allocated
		linked_list_destroy(buffer->buffer_ll);
		shared_buffer_destroy(buffer);
		return NULL;
	}

	// allocate space for consumer's condition variable
	buffer->consumer_cv = cv_create("consumer_cv");
        if (buffer->consumer_cv == NULL){
		// if alloc fails, destroy everything allocated
		cv_destroy(buffer->producer_cv);
                linked_list_destroy(buffer->buffer_ll);
                shared_buffer_destroy(buffer);
                return NULL;
        }
	
	// initialize count to 0 & max buffer size to BUFFER_SIZE
	// create the mutex lock
	buffer->count = 0;
	buffer->BUFFER_SIZE = BUFFER_SIZE;
	buffer->sb_lock = lock_create("sb_lock");
	
	// return the new buffer
	return buffer;
}

void shared_buffer_destroy(Shared_Buffer* buffer){
	// make sure the buffer exists before we try to destroy it
	KASSERT(buffer != NULL);
	
	// destroy it piece by piece
	lock_destroy(buffer->sb_lock);
	cv_destroy(buffer->producer_cv);
	cv_destroy(buffer->consumer_cv);
	linked_list_destroy(buffer->buffer_ll);
}

void shared_buffer_produce(Shared_Buffer* buffer, void* c){
	// acquire lock
        lock_acquire(buffer->sb_lock);
		
	// if buffer is full, spin wait until it isn't
	while(buffer->count == buffer->BUFFER_SIZE){
		// wait until receiving signal
		cv_wait(buffer->producer_cv, buffer->sb_lock);
	}
	KASSERT(buffer->count < buffer->BUFFER_SIZE);

	// write
	linked_list_prepend(buffer->buffer_ll, c);
	buffer->count++;
	
	// signal
	cv_signal(buffer->consumer_cv, buffer->sb_lock);

	// release lock
	lock_release(buffer->sb_lock);
}

void* shared_buffer_consume(Shared_Buffer* buffer){
	// acquire lock
        lock_acquire(buffer->sb_lock);	
	
	// if buffer is empty, spin wait until it isn't.
	while(buffer->count == 0){
		// wait until receiving signal
		cv_wait(buffer->consumer_cv, buffer->sb_lock);
	}
	KASSERT(buffer->count > 0);

	// read
	int* key = kmalloc(sizeof(int));
	void* to_return;
       	to_return = linked_list_remove_head(buffer->buffer_ll, key);
	buffer->count--;
	
	// signal
        cv_signal(buffer->producer_cv, buffer->sb_lock);

	// release lock
	lock_release(buffer->sb_lock);

	// free allocated memory
	kfree(key);
        
	// return saved return val
	return to_return;
}

void shared_buffer_printbuffer(Shared_Buffer* buffer, int which){
	lock_acquire(buffer->sb_lock);
	linked_list_printlist(buffer->buffer_ll, which);
	lock_release(buffer->sb_lock);
}
