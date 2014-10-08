#include <stdlib.h>   // exit
#include <stdio.h>    // printf

#include "cqueue.h"

typedef struct slot {
  _Atomic size_t used; //!< 1 when in use (has data), 0 otherwise
  unsigned char *data; //!< pointer to data provided to pushers/poppers
} slot;

int main() {
  cqueue_spsc *q = NULL;
  int i;
  int *p;

  q = cqueue_spsc_new(1024, 4);
  if (!q) {
    printf("cqueue_init failed\n");
    exit(EXIT_FAILURE);
  }

  uint32_t data[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };

  for (p = cqueue_spsc_trypush_slot(q), i=0; 
        p; 
        p = cqueue_spsc_trypush_slot(q), i=(i+1)%10) {
    *p = data[i];
    cqueue_spsc_push_slot_finish(q);
  }

  for (p = cqueue_spsc_trypop_slot(q); p; p = cqueue_spsc_trypop_slot(q)) {
    printf("%u ", *p);
    cqueue_spsc_pop_slot_finish(q);
  }
  printf("\n********************\n");

  for (i = 0; i < 1024; i++) {
    p = cqueue_spsc_trypush_slot(q);
    if (!p)
      continue;
    *p = data[i%10];
    cqueue_spsc_push_slot_finish(q);
    p = cqueue_spsc_trypop_slot(q);
    if (!p)
      continue;
    printf("%u ", *p);
    cqueue_spsc_pop_slot_finish(q);
  }
  printf("\n********************\n");

  exit(EXIT_SUCCESS);
}
