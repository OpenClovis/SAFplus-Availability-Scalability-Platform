ifndef SAFPLUS_LOG_LIB
$(LIB_DIR)/libclLog.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7
endif

$(LIB_DIR)/libclUtils7.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/utils7

$(LIB_DIR)/libclOsal7.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/osal7

$(LIB_DIR)/libclMgt7.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/mgt7/client

$(LIB_DIR)/libclCkpt.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/ckpt7

$(LIB_DIR)/libclMgt.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/ckpt7

SAFplusSOs := $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclOsal7.so $(LIB_DIR)/libclMgt7.so $(LIB_DIR)/libclLog.so 

ifndef SAFPLUS_LOG_TEST
$(TEST_DIR)/testLog:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7/test
endif

ifndef SAFPLUS_LOG_SERVER
$(SAFPLUS_TARGET)/bin/splogd:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7/server
endif

ifndef SAFPLUS_CKPT_TEST
$(TEST_DIR)/testCkpt:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/ckpt7/test
endif

ifndef SAFPLUS_MGT_TEST
$(TEST_DIR)/testmgt:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/mgt7/test
endif

ifndef SAFPLUS_NAME_TEST
$(TEST_DIR)/testName:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/name7/test
endif

SAFplusTests := $(TEST_DIR)/testLog $(TEST_DIR)/testCkpt $(TEST_DIR)/testmgt

SAFplusServices := $(SAFPLUS_TARGET)/bin/splogd

cleanall:
	rm -f $(SAFplusTests) $(SAFplusSOs) $(LIB_DIR)/* $(MWOBJ_DIR)/* $(OBJ_DIR)/*