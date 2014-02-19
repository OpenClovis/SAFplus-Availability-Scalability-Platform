
$(LIB_DIR)/clLog.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7

$(LIB_DIR)/clUtils7.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/utils7

$(LIB_DIR)/clOsal7.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/osal7

$(LIB_DIR)/clMgt7.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/mgt7/client

SAFplusSOs := $(LIB_DIR)/clUtils7.so $(LIB_DIR)/clOsal7.so $(LIB_DIR)/clMgt7.so $(LIB_DIR)/clLog.so 

ifndef SAFPLUS_LOG_TEST
$(TEST_DIR)/testLog:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7/test
endif

ifndef SAFPLUS_LOG_SERVER
$(SAFPLUS_TARGET)/bin/splogd:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7/test
endif

SAFplusTests := $(TEST_DIR)/testLog

SAFplusServices := $(SAFPLUS_TARGET)/bin/splogd