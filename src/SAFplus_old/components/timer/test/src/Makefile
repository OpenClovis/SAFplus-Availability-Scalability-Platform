CC=gcc
SRCS= timer_test.c
ifeq ($(CASCADE), 1)
	SRCS += timer_cascade.c
else
	SRCS += timer_tree.c rbtree.c
endif

OBJS=$(SRCS:%.c=%.o)
TARGET=clTimerTest
LD_LIBS=-lpthread -lrt -lm
DEBUG=-g
CFLAGS = $(DEBUG) -Wall  # -DTEST_REPETITIVE -fomit-frame-pointer -O2  

ifeq ($(CASCADE), 1)
	CFLAGS += -DTIMER_DRIFT=1000000
endif

ifeq ($(REP), 1)
	CFLAGS += -DTEST_REPETITIVE
endif

all: clean $(TARGET)

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(LD_LIBS)

%.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) *~
