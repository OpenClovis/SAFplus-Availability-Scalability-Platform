CC := gcc
DEBUG := -g	
EXTRA_FLAGS = #-DAPPEND_CONTINUATION
EXTRA_CFLAGS = 
CFLAGS = -Wall $(EXTRA_CFLAGS) $(DEBUG)
LIBS := libcontinuation.so
LDLIBS = -lpthread
LDFLAGS = 
SRCS := continuation.c
OBJS := $(SRCS:%.c=%.o)
TEST_BINARIES := continuation_test_1 continuation_test_2 continuation_test_3
TEST_OBJS := continuation_test_1.o continuation_test_2.o continuation_test_3.o continuation_player.o

ifeq ($(TEST), 1)
	TARGETS := $(TEST_BINARIES)
	LDFLAGS += -L./. 
	LDLIBS += -lcontinuation
	EXTRA_CFLAGS += $(EXTRA_FLAGS)
else
	TARGETS := $(LIBS) tests
	EXTRA_CFLAGS += -fPIC -shared
	LDFLAGS += -shared -fPIC
endif
ARCH_FLAGS :=

ifeq ("$(ARCH)", "x86")
	ARCH_FLAGS := -m32
endif

CFLAGS += $(ARCH_FLAGS)

all: $(TARGETS)

libcontinuation.so: $(OBJS)
	$(CC) $(ARCH_FLAGS) $(LDFLAGS) $(DEBUG) -o $@ $^ $(LDLIBS)

continuation_test_1: continuation_test_1.o continuation_player.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

continuation_test_2: continuation_test_2.o continuation_player.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

continuation_test_3: continuation_test_3.o continuation_player.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

tests:
	$(MAKE) TEST=1

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(LIBS) $(TEST_BINARIES) *~
