S7 := 1
SAFPLUS_GROUP_TEST := 1
include ../../mk/preface.mk

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(BOOST_DIR) -I. -I.. -I../include

SERVER_SRC := testGroupServer.cxx
SERVER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(SERVER_SRC)))

UNITTEST_SRC := testGroup.cxx
UNITTEST_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(UNITTEST_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clName clMgt clMsg clGroup clCkpt clUtils clOsal clLog clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS := -L$(INSTALL_DIR)/lib -lezxml -lxml2 -lstdc++ -lm $(LINK_STD_LIBS)

# $(TEST_DIR)/testGroupServer $(TEST_DIR)/testGroup $(TEST_DIR)/testMultiGroup
all:  $(TEST_DIR)/testGroup $(TEST_DIR)/dumpGroups $(TEST_DIR)/safplus_group


$(TEST_DIR)/safplus_group: $(OBJ_DIR)/safplus_group.o $(SAFPLUS_DEP_LIBS) Makefile
	$(LINK_EXE) $@ $<  $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/testGroup: $(OBJ_DIR)/testGroup.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $< $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(TEST_DIR)/dumpGroups: $(OBJ_DIR)/dumpGroups.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $< $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(TEST_DIR)/testMultiGroup: $(OBJ_DIR)/testMultiGroup.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $< $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

test: all
	cp -f $(TEST_DIR)/testGroupServer .
	cp -f $(TEST_DIR)/testGroup .

clean:
	rm -f $(OBJ_DIR)/testGroup.o $(OBJ_DIR)/testGroupServer.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
