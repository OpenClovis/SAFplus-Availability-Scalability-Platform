S7 := 1
include ../../../mk/preface.mk

# Specify the required libraries
SAFPLUS_LIBS := clMgt

# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

unitTest_SRC := $(wildcard *.cxx)
unitTest_H := $(wildcard *.cxx)

unitTest_OBJ := $(subst .cxx,.o,$(notdir $(unitTest_SRC)))
unitTest_OBJ := $(addprefix $(OBJ_DIR)/unitTest/,$(unitTest_OBJ))

unitTest_LIB := $(LIB_DIR)/unitTest.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

all: $(unitTest_LIB)

# Shared library providing functions that needs to provide that YANG data
$(unitTest_LIB): $(unitTest_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(unitTest_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lezxml -lxml2 -lboost_filesystem $(LINK_SO_LIBS)

$(OBJ_DIR)/unitTest/%.o: %.cxx $(unitTest_H)
	-mkdir -p $(OBJ_DIR)/unitTest
	$(COMPILE_CPP) $@ $<

clean:
	rm -rf $(OBJ_DIR)/unitTest

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
