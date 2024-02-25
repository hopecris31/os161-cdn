#include <linked_list.h>
#include <lib.h>
#include <thread.h>
#include <synch.h>

extern int test_mode;

Linked_List *linked_list_create(void)
{
	Linked_List * ptr = kmalloc(sizeof(Linked_List));
	ptr -> length = 0;
	ptr -> first = NULL;
	ptr -> last = NULL;

	const char* name = "n";	
	ptr -> ll_lock = lock_create(name);

	return ptr;
}

void linked_list_destroy(Linked_List* list){
	KASSERT(list != NULL);
	
	lock_acquire(list->ll_lock);	

	while (list->first != NULL){
		Linked_List_Node* to_remove = list -> first;
                list -> first = to_remove -> next;
                if (list -> first != NULL)  // if first == NULL, there will be no list -> first -> prev.
                {
                        list -> first -> prev = NULL;
                }
                to_remove -> next = NULL;
                // int newkey = to_remove -> key;
                if (list -> first == NULL)  // if first is null, the list is now empty.
                {
                        list -> last = NULL;  // therefore, last should also be null.
                }
                list -> length --;
                kfree(to_remove);
	}

	lock_release(list->ll_lock);
	lock_destroy(list->ll_lock);
}

Linked_List_Node *linked_list_create_node(int key, void *data)
{
	Linked_List_Node *newnode = kmalloc(sizeof(Linked_List_Node));
	if (test_mode == 1){ thread_yield(); }
	newnode -> prev = NULL;
	newnode -> next = NULL;
	newnode -> key = key;
	newnode -> data = data;

	return newnode;
}

void linked_list_prepend(Linked_List *list, void *data)
{
	lock_acquire(list->ll_lock);

	Linked_List_Node * newnode;
	Linked_List_Node * f = list -> first;

	if (list -> first == NULL) {
		newnode = linked_list_create_node(0, data);
		list -> first = newnode;
		list -> last = newnode;
	} else {
		newnode = linked_list_create_node(f -> key - 1, data);

		newnode -> next = list -> first;
		f -> prev = newnode;
	list -> first = newnode;
	}
  
	list -> length ++;

	lock_release(list->ll_lock);
}

void linked_list_printlist(Linked_List *list, int which)
{
	lock_acquire(list->ll_lock);

	Linked_List_Node *runner = list -> first;

	kprintf("%d: ", which);

	while (runner != NULL) {
		if (test_mode == 4){ thread_yield(); }
		kprintf("%d[%c] ", runner -> key, *((int *)runner -> data));
		runner = runner -> next;
	}

	kprintf(" size: %d", list -> length);

	kprintf("\n");

	lock_release(list->ll_lock);
}


void linked_list_insert(Linked_List *list, int key, void *data)
{
	lock_acquire(list->ll_lock);

	Linked_List_Node * newnode;

	if (list -> first == NULL) {
		// there is no first element, i.e. the list is empty
		// therefore we insert as the first (and only) element
		newnode = linked_list_create_node(key, data);
		list -> first = newnode;
		list -> last = newnode;
	} else if(list -> first -> key > key) {
		// the smallest key in the list is smaller than the key of the node we are inserting
		// therefore we insert this node as the new head
		newnode = linked_list_create_node(key, data);
		Linked_List_Node* f = list -> first;
		newnode -> next = list -> first;
		f -> prev = newnode;
		list -> first = newnode;
	} else {
		// otherwise, we are inserting somewhere else in the list
		// create a runner to find the correct location in the list
		newnode = linked_list_create_node(key, data);
		Linked_List_Node *runner = list -> first;
		while (runner -> next != NULL && runner -> next -> key < key)
		{
			runner = runner -> next;
		}
    
		if (runner -> next == NULL)
		{
			// the runner made it to the end of the list, so insert as last element
			newnode -> prev = runner;
			runner -> next = newnode;
			list -> last = newnode;
		} else {
			// otherwise we are inserting somewhere in the middle of the list
			newnode -> next = runner -> next;
			newnode -> prev = runner;
			newnode -> next -> prev = newnode;
			newnode -> prev -> next = newnode;
		}
	}
	if (test_mode == 3){ thread_yield(); }
	list -> length ++;
	if (test_mode == 3){ thread_yield(); }
	
	lock_release(list->ll_lock);
}


void* linked_list_remove_head(Linked_List *list, int *key)
{
	lock_acquire(list->ll_lock);

	if (list -> first == NULL){
		lock_release(list->ll_lock);
		return NULL;
	} else {
		if (test_mode == 2){ thread_yield(); }
		Linked_List_Node* to_remove = list -> first;
		list -> first = to_remove -> next;
		if (list -> first != NULL)  // if first == NULL, there will be no list -> first -> prev.
		{
			list -> first -> prev = NULL;
		}
		to_remove -> next = NULL;
		// int newkey = to_remove -> key;
		*key = to_remove -> key;
		key = key;
		if (list -> first == NULL)  // if first is null, the list is now empty.
		{
			list -> last = NULL;  // therefore, last should also be null.
		}
		list -> length --;
		void* to_return = to_remove -> data;
		kfree(to_remove);
		lock_release(list->ll_lock);
		return to_return;
	}
}

void linked_list_remove(Linked_List *list, Linked_List_Node *node) {
    lock_acquire(list->ll_lock);

    if (node == NULL) {
        lock_release(list->ll_lock);
        return;
    }

    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        list->first = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        list->last = node->prev;
    }

	//is it necessary to update keys? if not remove this
	Linked_List_Node *runner = list->first;
    while (runner != NULL) {
        runner->key--;
        runner = runner->next;
    }

    list->length--;
    kfree(node);

    lock_release(list->ll_lock);
}


