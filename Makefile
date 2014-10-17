CC?=gcc
CFLAGS=-march=native -O3 -pipe -std=c11 -Wall -Werror -Wextra -Wpedantic -fPIC
CFLAGS+=-g -DCQUEUE_DEBUG -DNDEBUG
#CFLAGS+=-fsanitize=address -fsanitize=undefined -DSANITIZE -D_GNU_SOURCE
#CFLAGS+=-fsanitize=thread -fsanitize=undefined -DSANITIZE -D_GNU_SOURCE
LDFLAGS=-pthread -pie
EXES=cqueue_test cqueue_test_singlethread cqueue_test_passing cqueue_test_spsc
OBJS=cqueue.o
TEMPDIR := $(shell mktemp -d)

default: $(EXES)
cqueue_test: $(OBJS)
		$(CC) $(CFLAGS) $@.c $(OBJS) -o $@ $(LDFLAGS)
cqueue_test_singlethread: $(OBJS)
		$(CC) $(CFLAGS) $@.c $(OBJS) -o $@ $(LDFLAGS)
cqueue_test_passing: $(OBJS)
		$(CC) $(CFLAGS) -D_GNU_SOURCE $@.c $(OBJS) -o $@ $(LDFLAGS)
cqueue_test_spsc: $(OBJS)
		$(CC) $(CFLAGS) -D_GNU_SOURCE $@.c $(OBJS) -o $@ $(LDFLAGS)
%.o: %.c
		$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

docs:
		doxygen Doxyfile
		sphinx-build -b html -d $(TEMPDIR) doc/sphinx doc/html

.PHONY: clean

clean:
	rm -f $(OBJS)
	rm -f $(EXES)

