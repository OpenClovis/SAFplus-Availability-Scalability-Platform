S7 := 1
SAFPLUS_AMF_SERVER:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I.. -std=c++11
DEP_LIBS += $(LIB_DIR)/libclMgt7.so $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclName.so $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclDbal7.so $(LIB_DIR)/libclOsal7.so 

# Management includes and libraries
CPP_FLAGS += -I../../mgt7/include -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/ -ISAFplusTypes -IietfYangTypes -ISAFplusAmf

SERVER_H := $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx) $(wildcard ../../../include7/*.hpp) $(wildcard SAFplusAmf/*.hxx) $(wildcard SAFplusTypes/*.hxx) $(wildcard ietfYangTypes/*.hxx) $(wildcard ENTITYSTATETCMIB/*.hxx)
SERVER_SRC := $(wildcard *.cxx) $(wildcard SAFplusAmf/*.cxx) $(wildcard SAFplusTypes/*.cxx) $(wildcard ietfYangTypes/*.cxx) $(wildcard ENTITYSTATETCMIB/*.cxx)
SERVER_OBJ := $(subst .cxx,.o,$(notdir $(SERVER_SRC)))
SERVER_OBJ := $(addprefix $(OBJ_DIR)/amfsvr/,$(SERVER_OBJ))


all: $(SAFPLUS_TARGET)/bin/safplus_amf

$(SAFPLUS_TARGET)/bin/safplus_amf: $(SERVER_OBJ) $(DEP_LIBS)
	$(LINK_EXE) $@ $+ -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 $(LINK_SO_LIBS)

$(OBJ_DIR)/amfsvr/%.o: %.cxx Makefile $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/amfsvr/%.o: SAFplusAmf/%.cxx Makefile $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/amfsvr/%.o: SAFplusTypes/%.cxx Makefile $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/amfsvr/%.o: ietfYangTypes/%.cxx Makefile $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/amfsvr/%.o: ENTITYSTATETCMIB/%.cxx Makefile $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<
gen:	
	PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/SAFplus/yang -f y2cpp $(SAFPLUS_SRC_DIR)/SAFplus/yang/SAFplusAmf.yang --y2cpp-output `pwd` --y2cpp-mgt $(MGT_SRC_DIR)

gentest:	
	PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/SAFplus/yang -f y2cpp $(SAFPLUS_SRC_DIR)/SAFplus/yang/SAFplusAmf.yang --y2cpp-output `pwd` --y2cpp-mgt $(MGT_SRC_DIR)


clean:
	rm -f $(OBJ_DIR)/amfsvr/*.o $(SAFPLUS_TARGET)/bin/safplus_amf

include ../../../mk/safplus_targets.mk