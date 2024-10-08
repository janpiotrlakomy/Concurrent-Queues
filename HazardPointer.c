#include <malloc.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#include "HazardPointer.h"

thread_local int _thread_id = -1;
int _num_threads = -1;

void HazardPointer_register(int thread_id, int num_threads)
{
    if(thread_id == 0){
        _num_threads = num_threads;
    }

    _thread_id = thread_id;
}

void HazardPointer_initialize(HazardPointer* hp)
{
    for (size_t i = 0; i < MAX_THREADS; i++)
    {
        atomic_init(&(hp->pointer[i]), NULL);
        for (size_t j = 0; j < RETIRED_THRESHOLD; j++){
            hp->retired[i][j] = NULL;
        }
    }
}

void HazardPointer_finalize(HazardPointer* hp)
{
    for (size_t i = 0; i < _num_threads; i++)
    {
        for (size_t j = 0; j < RETIRED_THRESHOLD; j++){
            if(hp->retired[i][j]){
                free(hp->retired[i][j]);
            }
        }
    }
}

void* HazardPointer_protect(HazardPointer* hp, const _Atomic(void*)* atom)
{
    void* ptr;
    do {
        ptr = atomic_load(atom);
        atomic_store(&(hp->pointer[_thread_id]), ptr);
    } while(atomic_load(&(hp->pointer[_thread_id])) != atomic_load(atom));

    return atomic_load(&(hp->pointer[_thread_id]));
}

void HazardPointer_clear(HazardPointer* hp)
{
    atomic_store(&(hp->pointer[_thread_id]), NULL);
}

void HazardPointer_retire(HazardPointer* hp, void* ptr)
{
    int first_free = 0;
    while( first_free < RETIRED_THRESHOLD && hp->retired[_thread_id][first_free] != NULL){
        first_free++;
    }

    // If the set of retired pointers is full.
    while (first_free >= RETIRED_THRESHOLD){
        for (size_t i = 0; i < RETIRED_THRESHOLD; i++){
            bool to_free = true;
            void* iptr = hp->retired[_thread_id][i];

            // Check if the pointer is protected;
            for (size_t j = 0; j < _num_threads; j++){
                if(atomic_load(&((hp->pointer)[j])) == iptr){
                    to_free = false;
                    break;
                }
            }

            // If it wasn't protected free.
            if(to_free){                    // At least 1 shoudln't be protected.
                first_free = i;
                hp->retired[_thread_id][i] = NULL;
                free(iptr);
            }

        }

    }

    hp->retired[_thread_id][first_free] = ptr;

}
