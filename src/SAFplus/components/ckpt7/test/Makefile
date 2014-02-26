S7 := 1
SAFPLUS_CKPT_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

DEP_LIBS := $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclOsal7.so

all: $(TEST_DIR)/testCkpt

$(TEST_DIR)/testCkpt:  $(OBJ_DIR)/testCkpt.o $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/try: $(CLIENT_OBJ) $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)
	cp $(TEST_DIR)/try .

$(OBJ_DIR)/%.o: %.cxx Makefile ../../../mk/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/testCkpt

cleanall:
	rm -f $(OBJ_DIR)/*.o $(MWOBJ_DIR)/*.o $(LIB_DIR)/*.so $(TEST_DIR)/testCkpt

include ../../../mk/safplus_targets.mk