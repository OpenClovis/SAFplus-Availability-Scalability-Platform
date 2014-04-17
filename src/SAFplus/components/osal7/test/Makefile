S7 := 1
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cpp)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cpp,.o,$(CLIENT_SRC)))

DEP_LIBS += $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so

all: $(TEST_DIR)/testOsal

$(TEST_DIR)/testOsal: $(CLIENT_OBJ) $(DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cpp Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<


include ../../../mk/safplus_targets.mk