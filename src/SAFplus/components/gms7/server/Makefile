S7 := 1
SAFPLUS_LOG_SERVER:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I..
DEP_LIBS += $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclName.so $(LIB_DIR)/libclIoc7.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclDbal7.so $(LIB_DIR)/libclOsal7.so

# Management includes and libraries
CPP_FLAGS += -I../../mgt7/include -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

COMPILE_CPP := g++ -g -O0 -fPIC -c $(CPP_FLAGS) -o
LINK_SO     := g++ -g -shared -o
LINK_EXE     := g++ -g -o

GROUP_SERVER_H := $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx) $(wildcard ../../../include7/*.hpp)
GROUP_SERVER_SRC := $(wildcard *.cxx)
GROUP_SERVER_OBJ := $(subst .cpp,.o, $(subst .cxx,.o,$(GROUP_SERVER_SRC)))
GROUP_SERVER_OBJ := $(addprefix $(OBJ_DIR)/groupsvr/,$(GROUP_SERVER_OBJ))


all: $(SAFPLUS_TARGET)/bin/spgroupd

$(SAFPLUS_TARGET)/bin/spgroupd: $(GROUP_SERVER_OBJ) $(DEP_LIBS)
	$(LINK_EXE) $@ $+ -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 $(LINK_SO_LIBS)

$(OBJ_DIR)/groupsvr/%.o: %.cxx Makefile $(LOG_SERVER_H)
	-mkdir -p $(OBJ_DIR)/groupsvr/
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(OBJ_DIR)/groupsvr/*.o $(SAFPLUS_TARGET)/bin/spgroupd

include ../../../mk/safplus_targets.mk
