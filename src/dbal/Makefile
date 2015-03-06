S7 := 1
SAFPLUS_DBAL_LIB :=1
include ../mk/preface.mk

CP =cp -f
PyLib := -lpython2.7 -lz -lutil
PyInc := -I/usr/include/python2.7

#SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))


CPP_FLAGS += -I$(SAFPLUS_SRC_DIR)/SAFplus/include -I$(SAFPLUS_INC_DIR) -I$(SAFPLUS_SRC_DIR)/SAFplus/3rdparty/ezxml/stable $(PyInc) #-DWITH_STATIC  
CLIENT_H := $(wildcard *.hxx) $(wildcard ../../include7/*.hxx)

# Specify the required libraries
SAFPLUS_LIBS := clOsal clUtils clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

CLIENT_DBAL_SRC := $(wildcard *.c)
CLIENT_DBAL_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .c,.o,$(notdir $(CLIENT_DBAL_SRC))))

.PHONY: all

all: $(LIB_DIR)/libclDbal.so $(LIB_DIR)/pyDbal.so $(BIN_DIR)/dbalpy.py $(LIB_DIR)/libclBerkeleyDB.so $(LIB_DIR)/libclGDBM.so $(LIB_DIR)/libclSQLiteDB.so

$(BIN_DIR)/dbalpy.py: dbalpy.py $(LIB_DIR)/common.py $(LIB_DIR)/microdom.py $(LIB_DIR)/pyDbal.py
	$(CP) $< $@

$(MWOBJ_DIR)/%.o: %.c Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $(<F)

$(LIB_DIR)/%.py: %.py
	$(CP) $< $@

$(LIB_DIR)/libclDbal.so: $(CLIENT_DBAL_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(CLIENT_DBAL_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(LIB_DIR)/pyDbal.so: $(SAFPLUS_TARGET)/obj/pyDbal.o $(DEP_LIBS) $(LIB_DIR)/libclSQLiteDB.so $(LIB_DIR)/libclGDBM.so $(LIB_DIR)/libclBerkeleyDB.so
	$(LINK_SO) $@ $+ $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) -lclDbal -L$(SAFPLUS_SDK_TARGET)/lib -lpthread -lstdc++ -ldl -lm  -lrt -lezxml $(PyLib) -L$(SAFPLUS_SDK_TARGET)/lib

$(LIB_DIR)/libclBerkeleyDB.so: $(SAFPLUS_TARGET)/obj/clovisBerkeleyDB.o $(SAFPLUS_TARGET)/obj/clist.o
	$(LINK_SO) $@ $+ -L$(SAFPLUS_SDK_TARGET)/lib -lpthread -lstdc++ -ldl -lm -ldb

$(LIB_DIR)/libclGDBM.so: $(SAFPLUS_TARGET)/obj/clovisGDBM.o
	$(LINK_SO) $@ $+ -L$(SAFPLUS_SDK_TARGET)/lib -lpthread -lstdc++ -ldl -lm -lgdbm

$(LIB_DIR)/libclSQLiteDB.so: $(SAFPLUS_TARGET)/obj/clovisSQLite.o
	$(LINK_SO) $@ $+ -L$(SAFPLUS_SDK_TARGET)/lib -lpthread -lstdc++ -ldl -lm -lsqlite3

$(SAFPLUS_TARGET)/obj/pyDbal.o: pyDbal.cxx
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/obj/clovisSQLite.o: plugins/sqlite/clovisSQLite.c
	cd plugins/sqlite && $(COMPILE_CPP) $@ $(<F)

$(SAFPLUS_TARGET)/obj/clovisGDBM.o: plugins/gdbm/clovisGDBM.c
	cd plugins/gdbm && $(COMPILE_CPP) $@ $(<F)

$(SAFPLUS_TARGET)/obj/clovisBerkeleyDB.o: plugins/berkeley/clovisBerkeleyDB.c
	cd plugins/berkeley && $(COMPILE_CPP) $@ $(<F)

$(SAFPLUS_TARGET)/obj/clist.o: plugins/berkeley/clist.c
	cd plugins/berkeley && $(COMPILE_CPP) $@ $(<F)

clean:
	rm -f $(SAFPLUS_TARGET)/obj/*.o $(LIB_DIR)/pyDbal.so $(BIN_DIR)/dbalpy.py $(LIB_DIR)/common.py $(LIB_DIR)/microdom.py $(LIB_DIR)/libclSQLiteDB.so $(LIB_DIR)/libclGDBM.so $(LIB_DIR)/libclBerkeleyDB.so $(CLIENT_DBAL_OBJ)

