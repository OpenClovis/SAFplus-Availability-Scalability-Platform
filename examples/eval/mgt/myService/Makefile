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

myService_SRC := $(wildcard *.cxx)
myService_H := $(wildcard *.hxx)

myService_OBJ := $(subst .cxx,.o,$(notdir $(myService_SRC)))
myService_OBJ := $(addprefix $(OBJ_DIR)/myService/,$(myService_OBJ))

myService_LIB := libmyService.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

Release all: $(LIB_DIR)/$(myService_LIB)

# Shared library providing functions that needs to provide that YANG data
$(LIB_DIR)/$(myService_LIB): $(myService_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(myService_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lxml2 $(LINK_SO_LIBS)

$(OBJ_DIR)/myService/%.o: %.cxx $(myService_H)
	-mkdir -p $(OBJ_DIR)/myService
	$(COMPILE_CPP) $@ $<

clean:
	rm -rf $(OBJ_DIR)/myService $(LIB_DIR)/$(myService_LIB)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
