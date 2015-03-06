S7 := 1
SAFPLUS_AMF_LIB:=1  # Identify what is being built, so safplus_targets does not override
include ../mk/preface.mk

CLIENT_H := $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx) $(wildcard amfAppRpc/*.hxx) $(wildcard amfAppRpc/*.h)
CLIENT_SRC := $(wildcard *.cxx) $(notdir $(wildcard amfAppRpc/*.cxx)) $(notdir $(wildcard amfAppRpc/*.cc))
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cc,.o,$(subst .cxx,.o,$(CLIENT_SRC))))

Release all: $(LIB_DIR)/libclAmf.so

$(SAFPLUS_TARGET)/lib/libclAmf.so: $(CLIENT_OBJ)
	$(LINK_SO) $@ $+  -lpthread -lrt

$(MWOBJ_DIR)/%.o: %.cxx Makefile 
	$(COMPILE_CPP) $@ $< -I amfAppRpc -I../rpc

$(MWOBJ_DIR)/%.o: amfAppRpc/%.cxx Makefile 
	$(COMPILE_CPP) $@ $< -I../rpc

$(MWOBJ_DIR)/%.o: amfAppRpc/%.cc Makefile
	$(COMPILE_CPP) $@ $<


clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclAmf.so $(CLIENT_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
