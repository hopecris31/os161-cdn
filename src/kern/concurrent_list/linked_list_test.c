#include <linked_list.h>
#include <lib.h>
#include <thread.h>
#include <spl.h>
#include <test.h>
#include <synch.h>

int test_mode = 0;
struct semaphore* sem;

static void linked_list_test_adder(void *list, unsigned long which)
{
  //splhigh();

  // kprintf("Test #%d: \n", (int) which);

  linked_list_printlist(list, which);

  kprintf("Insert test %d: \n", (int) which); 

  int *a = kmalloc(sizeof(int));
  int *b = kmalloc(sizeof(int));
  int *c = kmalloc(sizeof(int));

  *a = 'A';
  *b = 'B';
  *c = 'C';

  linked_list_prepend(list, a);
  linked_list_printlist(list, which);
  linked_list_prepend(list, b);
  linked_list_printlist(list, which);
  
  
  linked_list_insert(list, 0, c);
  linked_list_printlist(list, which);

  kprintf("Remove head test %d: \n", (int) which);

  int* key = kmalloc(sizeof(int));
  void* removed;
  removed = linked_list_remove_head(list, key);
  linked_list_printlist(list, which);
  removed = linked_list_remove_head(list, key);
  linked_list_printlist(list, which);
  removed = linked_list_remove_head(list, key);
  linked_list_printlist(list, which);
  removed = removed;
  
  V(sem);  
}

int linked_list_test_mode_1(int nargs, char **args){
  test_mode = 1;
  linked_list_test_run(nargs, args);
  return 0;
}

int linked_list_test_mode_2(int nargs, char **args){
  test_mode = 2;
  linked_list_test_run(nargs, args);
  return 0;
}

int linked_list_test_mode_3(int nargs, char **args){
  test_mode = 3;
  linked_list_test_run(nargs, args);
  return 0;
}

int linked_list_test_mode_4(int nargs, char **args){
  test_mode = 4;
  linked_list_test_run(nargs, args);
  return 0;
}


int linked_list_test_run(int nargs, char **args)
{
  sem = sem_create("ll_sem", 0);
  
  kprintf("test mode: %d\n", test_mode);

  int testnum = 0;

  if (nargs == 2) {
    testnum = args[1][0] - '0'; // XXX - Hack - only works for testnum 0 -- 9
  }
  
  kprintf("testnum: %d\n", testnum);
  
  Linked_List * list1 = linked_list_create();
  //Linked_List * list2 = linked_list_create();
  
  thread_fork("adder 1",
	      NULL,
	      linked_list_test_adder,
	      list1,
	      1);

  thread_fork("adder 2",
	      NULL,
	      linked_list_test_adder,
	      list1,
	      2);

  P(sem);
  P(sem);
  sem_destroy(sem);

  

  // XXX - Bug - We're returning from this function without waiting
  // for these two threads to finish.  The execution of these
  // threads may interleave with the kernel's main menu thread and
  // cause interleaving of console output.  We going to accept this
  // problem for the moment until we learn how to fix in Project 2.
  // An enterprising student might investigate why this is not a
  // problem with other tests suites the kernel uses.

  return 0;
}

