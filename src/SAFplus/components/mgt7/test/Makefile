S7 := 1
SAFPLUS_MGT_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

SAFPLUS_SDK_DIR := /opt/clovis/6.1/sdk
SAFPLUS_SDK_TARGET ?= $(shell (cd $(SAFPLUS_SDK_DIR)/prebuild/target/$(TARGET_PLATFORM)/$(CL_TARGET_OS); pwd))

CPP_FLAGS += -I$(BOOST_DIR)  -I. -I.. 
DEP_LIBS += $(LIB_DIR)/libclMgt7.so  $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclOsal7.so 

# Specify the required libraries
SAFPLUS_LIBS := clMgt7 clUtils7 clLog clOsal7
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))


# Management includes and libraries
CPP_FLAGS += -I../../mgt7/include -I$(MGT_SRC_DIR)/3rdparty/build/include/libxml2/

MGT_TEST_H := $(wildcard *.hpp) $(wildcard *.h) $(wildcard *.hxx) $(wildcard ../../../include7/*.hpp) 
MGT_TEST_SRC := $(wildcard *.cxx)
MGT_TEST_OBJ := $(subst .cpp,.o, $(subst .cxx,.o,$(MGT_TEST_SRC)))
MGT_TEST_OBJ := $(addprefix $(OBJ_DIR)/,$(MGT_TEST_OBJ))


all: $(TEST_DIR)/testmgt

$(TEST_DIR)/testmgt: $(MGT_TEST_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(MGT_TEST_OBJ) $(SAFPLUS_LINK_LIBS) -L$(SAFPLUS_SDK_TARGET)/lib -lmw -lezxml -L$(MGT_SRC_DIR)/3rdparty/build/lib -lxml2 $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(MGT_TEST_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(OBJ_DIR)/*.o $(TEST_DIR)/testmgt

include ../../../mk/safplus_targets.mk
