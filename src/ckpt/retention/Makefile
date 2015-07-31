S7 := 1

include ../../mk/preface.mk


CPP_FLAGS += -I$(BOOST_DIR)  -I. -I.. #-I$(BOOST_DIR)/libs/filesystem/include
SRC := $(wildcard *.cxx)
OBJ := $(subst .cxx,.o,$(notdir $(SRC)))
OBJ := $(addprefix $(MWOBJ_DIR)/,$(OBJ))

# Specify the required libraries
SAFPLUS_LIBS := clMsg clLog clUtils clOsal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(BIN_DIR)/ckptretention


$(BIN_DIR)/ckptretention:  $(OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $(OBJ)  $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) -lezxml $(LINK_STD_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx	
	$(COMPILE_CPP) $@ $<


clean:
	rm -f $(MWOBJ_DIR)/*.o
	rm -f $(BIN_DIR)/ckptretention

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
