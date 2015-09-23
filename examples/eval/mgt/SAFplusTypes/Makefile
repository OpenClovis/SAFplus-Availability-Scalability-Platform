S7 := 1
include /code/git/SAFplus7/src/mk/preface.mk

# Specify the required libraries
SAFPLUS_LIBS := clMgt

# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

SAFplusTypes_SRC := $(wildcard *.cxx)
SAFplusTypes_H := $(wildcard *.hxx)

SAFplusTypes_OBJ := $(subst .cxx,.o,$(notdir $(SAFplusTypes_SRC)))
SAFplusTypes_OBJ := $(addprefix $(OBJ_DIR)/SAFplusTypes/,$(SAFplusTypes_OBJ))

SAFplusTypes_LIB := $(LIB_DIR)/SAFplusTypes.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

all: $(SAFplusTypes_LIB)

# Shared library providing functions that needs to provide that YANG data
$(SAFplusTypes_LIB): $(SAFplusTypes_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(SAFplusTypes_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lezxml -lxml2 -lboost_filesystem $(LINK_SO_LIBS)

$(OBJ_DIR)/SAFplusTypes/%.o: %.cxx $(SAFplusTypes_H)
	-mkdir -p $(OBJ_DIR)/SAFplusTypes
	$(COMPILE_CPP) $@ $<

clean:
	rm -rf $(OBJ_DIR)/SAFplusTypes

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
