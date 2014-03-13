S7 := 1
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

# Management includes and libraries
CPP_FLAGS +=  -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

SENDER_SRC := ClientTest.cxx
SENDER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(SENDER_SRC)))

RECEIVER_SRC := MsgHandlerProtocols.cxx ServerTest.cxx
RECEIVER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(RECEIVER_SRC)))

DEP_LIBS := $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclIoc7.so

LINK_STD_LIBS +=-L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 -lpthread -lstdc++ -ldl -lrt -lm  -L$(BOOST_DIR)/stage/lib/ -lboost_system -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml

all: $(TEST_DIR)/ClientTest $(TEST_DIR)/ServerTest

$(TEST_DIR)/ClientTest: $(SENDER_OBJ) $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS) 

$(TEST_DIR)/ServerTest: $(RECEIVER_OBJ) $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclIoc7.so $(OBJ_DIR)/*.o  $(TEST_DIR)/*

include ../../../mk/safplus_targets.mk
