S7 := 1
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

SENDER_SRC := groupClient.cxx
SENDER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(SENDER_SRC)))

RECEIVER_SRC := groupServer.cxx
RECEIVER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(RECEIVER_SRC)))

UNITTEST_SRC := testGroup.cxx
UNITTEST_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(UNITTEST_SRC)))

DEP_LIBS :=  $(LIB_DIR)/libclName.so $(LIB_DIR)/libclIoc7.so $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclLog.so

LINK_STD_LIBS += -lpthread -lstdc++ -ldl -lrt -lm -L$(SAFPLUS_TOOLCHAIN_DIR)/lib -lboost_thread -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml

all: $(TEST_DIR)/ClientTest $(TEST_DIR)/ServerTest $(TEST_DIR)/TestGroup

$(TEST_DIR)/ClientTest: $(SENDER_OBJ) $(DEP_LIBS)
	$(LINK) $+  $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/ServerTest: $(RECEIVER_OBJ) $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/TestGroup: $(UNITTEST_OBJ) $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

test: all
	cp -f $(TEST_DIR)/ClientTest .
	cp -f $(TEST_DIR)/ServerTest .
	cp -f $(TEST_DIR)/TestGroup .

clean:
	rm -f $(OBJ_DIR)/groupClient.o $(OBJ_DIR)/groupServer.o $(OBJ_DIR)/testGroup.o $(TEST_DIR)/ClientTest $(TEST_DIR)/ServerTest $(TEST_DIR)/TestGroup

include ../../../mk/safplus_targets.mk
