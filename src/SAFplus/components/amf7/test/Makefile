S7 := 1
SAFPLUS_AMF_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

# TODO REMOVE -lmw requirement
SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
#SAFPLUS_LIBS := clAmf clGroup clName clCkpt clIoc7 SAFplusPBExt clLog clUtils7 clOsal7
SAFPLUS_LIBS := clAmf clRpc clName clCkpt clGroup clIoc7 clUtils7 clLog clOsal7 
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(TEST_DIR)/example

$(TEST_DIR)/example:  $(OBJ_DIR)/example.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(SAFPLUS_LINK_LIBS) -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lboost_filesystem $(LINK_SO_LIBS) -lprotobuf -lprotoc $(LINK_SO_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile ../../../mk/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/example

include ../../../mk/safplus_targets.mk