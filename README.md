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
- C11 atomics (tested with gcc 4.9 on Linux x86_64)
- pthreads (few compilers support C11 threads)
