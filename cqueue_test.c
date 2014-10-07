#include "cqueue.h"
#include <stdio.h>  // printf
#include <assert.h>
#include <stddef.h> // ptrdiff_t

#define PASSFAIL(fn) \
  if(fn) \
    printf("PASS: %s\n", #fn); \
  else \
    printf("FAIL: %s\n", #fn);

// function declarations
int spsc_new_pass();
int spsc_new_fail();


int main() {
  PASSFAIL(spsc_new_pass());
  PASSFAIL(spsc_new_fail());
  return 0;
}


int spsc_new_pass() {
  cqueue_spsc *q;
  ptrdiff_t d;

  q  = cqueue_spsc_new(26, LEVEL1_DCACHE_LINESIZE-sizeof(size_t));

  assert(q);
  assert(q->capacity == 32);  // round up to power of 2
  assert(q->elem_size == LEVEL1_DCACHE_LINESIZE);  // round up to cacheline

  // check initialization
  assert(q->push_idx == 0);
  assert(q->pop_idx == 0);
  for(size_t i=0; i < q->capacity; i++) {
    size_t used = *(size_t *)(q->array + i*q->elem_size);
    assert(used == 0);
  }

  // check struct layout
  d = ((void *)q - (void *)&q->capacity);
  assert(d == 0); // capacity is the first member

  d = (void *)&q->push_idx - (void *)&q->capacity;
  assert(d == LEVEL1_DCACHE_LINESIZE);  // should be 1 cacheline due to padding

  d = (void *)&q->pop_idx - (void *)&q->push_idx;
  assert(d == LEVEL1_DCACHE_LINESIZE); // should be 1 cacheline due to padding

  // todo: free q
  // todo: check elem_size padding out to cachelines
  return 1;
}

int spsc_new_fail() {
  cqueue_spsc *q;

  // fail on capacity
  q = cqueue_spsc_new(SIZE_MAX/2 + 2, sizeof(int));
  assert(!q);

  // fail on elem_size
  q = cqueue_spsc_new(32, 0);
  assert(!q);

  // fail on memory allocation
  q = cqueue_spsc_new(1, SIZE_MAX-sizeof(_Atomic size_t));
  assert(!q);

  // fail on overflow of capacity * elem_size
  q = cqueue_spsc_new(SIZE_MAX/LEVEL1_DCACHE_LINESIZE, sizeof(int));
  assert(!q);

  return 1;
}

