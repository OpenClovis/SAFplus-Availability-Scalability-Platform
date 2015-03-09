S7 := 1
SAFPLUS_RPC_LIB := 1
include ../mk/preface.mk

CPP_FLAGS += `pkg-config --cflags protobuf`

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.h) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC = $(wildcard *.cxx)
CLIENT_OBJ = $(addprefix $(MWOBJ_DIR)/, $(subst .cxx,.o,$(CLIENT_SRC)))

Release all: $(SAFPLUS_TARGET)/bin/protoc-gen-rpc $(LIB_DIR)/libclRpc.so

$(LIB_DIR)/libclRpc.so: $(CLIENT_OBJ)
	$(LINK_SO) $@ $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(SAFPLUS_TARGET)/bin/protoc-gen-rpc:
	make -C protoc

$(MWOBJ_DIR)/%.o: %.cxx
	$(COMPILE_CPP) $@ $<

gen: $(SAFPLUS_TARGET)/bin/protoc-gen-rpc
	$(call SAFPLUS_RPC_GEN, SAFplusPBExt,.,true)
	$(call SAFPLUS_RPC_GEN, SAFplusRpc,.,true)

cleangen:
	rm -rf *.pb.hxx *.pb.cxx

clean:
	rm -rf $(LIB_DIR)/libclRpc.so $(CLIENT_OBJ)
	make -C protoc clean

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
