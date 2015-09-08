S7 := 1
SAFPLUS_DBAL_LIB :=1
include ../mk/preface.mk

# Specify the required libraries
SAFPLUS_LIBS := clOsal clUtils clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

CLIENT_DBAL_H   := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.h) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_DBAL_SRC := $(wildcard *.c)
CLIENT_DBAL_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .c,.o, $(CLIENT_DBAL_SRC)))

Release all: $(LIB_DIR)/libclDbal.so

$(LIB_DIR)/libclDbal.so: $(CLIENT_DBAL_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(CLIENT_DBAL_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.c Makefile $(CLIENT_DBAL_H)
	$(COMPILE_CPP) $@ $(<F)

clean:
	rm -f $(LIB_DIR)/pyDbal.so $(CLIENT_DBAL_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
