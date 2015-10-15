S7 := 1
SAFPLUS_DBAL_SQLITE_PLUGIN :=1
SAFPLUS_DBAL_PLUGIN :=1
include ../../mk/preface.mk

SQLITE_SO_LIBS =-lsqlite3

GDBM_SO_LIBS =-lgdbm

BERKELEY_SO_LIBS +=-ldb

# Specify the required libraries
SAFPLUS_LIBS := clUtils clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

CLIENT_H   := $(wildcard *.h) $(wildcard $(SAFPLUS_INC_DIR)/*.h) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

Release all: $(PLUGIN_DIR)/libclSQLiteDB.so $(PLUGIN_DIR)/libclGDBM.so $(PLUGIN_DIR)/libclBerkeleyDB.so

$(OBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $(<F)

$(PLUGIN_DIR)/libclSQLiteDB.so: $(OBJ_DIR)/clDbalSqlite.o $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $^ $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(SQLITE_SO_LIBS)

$(PLUGIN_DIR)/libclGDBM.so: $(OBJ_DIR)/clDbalGdbm.o $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $^ $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(GDBM_SO_LIBS)

$(PLUGIN_DIR)/libclBerkeleyDB.so: $(OBJ_DIR)/clDbalBerkeley.o $(SAFPLUS_DEP_LIBS) $(OBJ_DIR)/clist.o
	$(LINK_SO) $@ $^ $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(BERKELEY_SO_LIBS)

clean:
	rm -f $(PLUGIN_DIR)/libclSQLiteDB.so rm -f $(OBJ_DIR)/clDbalSqlite.o rm -f $(PLUGIN_DIR)/libclGDBM.so rm -f $(OBJ_DIR)/clDbalGdbm.o rm -f $(PLUGIN_DIR)/libclBerkeleyDB.so rm -f $(OBJ_DIR)/clDbalBerkeley.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
