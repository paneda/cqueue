CC?=gcc
CFLAGS=-g -march=native -O3 -pipe -std=c11 -Wall -Werror
LDFLAGS=-g -lpthread
EXES=
OBJS=cqueue.o

default: $(EXES)
squeue_test: $(OBJS) squeue_test.c
		$(CC) squeue_test.c $(OBJS) -o $@ $(LDFLAGS)
squeue_test_threads: $(OBJS) squeue_test_threads.c
		$(CC) squeue_test_threads.c $(OBJS) -o $@ $(LDFLAGS)
squeue_test_passing: $(OBJS) squeue_test_passing.c
		$(CC) squeue_test_passing.c $(OBJS) -o $@ $(LDFLAGS)
%.o: %.c
		$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

.PHONY: clean

clean:
	rm -f $(OBJS)
	rm -f $(EXES)

