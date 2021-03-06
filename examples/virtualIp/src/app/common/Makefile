# The OpenClovis make infrastructure defines some variables before calling this
# makefile. If OpenClovis has defined the variable, it is important to use it 
# or features like cross-building may not work.

# But if the variables are not defined, then I define them so that this make
# can be called outside of the infrastructure.

CC      ?= g++
ifeq ($(CC),cc)  # Default make invocation assigns this variable, but we only want it if its assigned by OpenClovis framework
override CC := g++
endif

CLOVIS_ROOT ?= ../../../../../..
AR          ?= ar
BIN_DIR     ?=.
OBJ_DIR     ?=.
SISP_LIBS   ?=.
LIB_DIR     ?=$(SISP_LIBS)
MKDIR       ?=mkdir -p
CP          ?=cp -f

AspInclude      = -I$(CLOVIS_ROOT)/ASP/components/osal/include -I$(CLOVIS_ROOT)/ASP/components/timer/include -I$(CLOVIS_ROOT)/ASP/components/include -I$(CLOVIS_ROOT)/ASP/components/include/ipi -I$(CLOVIS_ROOT)/ASP/components/utils/include -I$(CLOVIS_ROOT)/ASP/components/log/include -I$(CLOVIS_ROOT)/ASP/components/ioc/include -I$(CLOVIS_ROOT)/ASP/components/buffer/include -I$(CLOVIS_ROOT)/ASP/components/debug/include -I$(CLOVIS_ROOT)/ASP/components/eo/include -I$(CLOVIS_ROOT)/ASP/components/cnt/include -I$(CLOVIS_ROOT)/ASP/components/rmd/include -I$(CLOVIS_ROOT)/ASP/components/amf/include -I$(CLOVIS_ROOT)/ASP/components/ckpt/include -I$(CLOVIS_ROOT)/ASP/components/osal/include 

CWFLAGS=-Wall
CIFLAGS=-I$(SoapDir) $(ASP_INCLUDE) $(AspInclude)
CMFLAGS=
CFLAGS= -g $(CWFLAGS) $(COFLAGS) $(CIFLAGS) $(CMFLAGS)


.PHONY: all

all: $(LIB_DIR)/libcommon.a $(BIN_DIR)/virtualIp

$(BIN_DIR)/virtualIp: virtualIp
		$(MKDIR) $(BIN_DIR)
		$(CP) $< $@

$(OBJ_DIR)/%.o: %.c 
		$(CC) -c $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: %.C 
		$(CC) -c $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: %.cxx
		$(CC) -c $(CFLAGS) -o $@ $<

$(LIB_DIR)/libcommon.a: $(OBJ_DIR)/net.o
		$(AR) -rs  $@ $+