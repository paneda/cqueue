/*!
  \file
  \copyright Copyright (c) 2014, Richard Fujiyama
  Licensed under the terms of the New BSD license.
*/

#include "cqueue.h"

/*! internal representation of a spsc slot

  This is a helper struct to maintain the illusion of an array of slots.
  Contains the used member that is atomically loaded or written to by a 
  pusher or popper, and a pointer to the slot's storage space to provide
  access to pushers and poppers. Although data is a zero length array, it
  allows access to bytes beyond it that are allocated specifically for its
  use by cqueue_spsc_new().
*/
typedef struct cqueue_spsc_slot {
  _Atomic size_t used; //!< 1 when in use (has data), 0 otherwise
  unsigned char data[]; //!< pointer to data provided to pushers/poppers
} cqueue_spsc_slot;


// utility function declarations
size_t next_power2(size_t i);
int is_power2(size_t i);


// public functions declared in the header

cqueue_spsc* cqueue_spsc_new(size_t capacity, size_t elem_size) {
  size_t realcap, i, n_cachelines;
  cqueue_spsc *q;
  cqueue_spsc_slot *slot;

  if (!elem_size)
    return NULL;

  realcap = next_power2(capacity);
  if (!realcap)
    return NULL;

  assert(is_power2(LEVEL1_DCACHE_LINESIZE));
  n_cachelines = sizeof(cqueue_spsc) / LEVEL1_DCACHE_LINESIZE;
  if (n_cachelines * LEVEL1_DCACHE_LINESIZE != sizeof(cqueue_spsc))
    n_cachelines++;
  q = aligned_alloc(LEVEL1_DCACHE_LINESIZE, 
                    LEVEL1_DCACHE_LINESIZE * n_cachelines);
  if (!q)
    return NULL;

  q->capacity = realcap;

  // round the elem size up to the nearest cacheline and account for 
  // slot overhead
  n_cachelines = (elem_size + sizeof(_Atomic size_t))/ LEVEL1_DCACHE_LINESIZE;
  if (n_cachelines * LEVEL1_DCACHE_LINESIZE != 
      (elem_size + sizeof(_Atomic size_t)))
    n_cachelines++;
  q->elem_size = n_cachelines * LEVEL1_DCACHE_LINESIZE;

  // check for overflow
  i = q->capacity * q->elem_size;
  if ((i < q->capacity) || (i < q->elem_size)) {
    free(q);
    return NULL;
  }
 
  // allocate array as a cacheline-aligned chunk of elements, where each
  // element has a size that is a multiple of the cacheline size
  q->array = aligned_alloc(LEVEL1_DCACHE_LINESIZE, i);
  if (!q->array) {
    free(q);
    return NULL;
  }

  for (i=0; i < q->capacity; i++) {
    slot = (cqueue_spsc_slot *)(q->array + i*q->elem_size);
    slot->used = ATOMIC_VAR_INIT(0);
  }

  q->push_idx = 0;
  q->pop_idx = 0;

  return q;
}

void cqueue_spsc_delete(cqueue_spsc **p) {
  cqueue_spsc *q = *p;
  if(!q)
    return;

  if(q->array)
    free(q->array);

  free(q);
  *p = NULL;
}

void *cqueue_spsc_trypush_slot(cqueue_spsc *q) {
  assert(q);

  cqueue_spsc_slot *slot;
  slot = (cqueue_spsc_slot *)(q->array + q->push_idx * q->elem_size);

  // check if the queue is full, ie we are trying to write to a used slot
  if (atomic_load_explicit(&slot->used, memory_order_acquire))
    return NULL;

  return slot->data;
}

void cqueue_spsc_push_slot_finish(cqueue_spsc *q) {
  assert(q);

  cqueue_spsc_slot *slot;
  slot = (cqueue_spsc_slot *)(q->array + q->push_idx * q->elem_size);
  
  atomic_store_explicit(&slot->used, 1, memory_order_release);
  q->push_idx = (q->push_idx + 1) & (q->capacity - 1);
}

void *cqueue_spsc_trypop_slot(cqueue_spsc *q) {
  assert(q);

  cqueue_spsc_slot *slot;
  slot = (cqueue_spsc_slot *)(q->array + q->pop_idx * q->elem_size);

  // check if the queue is empty, ie we are trying to read an unused slot
  if (!atomic_load_explicit(&slot->used, memory_order_acquire))
    return NULL;

  return slot->data;
}

void cqueue_spsc_pop_slot_finish(cqueue_spsc *q) {
  assert(q);

  cqueue_spsc_slot *slot;
  slot = (cqueue_spsc_slot *)(q->array + q->push_idx * q->elem_size);

  atomic_store_explicit(&slot->used, 0, memory_order_release);
  q->pop_idx = (q->pop_idx + 1) & (q->capacity - 1);
}


// private utility functions

/*! Round up to the next power of 2

  \param[in] i the int to round, 0 <= i <= SIZE_MAX/2 +1
  \returns the next highest power of 2
  \returns i if is already a power of 2
  \returns 0 when i > SIZE_MAX/2 +1
*/
size_t next_power2(size_t i) {
  if (i == 0)
    return 1;

  i--;
  for (int j = 1; j < CHAR_BIT*sizeof(size_t)-1; j*=2)
    i |= i >> j;
  i++;

  return i;
}

/*! Check if i is a power of 2

  \param[in] i the int to check, 0 <= i <= SIZE_MAX
  \returns 1 if i is a power of 2, 0 otherwise
*/
int is_power2(size_t i) {
  if (i == 0)
    return 0;

  if ((i & (i-1)) == 0)
    return 1;

  return 0;
}
