CC := gcc

ifeq ($(debug), y)
    CFLAGS := -g
else
    CFLAGS := -O2 -DNDEBUG
endif

CFLAGS := $(CFLAGS) -Wall -Werror
LIBS := -lpthread

OBJS := $(patsubst %.c, %.o, $(wildcard *.c))

TARGET := test_rbtree test_atomic test_str_util test_hash

.PHONY: all clean

all: $(TARGET)

test_rbtree: test_rbtree.o rbtree.o
	$(CC) $(CFLAGS) -o $@ $^

test_atomic: test_atomic.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test_str_util: test_str_util.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test_hash: test_hash.o hash.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
