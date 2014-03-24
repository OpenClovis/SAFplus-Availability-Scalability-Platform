S7 := 1
include ../../mk/preface.mk

GROUP_CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) $(wildcard ../../include7/*.hxx)
GROUP_CLIENT_SRC := $(wildcard *.cxx)
GROUP_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(GROUP_CLIENT_SRC)))

all: $(LIB_DIR)/libclGroup.so

$(SAFPLUS_TARGET)/lib/libclGroup.so: $(GROUP_CLIENT_OBJ)
	$(LINK_SO) $@ $+  -lpthread -lrt

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(GROUP_CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclGroup.so $(MWOBJ_DIR)/*.o
