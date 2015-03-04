S7 := 1
SAFPLUS_RPC_TEST := 1
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx) 

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include -I../../rpc -I./stubs -I./stubs/server

# Management includes and libraries
CPP_FLAGS +=  -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/ `pkg-config --cflags protobuf`

STUBS_SRC := $(notdir $(wildcard stubs/*.cxx))
STUBS_OBJ := $(addprefix $(OBJ_DIR)/test/rpc/, $(subst .cxx,.o,$(STUBS_SRC)))

CLIENT_SRC := TestClient.cxx RpcWakeable.cxx
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/test/rpc/client/, $(subst .cxx,.o,$(CLIENT_SRC))) $(STUBS_OBJ)

SERVER_SRC := TestServer.cxx $(notdir $(wildcard stubs/server/*.cxx))
SERVER_OBJ := $(addprefix $(OBJ_DIR)/test/rpc/svr/,$(subst .cxx,.o,$(SERVER_SRC))) $(STUBS_OBJ)

COMBINE_SRC := TestCombine.cxx RpcWakeable.cxx $(notdir $(wildcard stubs/server/*.cxx))
COMBINE_OBJ := $(addprefix $(OBJ_DIR)/test/rpc/combine/,$(subst .cxx,.o,$(COMBINE_SRC))) $(STUBS_OBJ)

# Specify the required libraries
SAFPLUS_LIBS := clOsal7 clUtils7 clLog clMsg clRpc clMgt7
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS +=-L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 -lpthread -lstdc++ -ldl -lrt -lm  -L$(BOOST_DIR)/stage/lib/ -lboost_system -L$(SAFPLUS_SDK_TARGET)/lib -lezxml -lclRpc

SAFplusRpcGen := $(SAFPLUS_TARGET)/bin/protoc-gen-rpc

all: $(TEST_DIR)/TestClient $(TEST_DIR)/TestServer $(TEST_DIR)/TestCombine

$(TEST_DIR)/TestClient: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) -lprotobuf -lprotoc 
# `pkg-config --libs protobuf`

$(TEST_DIR)/TestServer: $(SERVER_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(SERVER_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) -lprotobuf -lprotoc

$(TEST_DIR)/TestCombine: $(COMBINE_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(COMBINE_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) -lprotobuf -lprotoc

%.o: %.cxx
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/test/rpc/%.o: stubs/%.cxx
	-mkdir -p $(OBJ_DIR)/test/rpc
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/test/rpc/client/%.o: %.cxx
	-mkdir -p $(OBJ_DIR)/test/rpc/client
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/test/rpc/svr/%.o: %.cxx
	-mkdir -p $(OBJ_DIR)/test/rpc/svr
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/test/rpc/svr/%.o: stubs/server/%.cxx
	-mkdir -p $(OBJ_DIR)/test/rpc/svr
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/test/rpc/combine/%.o: %.cxx
	-mkdir -p $(OBJ_DIR)/test/rpc/combine
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/test/rpc/combine/%.o: stubs/server/%.cxx
	-mkdir -p $(OBJ_DIR)/test/rpc/combine
	$(COMPILE_CPP) $@ $<

gen: $(SAFplusRpcGen)
	-mkdir -p ./stubs/server
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/SAFplus/yang -f y2cpp $(SAFPLUS_SRC_DIR)/SAFplus/yang/rpcTest.yang --y2cpp-output `pwd` --y2cpp-mgt $(MGT_SRC_DIR) --y2cpp-rpc
	protoc -I../../../3rdparty -I../../rpc -I. --cpp_out=./stubs rpcTest.proto
	$(SAFplusRpcGen) -I../../../3rdparty -I../../rpc -I. --rpc_out=./stubs rpcTest.proto

cleangen:
	rm -rf stubs
	rm -rf $(SAFplusRpcGen)

clean:
	rm -rf $(OBJ_DIR)/test/rpc $(TEST_DIR)/TestClient $(TEST_DIR)/TestServer $(TEST_DIR)/TestCombine


include ../../../mk/safplus_targets.mk
