#include <malloc.h>
#include <pthread.h>
#include <stdatomic.h>

#include <stdio.h>
#include <stdlib.h>

#include "HazardPointer.h"
#include "RingsQueue.h"

struct RingsQueueNode;
typedef struct RingsQueueNode RingsQueueNode;

struct RingsQueueNode {
    _Atomic(RingsQueueNode*) next;
    Value ring[RING_SIZE];
    _Atomic(uint64_t) pop_idx;
    _Atomic(uint64_t) push_idx;
};

RingsQueueNode* RingsQueueNode_new(Value item)
{
    RingsQueueNode* node = (RingsQueueNode*)malloc(sizeof(RingsQueueNode));
    atomic_init(&node->next, NULL);
    atomic_init(&node->pop_idx,0);
    atomic_init(&node->push_idx,1);
    (node->ring)[0] = item;

    return node;
}

RingsQueueNode* RingsQueueEmptyNode_new()
{
    RingsQueueNode* node = (RingsQueueNode*)malloc(sizeof(RingsQueueNode));
    atomic_init(&node->next, NULL);
    atomic_init(&node->pop_idx,0);
    atomic_init(&node->push_idx,0);

    return node;
}


struct RingsQueue {
    RingsQueueNode* head;
    RingsQueueNode* tail;
    pthread_mutex_t pop_mtx;
    pthread_mutex_t push_mtx;
};

RingsQueue* RingsQueue_new(void)
{
    RingsQueue* queue = (RingsQueue*)malloc(sizeof(RingsQueue));
    queue->head = RingsQueueEmptyNode_new();
    queue->tail = queue->head;
    pthread_mutex_init(&(queue->pop_mtx),NULL);
    pthread_mutex_init(&(queue->push_mtx),NULL);
    return queue;
}


void RingsQueue_delete(RingsQueue* queue)
{
    while(queue->head){
        RingsQueueNode* temp = queue->head;
        queue->head = atomic_load(&queue->head->next);
        free(temp);
    }
    free(queue);
}

void RingsQueue_push(RingsQueue* queue, Value item)
{
    pthread_mutex_lock(&(queue->push_mtx));

    uint64_t push_idx = atomic_load(&(queue->tail->push_idx));

    // if the buffer is full
    if(push_idx - atomic_load(&(queue->tail->pop_idx))  == RING_SIZE){
        RingsQueueNode* new_node = RingsQueueNode_new(item);
        atomic_store(&(queue->tail->next), new_node);
        queue->tail = new_node;
    }
    else{
        queue->tail->ring[push_idx % RING_SIZE] = item;
        atomic_store(&(queue->tail->push_idx), push_idx+1);
    }


    pthread_mutex_unlock(&(queue->push_mtx));
}

Value RingsQueue_pop(RingsQueue* queue)
{
    pthread_mutex_lock(&(queue->pop_mtx));

    RingsQueueNode* head_node = queue->head;
    RingsQueueNode* next_node = atomic_load(&(queue->head->next));
    uint64_t pop_idx = atomic_load(&(head_node->pop_idx));

    if(next_node == NULL){  // if head is the only node.
        if(pop_idx == atomic_load(&(head_node->push_idx))){ // if the buffer is empty.
            pthread_mutex_unlock(&(queue->pop_mtx));
            return EMPTY_VALUE;
        }
    }
    else{
        if(pop_idx == atomic_load(&(head_node->push_idx))){ // if the buffer is empty.
            // move the head to the next node, which has at least 1 element.
            queue->head = next_node;
            free(head_node);
            pop_idx = atomic_load(&(next_node->pop_idx));
            head_node = next_node;
        }
    }

    Value ret = head_node->ring[pop_idx % RING_SIZE];
    atomic_store(&(head_node->pop_idx), pop_idx+1);
    pthread_mutex_unlock(&(queue->pop_mtx));

    return ret;
}

bool RingsQueue_is_empty(RingsQueue* queue)
{
    pthread_mutex_lock(&(queue->pop_mtx));

    volatile bool ret = !atomic_load(&(queue->head->next)) &&
        (atomic_load(&(queue->head->pop_idx)) ==
            atomic_load(&(queue->head->push_idx)));

    pthread_mutex_unlock(&(queue->pop_mtx));
    return ret;
}
