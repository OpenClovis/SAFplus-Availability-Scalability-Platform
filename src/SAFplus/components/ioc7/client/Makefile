S7 := 1
include ../../../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

# Management includes and libraries
MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt
CPP_FLAGS += -I$(SAFPLUS_SRC_DIR)/SAFplus/3rdparty/ezxml/stable -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

CLIENT_H := $(wildcard *.hxx) $(wildcard ../../include7/*.hxx) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

CLIENT_SRC_IOC := $(notdir $(wildcard ../../ioc/client/*.c) $(wildcard ../../ioc/common/*.c))
CLIENT_OBJ_IOC := $(addprefix $(MWOBJ_DIR)/,$(subst .c,.o,$(CLIENT_SRC_IOC)))

DEP_LIBS += $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclMgt7.so

all: $(LIB_DIR)/libclIoc7.so

$(LIB_DIR)/libclIoc7.so: $(CLIENT_OBJ_IOC) $(CLIENT_OBJ) $(DEP_LIBS)
	$(LINK_SO) $@ $+ $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: ../../ioc/client/%.c Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: ../../ioc/common/%.c Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/*.so $(MWOBJ_DIR)/*.o

include ../../../mk/safplus_targets.mk
