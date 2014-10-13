S7 := 1
SAFPLUS_NAME_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))
MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

#SAFPLUS_LIBS := clName clCkpt clLog clUtils7 clOsal7 clIoc7
#SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
#SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

SAFPLUS_LIBS := clName clCkpt clGroup clIoc7 clUtils7 clLog clOsal7
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS)) -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2

all: $(TEST_DIR)/testName $(TEST_DIR)/dumpName

$(TEST_DIR)/testName:  $(OBJ_DIR)/testName.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/dumpName:  $(OBJ_DIR)/dumpName.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile ../../../mk/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/testName

include ../../../mk/safplus_targets.mk
