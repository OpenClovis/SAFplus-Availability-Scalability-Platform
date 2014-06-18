S7 := 1
SAFPLUS_IOC_TEST := 1
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx) 

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

# Management includes and libraries
CPP_FLAGS +=  -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

SENDER_SRC := ClientTest.cxx
SENDER_OBJ := $(addprefix $(OBJ_DIR)/, $(subst .cxx,.o,$(subst .cc,.o,$(SENDER_SRC))))

RECEIVER_SRC := MsgHandlerProtocols.cxx ServerTest.cxx
RECEIVER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(subst .cc,.o,$(RECEIVER_SRC))))

# Specify the required libraries
SAFPLUS_LIBS := clOsal7 clUtils7 clLog clIoc7 clMgt7
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS +=-L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 -lpthread -lstdc++ -ldl -lrt -lm  -L$(BOOST_DIR)/stage/lib/ -lboost_system -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml

all: $(TEST_DIR)/TestSendMsg $(TEST_DIR)/TestReceiveMsg

$(TEST_DIR)/TestSendMsg: $(SENDER_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(SENDER_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(TEST_DIR)/TestReceiveMsg: $(RECEIVER_OBJ) $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $(RECEIVER_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)


$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -rf $(OBJ_DIR)/*.o $(TEST_DIR)/* $(OBJ_DIR)/Client/* $(OBJ_DIR)/Server/*
	

include ../../../mk/safplus_targets.mk
