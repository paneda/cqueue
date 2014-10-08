#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "cqueue.h"

#define NUM_THREADS 2

struct thread_args {
  int id;
  int passes;
  cqueue_spsc *in;
  cqueue_spsc *out;
};

static struct thread_args targs[NUM_THREADS];
static pthread_t threads[NUM_THREADS];

void *passer(void *targ);


int main(int argc, char** argv) {
  int passes, i;
  int *data;
  cqueue_spsc *first_q;
  cqueue_spsc *prev_q;

  if (argc != 2) {
    printf("Error: %s requires an int parameter that specifies the number of passes\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  passes = atoi(argv[1]);
  if (passes < 1) {
    printf("Error: %s requires an int parameter that specifies the number of passes\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  first_q = cqueue_spsc_new(10, sizeof(int));
  if (!first_q) {
      printf("Error: cqueue_spsc_new failed\n");
      exit(EXIT_FAILURE);
  }

  prev_q = first_q;

  for (i=0; i < NUM_THREADS; i++) {
    targs[i].id = i;
    targs[i].passes = 0;
    targs[i].in = prev_q;
    if (i == NUM_THREADS-1) {
      targs[i].out = first_q;
    } else {
      prev_q = cqueue_spsc_new(10, sizeof(int));
      if (!prev_q) {
        printf("Error: cqueue_spsc_new failed\n");
        exit(EXIT_FAILURE);
      }
      targs[i].out = prev_q;
    }
    pthread_create(&threads[i], NULL, &passer, &targs[i]);
  }

  while ((data = cqueue_spsc_trypush_slot(first_q)) == NULL);
  *data = passes;
  cqueue_spsc_push_slot_finish(first_q);

  for (i=0; i < NUM_THREADS; i++)
    pthread_join(threads[i], NULL);

  exit(EXIT_SUCCESS);
}


void *passer(void *targ) {
  struct thread_args *args = targ;
  int data; 
  int prev_data = 0;
  int *p;

  while(1) {
    while ((p = cqueue_spsc_trypop_slot(args->in)) == NULL);
    data = *p;
    cqueue_spsc_pop_slot_finish(args->in);

    if (prev_data)
      assert(data == prev_data - NUM_THREADS);
    prev_data = data;

    if (data == 0) {
      printf("thread[%d]: got zero\n", args->id);
      break;
    }
    data--;

    while ((p = cqueue_spsc_trypush_slot(args->out)) == NULL);
    *p = data;
    cqueue_spsc_push_slot_finish(args->out);

    args->passes++;
    pthread_yield();
  }

  for (int i=0; i < NUM_THREADS; i++)
    printf("thread[%d]: %d passes\n", targs[i].id, targs[i].passes);

  pthread_t self = pthread_self();

  if(self > 1)
    pthread_kill(self, SIGTERM);

  pthread_exit(NULL);
}
