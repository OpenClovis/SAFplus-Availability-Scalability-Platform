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

SAFplusLog_SRC := $(wildcard *.cxx)
SAFplusLog_H := $(wildcard *.hxx)

SAFplusLog_OBJ := $(subst .cxx,.o,$(notdir $(SAFplusLog_SRC)))
SAFplusLog_OBJ := $(addprefix $(OBJ_DIR)/SAFplusLog/,$(SAFplusLog_OBJ))

SAFplusLog_LIB := libSAFplusLog.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

Release all: $(LIB_DIR)/$(SAFplusLog_LIB)

# Shared library providing functions that needs to provide that YANG data
$(LIB_DIR)/$(SAFplusLog_LIB): $(SAFplusLog_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(SAFplusLog_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lxml2 $(LINK_SO_LIBS)

$(OBJ_DIR)/SAFplusLog/%.o: %.cxx $(SAFplusLog_H)
	-mkdir -p $(OBJ_DIR)/SAFplusLog
	$(COMPILE_CPP) $@ $(SAFPLUS_MGT_INC_FLAGS) $<

clean:
	rm -rf $(OBJ_DIR)/SAFplusLog $(LIB_DIR)/$(SAFplusLog_LIB)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
