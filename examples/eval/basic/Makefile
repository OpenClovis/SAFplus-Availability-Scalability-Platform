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
    BUILD_SAFPLUS ?=0
else
$(error Cannot find SAFPLUS SDK)
endif
endif
endif

include $(SAFPLUS_MAKE_DIR)/preface.mk

CPP_FLAGS += -I../../../src/alarm/server/SAFplusAlarm

# always return empty string: TMP2 := $(strip $(wildcard $(THIS_DIR)/../../../mk/preface.mk))
TMP2 := $(strip $(shell ls $(THIS_DIR)/../../../src/mk/preface.mk))

ifneq ($(TMP2),)  # Is this build happening within the SAFplus source tree?
  # yes, generate output in the SAFplus output directories
  $(info Building from within SAFplus)
  APP_BIN_DIR ?= $(BIN_DIR)
  APP_OBJ_DIR ?= $(OBJ_DIR)
else  # no, so generate output relative to this director  
  $(info Building outside of SAFplus)
  APP_BIN_DIR ?= $(THIS_DIR)/target/$(TARGET_PLATFORM)/bin
  APP_OBJ_DIR ?= $(THIS_DIR)/target/$(TARGET_PLATFORM)/obj
endif

NOOP := $(shell mkdir -p $(APP_BIN_DIR))
NOOP := $(shell mkdir -p $(APP_OBJ_DIR))

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

MODELS := $(wildcard *.xml)
MODELS_TGT := $(addprefix $(TEST_DIR)/,$(MODELS))

# Specify the required libraries
SAFPLUS_LIBS := clAmf clRpc clName clCkpt clGroup clMsg clLog clUtils clTimer clOsal clFault clDbal clEvent clEventServer
# Then use these in the make rule
ifeq ($(BUILD_SAFPLUS),1)  # If I do not want to build SAFplus then don't put the libs in the dependency line
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
else
SAFPLUS_DEP_LIBS := 
endif

SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(APP_BIN_DIR)/basicApp $(APP_BIN_DIR)/setup.basicApp

$(APP_BIN_DIR)/%.xml: %.xml
	cp -f $< $@

$(APP_BIN_DIR)/setup.basicApp: setup
	cp -f $< $@

$(APP_BIN_DIR)/basicApp:  $(APP_OBJ_DIR)/main.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(SAFPLUS_LINK_LIBS) -lboost_filesystem $(LINK_SO_LIBS) -lprotobuf -lprotoc $(LINK_SO_LIBS)

$(APP_OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

.PHONY: image docker clean

image: evalBasic.tgz

ifeq ($(APP_BIN_DIR),$(BIN_DIR))
evalBasic.tgz: $(SAFPLUS_MAKE_DIR)/safplus_packager.py $(APP_BIN_DIR)/basicApp $(SAFPLUS_DEP_LIBS) $(wildcard $(PLUGIN_DIR)/*.so) $(wildcard $(BIN_DIR)/*) Makefile
	$(SAFPLUS_MAKE_DIR)/safplus_packager.py -a $(TARGET_PLATFORM) -x "(cp setup *.xml {image_dir}/bin)" $@
else
evalBasic.tgz: $(SAFPLUS_MAKE_DIR)/safplus_packager.py $(APP_BIN_DIR)/basicApp $(SAFPLUS_DEP_LIBS) $(wildcard $(PLUGIN_DIR)/*.so) $(wildcard $(BIN_DIR)/*) Makefile
	$(SAFPLUS_MAKE_DIR)/safplus_packager.py -p $(THIS_DIR) -a $(TARGET_PLATFORM) -x "(cp setup *.xml {image_dir}/bin)" $@
endif

docker/model.xml: SAFplusAmf1Node1SG1Comp.xml
	cp SAFplusAmf1Node1SG1Comp.xml docker/model.xml

docker: evalBasic.tgz docker/Dockerfile docker/model.xml
	cp evalBasic.tgz docker/evalBasic.tgz
	(cd docker; docker build -t evalbasic .)

clean:
	rm -f $(CLIENT_OBJ) $(APP_BIN_DIR)/basicApp

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
