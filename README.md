cqueue
======

concurrent bounded queue (fifo)

**Features:**
- concurrent (thread-safe)
- bounded (array-based, memory allocated up front)
- stores arbitrary data (not just pointers or native types)
- written in C (uses C11 atomics)
- BSD licensed (3 clause)

**Requirements:**
- C11 atomics (tested with gcc 4.9.1 on Linux x86_64 and clang 3.3 on FreeBSD 10 x86_64)
- pthreads (few compilers support C11 threads)

**Status**
- pre-alpha code under active development
- SPSC (single consumer, single producer) lockless queue is implemented

**SPSC API Example**

    #include "cqueue.h"

    cqueue_spsc *q = cqueue_spsc_new(26, sizeof(char));
    if (!cqueue_spsc)
      exit(EXIT_FAILURE);
    
    char *data;
    while((data = cqueue_spsc_trypush_slot(q)) == NULL);
    *data = 'a';
    cqueue_spsc_push_slot_finish(q);
    
    while((data = cqueue_spsc_trypop_slot(q)) == NULL);
    printf("%c\n", *data);
    cqueue_spsc_pop_slot_finish(q);
    
  
