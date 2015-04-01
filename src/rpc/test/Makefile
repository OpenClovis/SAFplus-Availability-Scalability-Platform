S7 := 1
SAFPLUS_RPC_TEST := 1
include ../../mk/preface.mk

CPP_FLAGS += -I.. -I./stubs

STUBS_H   := $(wildcard *.hxx) $(wildcard stubs/*.hxx)

CLIENT_SRC := TestClient.cxx RpcWakeable.cxx $(notdir $(wildcard stubs/*.cxx))
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/test/rpc/, $(subst .cxx,.o,$(CLIENT_SRC)))

SERVER_SRC := TestServer.cxx $(notdir $(wildcard stubs/*.cxx)) $(notdir $(wildcard stubs/server/*.cxx))
SERVER_OBJ := $(addprefix $(OBJ_DIR)/test/rpc/,$(subst .cxx,.o,$(SERVER_SRC)))

COMBINE_SRC := TestCombine.cxx RpcWakeable.cxx $(notdir $(wildcard stubs/*.cxx)) $(notdir $(wildcard stubs/server/*.cxx))
COMBINE_OBJ := $(addprefix $(OBJ_DIR)/test/rpc/,$(subst .cxx,.o,$(COMBINE_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clOsal clUtils clLog clMsg clRpc

# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

Release all: $(TEST_DIR)/TestRpcClient $(TEST_DIR)/TestRpcServer $(TEST_DIR)/TestRpcCombined

$(TEST_DIR)/TestRpcClient: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) 

$(TEST_DIR)/TestRpcServer: $(SERVER_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(SERVER_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(TEST_DIR)/TestRpcCombined: $(COMBINE_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(COMBINE_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(TEST_DIR)/rpcReflector: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) 


$(OBJ_DIR)/test/rpc/%.o: %.cxx Makefile $(STUBS_H)
	-mkdir -p $(OBJ_DIR)/test/rpc
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/test/rpc/%.o: stubs/%.cxx Makefile $(STUBS_H)
	-mkdir -p $(OBJ_DIR)/test/rpc
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/test/rpc/%.o: stubs/server/%.cxx Makefile $(STUBS_H)
	-mkdir -p $(OBJ_DIR)/test/rpc
	$(COMPILE_CPP) $@ $<

gen:
	mkdir -p ./stubs
	$(call SAFPLUS_RPC_GEN,rpcTest,stubs)
	mkdir -p ./reflector
	$(call SAFPLUS_RPC_GEN,msgReflector,reflector)

cleangen:
	rm -rf stubs

clean:
	rm -rf $(OBJ_DIR)/test/rpc $(TEST_DIR)/TestClient $(TEST_DIR)/TestServer $(TEST_DIR)/TestCombine
