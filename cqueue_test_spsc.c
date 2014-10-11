#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>   // PRIu64
#include "cqueue.h"

#define NUM_THREADS 2

typedef enum {
  PRODUCER,
  CONSUMER
} thread_type;


struct thread_args {
  char pad1[LEVEL1_DCACHE_LINESIZE/2];
  uint64_t limit;
  cqueue_spsc *q;
  int id;
  thread_type t;
  uint64_t passes;
  char pad2[LEVEL1_DCACHE_LINESIZE/2];
};

static struct thread_args targs[NUM_THREADS];
static pthread_t threads[NUM_THREADS];

void *producer(void *targ);
void *consumer(void *targ);

int main(int argc, char** argv) {
  int passes, i;
  cqueue_spsc *q;

  if (argc != 2) {
    printf("Error: %s requires an int parameter that specifies the number of passes\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  passes = atoi(argv[1]);
  if (passes < 1) {
    printf("Error: %s requires an int parameter that specifies the number of passes\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  q = cqueue_spsc_new(1024, sizeof(uint64_t));
  if (!q) {
      printf("Error: cqueue_spsc_new failed\n");
      exit(EXIT_FAILURE);
  }

  for (i=0; i < NUM_THREADS; i++) {
    targs[i].id = i;
    targs[i].limit = passes;
    targs[i].passes = 0;
    targs[i].t = PRODUCER;
    targs[i].q = q;
    if (i == NUM_THREADS-1) {
      targs[i].t = CONSUMER;
      pthread_create(&threads[i], NULL, &consumer, &targs[i]);
    } else {
      pthread_create(&threads[i], NULL, &producer, &targs[i]);
    }
  }

  for (i=0; i < NUM_THREADS; i++)
    pthread_join(threads[i], NULL);

  exit(EXIT_SUCCESS);
}

void *producer(void *targ) {
  struct thread_args *args = targ;
  uint64_t data;
  uint64_t *p;

  for (data=1; data <= args->limit; data++) {
    p = cqueue_spsc_push_slot(args->q);
    *p = data;
    cqueue_spsc_push_slot_finish(args->q);
    args->passes++;
  }

  pthread_exit(NULL);
}

void *consumer(void *targ) {
  struct thread_args *args = targ;
  uint64_t data; 
  uint64_t *p;

  while(1) {
    p = cqueue_spsc_pop_slot(args->q);
    data = *p;
    cqueue_spsc_pop_slot_finish(args->q);

    args->passes++;
    if (data == args->limit) {
      printf("thread[%d]: got limit\n", args->id);
      break;
    }
  }

  for (int i=0; i < NUM_THREADS; i++)
    printf("thread[%d]: %" PRIu64 " passes\n", targs[i].id, targs[i].passes);

  pthread_kill(pthread_self(), SIGTERM);

  pthread_exit(NULL);
}
