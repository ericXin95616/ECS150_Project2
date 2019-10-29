# Target library
lib := libuthread.a
objs := uthread.o queue.o context.o preempt.o
CC	:= gcc
CFLAGS	:= -Wall -Werror

all: $(lib)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

$(lib): $(objs)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

clean:
	rm -f libuthread.a *.o *.d
## TODO: Phase 1.1
