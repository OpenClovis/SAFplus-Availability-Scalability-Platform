S7 := 1
SAFPLUS_DBAL_BERKELEY_PLUGIN :=1
SAFPLUS_DBAL_PLUGIN :=1
include ../../../mk/preface.mk

EXTRA_SO_LIBS +=-ldb

# Specify the required libraries
SAFPLUS_LIBS := clUtils clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

CLIENT_BERKELEY_H   := $(wildcard *.h) $(wildcard $(SAFPLUS_INC_DIR)/*.h) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_BERKELEY_SRC := $(wildcard *.c)
CLIENT_BERKELEY_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .c,.o,$(CLIENT_BERKELEY_SRC)))

Release all: $(PLUGIN_DIR)/libclBerkeleyDB.so

$(OBJ_DIR)/%.o: %.c Makefile $(CLIENT_BERKELEY_H)
	$(COMPILE_CPP) $@ $(<F)

$(PLUGIN_DIR)/libclBerkeleyDB.so: $(CLIENT_BERKELEY_OBJ)
	$(LINK_SO) $@ $(CLIENT_BERKELEY_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(EXTRA_SO_LIBS)

clean:
	rm -f $(PLUGIN_DIR)/libclBerkeleyDB.so $(CLIENT_BERKELEY_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
