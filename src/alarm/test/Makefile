S7 := 1
SAFPLUS_ALARM_TEST := 1
include ../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(SAFPLUS_MGT_INC_FLAGS) -I. -I../server -I../server/SAFplusAlarm -I../../include

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

CLIENT_SRC := alarm_client.cxx
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

CLIENT_SRC := alarm_client_child.cxx
CLIENT_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(CLIENT_SRC)))

SERVER_SRC := alarm_server.cxx
SERVER_OBJ := $(addprefix $(OBJ_DIR)/,$(subst .cxx,.o,$(SERVER_SRC)))


# Specify the required libraries
SAFPLUS_LIBS :=clRpc  clAlarm clAlarmServer clEvent clEventServer clFaultServer clTimer clCkpt clFault  clGroup clName clTimer clMsg clMgt clLog clUtils clOsal clDbal SAFplusAlarm
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS := $(XML2_LINK) $(LINK_STD_LIBS)
# -lstdc++ -lm

all: $(TEST_DIR)/alarm_client $(TEST_DIR)/alarm_client_child $(TEST_DIR)/alarm_server

$(TEST_DIR)/alarm_client: $(OBJ_DIR)/alarm_client.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)
	 
$(TEST_DIR)/alarm_client_child: $(OBJ_DIR)/alarm_client_child.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/alarm_server: $(OBJ_DIR)/alarm_server.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

test: all

clean:
	rm -f $(OBJ_DIR)/alarm_client.o $(OBJ_DIR)/alarm_client_child.o $(OBJ_DIR)/alarm_server.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
