S7 := 1
include ../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

# Management includes and libraries
CPP_FLAGS +=  -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/ `pkg-config --cflags protobuf`

# Specify the required libraries
#SAFPLUS_LIBS := clOsal7 clUtils7 clLog clIoc7 clMgt7
# Then use these in the make rule
#SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
#SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS +=-L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 -lpthread -lstdc++ -ldl -lrt -lm  -L$(BOOST_DIR)/stage/lib/ -lboost_system -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml

PROTOBUF_PLUGIN_SRC = $(wildcard *.cxx)
PROTOBUF_PLUGIN_OBJ = $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(PROTOBUF_PLUGIN_SRC)))

all: $(SAFPLUS_TARGET)/bin/protoc-gen-rpc $(LIB_DIR)/libSAFplusPBExt.so

$(LIB_DIR)/libSAFplusPBExt.so: $(OBJ_DIR)/SAFplusPBExt.pb.o $(OBJ_DIR)/SAFplusRpc.pb.o
	$(LINK_SO) $@ $(OBJ_DIR)/SAFplusPBExt.pb.o $(OBJ_DIR)/SAFplusRpc.pb.o

$(OBJ_DIR)/%.pb.o: %.pb.cc
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/bin/protoc-gen-rpc: $(PROTOBUF_PLUGIN_OBJ) 
	$(LINK_EXE) $@ $(PROTOBUF_PLUGIN_OBJ) -lprotobuf -lprotoc 

gen:
	protoc -I../../3rdparty -I. --cpp_out=. SAFplusPBExt.proto
	protoc -I../../3rdparty -I. --cpp_out=. SAFplusRpc.proto

cleangen:
	rm -rf *.pb.h *.pb.cc

clean:
	rm -rf $(PROTOBUF_PLUGIN_OBJ) $(SAFPLUS_TARGET)/bin/protoc-gen-rpc $(OBJ_DIR)/SAFplusPBExt.pb.o $(OBJ_DIR)/SAFplusRpc.pb.o

include ../../mk/safplus_targets.mk
