S7 := 1
SAFPLUS_CKPT_LIB:=1  # Identify what is being built, so safplus_targets does not override
include ../mk/preface.mk

CKPT_CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CKPT_CLIENT_SRC := $(wildcard *.cxx)
CKPT_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CKPT_CLIENT_SRC)))

TEMP := $(wildcard ../group/*.cxx)
GROUP_CLIENT_SRC := $(subst ../group/,,$(TEMP))
GROUP_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(GROUP_CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clDbal #clMsg clGroup
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

Release all: $(LIB_DIR)/libclCkpt.so

$(LIB_DIR)/libclCkpt.so: $(CKPT_CLIENT_OBJ) $(SAFPLUS_DEP_LIBS) $(GROUP_CLIENT_OBJ)
	$(LINK_SO) $@ $(CKPT_CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS) $(GROUP_CLIENT_OBJ)

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CKPT_CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(GROUP_CLIENT_OBJ):
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/group

clean:
	rm -f  $(SAFPLUS_TARGET)/lib/libclCkpt.so $(MWOBJ_DIR)/*.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
