S7 := 1
SAFPLUS_AMF_SERVER:=1  # Identify what is being built, so safplus_targets does not override
include ../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

#MakefileDepend := Makefile
MakefileDepend :=

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I..  -I../rpc -I./amfRpc $(SAFPLUS_MGT_INC_FLAGS)

# Specify the required libraries
SAFPLUS_LIBS := clRpc clMgt clName clFaultServer clFault clCkpt clGroup clMsg clUtils clLog clDbal clOsal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

POLICY_ENV_LIB := $(LIB_DIR)/libAmfPolicyEnv.so

COMMON_DIR=$(SAFPLUS_SRC_DIR)/common

# Management includes and libraries

CPP_FLAGS += -I$(COMMON_DIR)/SAFplusTypes -ISAFplusAmf

#SERVER_H := $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx) $(wildcard ../../../include7/*.hpp) $(wildcard SAFplusAmf/*.hxx) $(wildcard SAFplusTypes/*.hxx) $(wildcard ietfYangTypes/*.hxx) $(wildcard ENTITYSTATETCMIB/*.hxx)
SERVER_H := $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx) $(wildcard SAFplusAmf/*.hxx) $(wildcard $(COMMON_DIR)/SAFplusTypes/*.hxx)
SERVER_SRC := main.cxx amfcommon.cxx $(wildcard amfRpc/*.cc) $(wildcard amfRpc/*.cxx) amfAppRpcImpl.cxx

#POLICY_ENV_SRC := $(wildcard SAFplusAmf/*.cxx) $(wildcard SAFplusTypes/*.cxx) $(wildcard ietfYangTypes/*.cxx) $(wildcard ENTITYSTATETCMIB/*.cxx)
POLICY_ENV_SRC := amfOperations.cxx portAllocator.cxx $(wildcard SAFplusAmf/*.cxx) $(wildcard $(COMMON_DIR)/SAFplusTypes/*.cxx) $(wildcard amfRpc/server/*.cxx) $(wildcard amfAppRpc/*.cxx) $(wildcard amfAppRpc/*.cc)

SERVER_OBJ := $(subst .cc,.o,$(subst .cxx,.o,$(notdir $(SERVER_SRC))))
SERVER_OBJ := $(addprefix $(OBJ_DIR)/amfsvr/,$(SERVER_OBJ))

POLICY_ENV_OBJ := $(subst .cxx,.o,$(notdir $(POLICY_ENV_SRC)))
POLICY_ENV_OBJ := $(addprefix $(OBJ_DIR)/amfsvr/,$(POLICY_ENV_OBJ))


Release all: $(PLUGIN_DIR)/customAmfPolicy.so $(PLUGIN_DIR)/nPlusmAmfPolicy.so  $(SAFPLUS_TARGET)/bin/safplus_amf

# Shared library providing functions needed by the policy library & SAFplus AMF.
$(POLICY_ENV_LIB): $(POLICY_ENV_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(POLICY_ENV_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lezxml -lxml2 -lboost_filesystem $(LINK_SO_LIBS)

# Define policies as shared libraries
$(PLUGIN_DIR)/customAmfPolicy.so: $(OBJ_DIR)/amfsvr/customAmfPolicy.o $(POLICY_ENV_LIB) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(OBJ_DIR)/amfsvr/customAmfPolicy.o -lAmfPolicyEnv $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lezxml -lxml2 -lboost_filesystem -lprotobuf -lprotoc $(LINK_SO_LIBS)

$(PLUGIN_DIR)/nPlusmAmfPolicy.so: $(OBJ_DIR)/amfsvr/nPlusmAmfPolicy.o $(POLICY_ENV_LIB) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(OBJ_DIR)/amfsvr/nPlusmAmfPolicy.o -lAmfPolicyEnv $(SAFPLUS_LINK_LIBS)  -L$(INSTALL_DIR)/lib -lezxml -lxml2 -lboost_filesystem -lprotobuf -lprotoc $(LINK_SO_LIBS)

# SAFplus AMF binary
$(BIN_DIR)/safplus_amf: $(SERVER_OBJ) $(POLICY_ENV_LIB) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(SERVER_OBJ) -lAmfPolicyEnv $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lezxml  -lxml2 -lboost_filesystem -lprotobuf -lprotoc $(LINK_SO_LIBS)

# C code
$(OBJ_DIR)/amfsvr/%.o: %.cxx $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/amfsvr/%.o: SAFplusAmf/%.cxx $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/amfsvr/%.o: amfRpc/%.cc $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/amfsvr/%.o: amfRpc/%.cxx $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<


$(OBJ_DIR)/amfsvr/%.o: amfAppRpc/%.cxx $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/amfsvr/%.o: amfRpc/server/%.cxx $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/amfsvr/%.o: $(COMMON_DIR)/SAFplusTypes/%.cxx $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	(cd $(COMMON_DIR)/SAFplusTypes && $(COMPILE_CPP) $@ $(<F))

$(OBJ_DIR)/amfsvr/%.o: $(COMMON_DIR)/ietfYangTypes/%.cxx $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	(cd $(COMMON_DIR)/ietfYangTypes && $(COMPILE_CPP) $@ $(<F))

$(OBJ_DIR)/amfsvr/%.o: $(COMMON_DIR)/ENTITYSTATETCMIB/%.cxx $(MakefileDepend) $(SERVER_H)
	-mkdir -p $(OBJ_DIR)/amfsvr/
	(cd $(COMMON_DIR)/ENTITYSTATETCMIB && $(COMPILE_CPP) $@ $(<F))

$(OBJ_DIR)/amfsvr/%.o: stubs/server/%.cxx
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/amfsvr/%.o: stubs/%.cc
	-mkdir -p $(OBJ_DIR)/amfsvr/
	$(COMPILE_CPP) $@ $<

SAFplusRpcGen := $(SAFPLUS_TARGET)/bin/protoc-gen-rpc

gen:
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang -f y2cpp $(SAFPLUS_SRC_DIR)/yang/SAFplusAmf.yang --y2cpp-output `pwd` --y2cpp-mgt $(MGT_SRC_DIR)
	(rsync -avpDHL ietfYangTypes $(COMMON_DIR)/; rsync -avpDHL SAFplusTypes $(COMMON_DIR)/; rsync -avpDHL ENTITYSTATETCMIB $(COMMON_DIR)/)
	(rm -rf ietfYangTypes SAFplusTypes ENTITYSTATETCMIB)

genrpc: $(SAFplusRpcGen)
	-mkdir -p amfRpc
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang:/usr/local/lib PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang -f y2cpp amfRpc.yang --y2cpp-output `pwd` --y2cpp-mgt $(MGT_SRC_DIR) --y2cpp-rpc
	LD_LIBRARY_PATH=/usr/local/lib:/usr/lib protoc -I../../../3rdparty -I../../rpc -I. --cpp_out=./amfRpc amfRpc.proto
	LD_LIBRARY_PATH=/usr/local/lib:/usr/lib $(SAFplusRpcGen) -I../../../3rdparty -I../../rpc -I. --rpc_out=./amfRpc amfRpc.proto

genapprpc: $(SAFplusRpcGen)
	-mkdir -p amfAppRpc
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang:/usr/local/lib PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang -f y2cpp amfAppRpc.yang --y2cpp-output `pwd` --y2cpp-mgt $(MGT_SRC_DIR) --y2cpp-rpc
	LD_LIBRARY_PATH=/usr/local/lib:/usr/lib protoc -I../../../3rdparty -I../../rpc -I. --cpp_out=./amfAppRpc amfAppRpc.proto
	LD_LIBRARY_PATH=/usr/local/lib:/usr/lib $(SAFplusRpcGen) -I../../../3rdparty -I../../rpc -I. --rpc_out=./amfAppRpc amfAppRpc.proto

cleangen:
	(cd SAFplusAmf && find . -type f -not -name "*Common.hxx" -not -name "*Base.cxx" | xargs rm)
	(cd SAFplusTypes && find . -type f -not -name "*Common.hxx" -not -name "*Base.cxx" | xargs rm)
	(cd ietfYangTypes && find . -type f -not -name "*Common.hxx" -not -name "*Base.cxx" | xargs rm)

#	(cd ENTITYSTATETCMIB && find . -type f -not -name "*Common.hxx" -not -name "*Base.cxx" | xargs rm)


clean:
	rm -f $(OBJ_DIR)/amfsvr/*.o $(SAFPLUS_TARGET)/bin/safplus_amf $(POLICY_ENV_LIB) $(SAFPLUS_TARGET)/bin/customAmfPolicy.so $(SAFPLUS_TARGET)/bin/nPlusmAmfPolicy.so

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
