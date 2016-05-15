S7 := 1
SAFPLUS_DBAL_PLUGIN :=1
include ../../mk/preface.mk

SQLITE_SO_LIBS =-lsqlite3

GDBM_SO_LIBS =-lgdbm

BERKELEY_SO_LIBS +=-ldb

# Specify the required libraries
SAFPLUS_LIBS := clDbal clUtils clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

# Specify the required libraries for CkptDB
SAFPLUS_LIBS_FOR_CKPTDB := clName clCkpt clDbal clUtils clLog 
# Then use these in the make rule
SAFPLUS_DEP_LIBS_FOR_CKPTDB  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS_FOR_CKPTDB)))
SAFPLUS_LINK_LIBS_FOR_CKPTDB := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS_FOR_CKPTDB))
CLIENT_H   := $(wildcard *.h) $(wildcard $(SAFPLUS_INC_DIR)/*.h) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

SAFplusDbalPlugins:= $(PLUGIN_DIR)/libclCkptDB.so

# Skip compiling Berkeley plugin if SAFPLUS_WITH_BERKELEYDB exists and is set to 0 or false
skip_bdb:=0
ifdef SAFPLUS_WITH_BERKELEYDB
ifeq ($(strip $(SAFPLUS_WITH_BERKELEYDB)),0)
skip_bdb:=1
endif
ifeq ($(strip $(SAFPLUS_WITH_BERKELEYDB)),false)
skip_bdb:=1
endif
endif

ifeq ($(strip $(skip_bdb)),0)
SAFplusDbalPlugins+= $(PLUGIN_DIR)/libclBerkeleyDB.so
endif


# Skip compiling Gdbm plugin if SAFPLUS_WITH_GDBM exists and is set to 0 or false
skip_gdbm:=0
ifdef SAFPLUS_WITH_GDBM
ifeq ($(strip $(SAFPLUS_WITH_GDBM)),0)
skip_gdbm:=1
endif
ifeq ($(strip $(SAFPLUS_WITH_GDBM)),false)
skip_gdbm:=1
endif
endif

ifeq ($(strip $(skip_gdbm)),0)
SAFplusDbalPlugins+= $(PLUGIN_DIR)/libclGDBM.so
endif

# Skip compiling Sqlite plugin if SAFPLUS_WITH_GDBM exists and is set to 0 or false
skip_sqlite:=0
ifdef SAFPLUS_WITH_SQLITE
ifeq ($(strip $(SAFPLUS_WITH_SQLITE)),0)
skip_sqlite:=1
endif
ifeq ($(strip $(SAFPLUS_WITH_SQLITE)),false)
skip_sqlite:=1
endif
endif

ifeq ($(strip $(skip_sqlite)),0)
SAFplusDbalPlugins+= $(PLUGIN_DIR)/libclSQLiteDB.so
endif
#Release all: $(PLUGIN_DIR)/libclSQLiteDB.so $(PLUGIN_DIR)/libclGDBM.so $(PLUGIN_DIR)/libclBerkeleyDB.so $(PLUGIN_DIR)/libclCkptDB.so
Release all: $(SAFplusDbalPlugins)

$(OBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $(<F)

$(PLUGIN_DIR)/libclSQLiteDB.so: $(OBJ_DIR)/clDbalSqlite.o $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $< $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(SQLITE_SO_LIBS)

$(PLUGIN_DIR)/libclGDBM.so: $(OBJ_DIR)/clDbalGdbm.o $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $< $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(GDBM_SO_LIBS)

$(PLUGIN_DIR)/libclBerkeleyDB.so: $(OBJ_DIR)/clDbalBerkeley.o $(OBJ_DIR)/clist.o $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $< $(OBJ_DIR)/clist.o $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(BERKELEY_SO_LIBS)

$(PLUGIN_DIR)/libclCkptDB.so: $(OBJ_DIR)/clDbalCkpt.o $(SAFPLUS_DEP_LIBS_FOR_CKPTDB)
	$(LINK_SO) $@ $< $(SAFPLUS_LINK_LIBS_FOR_CKPTDB) $(LINK_SO_LIBS)

clean:
	rm -f $(PLUGIN_DIR)/libclSQLiteDB.so rm -f $(OBJ_DIR)/clDbalSqlite.o rm -f $(PLUGIN_DIR)/libclGDBM.so rm -f $(OBJ_DIR)/clDbalGdbm.o rm -f $(PLUGIN_DIR)/libclBerkeleyDB.so rm -f $(OBJ_DIR)/clDbalBerkeley.o rm -f $(PLUGIN_DIR)/libclCkptDB.so rm -f $(OBJ_DIR)/clDbalCkpt.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
