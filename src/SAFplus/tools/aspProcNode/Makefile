CC := gcc

SRCS := $(wildcard *.c)

OBJS := $(SRCS:%.c=%.o)

TARGET := aspCompNode

THIRD_PARTY_EZXML_PATH := $(CLOVIS_ROOT)/SAFplus/3rdparty/ezxml/stable

CFLAGS := -Wall -I$(THIRD_PARTY_EZXML_PATH)

DEBUG_FLAGS := -g

THIRDPARTY_LIBS := -L$(ASP_LIB) -lezxml

LDLIBS := $(THIRDPARTY_LIBS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(DEBUG_FLAGS) $(LDLIBS) -o $@ $^

%.o:%.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS) *~