S7 := 1
SAFPLUS_NAME_LIB := 1
include ../mk/preface.mk

NAME_CLIENT_SRC := clNameApi.cxx
NAME_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(NAME_CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clCkpt clUtils clOsal clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

Release all: $(LIB_DIR)/libclName.so $(BIN_DIR)/safplus_name

$(BIN_DIR)/safplus_name:  $(OBJ_DIR)/safplus_name.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) -lclName $(LINK_STD_LIBS) $(XML2_LINK) 

$(SAFPLUS_TARGET)/lib/libclName.so: $(NAME_CLIENT_OBJ) Makefile $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(NAME_CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx #Makefile
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/%.o: %.cxx #Makefile
	$(COMPILE_CPP) $@ $<

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclName.so $(OBJ_DIR)/safplus_name.o $(NAME_CLIENT_OBJ) $(BIN_DIR)/safplus_name

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
