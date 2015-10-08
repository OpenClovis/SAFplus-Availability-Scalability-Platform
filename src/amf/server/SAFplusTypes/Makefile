S7 := 1
ifeq ($(SAFPLUS_SRC_DIR),)
$(error You must run this as a submake or define the SAFPLUS_SRC_DIR environment variable)
endif

include $(SAFPLUS_SRC_DIR)/mk/preface.mk

# Specify the required libraries
SAFPLUS_LIBS := clMgt

# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

SAFplusTypes_SRC := $(wildcard *.cxx)
SAFplusTypes_H := $(wildcard *.hxx)

SAFplusTypes_OBJ := $(subst .cxx,.o,$(notdir $(SAFplusTypes_SRC)))
SAFplusTypes_OBJ := $(addprefix $(OBJ_DIR)/SAFplusTypes/,$(SAFplusTypes_OBJ))

SAFplusTypes_LIB := libSAFplusTypes.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

Release all: $(LIB_DIR)/$(SAFplusTypes_LIB)

# Shared library providing functions that needs to provide that YANG data
$(LIB_DIR)/$(SAFplusTypes_LIB): $(SAFplusTypes_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(SAFplusTypes_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lxml2 $(LINK_SO_LIBS)

$(OBJ_DIR)/SAFplusTypes/%.o: %.cxx $(SAFplusTypes_H)
	-mkdir -p $(OBJ_DIR)/SAFplusTypes
	$(COMPILE_CPP) $@ $(SAFPLUS_MGT_INC_FLAGS) $<

clean:
	rm -rf $(OBJ_DIR)/SAFplusTypes $(LIB_DIR)/$(SAFplusTypes_LIB)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
