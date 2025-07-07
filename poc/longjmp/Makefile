CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

TARGETS = basic_example.out error_handling.out wrapped_malloc.out

all: $(TARGETS)

basic_example.out: basic_example.c
	$(CC) $(CFLAGS) -o $@ $<

error_handling.out: error_handling.c
	$(CC) $(CFLAGS) -o $@ $<

wrapped_malloc.out: wrapped_malloc.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

.PHONY: all clean run-basic run-error