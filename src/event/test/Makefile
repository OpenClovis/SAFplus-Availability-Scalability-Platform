S7 := 1
SAFPLUS_EVENT_TEST := 1
include ../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(SAFPLUS_MGT_INC_FLAGS) -I. -I../../alarm/server/SAFplusAlarm

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)


CLIENT_SRC := event_client_local.cxx event_client_global.cxx
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))


# Specify the required libraries
SAFPLUS_LIBS := clRpc clEvent clEventServer clFaultServer clTimer clCkpt clFault  clName clMsg  clGroup clMgt clLog clUtils clOsal clDbal
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS := $(XML2_LINK) $(LINK_STD_LIBS)
# -lstdc++ -lm

all: $(TEST_DIR)/event_client_global $(TEST_DIR)/event_client_local

$(TEST_DIR)/event_client_global: $(OBJ_DIR)/event_client_global.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 
	
$(TEST_DIR)/event_client_local: $(OBJ_DIR)/event_client_local.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 


$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<


clean:
	rm -f $(OBJ_DIR)/event_client_global.o $(OBJ_DIR)/event_client_local.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
