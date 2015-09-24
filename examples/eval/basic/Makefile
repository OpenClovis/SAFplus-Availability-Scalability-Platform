S7 := 1
SAFPLUS_AMF_EXAMPLE_MGT:=1  # Identify what is being built, so safplus_targets does not override

ifneq ($(wildcard $(SAFPLUS_DIR)/src/mk/preface.mk),) 
    SAFPLUS_MAKE_DIR ?= $(SAFPLUS_DIR)/src/mk/
else
ifneq ($(wildcard ../../../src/mk/preface.mk),)   # within source tree
    SAFPLUS_MAKE_DIR ?= ../../../src/mk/
else
ifneq ($(wildcard /opt/safplus/7.0/sdk),) 
    SAFPLUS_MAKE_DIR ?= /opt/safplus/7.0/sdk/src/mk
else
$(error Cannot find SAFPLUS SDK)
endif
endif
endif

include $(SAFPLUS_MAKE_DIR)/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

MODELS := $(wildcard *.xml)
MODELS_TGT := $(addprefix $(TEST_DIR)/,$(MODELS))

# Specify the required libraries
SAFPLUS_LIBS := clAmf clRpc clName clCkpt clGroup clMsg clLog clUtils clOsal clFault clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(TEST_DIR)/safApp

$(TEST_DIR)/%.xml: %.xml
	cp -f $< $@

$(TEST_DIR)/safApp:  $(OBJ_DIR)/main.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(SAFPLUS_LINK_LIBS) -lboost_filesystem $(LINK_SO_LIBS) -lprotobuf -lprotoc $(LINK_SO_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/example

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk