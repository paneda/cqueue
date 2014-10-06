CC?=gcc
CFLAGS=-g -march=native -O3 -pipe -std=c11 -Wall -Werror
LDFLAGS=-g -lpthread
EXES=
OBJS=cqueue.o
TEMPDIR := $(shell mktemp -d)

default: $(EXES)
squeue_test: $(OBJS) squeue_test.c
		$(CC) squeue_test.c $(OBJS) -o $@ $(LDFLAGS)
squeue_test_threads: $(OBJS) squeue_test_threads.c
		$(CC) squeue_test_threads.c $(OBJS) -o $@ $(LDFLAGS)
squeue_test_passing: $(OBJS) squeue_test_passing.c
		$(CC) squeue_test_passing.c $(OBJS) -o $@ $(LDFLAGS)
%.o: %.c
		$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

docs:
		doxygen Doxyfile
		sphinx-build -b html -d $(TEMPDIR) doc/sphinx doc/html

.PHONY: clean

clean:
	rm -f $(OBJS)
	rm -f $(EXES)

