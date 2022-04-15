S7 := 1

include ../../../mk/preface.mk

# always return empty string: TMP2 := $(strip $(wildcard $(THIS_DIR)/../../../mk/preface.mk))
CLIENT_H := $(wildcard *.hpp) $(wildcard $(SAFPLUS_INC_DIR)/*.hpp) 
CLIENT_SRC := $(wildcard *.cxx)
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/test/amf/,$(subst .cxx,.o,$(CLIENT_SRC)))

# Specify the required libraries
SAFPLUS_LIBS := clName clCkpt clGroup clMsg clLog clUtils clTimer clOsal clFault clDbal clAmfMgmt clRpc clMsg clFault clName clAmf
# Then use these in the make rule
ifeq ($(BUILD_SAFPLUS),1)  # If I do not want to build SAFplus then don't put the libs in the dependency line
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
else
SAFPLUS_DEP_LIBS := 
endif

SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

all: $(TEST_DIR)/testMgmtApi


#$(TEST_DIR)/testMgmtApi: $(OBJ_DIR)/test/amf/testMgmtApi.o $(SAFPLUS_DEP_LIBS)
#	$(LINK_EXE) $@ $<  $(SAFPLUS_LINK_LIBS) -lboost_filesystem $(LINK_SO_LIBS) -lprotobuf -lprotoc

$(TEST_DIR)/%: $(CLIENT_OBJ) $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $< $(SAFPLUS_LINK_LIBS) -L$(SAFPLUS_TARGET)/lib -L$(INSTALL_DIR)/lib $(XML2_LINK) $(LINK_STD_LIBS)

$(OBJ_DIR)/test/amf/%.o: %.cxx #Makefile $(SAFPLUS_MAKE_DIR)/preface.mk $(CLIENT_H)
	-mkdir -p $(OBJ_DIR)/test/amf
	$(COMPILE_CPP) $@ $<

.PHONY: clean

clean:
	rm -rf $(OBJ_DIR)/test/amf/testMgmtApi.o $(TEST_DIR)/testMgmtApi

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
