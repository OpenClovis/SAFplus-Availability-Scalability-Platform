S7 := 1
SAFPLUS_ALARM_SERVER := 1
ifeq ($(SAFPLUS_SRC_DIR),)
$(error You must run this as a submake or define the SAFPLUS_SRC_DIR environment variable)
endif

include $(SAFPLUS_SRC_DIR)/mk/preface.mk

# Specify the required libraries
SAFPLUS_LIBS := clMgt

# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

SAFplusAlarm_SRC := $(wildcard *.cxx)
SAFplusAlarm_H := $(wildcard *.hxx)

SAFplusAlarm_OBJ := $(subst .cxx,.o,$(notdir $(SAFplusAlarm_SRC)))
SAFplusAlarm_OBJ := $(addprefix $(OBJ_DIR)/SAFplusAlarm/,$(SAFplusAlarm_OBJ))

SAFplusAlarm_LIB := libSAFplusAlarm.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS) -I../

Release all: $(LIB_DIR)/$(SAFplusAlarm_LIB)

# Shared library providing functions that needs to provide that YANG data
$(LIB_DIR)/$(SAFplusAlarm_LIB): $(SAFplusAlarm_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(SAFplusAlarm_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lxml2 $(LINK_SO_LIBS)

$(OBJ_DIR)/SAFplusAlarm/%.o: %.cxx $(SAFplusAlarm_H)
	-mkdir -p $(OBJ_DIR)/SAFplusAlarm
	$(COMPILE_CPP) $@ $(SAFPLUS_MGT_INC_FLAGS) $<

clean:
	rm -rf $(OBJ_DIR)/SAFplusAlarm $(LIB_DIR)/$(SAFplusAlarm_LIB)

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
