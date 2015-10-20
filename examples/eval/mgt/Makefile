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
SAFPLUS_LIBS := clAmf clMgt clRpc clName clCkpt clGroup clTimer clMsg clLog clUtils clOsal clFault clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

Release all: $(TEST_DIR)/mgtApp $(MODELS_TGT) $(SAFPLUS_TARGET)/share/netconf/modules/openclovis/myService.yang

$(TEST_DIR)/%.xml: %.xml
	cp -f $< $@

$(TEST_DIR)/mgtApp: $(CLIENT_OBJ) $(LIB_DIR)/libmyService.so $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(CLIENT_OBJ) -lmyService $(SAFPLUS_LINK_LIBS) -lmicrohttpd -lboost_filesystem $(LINK_SO_LIBS) -lprotobuf -lprotoc $(LINK_SO_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP)  $@ $(SAFPLUS_MGT_INC_FLAGS) $< 

$(LIB_DIR)/libmyService.so:
	$(MAKE) SAFPLUS_SRC_DIR=$(SAFPLUS_SRC_DIR) -C myService

yang:
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang -f y2cpp myService.yang --y2cpp-output `pwd` --y2cpp-sdkdir $(SAFPLUS_SRC_DIR)

$(SAFPLUS_TARGET)/share/netconf/modules/openclovis/myService.yang: myService.yang
	@mkdir -p $(SAFPLUS_TARGET)/share/netconf/modules/openclovis
	cp -rf $< $@

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/mgtApp $(OBJ_DIR)/myService/*
	$(MAKE) SAFPLUS_SRC_DIR=$(SAFPLUS_SRC_DIR) -C myService clean

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
