S7 := 1
SAFPLUS_ALARM_SERVER := 1
include ../../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. $(SAFPLUS_MGT_INC_FLAGS)

SERVER_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
SERVER_SRC := $(wildcard *.cxx)
SERVER_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(SERVER_SRC)))

# Management includes and libraries

CPP_FLAGS += -ISAFplusAlarm -I.. -I../../event -I../../include -I../common -I../../log



# Specify the required libraries
SAFPLUS_LIBS := clRpc clName clMsg clLog clEvent clUtils clOsal clFault SAFplusAlarm
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))  

all: $(LIB_DIR)/libclAlarmServer.so

ifdef SAFPLUS_GEN_STATIC_LIBS
all: $(LIB_DIR)/libclAlarmServer.a 

$(LIB_DIR)/libclAlarmServer.a: $(SERVER_OBJ) $(DEP_LIBS)
	ar -cvq $@ $+
endif

$(LIB_DIR)/libclAlarmServer.so: $(SERVER_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(SERVER_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(SERVER_H)
	$(COMPILE_CPP) $@ $<

gen:
	mkdir -p ./$(PROTOBUFVER)/stubs
	$(call SAFPLUS_RPC_GEN,rpcEvent,stubs)
	
clean:
	rm -f $(LIB_DIR)/libclAlarmServer.so $(LIB_DIR)/libclAlarmServer.a $(MWOBJ_DIR)/*.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
