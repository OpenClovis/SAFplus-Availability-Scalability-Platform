S7 := 1
include ../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard ../../include7/*.hxx) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

all: $(LIB_DIR)/libclOsal7.so

$(LIB_DIR)/libclOsal7.so: $(CLIENT_OBJ)
	$(LINK_SO) $@ $+ $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclOsal7.so $(MWOBJ_DIR)/*.o
