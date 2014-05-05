S7 := 1
include ../../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

# Management includes and libraries
CPP_FLAGS +=  -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

C_FLAGS += -I$(SAFPLUS_SRC_DIR)/SAFplus/include -I$(SAFPLUS_INC_DIR) -I$(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/common -I$(SAFPLUS_SRC_DIR)/SAFplus/3rdparty/ezxml/stable
COMPILE_C = g++ -g -O0 -fPIC -c $(C_FLAGS) -o

CLIENT_H := $(wildcard ../include/*.hxx) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

SAFPLUS_DBAL_OBJ = $(MWOBJ_DIR)/clDbalCfg.o $(MWOBJ_DIR)/clDbalParse.o $(MWOBJ_DIR)/clovisDbal.o $(MWOBJ_DIR)/clovisSQLite.o

all: $(LIB_DIR)/libclMgt7.so $(LIB_DIR)/libclDbal7.so

$(LIB_DIR)/libclMgt7.so: $(CLIENT_OBJ)
	$(LINK_SO) $@ $+ $(LINK_SO_LIBS) 

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(LIB_DIR)/libclDbal7.so: $(SAFPLUS_DBAL_OBJ)
	$(LINK_SO) $@  $+ -lsqlite3

$(MWOBJ_DIR)/clDbalCfg.o: ../../dbal/client/clDbalCfg.c
	$(COMPILE_C) $@ $<

$(MWOBJ_DIR)/clDbalParse.o: ../../dbal/client/clDbalParse.c
	$(COMPILE_C) $@ $<

$(MWOBJ_DIR)/clovisDbal.o: ../../dbal/client/clovisDbal.c
	$(COMPILE_C) $@ $<

$(MWOBJ_DIR)/clovisSQLite.o: ../../dbal/plugins/sqlite/clovisSQLite.c
	$(COMPILE_C) $@ $<

clean:
	rm -f $(LIB_DIR)/libclMgt7.so $(LIB_DIR)/libclDbal7.so $(MWOBJ_DIR)/*.o