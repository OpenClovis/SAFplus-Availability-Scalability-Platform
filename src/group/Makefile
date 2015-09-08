S7 := 1
SAFPLUS_GROUP_LIB := 1
include ../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clCkpt clName clMsg clUtils clLog clOsal clFault clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))


Release all: $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclGroup.a $(BIN_DIR)/spgroup $(BIN_DIR)/safplus_group 

$(LIB_DIR)/libclGroup.so: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(LIB_DIR)/libclGroup.a: $(CLIENT_OBJ_IOC) $(CLIENT_OBJ) $(DEP_LIBS)
	ar -cvq $@ $+

$(OBJ_DIR)/%.o: bin/%.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(BIN_DIR)/spgroup: $(OBJ_DIR)/spgroup.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@  $< -lclGroup $(SAFPLUS_LINK_LIBS) -lboost_program_options $(LINK_SO_LIBS)

$(BIN_DIR)/safplus_group: $(OBJ_DIR)/safplus_group.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@  $< -lclGroup $(SAFPLUS_LINK_LIBS) -lboost_program_options $(LINK_SO_LIBS)

clean:
	rm -f $(LIB_DIR)/libclGroup.so $(MWOBJ_DIR)/group.o $(MWOBJ_DIR)/groupshm.o $(BIN_DIR)/spgroup $(BIN_DIR)/safplus_group 

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
