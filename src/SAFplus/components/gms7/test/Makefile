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

# Specify the required libraries
SAFPLUS_LIBS := clGroupServer clName clMgt7 clIoc7 clGroup clUtils7 clOsal7 clCkpt clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS := -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml -lxml2 -L$(MGT_SRC_DIR)/3rdparty/build/lib -lstdc++ -lm $(LINK_STD_LIBS)
#-L$(SAFPLUS_TOOLCHAIN_DIR)/lib -L$(SAFPLUS_SDK_TARGET)/lib

all: $(TEST_DIR)/testGroupServer $(TEST_DIR)/testGroup

$(TEST_DIR)/testGroupServer: $(OBJ_DIR)/testGroupServer.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+  $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/testGroup: $(OBJ_DIR)/testGroup.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

test: all
	cp -f $(TEST_DIR)/testGroupServer .
	cp -f $(TEST_DIR)/testGroup .

clean:
	rm -f $(OBJ_DIR)/testGroup.o $(OBJ_DIR)/testGroupServer.o

include ../../../mk/safplus_targets.mk
