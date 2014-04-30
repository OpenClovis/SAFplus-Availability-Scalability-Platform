S7 := 1
SAFPLUS_LOG_SERVER:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I.. 

# Specify the required libraries
SAFPLUS_LIBS := clMgt7 clUtils7 clLog clDbal7 clOsal7 
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

# Management includes and libraries
CPP_FLAGS += -I../../mgt7/include -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

COMPILE_CPP := g++ -g -O0 -fPIC -c $(CPP_FLAGS) -o
LINK_SO     := g++ -g -shared -o 
LINK_EXE     := g++ -g -o 

LOG_SERVER_H := $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx) $(wildcard ../../../include7/*.hpp) 
LOG_SERVER_SRC := $(wildcard *.cxx)
LOG_SERVER_OBJ := $(subst .cpp,.o, $(subst .cxx,.o,$(LOG_SERVER_SRC)))
LOG_SERVER_OBJ := $(addprefix $(OBJ_DIR)/logsvr/,$(LOG_SERVER_OBJ))


all: $(SAFPLUS_TARGET)/bin/splogd

$(SAFPLUS_TARGET)/bin/splogd: $(LOG_SERVER_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(LOG_SERVER_OBJ) $(SAFPLUS_LINK_LIBS) -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 $(LINK_SO_LIBS)

$(OBJ_DIR)/logsvr/%.o: %.cxx Makefile $(LOG_SERVER_H)
	-mkdir -p $(OBJ_DIR)/logsvr/
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(OBJ_DIR)/logsvr/*.o $(SAFPLUS_TARGET)/bin/splogd

include ../../../mk/safplus_targets.mk