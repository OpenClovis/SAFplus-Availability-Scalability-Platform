S7 := 1
SAFPLUS_EVENT_SERVER := 1
include ../../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. $(SAFPLUS_MGT_INC_FLAGS) -I$(PROTOBUFVER) -I$(PROTOBUFVER)/stubs

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC := rpcEvent.cxx rpcEvent.pb.cxx EventServer.cxx EventCkpt.cxx EventChannel.cxx EventSharedMem.cxx
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(notdir $(CLIENT_SRC))))

# Specify the required libraries
SAFPLUS_LIBS := clRpc clCkpt clName clMsg clLog clUtils clOsal clGroup clMgt clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))  

all: $(LIB_DIR)/libclEventServer.so

ifdef SAFPLUS_GEN_STATIC_LIBS
all:  $(LIB_DIR)/libclEventServer.a 

$(LIB_DIR)/libclEventServer.a: $(CLIENT_OBJ) $(DEP_LIBS)
	ar -cvq $@ $+
endif

$(LIB_DIR)/libclEventServer.so: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<
	
gen:
	mkdir -p ./$(PROTOBUFVER)/stubs
	$(call SAFPLUS_RPC_GEN,rpcEvent,stubs)
	
clean:
	rm -f $(LIB_DIR)/libclEventServer.so $(LIB_DIR)/libclEventServer.a $(MWOBJ_DIR)/Event* $(MWOBJ_DIR)/rpcEvent*

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
