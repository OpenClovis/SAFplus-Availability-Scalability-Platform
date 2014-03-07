S7 := 1
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

SENDER_SRC := sender.cxx
SENDER_OBJ := $(OBJ_DIR)/sender.o

RECEIVER_SRC := receiver.cxx
RECEIVER_OBJ := $(OBJ_DIR)/receiver.o

DEP_LIBS := $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclIoc7.so 

LINK_STD_LIBS += -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml -lpthread -lstdc++ -ldl -lrt -lm  -L$(BOOST_DIR)/stage/lib/ -lboost_system

all: $(TEST_DIR)/sender $(TEST_DIR)/receiver

$(TEST_DIR)/sender: $(SENDER_OBJ) $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS) 

$(TEST_DIR)/receiver: $(RECEIVER_OBJ) $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclIoc7.so $(OBJ_DIR)/*.o

include ../../../mk/safplus_targets.mk
