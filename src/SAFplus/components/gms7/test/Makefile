S7 := 1
SAFPLUS_GROUP_TEST := 1
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

SERVER_SRC := testGroupServer.cxx
SERVER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(SERVER_SRC)))

UNITTEST_SRC := testGroup.cxx
UNITTEST_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(UNITTEST_SRC)))

DEP_LIBS :=  $(LIB_DIR)/libclGroupServer.so $(LIB_DIR)/libclName.so $(LIB_DIR)/libclIoc7.so $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclLog.so

LINK_STD_LIBS += -lpthread -lstdc++ -ldl -lrt -lm -L$(SAFPLUS_TOOLCHAIN_DIR)/lib -lboost_thread -L$(SAFPLUS_SDK_TARGET)/lib

all: $(TEST_DIR)/testGroupServer $(TEST_DIR)/testGroup

$(TEST_DIR)/testGroupServer: $(OBJ_DIR)/testGroupServer.o $(DEP_LIBS)
	$(LINK_EXE) $@ $+  $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)  -lClOsal -lClAmfClient -lClUtils -lClTimer -lClBuffer

$(TEST_DIR)/testGroup: $(OBJ_DIR)/testGroup.o $(DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS) 

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

test: all
	cp -f $(TEST_DIR)/testGroupServer .
	cp -f $(TEST_DIR)/testGroup .

clean:
	rm -f $(OBJ_DIR)/testGroup.o $(OBJ_DIR)/testGroupServer.o

include ../../../mk/safplus_targets.mk
