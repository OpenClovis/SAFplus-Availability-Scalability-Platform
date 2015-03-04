S7 := 1
SAFPLUS_RPC_LIB := 1
include ../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

#SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include -I../../3rdparty

# Management includes and libraries
CPP_FLAGS += `pkg-config --cflags protobuf`

# Specify the required libraries
#SAFPLUS_LIBS := clOsal7 clUtils7 clLog clIoc7 clMgt7
# Then use these in the make rule
#SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
#SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

#LINK_STD_LIBS +=
#-L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 -lpthread -lstdc++ -ldl -lrt -lm  -L$(BOOST_DIR)/stage/lib/ -lboost_system -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml

RPC_SRC = $(wildcard *.cxx)
RPC_OBJ = $(addprefix $(MWOBJ_DIR)/, $(subst .cxx,.o,$(RPC_SRC)))

PROTOBUF_PLUGIN_SRC = $(wildcard protoc/*.cxx)
PROTOBUF_PLUGIN_OBJ = $(addprefix $(OBJ_DIR)/protoc/,$(subst .cxx,.o, $(notdir $(PROTOBUF_PLUGIN_SRC))))

all: $(SAFPLUS_TARGET)/bin/protoc-gen-rpc $(LIB_DIR)/libclRpc.so

$(LIB_DIR)/libclRpc.so: $(RPC_OBJ)
	$(LINK_SO) $@ $(RPC_OBJ)

$(MWOBJ_DIR)/%.o: %.cxx
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/protoc/%.o: protoc/%.cxx $(wildcard protoc/*.hxx)
	-mkdir -p $(OBJ_DIR)/protoc/
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/bin/protoc-gen-rpc: $(PROTOBUF_PLUGIN_OBJ) 
	$(LINK_EXE) $@ $(PROTOBUF_PLUGIN_OBJ) -lprotobuf -lprotoc 

SAFplusRpcGen := $(SAFPLUS_TARGET)/bin/protoc-gen-rpc

gen: $(SAFplusRpcGen)
	protoc -I../../3rdparty -I. --cpp_out=. SAFplusPBExt.proto
#Rename pb.h => pb.hxx, pb.cc => pb.cxx
	$(SAFplusRpcGen) -I../../3rdparty -I. --rpc_out=. --rename=true SAFplusPBExt.proto
	protoc -I../../3rdparty -I. --cpp_out=. SAFplusRpc.proto
#Rename pb.h => pb.hxx, pb.cc => pb.cxx
	$(SAFplusRpcGen) -I../../3rdparty -I. --rpc_out=. --rename=true SAFplusRpc.proto

cleangen:
	rm -rf *.pb.hxx *.pb.cxx

clean:
	rm -rf $(LIB_DIR)/libclRpc.so $(RPC_OBJ) $(OBJ_DIR)/protoc $(SAFPLUS_TARGET)/bin/protoc-gen-rpc

include ../../mk/safplus_targets.mk
