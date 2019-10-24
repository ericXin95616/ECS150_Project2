# Target library
lib := libuthread.a
CC	:= gcc
CFLAGS	:= -Wall -Werror

all: $(lib)

libuthread.a: queue.o
	ar rcs libuthread.a queue.o

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c -o queue.o queue.c

clean:
	rm -f libuthread.a queue.o
## TODO: Phase 1.1
