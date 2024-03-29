S7 := 1
APP_COMPONENT_${name}:=1  # Identify what is being built, so safplus_targets does not override
${baseDir}
ifeq ($(SAFPLUS_SRC_DIR),)
$(error You must run this as a submake or define the SAFPLUS_SRC_DIR environment variable)
endif

include $(SAFPLUS_SRC_DIR)/mk/preface.mk

# Create target folder
TARGET:=$(subst $(shell basename $(subst $(shell basename $(CURDIR)),,$(CURDIR)))/$(shell basename $(CURDIR)),,$(CURDIR))target/obj/$(shell basename $(CURDIR))
$(shell mkdir -p $(TARGET))

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(TARGET)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clAmf clMgt clRpc clName clCkpt clGroup clMsg clLog clUtils clOsal clTimer clFault clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(BASE_DIR)/target/bin/${instantiate_command}

$(BASE_DIR)/target/bin/${instantiate_command}: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(TARGET)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $< 

clean:
	rm -f $(CLIENT_OBJ) $(BIN_DIR)/${instantiate_command}

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
