
ifndef SAFPLUS_LOG_LIB
$(LIB_DIR)/libclLog.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/log7
endif

ifndef SAFPLUS_UTILS_LIB
$(LIB_DIR)/libclUtils7.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/utils7
endif

ifndef SAFPLUS_IOC_LIB
$(LIB_DIR)/libclIoc7.so $(LIB_DIR)/libclTIPC.so  $(LIB_DIR)/libclUDP.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/ioc7/client
endif

ifndef SAFPLUS_RPC_LIB
$(LIB_DIR)/libclRpc.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/rpc
endif

ifndef SAFPLUS_OSAL_LIB
$(LIB_DIR)/libclOsal7.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/osal7
endif

ifndef SAFPLUS_MGT_LIB
$(LIB_DIR)/libclMgt7.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/mgt7/client
endif

ifndef SAFPLUS_DBAL_LIB
$(LIB_DIR)/libclDbal7.so $(LIB_DIR)/pyDbal.so $(BIN_DIR)/dbalpy.py $(LIB_DIR)/libclBerkeleyDB.so $(LIB_DIR)/libclGDBM.so $(LIB_DIR)/libclSQLiteDB.so:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/dbal7
endif

ifndef SAFPLUS_CKPT_LIB
$(LIB_DIR)/libclCkpt.so: $(wildcard $(SAFPLUS_SRC_DIR)/SAFplus/components/ckpt7/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/SAFplus/include7/*.hxx) 
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/ckpt7
endif

ifndef SAFPLUS_GROUP_LIB
$(LIB_DIR)/libclGroup.so: 
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/gms7
endif


ifndef SAFPLUS_NAME_LIB
$(LIB_DIR)/libclName.so: 
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/name7
endif

# ordered by dependency
SAFplusSOs := $(LIB_DIR)/libclUtils7.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclOsal7.so  $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclMgt7.so $(LIB_DIR)/libclIoc7.so $(LIB_DIR)/libclRpc.so $(LIB_DIR)/libclName.so $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclDbal7.so $(LIB_DIR)/pyDbal.so $(BIN_DIR)/dbalpy.py


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

ifndef SAFPLUS_IOC_TEST
$(TEST_DIR)/TestSendMsg $(TEST_DIR)/TestReceiveMsg:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/ioc7/test
endif

ifndef SAFPLUS_RPC_TEST
$(TEST_DIR)/TestClient $(TEST_DIR)/TestServer $(TEST_DIR)/TestCombine:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/rpc/test
endif

ifndef SAFPLUS_MGT_TEST
$(TEST_DIR)/testmgt:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/mgt7/test
endif

ifndef SAFPLUS_GROUP_TEST
$(TEST_DIR)/testGroup $(TEST_DIR)/testGroupServer:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/gms7/test
endif

ifndef SAFPLUS_NAME_TEST
$(TEST_DIR)/testName:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/name7/test
endif

ifndef SAFPLUS_AMF_SERVER
$(SAFPLUS_TARGET)/bin/safplus_amf:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/amf7/server
endif

ifndef SAFPLUS_RPC_LIB
$(SAFPLUS_TARGET)/bin/protoc-gen-rpc:
	make -C $(SAFPLUS_SRC_DIR)/SAFplus/components/rpc
endif

SAFplusTests := $(TEST_DIR)/testLog $(TEST_DIR)/testmgt $(TEST_DIR)/TestSendMsg $(TEST_DIR)/TestReceiveMsg  $(TEST_DIR)/TestClient $(TEST_DIR)/TestServer $(TEST_DIR)/TestCombine

# $(TEST_DIR)/testCkpt $(TEST_DIR)/testGroup $(TEST_DIR)/testGroupServer

SAFplusServices := 
# $(SAFPLUS_TARGET)/bin/safplus_amf
# $(SAFPLUS_TARGET)/bin/splogd $(SAFPLUS_TARGET)/bin/safplus_amf

SAFplusTools := $(SAFPLUS_TARGET)/bin/protoc-gen-rpc



cleanall:
	rm -rf $(SAFplusTests) $(SAFplusSOs) $(SAFplusServices) $(LIB_DIR)/* $(MWOBJ_DIR)/* $(OBJ_DIR)/* $(TEST_DIR)/*