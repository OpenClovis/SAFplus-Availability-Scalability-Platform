S7 := 1
include ../../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I.. 
DEP_LIBS += $(LIB_DIR)/clOsal7.so $(LIB_DIR)/clMgt7.so $(LIB_DIR)/clLog.so $(LIB_DIR)/clUtils7.so $(LIB_DIR)/clDbal7.so

# Management includes and libraries
CPP_FLAGS += -I../../mgt7/include -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

COMPILE_CPP := g++ -g -O0 -fPIC -c $(CPP_FLAGS) -o
LINK_SO     := g++ -g -shared -o 
LINK_EXE     := g++ -g -o 

LOG_SERVER_H := $(wildcard *.hpp) $(wildcard ../../../include7/*.hpp) 
LOG_SERVER_SRC := $(wildcard *.cxx)
LOG_SERVER_OBJ := $(subst .cpp,.o, $(subst .cxx,.o,$(LOG_SERVER_SRC)))
LOG_SERVER_OBJ := $(addprefix $(SAFPLUS_TARGET)/obj/,$(LOG_SERVER_OBJ))


all: $(SAFPLUS_TARGET)/bin/splogd

$(SAFPLUS_TARGET)/bin/splogd: $(LOG_SERVER_OBJ) $(DEP_LIBS)
	$(LINK_EXE) $@ $+ -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml

$(SAFPLUS_TARGET)/obj/%.o: %.cxx Makefile $(LOG_SERVER_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(SAFPLUS_TARGET)/obj/*.o $(SAFPLUS_TARGET)/bin/splogd

include ../../../mk/safplus_targets.mk