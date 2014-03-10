S7 := 1
include ../../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

# Management includes and libraries
CPP_FLAGS +=  -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

CLIENT_H := $(wildcard *.hxx) $(wildcard ../../include7/*.hxx) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

DEP_LIBS += $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclMgt7.so

all: $(LIB_DIR)/libclIoc7.so

$(LIB_DIR)/libclIoc7.so: $(CLIENT_OBJ) $(DEP_LIBS)
	$(LINK_SO) $@ $+ $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclIoc7.so $(MWOBJ_DIR)/*.o
