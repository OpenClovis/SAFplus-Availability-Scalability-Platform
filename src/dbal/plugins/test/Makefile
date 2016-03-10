S7 := 1

include ../../../mk/preface.mk


CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clLog clUtils clOsal clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))


all: $(TEST_DIR)/testDBPlugin


ifdef DIRECTLY_LINK
$(TEST_DIR)/%:  $(OBJ_DIR)/%.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS) $(DIRECTLY_LINK) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP)  $@ -DDIRECTLY_LINKED $<
else
$(TEST_DIR)/%:  $(OBJ_DIR)/%.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS)  $(SAFPLUS_LINK_LIBS) -lboost_program_options $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<
endif


.PRECIOUS: $(OBJ_DIR)/%.o

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/testDBPlugin

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
