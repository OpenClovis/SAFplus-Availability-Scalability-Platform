S7 := 1
SAFPLUS_AMF_LIB:=1  # Identify what is being built, so safplus_targets does not override
SAFPLUS_NOTIFICATION_LIB:=1
include ../mk/preface.mk

CLIENT_H := $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx) $(wildcard amfAppRpc/$(PROTOBUFVER)/*.hxx) $(wildcard amfAppRpc/$(PROTOBUFVER)/*.h)
CLIENT_SRC := amfAppRpcImpl.cxx amf.cxx $(notdir $(wildcard amfAppRpc/$(PROTOBUFVER)/*.cxx)) $(notdir $(wildcard amfAppRpc/$(PROTOBUFVER)/*.cc))
AMF_MGMT_CLIENT_SRC := clAmfMgmtApi.cxx $(notdir $(wildcard amfMgmtRpc/$(PROTOBUFVER)/*.cxx)) $(notdir $(wildcard amfMgmtRpc/$(PROTOBUFVER)/*.cc))
AMF_NOTIFICATION_CLIENT_SRC := notificationClient.cxx
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cc,.o,$(subst .cxx,.o,$(CLIENT_SRC))))
AMF_MGMT_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cc,.o,$(subst .cxx,.o,$(AMF_MGMT_CLIENT_SRC))))
AMF_NOTIFICATION_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cc,.o,$(subst .cxx,.o,$(AMF_NOTIFICATION_CLIENT_SRC))))

# Specify the required libraries
SAFPLUS_LIBS := clEvent clEventServer
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

Release all: $(LIB_DIR)/libclAmf.so $(LIB_DIR)/libclAmfMgmt.so $(LIB_DIR)/libclNotification.so

$(SAFPLUS_TARGET)/lib/libclAmf.so: $(CLIENT_OBJ)
	$(LINK_SO) $@ $+  -lpthread -lrt

$(SAFPLUS_TARGET)/lib/libclAmfMgmt.so: $(AMF_MGMT_CLIENT_OBJ)
	$(LINK_SO) $@ $+ #$(MWOBJ_DIR)/amf.o #-lpthread -lrt

$(SAFPLUS_TARGET)/lib/libclNotification.so: $(AMF_NOTIFICATION_CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(AMF_NOTIFICATION_CLIENT_OBJ) $(LINK_SO_LIBS) -lboost_serialization $(SAFPLUS_LINK_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx Makefile 
	$(COMPILE_CPP) $@ $< -I amfAppRpc/$(PROTOBUFVER) -I../rpc -I amfMgmtRpc/$(PROTOBUFVER) -I./server/SAFplusAmf $(XML2_CFLAGS)

$(MWOBJ_DIR)/%.o: amfAppRpc/$(PROTOBUFVER)/%.cxx Makefile 
	$(COMPILE_CPP) $@ $< -I../rpc

$(MWOBJ_DIR)/%.o: amfMgmtRpc/$(PROTOBUFVER)/%.cc Makefile
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: amfMgmtRpc/$(PROTOBUFVER)/%.cxx Makefile
	$(COMPILE_CPP) $@ $< -I../rpc

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclAmf.so $(CLIENT_OBJ) $(SAFPLUS_TARGET)/lib/libclAmfMgmt.so $(AMF_MGMT_CLIENT_OBJ) $(SAFPLUS_TARGET)/lib/libclNotification.so $(AMF_NOTIFICATION_CLIENT_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
