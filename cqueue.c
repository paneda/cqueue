#include "cqueue.h"

typedef struct cqueue_spsc_slot {
  _Atomic size_t used;
  unsigned char data[];
} cqueue_spsc_slot;

// utility functions
uint64_t next_power2(uint64_t i);
int is_power2(uint64_t i);


// allocates and initializes a queue capable of holding at least capacity
// number of elements of at most elem_size size
// returns the address of the newly allocated queue, or NULL on error
cqueue_spsc* cqueue_spsc_new(size_t capacity, size_t elem_size) {
  int n_cachelines;
  size_t realcap, i;
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
  if (n_cachelines * LEVEL1_DCACHE_LINESIZE != elem_size)
    n_cachelines++;
  q->elem_size = n_cachelines * LEVEL1_DCACHE_LINESIZE;
 
  // allocate array as a cacheline-aligned chunk of elements, where each
  // element has a size that is a multiple of the cacheline size
  q->array = aligned_alloc(LEVEL1_DCACHE_LINESIZE,
                          q->capacity * q->elem_size);
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

// returns a pointer to the next available queue slot for pushing
// returns NULL when the queue is full
// cqueue_spsc_push_slot_finish must be called after a successful call
// ex: cqueue_spsc_trypush_slot, write data, cqueue_spsc_push_slot_finish
void *cqueue_spsc_trypush_slot(cqueue_spsc *q) {
  assert(q);

  cqueue_spsc_slot *slot;
  slot = (cqueue_spsc_slot *)(q->array + q->push_idx * q->elem_size);

  // check if the queue is full, ie we are trying to write to a used slot
  if (atomic_load_explicit(&slot->used, memory_order_acquire))
    return NULL;

  return slot->data;
}

// publish the fact that the push slot is now used
// must be called after a successful cqueue_spsc_trypush_slot call
// ex: cqueue_spsc_trypush_slot, write data, cqueue_spsc_push_slot_finish
// not calling this function may result in queue inconsistency
void cqueue_spsc_push_slot_finish(cqueue_spsc *q) {
  assert(q);

  cqueue_spsc_slot *slot;
  slot = (cqueue_spsc_slot *)(q->array + q->push_idx * q->elem_size);
  
  atomic_store_explicit(&slot->used, 1, memory_order_release);
  q->push_idx = (q->push_idx + 1) & (q->capacity - 1);
}

// returns a pointer to the next available queue slot for popping
// returns NULL when the queue is empty
// cqueue_spsc_pop_slot_finish must be called after a successful call
// ex: cqueue_spsc_trypop_slot, read data, cqueue_spsc_pop_slot_finish
void *cqueue_spsc_trypop_slot(cqueue_spsc *q) {
  assert(q);

  cqueue_spsc_slot *slot;
  slot = (cqueue_spsc_slot *)(q->array + q->pop_idx * q->elem_size);

  // check if the queue is empty, ie we are trying to read an unused slot
  if (!atomic_load_explicit(&slot->used, memory_order_acquire))
    return NULL;

  return slot->data;
}

// publish the fact that the pop slot is now unused
// must be called after a successful cqueue_spsc_trypop_slot call
// ex: cqueue_spsc_trypop_slot, read data, cqueue_spsc_pop_slot_finish
// not calling this function may result in queue inconsistency
void cqueue_spsc_pop_slot_finish(cqueue_spsc *q) {
  assert(q);

  cqueue_spsc_slot *slot;
  slot = (cqueue_spsc_slot *)(q->array + q->push_idx * q->elem_size);

  atomic_store_explicit(&slot->used, 0, memory_order_release);
  q->pop_idx = (q->pop_idx + 1) & (q->capacity - 1);
}

// returns the next highest power of 2
// if i is already a power of 2, just return it
// returns 0 when i > (1UL << 63))
uint64_t next_power2(uint64_t i) {
  if (i == 0)
    return 1;

  i--;
  i |= i >> 1;
  i |= i >> 2;
  i |= i >> 4;
  i |= i >> 8;
  i |= i >> 16;
  i |= i >> 32;
  i++;

  return i;
}

int is_power2(uint64_t i) {
  if (i == 0)
    return 0;

  if ((i & (i-1)) == 0)
    return 1;

  return 0;
}
