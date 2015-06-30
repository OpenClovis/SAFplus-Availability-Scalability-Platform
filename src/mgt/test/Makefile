S7 := 1
SAFPLUS_MGT_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

#SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I.. 

# Specify the required libraries
SAFPLUS_LIBS := clMgt clUtils clLog clCkpt clGroup clMsg clOsal clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

UNIT_TEST_SRC := $(wildcard unitTest/*.cxx)
UNIT_TEST_H := $(wildcard unitTest/*.cxx)

UNIT_TEST_OBJ := $(subst .cxx,.o,$(notdir $(UNIT_TEST_SRC)))
UNIT_TEST_OBJ := $(addprefix $(OBJ_DIR)/test/mgt/unitTest/,$(UNIT_TEST_OBJ))

UNIT_TEST_LIB := $(LIB_DIR)/libTestEnv.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

MGT_TEST_H := $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx)  $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
MGT_TEST_SRC := $(wildcard testmgt.cxx)
MGT_TEST_OBJ := $(subst .cpp,.o, $(subst .cxx,.o, $(notdir $(MGT_TEST_SRC))))
MGT_TEST_OBJ := $(addprefix $(OBJ_DIR)/test/mgt/,$(MGT_TEST_OBJ))

all: $(TEST_DIR)/testmgt $(TEST_DIR)/testapi

# Shared library providing functions needed by the policy library & SAFplus AMF.
$(UNIT_TEST_LIB): $(UNIT_TEST_OBJ) $(SAFPLUS_DEP_LIBS)
	@echo "build Policy"
	$(LINK_SO) $@  $(UNIT_TEST_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lezxml -lxml2 -lboost_filesystem $(LINK_SO_LIBS)
	
$(OBJ_DIR)/test/mgt/unitTest/%.o: unitTest/%.cxx $(UNIT_TEST_H)
	-mkdir -p $(OBJ_DIR)/test/mgt/unitTest
	$(COMPILE_CPP) $@ $<

$(TEST_DIR)/%: $(MGT_TEST_OBJ) $(UNIT_TEST_LIB) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(MGT_TEST_OBJ) $(SAFPLUS_LINK_LIBS) -lTestEnv -L$(SAFPLUS_SDK_TARGET)/lib -lezxml -L$(INSTALL_DIR)/lib -lxml2 $(LINK_STD_LIBS)

$(OBJ_DIR)/test/mgt/%.o: %.cxx
	-mkdir -p $(OBJ_DIR)/test/mgt
	$(COMPILE_CPP) $@ $<

gen:
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang -f y2cpp unitTest.yang --y2cpp-output `pwd`

cleangen:
	rm -rf unitTest

clean:
	rm -rf $(OBJ_DIR)/test/mgt $(TEST_DIR)/testmgt $(TEST_DIR)/testapi

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
