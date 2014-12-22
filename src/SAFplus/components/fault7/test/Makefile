S7 := 1
SAFPLUS_FAULT_TEST := 1
include ../../../mk/preface.mk

CPP_FLAGS += -I/usr/include/libxml2

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

UNITTEST_SRC := safplus_fault.cxx
UNITTEST_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(UNITTEST_SRC)))

CLIENT_SRC := fault_client.cxx
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

SERVER_SRC := fault_server.cxx
SERVER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(SERVER_SRC)))


# Specify the required libraries
SAFPLUS_LIBS := clFault clCkpt clName clIoc7 clLog clUtils7 clOsal7 clGroup clMgt7
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS := -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 -lstdc++ -lm $(LINK_STD_LIBS) -lboost_filesystem -lboost_system
#-L$(SAFPLUS_TOOLCHAIN_DIR)/lib -L$(SAFPLUS_SDK_TARGET)/lib

all: $(TEST_DIR)/safplus_fault $(TEST_DIR)/fault_client $(TEST_DIR)/fault_server

$(TEST_DIR)/safplus_fault: $(OBJ_DIR)/safplus_fault.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(TEST_DIR)/fault_client: $(OBJ_DIR)/fault_client.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(TEST_DIR)/fault_server: $(OBJ_DIR)/fault_server.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

test: all
	cp -f $(TEST_DIR)/safplus_fault .

clean:
	rm -f $(OBJ_DIR)/safplus_fault.o  $(OBJ_DIR)/fault_client.o $(OBJ_DIR)/fault_server.o

include ../../../mk/safplus_targets.mk
