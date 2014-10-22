S7 := 1
SAFPLUS_AMF_LIB:=1  # Identify what is being built, so safplus_targets does not override
include ../../mk/preface.mk

CKPT_CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) $(wildcard ../../include7/*.hxx) $(wildcard amfAppRpc/*.hxx) $(wildcard amfAppRpc/*.h)
CKPT_CLIENT_SRC := $(wildcard *.cxx) $(notdir $(wildcard amfAppRpc/*.cxx)) $(notdir $(wildcard amfAppRpc/*.cc))
CKPT_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cc,.o,$(subst .cxx,.o,$(CKPT_CLIENT_SRC))))

Release all: $(LIB_DIR)/libclAmf.so

$(SAFPLUS_TARGET)/lib/libclAmf.so: $(CKPT_CLIENT_OBJ)
	$(LINK_SO) $@ $+  -lpthread -lrt

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CKPT_SERVER_H)
	$(COMPILE_CPP) $@ $< -I amfAppRpc -I../rpc

$(MWOBJ_DIR)/%.o: amfAppRpc/%.cxx Makefile $(CKPT_SERVER_H)
	$(COMPILE_CPP) $@ $< -I../rpc

$(MWOBJ_DIR)/%.o: amfAppRpc/%.cc Makefile $(CKPT_SERVER_H)
	$(COMPILE_CPP) $@ $<


clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclCkpt.so $(MWOBJ_DIR)/*.o
