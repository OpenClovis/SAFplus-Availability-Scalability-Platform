S7 := 1
SAFPLUS_MSG_PLUGIN := 1
include ../../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../../protoc

# Management includes and libraries
CPP_FLAGS += -I$(SAFPLUS_SRC_DIR)/SAFplus/3rdparty/ezxml/stable

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

CHECK_INOTIFY := $(shell ../check_inotify.sh $(COMPILE_CPP))
ifeq ($(strip $(CHECK_INOTIFY)), )
	CPP_FLAGS += -DHAVE_INOTIFY
endif

CHECK_SCTP := $(shell ../check_sctp.sh $(COMPILE_CPP))
ifeq ($(strip $(CHECK_SCTP)), )
	CPP_FLAGS += -DHAVE_SCTP
endif

# Specify the required libraries
SAFPLUS_LIBS := 
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_SO_LIBS += -lboost_thread

Release all: $(LIB_DIR)/clMsgUdp.so

$(LIB_DIR)/clMsgUdp.so: $(OBJ_DIR)/msgUdp.o $(OBJ_DIR)/clPluginHelper.o
	$(LINK_SO) $@ $^ $(LINK_SO_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/clMsgUdp.so $(CLIENT_OBJ)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
