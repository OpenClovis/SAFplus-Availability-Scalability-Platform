S7 := 1
SAFPLUS_ALARM_TEST := 1
include ../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

CPP_FLAGS += -I$(SAFPLUS_MGT_INC_FLAGS) -I. -I../server -I../server/SAFplusAlarm -I../../include

CLIENT_H := $(wildcard *.hxx) $(wildcard $(SAFPLUS_INC_DIR)/*.hxx)

# Specify the required libraries
SAFPLUS_LIBS :=clRpc  clAlarm clAlarmServer clEvent clEventServer clFaultServer clTimer clCkpt clFault  clGroup clName clTimer clMsg clMgt clLog clUtils clOsal clDbal SAFplusAlarm
# Then use these in the make rule
SAFPLUS_DEP_LIBS     := $(addsuffix .so,$(addprefix $(LIB_DIR)/lib,$(SAFPLUS_LIBS)))
SAFPLUS_LINK_LIBS := -L$(LIB_DIR) $(addprefix -l,$(SAFPLUS_LIBS))

LINK_STD_LIBS := $(XML2_LINK) $(LINK_STD_LIBS)
# -lstdc++ -lm

all: $(TEST_DIR)/alarm_client $(TEST_DIR)/alarm_client_subscriber $(TEST_DIR)/alarm_client1 $(TEST_DIR)/alarm_client2 $(TEST_DIR)/alarm_client3 $(TEST_DIR)/alarm_client_child $(TEST_DIR)/alarm_client_parent $(TEST_DIR)/alarm_client_assert_soaking_time_switchover $(TEST_DIR)/alarm_client_clear_soaking_time_switchover $(TEST_DIR)/alarm_server

$(TEST_DIR)/alarm_client: $(OBJ_DIR)/alarm_client.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/alarm_client1: $(OBJ_DIR)/alarm_client1.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)
	 
$(TEST_DIR)/alarm_client2: $(OBJ_DIR)/alarm_client2.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)
	 
$(TEST_DIR)/alarm_client3: $(OBJ_DIR)/alarm_client3.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)
	 	 
$(TEST_DIR)/alarm_client_child: $(OBJ_DIR)/alarm_client_child.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/alarm_client_subscriber: $(OBJ_DIR)/alarm_client_subscriber.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/alarm_client_parent: $(OBJ_DIR)/alarm_client_parent.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS)

$(TEST_DIR)/alarm_server: $(OBJ_DIR)/alarm_server.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 

$(TEST_DIR)/alarm_client_assert_soaking_time_switchover: $(OBJ_DIR)/alarm_client_assert_soaking_time_switchover.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 
	
$(TEST_DIR)/alarm_client_clear_soaking_time_switchover: $(OBJ_DIR)/alarm_client_clear_soaking_time_switchover.o $(SAFPLUS_DEP_LIBS)
	$(LINK_EXE) $@ $+ $(LINK_LIBS) $(SAFPLUS_LINK_LIBS) $(LINK_STD_LIBS) 	

$(OBJ_DIR)/%.o: %.cxx $(CLIENT_H)
	$(COMPILE_CPP) $@ $<

test: all

clean:
	rm -f $(OBJ_DIR)/alarm_client.o $(OBJ_DIR)/alarm_client1.o $(OBJ_DIR)/alarm_client2.o $(OBJ_DIR)/alarm_client3.o  $(OBJ_DIR)/alarm_client_child.o $(OBJ_DIR)/alarm_client_parent.o $(OBJ_DIR)/alarm_server.o

include $(SAFPLUS_MAKE_DIR)/safplus_targets.mk
