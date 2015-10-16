S7 := 1

THIS_DIR := $(patsubst %/,%,$(dir $(realpath $(lastword $(MAKEFILE_LIST)))))

ifneq ($(wildcard $(SAFPLUS_DIR)/src/mk/preface.mk),) 
    SAFPLUS_MAKE_DIR ?= $(SAFPLUS_DIR)/src/mk/
    BUILD_SAFPLUS ?=1
else
ifneq ($(wildcard ../../../src/mk/preface.mk),)   # within source tree
    SAFPLUS_MAKE_DIR ?= ../../../src/mk/
    BUILD_SAFPLUS ?=1
else
ifneq ($(wildcard /opt/safplus/7.0/sdk),) 
    SAFPLUS_MAKE_DIR ?= /opt/safplus/7.0/sdk/src/mk
    BUILD_SAFPLUS ?=1
else
$(error Cannot find SAFPLUS SDK)
endif
endif
endif

include $(SAFPLUS_MAKE_DIR)/preface.mk

APP_BIN_DIR ?= $(THIS_DIR)/target/$(TARGET_PLATFORM)
APP_OBJ_DIR ?= $(THIS_DIR)/target/$(TARGET_PLATFORM)

NOOP := $(shell mkdir -p $(APP_BIN_DIR))
NOOP := $(shell mkdir -p $(APP_OBJ_DIR))

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

MODELS := $(wildcard *.xml)
MODELS_TGT := $(addprefix $(TEST_DIR)/,$(MODELS))

# Specify the required libraries
SAFPLUS_LIBS := clAmf clRpc clName clCkpt clGroup clMsg clLog clUtils clTimer clOsal clFault clDbal
# Then use these in the make rule
ifeq ($(BUILD_SAFPLUS),1)  # If I do not want to build SAFplus then don't put the libs in the dependency line
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
else
SAFPLUS_DEP_LIBS := 
endif

SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(APP_BIN_DIR)/basicApp

$(APP_BIN_DIR)/%.xml: %.xml
	cp -f $< $@

$(APP_BIN_DIR)/basicApp:  $(APP_OBJ_DIR)/main.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(SAFPLUS_LINK_LIBS) -lboost_filesystem $(LINK_SO_LIBS) -lprotobuf -lprotoc $(LINK_SO_LIBS)

$(APP_OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(APP_BIN_DIR)/basicApp

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
