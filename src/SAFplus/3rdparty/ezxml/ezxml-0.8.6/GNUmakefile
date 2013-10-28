# Makefile
#
# Copyright 2005 Aaron Voisine <aaron@voisine.org>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

COMPNAME	= $(shell pwd | awk -FSAFplus/ '{print $$2}')
OBJ_DIR		:= $(ASP_TARGET)/obj/$(COMPNAME)
DEP_DIR		:= $(ASP_TARGET)/dep/$(COMPNAME)
LIB_DIR		:= $(ASP_TARGET)/lib
include $(CLOVIS_ROOT)/SAFplus/mk/prefix.mk

override CC = gcc
#AR = ar
RM = rm -f
CFLAGS += -Wall -O2 
ifeq ($(BUILD_SHARED), 1)
 CFLAGS += -fPIC
endif
DEBUG_CFLAGS += -O0 -g
OBJS = $(OBJ_DIR)/ezxml.o
LIB = $(LIB_DIR)/libezxml.a
SHLIB = $(LIB_DIR)/libezxml.so
TEST = ezxmltest
ifdef NOMMAP
CFLAGS += -D EZXML_NOMMAP
endif
ifdef DEBUG
CFLAGS += $(DEBUG_CFLAGS)
endif

ifeq ($(BUILD_SHARED), 1)
all: $(LIB) $(SHLIB)
else
all: $(LIB)
endif

$(LIB): $(OBJS)
	$(AR) rcs $(LIB) $(OBJS)

$(SHLIB): $(OBJS)
	$(CC)  -shared -fPIC -o $@ $^

nommap: CFLAGS += -D EZXML_NOMMAP
nommap: all

debug: CFLAGS += $(DEBUG_CFLAGS)
debug: all

test: CFLAGS += $(DEBUG_CFLAGS)
test: $(TEST)

$(TEST): CFLAGS += -D EZXML_TEST
$(TEST): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJ_DIR)/ezxml.o: ezxml.h ezxml.c

#.c.o:
	$(shell mkdir -p $(DEP_DIR) $(OBJ_DIR) $(LIB_DIR))

	$(CC) $(CFLAGS) -c -o $(OBJ_DIR)/ezxml.o ezxml.c

clean:
	$(RM) $(OBJS) $(LIB) $(SHLIB) $(TEST) *~
