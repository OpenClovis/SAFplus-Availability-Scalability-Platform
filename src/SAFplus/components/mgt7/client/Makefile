S7 := 1
include ../../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

#SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
#SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

# Management includes and libraries
CPP_FLAGS +=  -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/
#LINK_STD_LIBS +=   -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml
LINK_STD_LIBS +=   -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2

COMPILE_CPP := g++ -g -O0 -fPIC -c $(CPP_FLAGS) -o
LINK_SO     := g++ -g -shared -o 

C_FLAGS += -I$(SAFPLUS_SRC_DIR)/SAFplus/include -I$(SAFPLUS_INC_DIR) -I$(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/common -I$(SAFPLUS_SRC_DIR)/SAFplus/3rdparty/ezxml/stable
COMPILE_C = g++ -g -O0 -fPIC -c $(C_FLAGS) -o

CLIENT_H := $(wildcard ../include/*.hxx) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

SAFPLUS_DBAL_OBJ = $(MWOBJ_DIR)/clDbalCfg.o $(MWOBJ_DIR)/clDbalParse.o $(MWOBJ_DIR)/clovisDbal.o $(MWOBJ_DIR)/clovisSQLite.o

all: $(LIB_DIR)/clMgt7.so

$(LIB_DIR)/clMgt7.so: $(CLIENT_OBJ) $(SAFPLUS_DBAL_OBJ)
	$(LINK_SO) $@ $+ $(LINK_SO_LIBS) $(LINK_STD_LIBS) -lpthread -lstdc++ -ldl -lrt -lm -lsqlite3

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/clDbalCfg.o: ../../dbal/client/clDbalCfg.c
	$(COMPILE_C) $@ $<

$(MWOBJ_DIR)/clDbalParse.o: ../../dbal/client/clDbalParse.c
	$(COMPILE_C) $@ $<

$(MWOBJ_DIR)/clovisDbal.o: ../../dbal/client/clovisDbal.c
	$(COMPILE_C) $@ $<

$(MWOBJ_DIR)/clovisSQLite.o: ../../dbal/plugins/sqlite/clovisSQLite.c
	$(COMPILE_C) $@ $<

clean:
	rm -f $(LIB_DIR)/clMgt7.so $(MWOBJ_DIR)/*.o