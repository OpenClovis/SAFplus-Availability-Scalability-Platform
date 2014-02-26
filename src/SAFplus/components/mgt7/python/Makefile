S7 := 1
include ../../../mk/preface.mk


CP =cp -f
PyLib := -lpython2.7 -lz -lutil
PyInc := -I/usr/include/python2.7

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CPP_FLAGS = -I$(SAFPLUS_SRC_DIR)/SAFplus/components/utils/include -I$(SAFPLUS_SRC_DIR)/SAFplus/include -I$(SAFPLUS_INC_DIR) -I$(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/common -I$(SAFPLUS_SRC_DIR)/SAFplus/3rdparty/ezxml/stable $(PyInc) -DWITH_STATIC  
DEP_LIBS += $(LIB_DIR)/libclDbal7.so

COMPILE_CPP := g++ -g -fPIC -c -fno-strict-aliasing $(CPP_FLAGS) -o
LINK_SO     := g++ -g -shared -o 

SAFPLUS_DBAL_OBJ = $(SAFPLUS_TARGET)/obj/clDbalCfg.o $(SAFPLUS_TARGET)/obj/clDbalParse.o $(SAFPLUS_TARGET)/obj/clovisDbal.o $(SAFPLUS_TARGET)/obj/clovisSQLite.o

.PHONY: all

all: $(LIB_DIR)/pyDbal.so $(LIB_DIR)/dbalpy.py

$(LIB_DIR)/%.py: %.py
	$(CP) $< $@

$(BIN_DIR)/%.py: %.py
	$(MKDIR) $(BIN_DIR)
	$(CP) $< $@

$(LIB_DIR)/pyDbal.so: $(SAFPLUS_TARGET)/obj/pyDbal.o $(DEP_LIBS)
	$(LINK_SO) $@ $+ -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lpthread -lstdc++ -ldl -lrt -lm -lezxml -lsqlite3 $(PyLib)

$(LIB_DIR)/libclDbal7.so: $(SAFPLUS_DBAL_OBJ)
	$(LINK_SO) $@  $+ -lsqlite3

$(SAFPLUS_TARGET)/obj/pyDbal.o: pyDbal.cxx
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/obj/clDbalCfg.o: $(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/client/clDbalCfg.c
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/obj/clDbalParse.o: $(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/client/clDbalParse.c
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/obj/clovisDbal.o: $(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/client/clovisDbal.c
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/obj/clovisSQLite.o: $(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/plugins/sqlite/clovisSQLite.c
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(SAFPLUS_TARGET)/obj/*.o $(LIB_DIR)/pyDbal.so

