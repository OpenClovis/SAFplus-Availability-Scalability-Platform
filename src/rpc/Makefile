S7 := 1
SAFPLUS_RPC_LIB := 1
include ../mk/preface.mk

CPP_FLAGS += -I./$(PROTOBUFVER)

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.h) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC = $(wildcard *.cxx) $(wildcard $(PROTOBUFVER)/*.cc)
CLIENT_OBJ = $(addprefix $(MWOBJ_DIR)/, $(subst .cc,.o, $(subst .cxx,.o,$(notdir $(CLIENT_SRC)))))

Release all: SAFplusPBExt.pb.hxx $(SAFPLUS_TOOL_TARGET)/bin/protoc-gen-rpc $(LIB_DIR)/libclRpc.so

SAFplusPBExt.pb.hxx: $(PROTOBUFVER)/SAFplusPBExt.pb.h
	ln -s $(PROTOBUFVER)/SAFplusPBExt.pb.h $@

$(LIB_DIR)/libclRpc.so: $(CLIENT_OBJ)
	$(LINK_SO) $@ $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(SAFPLUS_TOOL_TARGET)/bin/protoc-gen-rpc:
	make -C protoc

$(MWOBJ_DIR)/%.o: %.cxx SAFplusPBExt.pb.hxx
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: $(PROTOBUFVER)/%.cc SAFplusPBExt.pb.hxx
	$(COMPILE_CPP) $@ $<

$(PROTOBUFVER)/SAFplusPBExt.pb.h:
	@mkdir -p $(PROTOBUFVER)
	$(call SAFPLUS_RPC_GEN, SAFplusPBExt,.,rename=true)
	$(call SAFPLUS_RPC_GEN, SAFplusRpc,.,rename=true)


gen: SAFplusPBExt.pb.hxx $(SAFPLUS_TOOL_TARGET)/bin/protoc-gen-rpc

cleangen:
	rm -rf *.pb.hxx *.pb.cxx

clean:
	rm -rf $(LIB_DIR)/libclRpc.so $(CLIENT_OBJ)
	make -C protoc clean

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
