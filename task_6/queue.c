#include "queue.h"
#define MEMORY_ERROR 101
#define EXTEND_MODE 1
#define REDUCE_MODE -1
#define EXTEND_CONST 2
#define REDUCE_CONST 3


queue_t* create_queue(int capacity) {
    queue_t* q= (queue_t*)malloc(sizeof(queue_t));
    if (q== NULL) {
        exit(MEMORY_ERROR);
    }
    q->cap = capacity;
    q->num_of_elem= 0;
    q->start = (void**) malloc(capacity* sizeof(void*));
    if (q->start== NULL) {
        exit(MEMORY_ERROR);
    }
    q->first = q->start;
    q->insert = q->start;
    q->end = q->start + (capacity - 1);
    return q;
}

/* deletes the queue and all allocated memory */
void delete_queue(queue_t *queue) {
    free(queue->start);
    free(queue);
}

/* extends or reduces the size of queue dynamically */
void change_size(queue_t* queue, int mode) {
    if (mode == EXTEND_MODE) {
        // carefully realloc to new array:
        void** new_data = (void**) malloc(EXTEND_CONST*queue->cap * sizeof(void*));
        int cnt=0;
        for (int i= 0; i<queue->cap; i++) {
            if (queue->first + i <= queue->end) {
                *(new_data + i) = *(queue->first + i);
                //printf("new_data=%d\n", *(int*)*(new_data + i) );
                cnt++;
            } else {
                int increment = i - cnt;
                *(new_data + i) = *(queue->start + increment);
            }
        }
        // new queue values:
        free(queue->start);
        queue->start = new_data;
        queue->first = queue->start;
        queue->insert = queue->start + queue->cap;   // old capacity used here
        queue->cap = EXTEND_CONST*queue->cap;        // new capacity is defined here
        queue->end = queue->start + (queue->cap -1 );

    } else if (mode == REDUCE_MODE) {
        // carefully realloc to new array:
        void** new_data = (void**)malloc( (queue->cap/REDUCE_CONST) *sizeof(void*));
        int cnt = 0;
        for (int i=0; i< (queue->num_of_elem); i++) {
            if (queue->first + i <= queue->end) {
                *(new_data + i) = *(queue->first + i);
                cnt++;
            } else {
                int increment = i - cnt;
                *(new_data + i) = *(queue->start + increment);
            }
        }
        // new queue values:
        free(queue->start);
        queue->start = new_data;
        queue->first = queue->start;
        queue->insert = queue->start + queue->num_of_elem;   
        queue->cap = queue->cap/REDUCE_CONST;
        queue->end = queue->start + (queue->cap -1 );
    }   
}

/*
 * inserts a reference to the element into the queue
 * returns: true on success; false otherwise
 */
bool push_to_queue(queue_t *queue, void *data) {
    
    // check if the the size of the queue needs to be changed
    //printf("num_of_elem= %d\n", queue->num_of_elem);
    //printf("capacity= %d\n", queue->cap);
    if (queue->num_of_elem == queue->cap) {
        //printf("...extending\n");
        change_size(queue, EXTEND_MODE);
    }
    
    *(queue->insert) = data;

    // shift the ''insert'' pointer
    if (queue->insert == queue->end) {
        queue->insert = queue->start;
    } else {
        queue->insert++;
    }

    queue->num_of_elem++; 

    return true;
}

/*
 * gets the first element from the queue and removes it from the queue
 * returns: the first element on success; NULL otherwise
 */
void* pop_from_queue(queue_t *queue) {
    void* ret;

    // check if the queue is empty
    if (queue->num_of_elem == 0) {
        ret = NULL;
        goto SKIP;
    }

    ret = *(queue->first);
    *(queue->first) = NULL;
    
    // shift the ''first'' pointer
    if (queue->first == queue->end) {
        queue->first = queue->start;
    } else {
        queue->first++;
    }
    
    queue->num_of_elem--;

    // check if size does not need to be changed:
    if ( (queue->num_of_elem) < (queue->cap/REDUCE_CONST) ) {  
        //printf("...reducing\n");          
        change_size(queue, REDUCE_MODE);
    }
    SKIP:
    return ret;
}

/*
 * gets idx-th element from the queue, i.e., it returns the element that 
 * will be popped after idx calls of the pop_from_queue()
 * returns: the idx-th element on success; NULL otherwise
 */
void* get_from_queue(queue_t *queue, int idx) {
    void* ret;

    // check if idx-th element exists
    if ( (idx > (queue->num_of_elem)-1) || idx < 0) {
        ret= NULL;
        goto SKIP;
    }

    ret = *(queue->first + idx );
    SKIP:
    return ret;
}

/* gets number of stored elements */
int get_queue_size(queue_t *queue) {
    return queue->num_of_elem;
}
