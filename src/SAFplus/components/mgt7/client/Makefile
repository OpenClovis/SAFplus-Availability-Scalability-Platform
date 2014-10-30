S7 := 1
include ../../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

# Management includes and libraries
CPP_FLAGS +=  -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/
C_FLAGS += -I$(SAFPLUS_SRC_DIR)/SAFplus/include -I$(SAFPLUS_INC_DIR) -I$(SAFPLUS_SRC_DIR)/SAFplus/3rdparty/ezxml/stable

CLIENT_H := $(wildcard ../include/*.hxx)
CLIENT_SRC := $(wildcard *.cxx) $(wildcard *.cc)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cc,.o, $(subst .cxx,.o,$(CLIENT_SRC))))

Release all: $(LIB_DIR)/libclMgt7.so

$(LIB_DIR)/libclMgt7.so: $(CLIENT_OBJ)
	$(LINK_SO) $@ $+ $(LINK_SO_LIBS) `pkg-config --libs protobuf`

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: %.cc Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclMgt7.so $(MWOBJ_DIR)/*.o
