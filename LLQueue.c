#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "HazardPointer.h"
#include "LLQueue.h"

struct LLNode;
typedef struct LLNode LLNode;
typedef _Atomic(LLNode*) AtomicLLNodePtr;

struct LLNode {
    AtomicLLNodePtr next;
    _Atomic(Value) item;
};

LLNode* LLNode_new(Value item)
{
    LLNode* node = (LLNode*)malloc(sizeof(LLNode));
    atomic_init(&node->next, NULL);
    atomic_init(&node->item, item);;
    return node;
}

struct LLQueue {
    AtomicLLNodePtr head;
    AtomicLLNodePtr tail;
    HazardPointer hp;
};

LLQueue* LLQueue_new(void)
{
    LLQueue* queue = (LLQueue*)malloc(sizeof(LLQueue));
    HazardPointer_initialize(&queue->hp);
    LLNode* node = LLNode_new(EMPTY_VALUE);
    atomic_init(&queue->head,node);
    atomic_init(&queue->tail,node);
    return queue;
}

void LLQueue_delete(LLQueue* queue)
{

    LLNode*  node = atomic_load(&(queue->head));
    while(node){
        LLNode* next =  atomic_load(&(node->next));
        HazardPointer_retire(&queue->hp, node);
        node = next;
    }
    HazardPointer_finalize(&queue->hp);
    free(queue);
}

void LLQueue_push(LLQueue* queue, Value item)
{
    LLNode* node = LLNode_new(item);

    while(true){
        LLNode* tail = HazardPointer_protect(&queue->hp,&queue->tail);
        LLNode* next = atomic_load(&tail->next);

        if(tail == atomic_load(&queue->tail)){
            if(next == NULL){
                if(atomic_compare_exchange_strong(&(tail->next), &next, node)){
                    atomic_compare_exchange_strong(&queue->tail, &tail, node);
                    HazardPointer_clear(&queue->hp);
                    return;
                }
            }
            else{
                atomic_compare_exchange_strong(&queue->tail, &tail, next);
                HazardPointer_clear(&queue->hp);
            }
        }

    }
}

Value LLQueue_pop(LLQueue* queue)
{
    while (true)
    {
        LLNode* head = HazardPointer_protect(&queue->hp, &queue->head); //1
        LLNode* next = atomic_load(&head->next);


        if(head == atomic_load(&queue->head)){
            Value read = atomic_exchange(&head->item, EMPTY_VALUE); //2

            if(read != EMPTY_VALUE){ //3a
                if(next){
                    if(atomic_compare_exchange_strong(&queue->head, &head, next)){
                        HazardPointer_retire(&queue->hp, head);
                    }
                }
                HazardPointer_clear(&queue->hp);
                return read;
            }
            else{ //3b
                if(!next){
                    HazardPointer_clear(&queue->hp);
                    return EMPTY_VALUE;
                }
                else{
                    if(atomic_compare_exchange_strong(&queue->head, &head, next)){
                        HazardPointer_retire(&queue->hp, head);
                    }
                }
            }
        }
    }

}

bool LLQueue_is_empty(LLQueue* queue)
{
    LLNode* head = HazardPointer_protect(&queue->hp, &queue->head);
    LLNode* next = atomic_load(&head->next);
    Value v = atomic_load(&head->item);
    HazardPointer_clear(&queue->hp);
    return !next && !v;
}
