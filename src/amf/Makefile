S7 := 1
SAFPLUS_AMF_LIB:=1  # Identify what is being built, so safplus_targets does not override
include ../mk/preface.mk

CLIENT_H := $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx) $(wildcard amfAppRpc/$(PROTOBUFVER)/*.hxx) $(wildcard amfAppRpc/$(PROTOBUFVER)/*.h)
CLIENT_SRC := amfAppRpcImpl.cxx amf.cxx $(notdir $(wildcard amfAppRpc/$(PROTOBUFVER)/*.cxx)) $(notdir $(wildcard amfAppRpc/$(PROTOBUFVER)/*.cc))
AMF_MGMT_CLIENT_SRC := clAmfMgmtApi.cxx $(notdir $(wildcard amfMgmtRpc/$(PROTOBUFVER)/*.cxx)) $(notdir $(wildcard amfMgmtRpc/$(PROTOBUFVER)/*.cc))
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cc,.o,$(subst .cxx,.o,$(CLIENT_SRC))))
AMF_MGMT_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cc,.o,$(subst .cxx,.o,$(AMF_MGMT_CLIENT_SRC))))

Release all: $(LIB_DIR)/libclAmf.so $(LIB_DIR)/libclAmfMgmt.so

$(SAFPLUS_TARGET)/lib/libclAmf.so: $(CLIENT_OBJ)
	$(LINK_SO) $@ $+  -lpthread -lrt

$(SAFPLUS_TARGET)/lib/libclAmfMgmt.so: $(AMF_MGMT_CLIENT_OBJ)
	$(LINK_SO) $@ $+ #$(MWOBJ_DIR)/amf.o #-lpthread -lrt

$(MWOBJ_DIR)/%.o: %.cxx Makefile 
	$(COMPILE_CPP) $@ $< -I amfAppRpc/$(PROTOBUFVER) -I../rpc -I amfMgmtRpc/$(PROTOBUFVER) -I./server/SAFplusAmf $(XML2_CFLAGS)

$(MWOBJ_DIR)/%.o: amfAppRpc/$(PROTOBUFVER)/%.cxx Makefile 
	$(COMPILE_CPP) $@ $< -I../rpc

$(MWOBJ_DIR)/%.o: amfMgmtRpc/$(PROTOBUFVER)/%.cc Makefile
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: amfMgmtRpc/$(PROTOBUFVER)/%.cxx Makefile
	$(COMPILE_CPP) $@ $< -I../rpc

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclAmf.so $(CLIENT_OBJ) $(SAFPLUS_TARGET)/lib/libclAmfMgmt.so $(AMF_MGMT_CLIENT_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
