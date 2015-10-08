S7 := 1
SAFPLUS_DBAL_GDBM_PLUGIN :=1
SAFPLUS_DBAL_PLUGIN :=1
include ../../../mk/preface.mk

# Skip compiling this plugin if SAFPLUS_WITH_GDBM exists and is set to 0 or false
skip_gdbm:=0
ifdef SAFPLUS_WITH_GDBM
ifeq ($(strip $(SAFPLUS_WITH_GDBM)),0)
skip_gdbm:=1
endif
ifeq ($(strip $(SAFPLUS_WITH_GDBM)),false)
skip_gdbm:=1
endif
endif

ifeq ($(strip $(skip_gdbm)),1)
all:

else

EXTRA_SO_LIBS +=-lgdbm

# Specify the required libraries
SAFPLUS_LIBS := clUtils clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

CLIENT_GDBM_H   := $(wildcard *.h) $(wildcard $(SAFPLUS_INC_DIR)/*.h) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_GDBM_SRC := $(wildcard *.c)
CLIENT_GDBM_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .c,.o,$(CLIENT_GDBM_SRC)))

Release all: $(PLUGIN_DIR)/libclGDBM.so

$(OBJ_DIR)/%.o: %.c Makefile $(CLIENT_GDBM_H)
	$(COMPILE_CPP) $@ $(<F)

$(PLUGIN_DIR)/libclGDBM.so: $(CLIENT_GDBM_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(CLIENT_GDBM_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(EXTRA_SO_LIBS)

clean:
	rm -f $(PLUGIN_DIR)/libclGDBM.so $(CLIENT_GDBM_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk

endif
