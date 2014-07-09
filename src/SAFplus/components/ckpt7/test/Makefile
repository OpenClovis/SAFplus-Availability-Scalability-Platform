S7 := 1
SAFPLUS_CKPT_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

# TODO REMOVE -lmw requirement
SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clCkpt clGroup clName clIoc7 clLog clUtils7 clOsal7
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))


all: $(TEST_DIR)/testCkpt $(TEST_DIR)/testCkptSync

$(TEST_DIR)/%:  $(OBJ_DIR)/%.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml $(LINK_STD_LIBS)

$(TEST_DIR)/try: $(CLIENT_OBJ) $(DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(DEP_LIBS) -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml $(LINK_STD_LIBS)
	cp $(TEST_DIR)/try .

$(OBJ_DIR)/%.o: %.cxx Makefile ../../../mk/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/testCkpt

include ../../../mk/safplus_targets.mk