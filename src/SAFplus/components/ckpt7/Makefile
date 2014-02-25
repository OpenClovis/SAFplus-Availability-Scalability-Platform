S7 := 1
include ../../mk/preface.mk

CKPT_CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) $(wildcard ../../include7/*.hxx) 
CKPT_CLIENT_SRC := $(wildcard *.cxx)
CKPT_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CKPT_CLIENT_SRC)))

all: $(LIB_DIR)/clCkpt.so

$(SAFPLUS_TARGET)/lib/clCkpt.so: $(CKPT_CLIENT_OBJ)
	$(LINK_SO) $@ $+  -lpthread -lrt

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CKPT_SERVER_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(SAFPLUS_TARGET)/lib/clLog.o $(MWOBJ_DIR)/*.o
