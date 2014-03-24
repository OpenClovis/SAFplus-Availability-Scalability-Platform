S7 := 1

include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp)
CLIENT_SRC := testGroup.cxx
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

DEP_LIBS := $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclLog.so

all: $(TEST_DIR)/testGroup

$(TEST_DIR)/testGroup:  $(OBJ_DIR)/testGroup.o $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/try: $(CLIENT_OBJ) $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) $(LINK_STD_LIBS)
	cp $(TEST_DIR)/try .

$(OBJ_DIR)/%.o: %.cxx Makefile ../../../mk/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/testGroup

cleanall:
	rm -f $(OBJ_DIR)/*.o $(MWOBJ_DIR)/*.o $(LIB_DIR)/*.so $(TEST_DIR)/testGroup

include ../../../mk/safplus_targets.mk
