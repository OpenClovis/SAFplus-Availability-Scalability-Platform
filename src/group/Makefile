S7 := 1
SAFPLUS_GROUP_LIB := 1
include ../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include -I../rpc -IgroupCliRpc/$(PROTOBUFVER)

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

SERVER_H := $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx) $(wildcard $(COMMON_DIR)/SAFplusTypes/*.hxx)
SERVER_SRC := $(wildcard groupCliRpc/$(PROTOBUFVER)/*.cc) $(wildcard groupCliRpc/$(PROTOBUFVER)/*.cxx) $(wildcard groupCliRpc/$(PROTOBUFVER)/server/*.cxx)
SERVER_OBJ := $(subst .cc,.o,$(subst .cxx,.o,$(notdir $(SERVER_SRC))))
SERVER_OBJ := $(addprefix $(OBJ_DIR)/groupCli/,$(SERVER_OBJ))

LINK_SO_LIBS += -lprotobuf -lprotoc

# Specify the required libraries
SAFPLUS_LIBS := clTimer clCkpt clName clMsg clUtils clLog clOsal clFault clDbal clRpc
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

# Specify the required libraries
GROUP_LIB := clGroup
# Then use these in the make rule
GROUP_DEP_LIB     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(GROUP_LIB)))
GROUP_LINK_LIB := -L$(LIB_DIR) $(addprefix -l,$(GROUP_LIB))


Release all: $(LIB_DIR)/libclGroup.so $(BIN_DIR)/spgroupd $(BIN_DIR)/safplus_group $(LIB_DIR)/libclGroupCli.so
# $(LIB_DIR)/libclGroup.a 

$(LIB_DIR)/libclGroup.so: $(CLIENT_OBJ) $(MWOBJ_DIR)/clGroupCli.o $(SERVER_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_OBJ) $(MWOBJ_DIR)/clGroupCli.o $(SERVER_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)


$(LIB_DIR)/libclGroupCli.so: $(MWOBJ_DIR)/clGroupCliApi.o $(SAFPLUS_DEP_LIBS) $(GROUP_DEP_LIB)
	$(LINK_SO) $@  $(MWOBJ_DIR)/clGroupCliApi.o $(SAFPLUS_LINK_LIBS)  $(GROUP_LINK_LIB) $(LINK_SO_LIBS)

#$(LIB_DIR)/libclGroupServerCli.so: $(CLIENT_OBJ) $(MWOBJ_DIR)/clGroupCli.o $(SERVER_OBJ) $(SAFPLUS_DEP_LIBS)
#	$(LINK_SO) $@  $(CLIENT_OBJ) $(MWOBJ_DIR)/clGroupCli.o $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)
#$(LIB_DIR)/libclGroup.a: $(CLIENT_OBJ) $(DEP_LIBS)
#	ar -cvq $@ $+

$(OBJ_DIR)/%.o: bin/%.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/clGroupCli.o: clGroupCli.cpp Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/clGroupCliApi.o: clGroupCliApi.cpp Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(BIN_DIR)/spgroupd: $(OBJ_DIR)/spgroupd.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@  $< -lclGroup $(SAFPLUS_LINK_LIBS) -lboost_program_options $(LINK_SO_LIBS)

$(BIN_DIR)/safplus_group: $(OBJ_DIR)/safplus_group.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@  $< -lclGroup $(SAFPLUS_LINK_LIBS) -lboost_program_options $(LINK_SO_LIBS)

gengroupclirpc: $(SAFplusRpcGen)
	-mkdir -p groupCliRpc/$(PROTOBUFVER)
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang:/usr/local/lib PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang -f y2cpp groupCliRpc.yang --y2cpp-output `pwd` --y2cpp-rpc --y2cpp-sdkdir $(SAFPLUS_SRC_DIR)
	LD_LIBRARY_PATH=$(INSTALL_DIR)/lib:$$LD_LIBRARY_PATH $(PROTOC) -I../3rdparty -I../rpc -I. --cpp_out=./groupCliRpc/$(PROTOBUFVER) groupCliRpc.proto
	LD_LIBRARY_PATH=$(INSTALL_DIR)/lib:$$LD_LIBRARY_PATH $(SAFplusRpcGen) -I../3rdparty -I../rpc -I. --rpc_out=./groupCliRpc/$(PROTOBUFVER) groupCliRpc.proto

$(OBJ_DIR)/groupCli/%.o: groupCliRpc/$(PROTOBUFVER)/%.cxx $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/groupCli/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/groupCli/%.o: groupCliRpc/$(PROTOBUFVER)/server/%.cxx $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/groupCli/
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclGroup.so $(MWOBJ_DIR)/group.o $(MWOBJ_DIR)/groupshm.o $(BIN_DIR)/spgroup $(BIN_DIR)/safplus_group $(LIB_DIR)/libclGroupCli.so $(MWOBJ_DIR)/clGroupCli.o $(MWOBJ_DIR)/clGroupCliApi.o $(OBJ_DIR)/groupCli/*.o
 
include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
