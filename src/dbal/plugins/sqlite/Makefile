S7 := 1
SAFPLUS_DBAL_PLUGIN :=1
include ../../../mk/preface.mk

EXTRA_SO_LIBS +=-lsqlite3

# Specify the required libraries
SAFPLUS_LIBS := clUtils clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

CLIENT_SQLITE_H   := $(wildcard *.h) $(wildcard $(SAFPLUS_INC_DIR)/*.h) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SQLITE_SRC := $(wildcard *.c)
CLIENT_SQLITE_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .c,.o,$(CLIENT_SQLITE_SRC)))

Release all: $(LIB_DIR)/libclSQLiteDB.so

$(OBJ_DIR)/%.o: %.c Makefile $(CLIENT_SQLITE_H)
	$(COMPILE_CPP) $@ $(<F)

$(LIB_DIR)/libclSQLiteDB.so: $(CLIENT_SQLITE_OBJ)
	$(LINK_SO) $@ $(CLIENT_SQLITE_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(EXTRA_SO_LIBS)

clean:
	rm -f $(LIB_DIR)/libclSQLiteDB.so $(CLIENT_SQLITE_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
