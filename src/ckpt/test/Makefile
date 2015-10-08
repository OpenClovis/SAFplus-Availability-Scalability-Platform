S7 := 1
SAFPLUS_CKPT_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.h) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clCkpt clGroup clName clTimer clMsg clLog clUtils clOsal clFault clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))


all: $(TEST_DIR)/testCkpt $(TEST_DIR)/testCkptSync $(TEST_DIR)/testCkptRetention

$(TEST_DIR)/%:  $(OBJ_DIR)/%.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $<  $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) -lboost_program_options

$(TEST_DIR)/try: $(CLIENT_OBJ) $(DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)
	cp $(TEST_DIR)/try .

$(OBJ_DIR)/%.o: %.cxx Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/testCkpt
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/testCkptRetention

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
