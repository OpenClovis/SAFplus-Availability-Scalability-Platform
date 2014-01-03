S7 := 1
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cpp)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cpp,.o,$(CLIENT_SRC)))

all: $(TEST_DIR)/testOsal

$(TEST_DIR)/testOsal: $(CLIENT_OBJ)
	$(LINK) $+ $(LINK_LIBS) $(LIB_DIR)/clOsal.so $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cpp Makefile $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

