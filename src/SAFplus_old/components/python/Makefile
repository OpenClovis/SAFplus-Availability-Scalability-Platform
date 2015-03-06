# The OpenClovis make infrastructure defines some variables before calling this
# makefile. If OpenClovis has defined the variable, it is important to use it 
# or features like cross-building may not work.

# But if the variables are not defined, then I define them so that this make
# can be called outside of the infrastructure.

CC      ?= gcc
CPL     ?= g++
ifeq ($(CC),cc)  # Default make invocation assigns this variable, but we only want it if its assigned by OpenClovis framework
override CC := gcc
endif
ifeq ($(CC),g++)  # Default make invocation assigns this variable, but we only want it if its assigned by OpenClovis framework
override CC := gcc
endif

CLOVIS_ROOT ?= /code/svn/clasp/root/pkg/src
AR          ?= ar
BIN_DIR     ?=.
OBJ_DIR     ?=.
SISP_LIBS   ?=.
LIB_DIR     ?=$(SISP_LIBS)
MKDIR       ?=mkdir -p
CP          ?=cp -f

ifndef PYTHON_INC
PYTHON_INC := $(wildcard /usr/local/include/python2.*)
PYTHON_LIB := $(wildcard /usr/local/lib/libpython2.*.so)
ifeq ($(PYTHON_INC),)
PYTHON_INC := $(wildcard /usr/include/python2.*)
PYTHON_LIB := $(wildcard /usr/lib/libpython2.*.so)
endif

ifeq ($(PYTHON_INC),)
$(warning Python Headers are NOT found in your distribution.  Please install)
else
PYTHON_INC := -I$(PYTHON_INC)
endif

endif



# Prebuild attempts
#ifdef ASP_INCLUDE
#AspInclude      := -I$(ASP_INCLUDE)
#else
#AspInclude      := -I/code/build/include
#endif

AspInclude := -I$(CLOVIS_ROOT)/SAFplus/include

#LIB_DIR?=/code/build/target/i686/linux-2.6.20/lib

#ASP_LIB_LIST2:=$(shell ls $(LIB_DIR)/*.a)
#ASP_LIB_LIST:=$(filter-out %Dummy.a, $(ASP_LIB_LIST2))
#ASP_LIB_LIST:=$(filter-out %Ground.a, $(ASP_LIB_LIST))
ASP_LIB_LIST:= -L$(ASP_LIB) -L$(ASP_LIBDIR) -lmw -lClEo -lezxml

#  -fno-threadsafe-statics # Do not emit the extra code to use the routines specified in the C++ ABI for thread-safe initialization of local statics. You can use this option to reduce code size slightly in code that doesn't need to be thread-safe.

CWFLAGS=-Wall
CIFLAGS=$(AspInclude)
CMFLAGS=

ArdRoot := $(shell pwd)/../../../..

CFLAGS += -g -fPIC $(CWFLAGS) $(COFLAGS) $(CIFLAGS) $(CMFLAGS) $(PYTHON_INC)
SYS_LIBS       := $(PYTHON_LIB) -lpthread -lstdc++ -ldl -lrt -lm $(SYS_LIBS)

.PHONY: all

PY_CODE := $(addprefix $(LIB_DIR)/,$(notdir $(wildcard *.py)))

all: $(LIB_DIR)/asppycustom.so $(LIB_DIR)/amfpy.so $(LIB_DIR)/asppycor.so libs  #asp.py is here explicitly because it is generated...

libs: $(LIB_DIR)/libmwpy.a $(LIB_DIR)/libmwpy.so $(LIB_DIR)/_safplusCApi.so $(PY_CODE)

safplusCApi.c safplusCApi.py: safplusCApi.i
		swig -python $(AspInclude) -o safplusCApi.c  $<

#asp.py: binding.i
#		swig -python $(AspInclude) $<

$(OBJ_DIR)/safplusCApi.o: safplusCApi.c $(OBJ_DIR)

$(OBJ_DIR)/asppycustom.o: asppycustom.c $(OBJ_DIR)

$(OBJ_DIR)/asppycor.o: asppycor.c $(OBJ_DIR)

$(OBJ_DIR)/amfpy.o: amfpy.c $(OBJ_DIR)

# -std=c99 -pedantic

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: %.c Makefile
	$(CC) -c -fno-strict-aliasing $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: %.C Makefile
	$(CPL) -c $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: %.cxx Makefile
	$(CPL) -c $(CFLAGS) -o $@ $<

$(LIB_DIR)/libmwpy.a: $(OBJ_DIR)/safplusCApi.o $(OBJ_DIR)/asppycustom.o  $(OBJ_DIR)/amfpy.o $(OBJ_DIR)/asppycor.o $(OBJ_DIR)/pyglue.o
	$(AR) -rs  $@ $+

$(LIB_DIR)/libmwpy.so: $(OBJ_DIR)/safplusCApi.o $(OBJ_DIR)/asppycustom.o $(OBJ_DIR)/amfpy.o $(OBJ_DIR)/asppycor.o $(OBJ_DIR)/pyglue.o
	gcc  -o $@ -shared $+ $(ASP_LIB_LIST) $(SYS_LIBS) 

$(LIB_DIR)/amfpy.so: $(OBJ_DIR)/amfpy.o
	gcc  -o $@ -shared $+ $(ASP_LIB_LIST) $(SYS_LIBS) 

$(LIB_DIR)/_safplusCApi.so: $(OBJ_DIR)/safplusCApi.o
	gcc  -o $@ -shared $+ $(ASP_LIB_LIST) $(SYS_LIBS) 

$(LIB_DIR)/asppycustom.so: $(OBJ_DIR)/asppycustom.o 
	gcc  -o $@ -shared $+ $(ASP_LIB_LIST) $(SYS_LIBS) 

$(LIB_DIR)/asppycor.so: $(OBJ_DIR)/asppycor.o
	gcc  -o $@ -shared $+ $(ASP_LIB_LIST) $(SYS_LIBS) 



$(LIB_DIR)/%.py: %.py
	$(MKDIR) -p $(LIB_DIR)
	$(CP) $< $@

clean: 
	rm -rvf *.a *.so *.pyc *.o
