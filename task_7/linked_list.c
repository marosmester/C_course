#include "linked_list.h"
#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"

#define MEMORY_ERROR 101
#define EMPTY_OR_NONEXISTING -1

typedef struct node {
    int data;
    struct node* next;
    //struct node* prev;
} NODE;

typedef struct linked_list {
    int num_of_nodes;
    struct node* first;
    struct node* last;
} LINKED_LIST;

//initialization of one global linked list
LINKED_LIST mylist = {.num_of_nodes = 0, .first= NULL, .last=NULL};

// [DEBUGGING] print LINKED_LIST
void print_linked_list(LINKED_LIST linlist) {
    printf("number of items= %d\n", linlist.num_of_nodes);
    NODE* tmp_n = linlist.first; 
    for (int i=0; i< linlist.num_of_nodes; i++) {
        printf("node idx= %d\n", i);
        printf("node data= %d\n", tmp_n->data );
        printf("node next= %p\n\n", tmp_n->next);
        tmp_n = tmp_n->next;
    }
}

// node creation
NODE* create_node() {
    NODE* node_ptr = (NODE*)malloc(sizeof(NODE));
    if (node_ptr== NULL) {
        exit(MEMORY_ERROR);
    } 
    return node_ptr;
}

/*
 * Push the entry value to the queue. The value of the entry must be >= 0.
 * return: true on success and false otherwise.
 */
_Bool push(int entry) {
    bool ret;
    if (entry >= 0) {
        NODE* n = create_node();
        n->data = entry;
        n->next= NULL;
        
        // RE-LINK THE LIST:
        if (mylist.num_of_nodes != 0) {             // let the item that was last until now point to the new node
            mylist.last->next = n; 
        }
        mylist.last = n;                            // change 'last' ptr to the new node
        if (mylist.num_of_nodes == 0) {             // in case of an empty list, also change the 'first' ptr
            mylist.first = n;
        }

        mylist.num_of_nodes++;
        ret = true;
    } else {
        ret = false;
    }
    return ret;
}

/*
 * Pop an entry from the head of the queue
 * return: the stored int value or value less than 0 indicating the queue is empty
 */
int pop(void) {
    int ret;
    if (mylist.first != NULL) {
        NODE* head_node = mylist.first;
        // change linked list:
        mylist.first = head_node->next;
        mylist.num_of_nodes--;
        // save data into return variable:
        ret = head_node->data;
        // delete node:
        free(head_node);
    } else {
        ret = EMPTY_OR_NONEXISTING;
    }
    return ret;
}

/*
 * Insert the given entry to the queue in the InsertSort style.
 *
 * Since push and insert functions can be combined, it cannot be 
 * guaranteed, the internal structure of the queue is always sorted.
 *
 * The expected behaviour is that insert proceeds from the head of
 * the queue to the tail in such a way that it is insert before the entry 
 * with the lower value, i.e., it becomes a new head if its value is the
 * new maximum or a new tail if its value is a new minimum of the values
 * in the queue.
 *
 * return: true on success; false otherwise
 */
_Bool insert(int entry) {
    bool ret = false;
    if (entry >= 0) {
        NODE* n = create_node();
        n->data = entry;
        // cycle through linked list until you find the spot to put the node:
        NODE* tmp_node = mylist.first;
        NODE* tmp_node_prev = NULL;
        while (tmp_node != NULL && tmp_node->data > n->data ) {
            tmp_node_prev = tmp_node;
            tmp_node = tmp_node->next;
        }
        // put the node into place:
        if (tmp_node == NULL) {                 // CASE: insert() is simply push()
            if (tmp_node_prev != NULL) {
                tmp_node_prev->next = n;
            } else {
                mylist.first = n;
            }
            mylist.last = n;
            n->next = NULL;
        } else {                                // CASE: other
            if (tmp_node_prev == NULL) {
                mylist.first = n;
            } else {
                tmp_node_prev->next = n;
            }
            n->next = tmp_node;
        }
        // increase the number of nodes:
        mylist.num_of_nodes++;
        ret = true;
    } 
    return ret;
}

/*
 * Erase all entries with the value entry, if such exists
 * return: true on success; false to indicate no such value has been removed
 */
_Bool erase(int entry) {
    bool ret = false;
    NODE* tmp_n = mylist.first;
    NODE* tmp_n_prev= NULL;
    while (tmp_n != NULL) {
        if (tmp_n->data == entry) {
            // RE-LINKING:
            if (mylist.num_of_nodes > 1) {
                if (tmp_n_prev == NULL) {                // CASE: tmp_n is the first item
                    mylist.first = tmp_n->next;
                } 
                if (tmp_n->next == NULL) {              // CASE: tmp_n is the last item
                    mylist.last = tmp_n_prev;
                    mylist.last->next = NULL;
                }
                if (tmp_n_prev != NULL && tmp_n->next != NULL) {  // CASE: tmp_n is neither the first nor the last item
                    tmp_n_prev->next = tmp_n->next;
                }

            } else {
                mylist.first = NULL;
                mylist.last = NULL;
            }
            // DELETE THE NODE:
            NODE* delete_ptr = tmp_n;
            tmp_n = tmp_n->next; // continue the cycle
            free(delete_ptr);
            mylist.num_of_nodes--;   // decrease the total number of nodes  
            ret = true;         
        } else {
            tmp_n_prev = tmp_n;
            tmp_n = tmp_n->next;
        }
    }
    return ret;
}

/*
 * For idx >= 0 and idx < size(queue), it returns the particular item
 * stored at the idx-th position of the queue. The head of the queue
 * is the entry at idx = 0.
 *
 * return: the particular value of the entry at the idx-th position or
 * value less than 0 to indicate the requested position is not presented
 * in the queue 
 */
int getEntry(int idx) {
    int ret;
    if (idx >= 0 && idx < mylist.num_of_nodes ) {
        NODE* tmp_n = mylist.first;
        for (int i = 0; i < idx; i++) {
            tmp_n = tmp_n->next;
        }
        ret = tmp_n->data; 
    } else {
        ret = EMPTY_OR_NONEXISTING;
    }
    return ret;
}

/*
 * return: the number of stored items in the queue
 */
int size(void) {
    //print_linked_list(mylist);
    return mylist.num_of_nodes;
}

/*
 * Remove all entries in the linked list
 */
void clear() {
    NODE* tmp_n = mylist.first;
    NODE* deletion_ptr;
    while (tmp_n != NULL) {
        deletion_ptr = tmp_n;           // copy current ptr to another ptr for deletion
        tmp_n = tmp_n->next;            // move current ptr to next item  
        free(deletion_ptr);             // delete item 
    }
    mylist.num_of_nodes = 0;
    mylist.first = NULL;
    mylist.last = NULL;
}
