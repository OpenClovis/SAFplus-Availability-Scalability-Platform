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

REFLECTOR_SRC := rpcReflector.cxx $(notdir $(wildcard reflection/*.cxx))
REFLECTOR_OBJ := $(addprefix $(OBJ_DIR)/test/rpc/,$(subst .cxx,.o,$(REFLECTOR_SRC)))

PERF_SRC := testRpcPerf.cxx $(notdir $(wildcard reflection/*.cxx))
PERF_OBJ := $(addprefix $(OBJ_DIR)/test/rpc/,$(subst .cxx,.o,$(PERF_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clRpc clFault clMsg clTimer clUtils clLog  clOsal 

# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))

SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

Release all: $(TEST_DIR)/rpcReflector $(TEST_DIR)/testRpcPerf 

# $(TEST_DIR)/TestRpcClient $(TEST_DIR)/TestRpcServer $(TEST_DIR)/TestRpcCombined

$(TEST_DIR)/TestRpcClient: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) 

$(TEST_DIR)/TestRpcServer: $(SERVER_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(SERVER_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(TEST_DIR)/TestRpcCombined: $(COMBINE_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(COMBINE_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(TEST_DIR)/rpcReflector: $(LIB_DIR)/libclOsal.so $(REFLECTOR_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(REFLECTOR_OBJ) $(SAFPLUS_LINK_LIBS)  -lboost_program_options $(LINK_SO_LIBS) 

$(TEST_DIR)/testRpcPerf: $(PERF_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(PERF_OBJ) $(SAFPLUS_LINK_LIBS)  -lboost_program_options $(LINK_SO_LIBS) 


$(OBJ_DIR)/test/rpc/%.o: %.cxx Makefile $(STUBS_H)
	-mkdir -p $(OBJ_DIR)/test/rpc
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/test/rpc/%.o: stubs/%.cxx Makefile $(STUBS_H)
	-mkdir -p $(OBJ_DIR)/test/rpc
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/test/rpc/%.o: reflection/%.cxx Makefile $(STUBS_H)
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
	-mkdir -p reflection
	$(call SAFPLUS_YANG_RPC_GEN, msgReflection, reflection)

cleangen:
	rm -rf stubs

clean:
	rm -rf $(OBJ_DIR)/test/rpc $(TEST_DIR)/TestClient $(TEST_DIR)/TestServer $(TEST_DIR)/TestCombine $(TEST_DIR)/TestRpcPerf

# TAE packaging stuff
image: $(IMAGE_FILE)

$(IMAGE_FILE): $(TEST_DIR)/testRpcPerf
	cp setup start cleanup $(SAFPLUS_TARGET)
	$(SAFPLUS_MAKE_DIR)/safplus_packager.py -x "(cp setup start cleanup {image_dir})" $@


include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk

