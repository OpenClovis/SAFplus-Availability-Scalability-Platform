S7 := 1
SAFPLUS_IOC_TEST := 1
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx) 

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include -I../../protoc -I./Common

# Management includes and libraries
CPP_FLAGS +=  -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/ `pkg-config --cflags protobuf`

SENDER_SRC := ClientTest.cxx $(wildcard Common/*.pb.cc) $(wildcard Common/*.cxx) 
SENDER_OBJ := $(addprefix $(OBJ_DIR)/, $(subst .cxx,.o,$(subst .cc,.o,$(SENDER_SRC))))

RECEIVER_SRC := MsgHandlerProtocols.cxx ServerTest.cxx $(wildcard Common/*.pb.cc) $(wildcard Common/*.cxx)
RECEIVER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(subst .cc,.o,$(RECEIVER_SRC))))

# Specify the required libraries
SAFPLUS_LIBS := clOsal7 clUtils7 clLog clIoc7 clMgt7
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS +=-L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 -lpthread -lstdc++ -ldl -lrt -lm  -L$(BOOST_DIR)/stage/lib/ -lboost_system -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml -lSAFplusPBExt

SAFplusRpcGen := $(SAFPLUS_TARGET)/bin/protoc-gen-rpc

all: $(TEST_DIR)/ClientTest $(TEST_DIR)/ServerTest

$(TEST_DIR)/ClientTest: $(SENDER_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(SENDER_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) -lprotobuf -lprotoc 
# `pkg-config --libs protobuf`

$(TEST_DIR)/ServerTest: $(RECEIVER_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(RECEIVER_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) -lprotobuf -lprotoc

$(OBJ_DIR)/Common/%.pb.o: Common/%.pb.cc
	-mkdir -p $(OBJ_DIR)/Common
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/Client/%.o: Client/%.cxx
	-mkdir -p $(OBJ_DIR)/Client
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(SAFplusRpcGen):
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/protoc

gen: $(SAFplusRpcGen)
	-mkdir -p ./Common
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/SAFplus/yang -f y2cpp $(SAFPLUS_SRC_DIR)/SAFplus/yang/rpcTest.yang --y2cpp-output `pwd` --y2cpp-mgt $(MGT_SRC_DIR) --y2cpp-rpc
	protoc -I../../../3rdparty -I../../protoc -I. --cpp_out=./Common rpcTest.proto
#$(SAFplusRpcGen) -I../../../3rdparty -I../../protoc -I. --rpc_out=. rpcTest.proto

cleangen:
	(find rpcTest/ -type f -not -name "*Common.hxx" -not -name "*Base.cxx" | xargs rm)

clean:
	rm -rf $(OBJ_DIR)/*.o $(TEST_DIR)/* $(SAFplusRpcGen) $(OBJ_DIR)/Client/* $(OBJ_DIR)/Server/* $(OBJ_DIR)/Common/*
	

include ../../../mk/safplus_targets.mk
