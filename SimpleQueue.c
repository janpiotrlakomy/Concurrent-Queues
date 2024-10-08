#include <malloc.h>
#include <pthread.h>
#include <stdatomic.h>

#include "SimpleQueue.h"

struct SimpleQueueNode;
typedef struct SimpleQueueNode SimpleQueueNode;

struct SimpleQueueNode {
    _Atomic(SimpleQueueNode*) next;
    Value item;
};

SimpleQueueNode* SimpleQueueNode_new(Value item)
{
    SimpleQueueNode* node = (SimpleQueueNode*)malloc(sizeof(SimpleQueueNode));
    atomic_init(&node->next, NULL);
    node->item = item;
    return node;
}


struct SimpleQueue {
    SimpleQueueNode* head;  // always points to an empty node
    SimpleQueueNode* tail;  // points to the end of the queue
    pthread_mutex_t head_mtx;
    pthread_mutex_t tail_mtx;
};

SimpleQueue* SimpleQueue_new(void)
{
    SimpleQueue* queue = (SimpleQueue*)malloc(sizeof(SimpleQueue));
    queue->head = SimpleQueueNode_new(EMPTY_VALUE);
    queue->tail = queue->head;
    pthread_mutex_init(&(queue->head_mtx),NULL);
    pthread_mutex_init(&(queue->tail_mtx),NULL);

    return queue;
}


void SimpleQueue_delete(SimpleQueue* queue)
{
    while(queue->head){
        SimpleQueueNode* temp = queue->head;
        queue->head = atomic_load(&(queue->head->next));
        free(temp);
    }
    free(queue);
}

void SimpleQueue_push(SimpleQueue* queue, Value item)
{
    SimpleQueueNode* new_node = SimpleQueueNode_new(item);

    pthread_mutex_lock(&(queue->tail_mtx));
    atomic_store(&(queue->tail->next), new_node);
    queue->tail = new_node;
    pthread_mutex_unlock(&(queue->tail_mtx));


}

Value SimpleQueue_pop(SimpleQueue* queue)
{
    pthread_mutex_lock(&(queue->head_mtx));

    SimpleQueueNode* head_node = queue->head;
    SimpleQueueNode* next_node = atomic_load(&(head_node->next));
    if(next_node == NULL){
        pthread_mutex_unlock(&(queue->head_mtx));
        return EMPTY_VALUE;
    }
    Value ret = next_node->item;
    next_node->item = EMPTY_VALUE;
    queue->head = next_node;
    free(head_node);

    pthread_mutex_unlock(&(queue->head_mtx));
    return ret;
}

bool SimpleQueue_is_empty(SimpleQueue* queue)
{
    pthread_mutex_lock(&(queue->head_mtx));
    bool ret = (atomic_load(&(queue->head->next)) == NULL);
    pthread_mutex_unlock(&(queue->head_mtx));
    return ret;
}
