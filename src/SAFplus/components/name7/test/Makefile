S7 := 1
SAFPLUS_NAME_TEST:=1  # Identify what is being built, so safplus_targets does not override
include ../../../mk/preface.mk

CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clName clCkpt clUtils7 clOsal7 clLog
# Then use these in the make rule
SAFPLUS_DEP_LIBS  := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(TEST_DIR)/testName

$(TEST_DIR)/testName:  $(OBJ_DIR)/testName.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(OBJ_DIR)/%.o: %.cxx Makefile ../../../mk/preface.mk $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

clean:
	rm -f $(CLIENT_OBJ) $(TEST_DIR)/testName

include ../../../mk/safplus_targets.mk
