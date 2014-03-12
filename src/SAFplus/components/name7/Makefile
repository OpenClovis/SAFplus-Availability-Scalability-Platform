S7 := 1
include ../../mk/preface.mk

NAME_CLIENT_SRC := $(wildcard *.cxx)
NAME_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(NAME_CLIENT_SRC)))

all: $(LIB_DIR)/libclName.so

$(SAFPLUS_TARGET)/lib/libclName.so: $(NAME_CLIENT_OBJ)
	$(LINK_SO) $@ $+  -lpthread -lrt

$(MWOBJ_DIR)/%.o: %.cxx #Makefile
	$(COMPILE_CPP) $@ $<

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclName.so $(MWOBJ_DIR)/*.o
