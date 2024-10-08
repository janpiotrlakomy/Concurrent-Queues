#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "BLQueue.h"
#include "HazardPointer.h"

struct BLNode;
typedef struct BLNode BLNode;
typedef _Atomic(BLNode*) AtomicBLNodePtr;

struct BLNode {
    AtomicBLNodePtr next;
    _Atomic(Value) buffer[BUFFER_SIZE];
    _Atomic(uint64_t) push_idx;
    _Atomic(uint64_t) pop_idx;
};

BLNode* BLNode_new(Value item)
{
    BLNode* node = (BLNode*)malloc(sizeof(BLNode));
    atomic_init(&node->next, NULL);
    atomic_init(&node->pop_idx,0);
    atomic_init(&node->push_idx,1);

    atomic_init(&node->buffer[0], item);
    for (size_t i = 1; i < BUFFER_SIZE; i++)
    {
        atomic_init(&node->buffer[i], EMPTY_VALUE);
    }

    return node;

}

BLNode* BLEmptyNode_new(void)
{
    BLNode* node = (BLNode*)malloc(sizeof(BLNode));
    atomic_init(&node->next, NULL);
    atomic_init(&node->pop_idx,0);
    atomic_init(&node->push_idx,0);

    for (size_t i = 0; i < BUFFER_SIZE; i++)
    {
        atomic_init(&node->buffer[i], EMPTY_VALUE);
    }

    return node;
}

struct BLQueue {
    AtomicBLNodePtr head;
    AtomicBLNodePtr tail;
    HazardPointer hp;
};

BLQueue* BLQueue_new(void)
{
    BLQueue* queue = (BLQueue*)malloc(sizeof(BLQueue));
    HazardPointer_initialize(&queue->hp);
    BLNode* node = BLEmptyNode_new();
    atomic_init(&queue->head,node);
    atomic_init(&queue->tail,node);

    return queue;
}

void BLQueue_delete(BLQueue* queue)
{
    BLNode*  node = atomic_load(&(queue->head));
    while(node){
        BLNode* next =  atomic_load(&(node->next));
        HazardPointer_retire(&queue->hp, node);
        node = next;
    }
    HazardPointer_finalize(&queue->hp);
    free(queue);
}

void BLQueue_push(BLQueue* queue, Value item)
{
    while(true){
        BLNode* tail = HazardPointer_protect(&queue->hp, &queue->tail);

        uint64_t push_idx = atomic_fetch_add(&tail->push_idx,1);

        if(push_idx < BUFFER_SIZE){
            Value temp = EMPTY_VALUE;
            if(atomic_compare_exchange_strong(&tail->buffer[push_idx], &temp , item)){
                HazardPointer_clear(&queue->hp);
                return;
            }
        }
        else{ // push_idx >= BUFFER_SIZE
            BLNode* next = atomic_load(&tail->next);
            BLNode* new_next = BLNode_new(item);
            if (next == NULL){
                if(atomic_compare_exchange_strong(&tail->next, &next, new_next)){
                    atomic_compare_exchange_strong(&queue->tail, &tail, new_next);
                    HazardPointer_clear(&queue->hp);
                    return;
                }
            }
            else{
                atomic_compare_exchange_strong(&queue->tail, &tail, next);
            }
            free(new_next);

        }
    }
}

Value BLQueue_pop(BLQueue* queue)
{
    while(true){
        BLNode* head = HazardPointer_protect(&queue->hp, &queue->head);
        uint64_t pop_idx = atomic_fetch_add(&head->pop_idx,1);

        if(pop_idx < BUFFER_SIZE){ //3a
            Value read = atomic_exchange(&head->buffer[pop_idx], TAKEN_VALUE);

            if(read != EMPTY_VALUE){
                HazardPointer_clear(&queue->hp);
                return read;
            }
        }
        else{ // pop_idx >= BUFFER_SIZE; 3b
            if(atomic_load(&head->next) == NULL){
                HazardPointer_clear(&queue->hp);
                return EMPTY_VALUE;
            }
            else{
                if(atomic_compare_exchange_strong(&queue->head, &head, head->next)){
                    HazardPointer_retire(&queue->hp, head);
                }
            }
        }
    }
}

bool BLQueue_is_empty(BLQueue* queue)
{
    BLNode* head = HazardPointer_protect(&queue->hp, &queue->head);
    BLNode* next = atomic_load(&head->next);

    if(next){
        return false;
    }
    volatile bool ret = !next && (atomic_load(&head->pop_idx) >= atomic_load(&head->push_idx));
    HazardPointer_clear(&queue->hp);
    return ret;
}
