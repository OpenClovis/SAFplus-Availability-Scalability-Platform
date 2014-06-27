S7 := 1
SAFPLUS_DBAL_LIB :=1
include ../../mk/preface.mk

CP =cp -f
PyLib := -lpython2.7 -lz -lutil
PyInc := -I/usr/include/python2.7

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CPP_FLAGS += -I$(SAFPLUS_SRC_DIR)/SAFplus/components/utils/include -I$(SAFPLUS_SRC_DIR)/SAFplus/include -I$(SAFPLUS_INC_DIR) -I$(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/common -I$(SAFPLUS_SRC_DIR)/SAFplus/3rdparty/ezxml/stable $(PyInc) #-DWITH_STATIC  
# Specify the required libraries
SAFPLUS_LIBS := clOsal7 clUtils7 clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

.PHONY: all

all: $(LIB_DIR)/pyDbal.so $(BIN_DIR)/dbalpy.py

$(BIN_DIR)/dbalpy.py: dbalpy.py $(LIB_DIR)/common.py $(LIB_DIR)/microdom.py $(LIB_DIR)/pyDbal.py
	$(CP) $< $@

$(LIB_DIR)/%.py: %.py
	$(CP) $< $@

$(LIB_DIR)/pyDbal.so: $(SAFPLUS_TARGET)/obj/pyDbal.o $(DEP_LIBS) $(LIB_DIR)/libclSQLiteDB.so $(LIB_DIR)/libclGDBM.so $(LIB_DIR)/libclBerkeleyDB.so
	$(LINK_SO) $@ $+ $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lpthread -lstdc++ -ldl -lm  -lrt -lezxml $(PyLib)

$(LIB_DIR)/libclBerkeleyDB.so: $(SAFPLUS_TARGET)/obj/clovisBerkeleyDB.o
	$(LINK_SO) $@ $+ -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lpthread -lstdc++ -ldl -lm -ldb

$(LIB_DIR)/libclGDBM.so: $(SAFPLUS_TARGET)/obj/clovisGDBM.o
	$(LINK_SO) $@ $+ -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lpthread -lstdc++ -ldl -lm -lgdbm

$(LIB_DIR)/libclSQLiteDB.so: $(SAFPLUS_TARGET)/obj/clovisSQLite.o
	$(LINK_SO) $@ $+ -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lpthread -lstdc++ -ldl -lm -lsqlite3

$(SAFPLUS_TARGET)/obj/pyDbal.o: pyDbal.cxx
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/obj/clovisSQLite.o: $(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/plugins/sqlite/clovisSQLite.c
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/obj/clovisGDBM.o: $(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/plugins/gdbm/clovisGDBM.c
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/obj/clovisBerkeleyDB.o: $(SAFPLUS_SRC_DIR)/SAFplus/components/dbal/plugins/berkeley/clovisBerkeleyDB.c
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(SAFPLUS_TARGET)/obj/*.o $(LIB_DIR)/pyDbal.so $(BIN_DIR)/dbalpy.py $(LIB_DIR)/common.py $(LIB_DIR)/microdom.py $(LIB_DIR)/libclSQLiteDB.so $(LIB_DIR)/libclGDBM.so $(LIB_DIR)/libclBerkeleyDB.so

