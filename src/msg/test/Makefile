S7 := 1
SAFPLUS_MSG_TEST:=1  # Identify what is being built, so safplus_targets does not override
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
  TEST_DIR := $(APP_BIN_DIR)
  PERF_ONLY?=1
endif

NOOP := $(shell mkdir -p $(APP_BIN_DIR))
NOOP := $(shell mkdir -p $(APP_OBJ_DIR))

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clTimer clMsg clFault clLog clUtils clOsal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

IMAGE_FILE ?= msgUnitTest.zip

MSG_TESTS = $(TEST_DIR)/testTransport  $(TEST_DIR)/testMsgServer $(TEST_DIR)/testMsgServerSegmentation $(TEST_DIR)/testMsgPerf $(TEST_DIR)/msgReflector $(TEST_DIR)/reliableClient $(TEST_DIR)/reliableServer $(TEST_DIR)/testMsgServerReliable $(TEST_DIR)/testMsgServerShaping

ifdef PERF_ONLY
all: $(TEST_DIR)/testMsgPerf $(TEST_DIR)/msgReflector
else
all: $(MSG_TESTS)
endif

deps:
	make -C $(SAFPLUS_SRC_DIR)/msg/transports
	make -C $(SAFPLUS_SRC_DIR)/msg

ifdef DIRECTLY_LINK
$(TEST_DIR)/%:  $(APP_OBJ_DIR)/%.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS) $(DIRECTLY_LINK) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(APP_OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP)  $@ -DDIRECTLY_LINKED $<
else
$(TEST_DIR)/%:  $(APP_OBJ_DIR)/%.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS)  $(SAFPLUS_LINK_LIBS) -lboost_program_options $(LINK_STD_LIBS)

$(APP_OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<
endif

image: $(IMAGE_FILE)

$(IMAGE_FILE): $(MSG_TESTS)
	cp setup start cleanup $(SAFPLUS_TARGET)
	$(SAFPLUS_MAKE_DIR)/safplus_packager.py  -a $(TARGET_PLATFORM) -x "(cp setup start {image_dir})" $@


.PRECIOUS: $(APP_OBJ_DIR)/%.o

clean:
	rm -f $(CLIENT_OBJ) $(MSG_TESTS)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
