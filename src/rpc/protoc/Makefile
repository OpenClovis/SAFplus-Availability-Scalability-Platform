S7 := 1
SAFPLUS_RPC_LIB := 1
include ../../mk/preface.mk

CPP_FLAGS += -I/usr/include
# -I$(SAFPLUS_3RDPARTY_DIR)

PROTOC_TOOL_H   := $(wildcard *.hxx)
PROTOC_TOOL_SRC := $(wildcard *.cxx)
PROTOC_TOOL_OBJ := $(addprefix $(LOCAL_OBJ_DIR)/protoc/,$(subst .cxx,.o, $(notdir $(PROTOC_TOOL_SRC))))

NOOP := $(shell mkdir -p $(LOCAL_OBJ_DIR)/protoc)

Release all: $(SAFPLUS_TOOL_TARGET)/bin/protoc-gen-rpc

$(LOCAL_OBJ_DIR)/protoc/%.o: %.cxx Makefile $(PROTOC_TOOL_H)
	$(LOCAL_COMPILE_CPP) $@ $<

$(SAFPLUS_TOOL_TARGET)/bin/protoc-gen-rpc: $(PROTOC_TOOL_OBJ)
	$(LOCAL_LINK_EXE) $@ $(PROTOC_TOOL_OBJ) $(PROTOBUF_LINK)

clean:
	rm -rf $(OBJ_DIR)/protoc $(SAFPLUS_TARGET)/bin/protoc-gen-rpc

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
