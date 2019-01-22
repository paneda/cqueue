#ifdef NDEBUG
#undef NDEBUG
#endif

#include "cqueue.h"
#include <stdio.h>  // printf
#include <assert.h>
#include <stddef.h> // ptrdiff_t
#include <stdio.h>  // printf

#define PASSFAIL(fn) \
  if(fn) \
    printf("PASS: %s\n", #fn); \
  else \
    printf("FAIL: %s\n", #fn);

// function declarations
int spsc_new_pass();
int spsc_new_fail();
int spsc_trypush_slot_pass();
int spsc_trypop_slot_pass();


int main() {
  PASSFAIL(spsc_new_pass());
  PASSFAIL(spsc_new_fail());
  PASSFAIL(spsc_trypush_slot_pass());
  PASSFAIL(spsc_trypop_slot_pass());

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
  d = ((char *)(void *)q - (char *)(void *)&q->capacity);
  assert(d == 0); // capacity is the first member

  d = (char *)(void *)&q->push_idx - (char*)(void *)&q->capacity;
  assert(d == LEVEL1_DCACHE_LINESIZE);  // should be 1 cacheline due to padding

  d = (char *)(void *)&q->pop_idx - (char *)(void *)&q->push_idx;
  assert(d == LEVEL1_DCACHE_LINESIZE); // should be 1 cacheline due to padding

  // check deletion
  cqueue_spsc_delete(&q);
  assert(!q);

  // check elem_size padding rounding up
  q  = cqueue_spsc_new(26, LEVEL1_DCACHE_LINESIZE-sizeof(size_t)+1);
  assert(q);
  assert(q->elem_size == 2*LEVEL1_DCACHE_LINESIZE);  // round up to 2 cachelines

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
  q = cqueue_spsc_new(1, SIZE_MAX-sizeof(_Atomic size_t)-LEVEL1_DCACHE_LINESIZE);
  assert(!q);

  // fail on overflow of capacity * elem_size
  q = cqueue_spsc_new(SIZE_MAX/LEVEL1_DCACHE_LINESIZE, sizeof(int));
  assert(!q);

  return 1;
}

int spsc_trypush_slot_pass() {
  cqueue_spsc *q;
  int i;
  char data;
  char *p;

  q  = cqueue_spsc_new(26, sizeof(char));
  assert(q);

  // insert dummy data for testing
  for(i=0, data='A'; i < 26; i++, data++) {
    p = cqueue_spsc_trypush_slot(q);
    assert(p);
    *p = data;
    cqueue_spsc_push_slot_finish(q);
	assert(cqueue_spsc_get_no_used_slots(q) == (size_t)i + 1);
  }

  // check index, used flag, and data
  assert(q->push_idx == 26);
  void *offset = (char *)(q->array + 12*q->elem_size);
  size_t used = *(size_t *)offset;
  assert(used == 1);
  char *c = (char *)offset + sizeof(size_t);
  assert(*c == 'M');

  return 1;
}


int spsc_trypop_slot_pass() {
  cqueue_spsc *q;
  int i;
  char data;
  char *p;

  q  = cqueue_spsc_new(26, sizeof(char));
  assert(q);

  // insert dummy data for testing
  for(i=0, data='A'; i < 26; i++, data++) {
    p = cqueue_spsc_trypush_slot(q);
    assert(p);
    *p = data;
    cqueue_spsc_push_slot_finish(q);
	assert(cqueue_spsc_get_no_used_slots(q) == (size_t) i + 1);
  }

  // pop some data so we're not at the start
  size_t start_count = cqueue_spsc_get_no_used_slots(q);
  for(i=0; i < 13; i++) {
    p = cqueue_spsc_trypop_slot(q);
    assert(p);
    cqueue_spsc_pop_slot_finish(q);
	assert(cqueue_spsc_get_no_used_slots(q) == start_count - i - 1);
  }

  // check index, used flag, and data
  assert(q->pop_idx == 13);
  void *offset = (char *)(q->array + 12*q->elem_size);
  size_t used = *(size_t *)offset;
  assert(used == 0);
  assert(*p == 'M');

  return 1;
}
