S7 := 1
SAFPLUS_MSG_LIB := 1
include ../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. -I../../protoc

# Management includes and libraries
CPP_FLAGS += -I$(SAFPLUS_SRC_DIR)/SAFplus/3rdparty/ezxml/stable

CLIENT_H := $(wildcard *.hxx) $(wildcard ../../include7/*.hxx) $(wildcard ../../ioc/client/*.h) $(wildcard ../../ioc/common/*.h)
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

CHECK_INOTIFY := $(shell ./check_inotify.sh $(COMPILE_CPP))
ifeq ($(strip $(CHECK_INOTIFY)), )
	CPP_FLAGS += -DHAVE_INOTIFY
endif

CHECK_SCTP := $(shell ./check_sctp.sh $(COMPILE_CPP))
ifeq ($(strip $(CHECK_SCTP)), )
	CPP_FLAGS += -DHAVE_SCTP
endif

# Specify the required libraries
SAFPLUS_LIBS := clUtils clOsal clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

Release all: $(LIB_DIR)/libclMsg.so

$(LIB_DIR)/libclMsg.so: $(CLIENT_OBJ) $(CLIENT_IOC_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(CLIENT_OBJ) $(CLIENT_IOC_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/*.so $(MWOBJ_DIR)/*.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk