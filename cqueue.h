#ifndef _CQUEUE_
#define _CQUEUE_

#include <stdlib.h>     // aligned_alloc
#include <stdint.h>     // uintXX_t
#include <assert.h>
#include <stdatomic.h>  // atomics

#ifndef LEVEL1_DCACHE_LINESIZE
#define LEVEL1_DCACHE_LINESIZE 64
#endif

typedef struct cqueue_spsc {
  // stick read-only elements in front to account for reads outside the struct
  // reading into the struct at most 1 cacheline
  size_t capacity;
  size_t elem_size;
  unsigned char *array;
  char pad1[LEVEL1_DCACHE_LINESIZE - 2 * sizeof(size_t) 
            - sizeof(unsigned char*)]; 
  // ensure that push_idx and pop_idx are on their own cachelines to
  // prevent false sharing
  size_t push_idx;
  char pad2[LEVEL1_DCACHE_LINESIZE - sizeof(size_t)];
  size_t pop_idx;
  char pad3[LEVEL1_DCACHE_LINESIZE - sizeof(size_t)];
} cqueue_spsc;

// allocates and initializes a queue capable of holding at least capacity
// number of elements of at most elem_size size
// returns the address of the newly allocated queue, or NULL on error
cqueue_spsc* cqueue_spsc_new(size_t capacity, size_t elem_size);

// returns a pointer to the next available queue slot for pushing
// returns NULL when the queue is full
// cqueue_spsc_push_slot_finish must be called after a successful call
// ex: cqueue_spsc_trypush_slot, write data, cqueue_spsc_push_slot_finish
void *cqueue_spsc_trypush_slot(cqueue_spsc *q);

// publish the fact that the push slot is now used
// must be called after a successful cqueue_spsc_trypush_slot call
// ex: cqueue_spsc_trypush_slot, write data, cqueue_spsc_push_slot_finish
// not calling this function may result in queue inconsistency
void cqueue_spsc_push_slot_finish(cqueue_spsc *q);

// returns a pointer to the next available queue slot for popping
// returns NULL when the queue is empty
// cqueue_spsc_pop_slot_finish must be called after a successful call
// ex: cqueue_spsc_trypop_slot, read data, cqueue_spsc_pop_slot_finish
void *cqueue_spsc_trypop_slot(cqueue_spsc *q);

// publish the fact that the pop slot is now unused
// must be called after a successful cqueue_spsc_trypop_slot call
// ex: cqueue_spsc_trypop_slot, read data, cqueue_spsc_pop_slot_finish
// not calling this function may result in queue inconsistency
void cqueue_spsc_pop_slot_finish(cqueue_spsc *q);

#endif
