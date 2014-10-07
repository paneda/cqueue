/*!
  \file
  \copyright Copyright (c) 2014, Richard Fujiyama
  Licensed under the terms of the New BSD license.
*/

#ifndef _CQUEUE_
#define _CQUEUE_

#include <stdlib.h>     // aligned_alloc
#include <stdint.h>     // uintXX_t
#include <limits.h>     // CHAR_BIT
#include <assert.h>
#include <stdatomic.h>  // atomics


#ifndef LEVEL1_DCACHE_LINESIZE
/*! Used to align and place objects to avoid false sharing

  On modern Intel x86 CPUs, this is typically 64 bytes
  On Linux it can be defined while compiling with 
  \verbatim -DLEVEL1_DCACHE_LINESIZE=`getconf LEVEL1_DCACHE_LINESIZE` \endverbatim
*/
#define LEVEL1_DCACHE_LINESIZE 64
#endif

/*! The main struct for spsc cqueues

  These should only be allocated by cqueue_spsc_new() since there are strict
  cacheline alignment and padding issues to enable lockless operation.

  Push and pop operations are thread safe for at most one concurrent push and
  pop operation (ie, a single reader and a single writer).
*/
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

/*! Allocates and initializes a queue capable of holding at least capacity
 number of elements of at most elem_size size
  \param[in] capacity the minimum number of elements that the queue will hold
  \param[out] elem_size the maximum size of any element which is stored in the queue
  \return the address of the newly allocated queue, or NULL on error
*/
cqueue_spsc* cqueue_spsc_new(size_t capacity, size_t elem_size);

/*! Deallocates the queue

  \param[in,out] p a pointer to the pointer to the queue to be deallocated. 
  On success, *p will be set to NULL.
*/
void cqueue_spsc_delete(cqueue_spsc **p);

/*! Get a pointer to the next available queue slot for pushing

  cqueue_spsc_push_slot_finish must be called after a successful call

  ex: cqueue_spsc_trypush_slot(), write data, cqueue_spsc_push_slot_finish()
  \returns a pointer to the next available queue slot, or NULL when the queue is full
*/
void *cqueue_spsc_trypush_slot(cqueue_spsc *q);

/*! Publish the fact that the push slot is now used

  Must be called after a successful cqueue_spsc_trypush_slot call

  ex: cqueue_spsc_trypush_slot(), write data, cqueue_spsc_push_slot_finish()
  \warning Not calling this function may result in queue inconsistency
*/
void cqueue_spsc_push_slot_finish(cqueue_spsc *q);

/*! Get a pointer to the next available queue slot for popping

  cqueue_spsc_pop_slot_finish must be called after a successful call

  ex: cqueue_spsc_trypop_slot(), read data, cqueue_spsc_pop_slot_finish()
  \returns a pointer to the next available queue slot, or NULL when the queue is empty
*/
void *cqueue_spsc_trypop_slot(cqueue_spsc *q);

/*! Publish the fact that the pop slot is now unused

  Must be called after a successful cqueue_spsc_trypop_slot call

  ex: cqueue_spsc_trypop_slot(), read data, cqueue_spsc_pop_slot_finish()
  \warning Not calling this function may result in queue inconsistency
*/
void cqueue_spsc_pop_slot_finish(cqueue_spsc *q);

#endif
