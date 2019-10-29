S7 := 1
APP_COMPONENT_Component1:=1  # Identify what is being built, so safplus_targets does not override

ifeq ($(SAFPLUS_SRC_DIR),)
$(error You must run this as a submake or define the SAFPLUS_SRC_DIR environment variable)
endif

include $(SAFPLUS_SRC_DIR)/mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clAmf clMgt clRpc clName clCkpt clGroup clMsg clLog clUtils clOsal clTimer clFault clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(BIN_DIR)/Component1

$(BIN_DIR)/Component1: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $< 

clean:
	rm -f $(CLIENT_OBJ) $(BIN_DIR)/Component1

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
