S7 := 1
SAFPLUS_MGT_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I.. 

# Specify the required libraries
SAFPLUS_LIBS := clRpc clName clMgt clUtils clLog clCkpt clGroup clTimer clMsg clOsal clDbal clFault
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

MGT_TEST_H := $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx)  $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
MGT_TEST_SRC := $(wildcard testmgt.cxx)
MGT_TEST_OBJ := $(subst .cpp,.o, $(subst .cxx,.o, $(notdir $(MGT_TEST_SRC))))
MGT_TEST_OBJ := $(addprefix $(OBJ_DIR)/test/mgt/,$(MGT_TEST_OBJ))

all: $(TEST_DIR)/testmgt $(TEST_DIR)/testmgtapi

$(TEST_DIR)/testmgtapi: $(OBJ_DIR)/test/mgt/testapi.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@  $(OBJ_DIR)/test/mgt/testapi.o $(SAFPLUS_LINK_LIBS) -L$(SAFPLUS_TARGET)/lib -L$(INSTALL_DIR)/lib $(XML2_LINK) $(LINK_STD_LIBS)

$(TEST_DIR)/%: $(MGT_TEST_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(MGT_TEST_OBJ) $(SAFPLUS_LINK_LIBS) -L$(SAFPLUS_TARGET)/lib -L$(INSTALL_DIR)/lib $(XML2_LINK) $(LINK_STD_LIBS)

$(OBJ_DIR)/test/mgt/%.o: %.cxx
	-mkdir -p $(OBJ_DIR)/test/mgt
	$(COMPILE_CPP) $@ $<

gen:
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang -f y2cpp unitTest.yang --y2cpp-output `pwd`

genRFC:
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang -f y2cpp ietf-interfaces.yang --y2cpp-output `pwd` --y2cpp-sdkdir $(SAFPLUS_SRC_DIR)

cleangen:
	rm -rf unitTest

clean:
	rm -rf $(OBJ_DIR)/test/mgt $(TEST_DIR)/testmgt $(TEST_DIR)/testmgtapi

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
