S7 := 1
SAFPLUS_LOG_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cpp)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cpp,.o,$(CLIENT_SRC)))

DEP_LIBS := $(LIB_DIR)/clLog.so $(LIB_DIR)/clUtils7.so $(LIB_DIR)/clOsal7.so

all: $(TEST_DIR)/testLog

$(TEST_DIR)/testLog: $(CLIENT_OBJ) $(DEP_LIBS)
	$(LINK) $+ $(LINK_LIBS) $(DEP_LIBS) -L$(BOOST_DIR)/stage/lib/ -lboost_system

$(OBJ_DIR)/%.o: %.cpp Makefile ../../../mk/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(OBJ_DIR)/*.o $(TEST_DIR)/testLog

include ../../../mk/safplus_targets.mk