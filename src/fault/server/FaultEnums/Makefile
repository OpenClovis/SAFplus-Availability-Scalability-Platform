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

FaultEnums_SRC := $(wildcard *.cxx)
FaultEnums_H := $(wildcard *.hxx)

FaultEnums_OBJ := $(subst .cxx,.o,$(notdir $(FaultEnums_SRC)))
FaultEnums_OBJ := $(addprefix $(OBJ_DIR)/FaultEnums/,$(FaultEnums_OBJ))

FaultEnums_LIB := libFaultEnums.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

Release all: $(LIB_DIR)/$(FaultEnums_LIB)

# Shared library providing functions that needs to provide that YANG data
$(LIB_DIR)/$(FaultEnums_LIB): $(FaultEnums_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(FaultEnums_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lxml2 $(LINK_SO_LIBS)

$(OBJ_DIR)/FaultEnums/%.o: %.cxx $(FaultEnums_H)
	-mkdir -p $(OBJ_DIR)/FaultEnums
	$(COMPILE_CPP) $@ $(SAFPLUS_MGT_INC_FLAGS) $<

clean:
	rm -rf $(OBJ_DIR)/FaultEnums $(LIB_DIR)/$(FaultEnums_LIB)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
