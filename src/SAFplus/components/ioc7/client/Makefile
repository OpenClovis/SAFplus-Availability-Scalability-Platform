S7 := 1
SAFPLUS_IOC_LIB := 1
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

# Specify the required libraries
SAFPLUS_LIBS := clOsal7 clLog clMgt7
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_SO_LIBS += -lboost_thread
all: $(LIB_DIR)/libclIoc7.so

$(LIB_DIR)/libclIoc7.so: $(CLIENT_OBJ_IOC) $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(CLIENT_OBJ_IOC) $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: ../../ioc/client/%.c Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: ../../ioc/common/%.c Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

gen:
	PYTHONPATH=$(MGT_SRC_DIR)/3rdparty/pyang PYANG_PLUGINPATH=$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/SAFplus/yang -f y2cpp $(SAFPLUS_SRC_DIR)/SAFplus/yang/SAFplusMsgServer.yang --y2cpp-output `pwd` --y2cpp-mgt $(MGT_SRC_DIR)

cleangen:
	(cd SAFplusMsgServer; find . -type f -not -name "*Common.hxx" -not -name "*Base.cxx" | xargs rm)

clean:
	rm -f $(LIB_DIR)/*.so $(MWOBJ_DIR)/*.o

include ../../../mk/safplus_targets.mk
