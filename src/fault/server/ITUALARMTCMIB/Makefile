S7 := 1
include /home/thienluong/S7/src/mk/preface.mk

# Specify the required libraries
SAFPLUS_LIBS := clMgt

# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

ITUALARMTCMIB_SRC := $(wildcard *.cxx)
ITUALARMTCMIB_H := $(wildcard *.hxx)

ITUALARMTCMIB_OBJ := $(subst .cxx,.o,$(notdir $(ITUALARMTCMIB_SRC)))
ITUALARMTCMIB_OBJ := $(addprefix $(OBJ_DIR)/ITUALARMTCMIB/,$(ITUALARMTCMIB_OBJ))

ITUALARMTCMIB_LIB := $(LIB_DIR)/ITUALARMTCMIB.so

# Management includes and libraries
CPP_FLAGS += $(SAFPLUS_MGT_INC_FLAGS)

all: $(ITUALARMTCMIB_LIB)

# Shared library providing functions that needs to provide that YANG data
$(ITUALARMTCMIB_LIB): $(ITUALARMTCMIB_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(ITUALARMTCMIB_OBJ) $(SAFPLUS_LINK_LIBS) -L$(INSTALL_DIR)/lib -lezxml -lxml2 -lboost_filesystem $(LINK_SO_LIBS)

$(OBJ_DIR)/ITUALARMTCMIB/%.o: %.cxx $(ITUALARMTCMIB_H)
	-mkdir -p $(OBJ_DIR)/ITUALARMTCMIB
	$(COMPILE_CPP) $@ $<

clean:
	rm -rf $(OBJ_DIR)/ITUALARMTCMIB

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
