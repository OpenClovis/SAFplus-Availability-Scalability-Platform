S7 := 1
SAFPLUS_NAME_LIB := 1
include ../mk/preface.mk

NAME_CLIENT_SRC := $(wildcard *.cxx)
NAME_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(NAME_CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clCkpt clUtils clOsal clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

Release all: $(LIB_DIR)/libclName.so

$(SAFPLUS_TARGET)/lib/libclName.so: $(NAME_CLIENT_OBJ) Makefile $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(NAME_CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx #Makefile
	$(COMPILE_CPP) $@ $<

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclName.so $(MWOBJ_DIR)/*.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
