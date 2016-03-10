S7 := 1
SAFPLUS_GROUP_TEST := 1
include ../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

UNITTEST_SRC := testTimer.cxx
UNITTEST_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(UNITTEST_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clTimer clUtils clLog  clOsal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS := -L$(SAFPLUS_SDK_TARGET)/lib -L$(INSTALL_DIR)/lib -lxml2 -lstdc++ -lm $(LINK_STD_LIBS) 

all: $(TEST_DIR)/testTimer

$(TEST_DIR)/testTimer: $(OBJ_DIR)/testTimer.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

test: all
	cp -f $(TEST_DIR)/testTimer .

clean:
	rm -f $(OBJ_DIR)/testTimer.o 

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
