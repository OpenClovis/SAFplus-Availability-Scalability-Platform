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

SAFplusAmf_SRC := $(wildcard *.cxx)
SAFplusAmf_H := $(wildcard *.hxx)

SAFplusAmf_OBJ := $(subst .cxx,.o,$(notdir $(SAFplusAmf_SRC)))
SAFplusAmf_OBJ := $(addprefix $(OBJ_DIR)/SAFplusAmf/,$(SAFplusAmf_OBJ))

SAFplusAmf_LIB := libSAFplusAmf.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

Release all: $(LIB_DIR)/$(SAFplusAmf_LIB)

# Shared library providing functions that needs to provide that YANG data
$(LIB_DIR)/$(SAFplusAmf_LIB): $(SAFplusAmf_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(SAFplusAmf_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(XML2_LINK)

$(OBJ_DIR)/SAFplusAmf/%.o: %.cxx $(SAFplusAmf_H)
	-mkdir -p $(OBJ_DIR)/SAFplusAmf
	$(COMPILE_CPP) $@ $(SAFPLUS_MGT_INC_FLAGS) $<

clean:
	rm -rf $(OBJ_DIR)/SAFplusAmf $(LIB_DIR)/$(SAFplusAmf_LIB)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
