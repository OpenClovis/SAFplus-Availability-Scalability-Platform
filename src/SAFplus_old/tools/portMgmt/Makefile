CC := gcc
DEBUG_CFLAGS := -DDEBUG
CFLAGS := -Wall -g  $(DEBUG_CFLAGS) -std=c99 -pedantic -D_GNU_SOURCE
SOURCES := port_mgmt.c port_mgmt_main.c
OBJS :=$(SOURCES:%.c=%.o)
TARGETS = port_mgmt
LD_LIBS = -lrt

all: $(TARGETS)

port_mgmt: $(OBJS)
	$(CC) -o $@ $^ $(LD_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

clean:
	rm -f $(TARGETS) $(OBJS) *~