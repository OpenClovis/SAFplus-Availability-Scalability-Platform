S7 := 1
SAFPLUS_RPC_LIB := 1
include ../../mk/preface.mk

CPP_FLAGS += `pkg-config --cflags protobuf` -I$(SAFPLUS_3RDPARTY_DIR)
PROTOC_LIBS = `pkg-config --libs protobuf` -lprotoc

PROTOC_TOOL_H   := $(wildcard *.hxx)
PROTOC_TOOL_SRC := $(wildcard *.cxx)
PROTOC_TOOL_OBJ := $(addprefix $(OBJ_DIR)/protoc/,$(subst .cxx,.o, $(notdir $(PROTOC_TOOL_SRC))))

NOOP := $(shell mkdir -p $(OBJ_DIR)/protoc)

Release all: $(SAFPLUS_TARGET)/bin/protoc-gen-rpc

$(OBJ_DIR)/protoc/%.o: %.cxx Makefile $(PROTOC_TOOL_H)
	$(COMPILE_CPP) $@ $<

$(SAFPLUS_TARGET)/bin/protoc-gen-rpc: $(PROTOC_TOOL_OBJ)
	$(LINK_EXE) $@ $(PROTOC_TOOL_OBJ) $(PROTOC_LIBS)

clean:
	rm -rf $(OBJ_DIR)/protoc $(SAFPLUS_TARGET)/bin/protoc-gen-rpc

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
