S7 := 1
include /home/thienluong/S7/src/mk/preface.mk

# Specify the required libraries
SAFPLUS_LIBS := clMgt

# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

IANAITUALARMTCMIB_SRC := $(wildcard *.cxx)
IANAITUALARMTCMIB_H := $(wildcard *.hxx)

IANAITUALARMTCMIB_OBJ := $(subst .cxx,.o,$(notdir $(IANAITUALARMTCMIB_SRC)))
IANAITUALARMTCMIB_OBJ := $(addprefix $(OBJ_DIR)/IANAITUALARMTCMIB/,$(IANAITUALARMTCMIB_OBJ))

IANAITUALARMTCMIB_LIB := $(LIB_DIR)/IANAITUALARMTCMIB.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

all: $(IANAITUALARMTCMIB_LIB)

# Shared library providing functions that needs to provide that YANG data
$(IANAITUALARMTCMIB_LIB): $(IANAITUALARMTCMIB_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(IANAITUALARMTCMIB_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lezxml -lxml2 -lboost_filesystem $(LINK_SO_LIBS)

$(OBJ_DIR)/IANAITUALARMTCMIB/%.o: %.cxx $(IANAITUALARMTCMIB_H)
	-mkdir -p $(OBJ_DIR)/IANAITUALARMTCMIB
	$(COMPILE_CPP) $@ $<

clean:
	rm -rf $(OBJ_DIR)/IANAITUALARMTCMIB

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
