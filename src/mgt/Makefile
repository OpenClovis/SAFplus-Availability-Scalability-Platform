S7 := 1
SAFPLUS_MGT_LIB := 1
include ../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I$(SAFPLUS_INC_DIR)

# Management includes and libraries
CPP_FLAGS +=  $(SAFPLUS_MGT_INC_FLAGS)

CLIENT_H := $(wildcard ../include/*.hxx)
CLIENT_SRC := $(wildcard *.cxx) $(wildcard *.cc) $(wildcard $(PROTOBUFVER)/*.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cc,.o, $(subst .cxx,.o,$(notdir $(CLIENT_SRC)))))

# Specify the required libraries
SAFPLUS_LIBS := clCkpt clGroup clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))  


Release all: MgtMsgPbHeader $(LIB_DIR)/libclMgt.so

MgtMsgPbHeader: MgtMsg.pb.hxx

MgtMsg.pb.hxx: $(PROTOBUFVER)/MgtMsg.pb.hxx
	rm -f $@
	ln -s $(PROTOBUFVER)/$@ $@

$(LIB_DIR)/libclMgt.so: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(CLIENT_OBJ) $(LINK_SO_LIBS) $(SAFPLUS_LINK_LIBS) $(PROTOBUF_LINK) $(XML2_LINK)

$(MWOBJ_DIR)/%.o: $(PROTOBUFVER)/%.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: %.cc Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(PROTOBUFVER)/MgtMsg.pb.hxx:
	@mkdir -p $(PROTOBUFVER)
	$(PROTOC) -I../3rdparty -I. --cpp_out=./$(PROTOBUFVER) MgtMsg.proto
	export LD_LIBRARY_PATH=$(INSTALL_DIR)/lib; $(SAFplusRpcGen) -I../3rdparty -I. --rpc_out=./$(PROTOBUFVER) --rpc_opts=rename=true MgtMsg.proto


gen:  $(PROTOBUFVER)/MgtMsg.pb.hxx MgtMsg.pb.hxx 

clean:
	rm -f $(LIB_DIR)/libclMgt.so $(CLIENT_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
