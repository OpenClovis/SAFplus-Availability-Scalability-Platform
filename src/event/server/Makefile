S7 := 1
SAFPLUS_EVENT_SERVER := 1
include ../../mk/preface.mk

CPP_FLAGS += -I$(BOOST_DIR) 

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)
CLIENT_SRC := EventCkpt.cxx EventServer.cxx 
CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clCkpt clName clMsg clLog clUtils clOsal clGroup clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))  

all:$(LIB_DIR)/libclEventServer.so

ifdef SAFPLUS_GEN_STATIC_LIBS
all:  $(LIB_DIR)/libclEventServer.a 

$(LIB_DIR)/libclEventServer.a: $(CLIENT_OBJ) $(DEP_LIBS)
	ar -cvq $@ $+
endif

$(LIB_DIR)/libclEventServer.so: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@  $(CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

$(OBJ_DIR)/%.o: %.cxx Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(LIB_DIR)/libclEventServer.so 

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
