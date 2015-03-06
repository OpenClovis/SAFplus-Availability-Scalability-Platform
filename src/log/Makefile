S7 := 1
SAFPLUS_LOG_LIB:=1
include ../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(subst .cxx,.o,$(CLIENT_SRC))
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clOsal clUtils
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))


Release all: $(LIB_DIR)/libclLog.so

$(SAFPLUS_TARGET)/lib/libclLog.so: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(SERVER_H)
	$(COMPILE_CPP)  $@ $<

clean:
	rm -f $(SAFPLUS_TARGET)/lib/clLog.o $(CLIENT_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
