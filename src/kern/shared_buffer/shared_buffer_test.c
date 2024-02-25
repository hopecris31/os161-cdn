#include <shared_buffer.h>
#include <lib.h>
#include <thread.h>
#include <spl.h>
#include <test.h>
#include <synch.h>
#include <linked_list.h>

struct semaphore* sem;

static void shared_buffer_test_produce(void* buffer, unsigned long which){
	//(void)buffer;
	//(void)which;

	//do test things
	
	int i;
	int* c;
		
	for (i=0; i<10; i++) {
		c = kmalloc(sizeof(int));
		*c = 'A' + i;
		//kprintf("%d\n", *c);
		shared_buffer_produce(buffer, c);
		shared_buffer_printbuffer(buffer, which);
	}
	
	V(sem);
}

static void shared_buffer_test_consume(void* buffer, unsigned long which){
	
	int i;
	int* c;

	for (i=0; i<10; i++) {
                c = kmalloc(sizeof(int));
                *c = 'A' + i;
                //kprintf("%d\n", *c);
                shared_buffer_consume(buffer);
                shared_buffer_printbuffer(buffer, which);
        }

        V(sem);
}               


int shared_buffer_test_run(int nargs, char** args){
	sem = sem_create("sb_sem", 0);

	int testnum = 0;

	if (nargs == 2) {
		testnum = args[1][0] - '0'; // XXX - Hack - only works for testnum 0 -- 9
	}
	
	kprintf("testnum: %d\n", testnum);

	Shared_Buffer* buffer = shared_buffer_create(5);

	thread_fork("produce 1",
              NULL,
              shared_buffer_test_produce,
	      buffer,
              1);

	thread_fork("consume 1",
              NULL,
              shared_buffer_test_consume,
              buffer,
              2);

	thread_fork("produce 2",
              NULL,
              shared_buffer_test_produce,
              buffer,
              2);

        thread_fork("consume 2",
              NULL,
              shared_buffer_test_consume,
              buffer,
              3);

	P(sem);
	P(sem);
	P(sem);
	P(sem);
	sem_destroy(sem);

	return 0;
}

