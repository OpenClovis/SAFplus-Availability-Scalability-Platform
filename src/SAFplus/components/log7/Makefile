S7 := 1
SAFPLUS_LOG_LIB:=1
include ../../mk/preface.mk

LOG_CLIENT_H := $(wildcard *.hpp) $(wildcard ../../include7/*.hpp) $(wildcard ../../include7/*.hxx)
LOG_CLIENT_SRC := $(wildcard *.cxx)
LOG_CLIENT_OBJ := $(subst .cxx,.o,$(LOG_CLIENT_SRC))
LOG_CLIENT_OBJ := $(addprefix $(MWOBJ_DIR)/,$(subst .cxx,.o,$(LOG_CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clOsal7 clUtils7
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))


Release all: $(LIB_DIR)/libclLog.so

$(SAFPLUS_TARGET)/lib/libclLog.so: $(LOG_CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_SO) $@ $(LOG_CLIENT_OBJ) $(SAFPLUS_LINK_LIBS) $(LINK_SO_LIBS)

$(MWOBJ_DIR)/%.o: %.cxx Makefile $(LOG_SERVER_H)
	$(COMPILE_CPP)  $@ $<

clean:
	rm -f $(SAFPLUS_TARGET)/lib/clLog.o $(MWOBJ_DIR)/*.o

include $(MAKE_DIR)/safplus_targets.mk