S7 := 1
SAFPLUS_CKPT_LIB:=1  # Identify what is being built, so safplus_targets does not override
include ../../mk/preface.mk

CKPT_CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) $(wildcard ../../include7/*.hxx) 
CKPT_CLIENT_SRC := $(wildcard *.cxx)
CKPT_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CKPT_CLIENT_SRC)))

all: $(LIB_DIR)/libclCkpt.so

$(SAFPLUS_TARGET)/lib/libclCkpt.so: $(CKPT_CLIENT_OBJ)
	$(LINK_SO) $@ $+  -lpthread -lrt

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CKPT_CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclCkpt.so $(MWOBJ_DIR)/*.o
