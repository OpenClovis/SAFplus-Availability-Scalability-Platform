S7 := 1
SAFPLUS_EVENT_LIB := 1
include ../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. $(SAFPLUS_MGT_INC_FLAGS)

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC := EventClient.cxx
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))


# Specify the required libraries
SAFPLUS_LIBS := clLog clUtils clOsal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))  

all: $(LIB_DIR)/libclEvent.so 

ifdef SAFPLUS_GEN_STATIC_LIBS
all: $(LIB_DIR)/libclEvent.a 

$(LIB_DIR)/libclEvent.a: $(CLIENT_OBJ) $(DEP_LIBS)
	ar -cvq $@ $+
endif

$(LIB_DIR)/libclEvent.so: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)


$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<
	
	
clean:
	rm -f $(LIB_DIR)/libclEvent.so $(LIB_DIR)/libclEvent.a 

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
