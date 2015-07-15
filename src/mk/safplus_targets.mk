
ifndef SAFPLUS_LOG_LIB
$(LIB_DIR)/libclLog.so:
	make -C $(SAFPLUS_SRC_DIR)/log
endif

ifndef SAFPLUS_LOGREP_LIB
$(LIB_DIR)/libclLogRep.so:
	make -C $(SAFPLUS_SRC_DIR)/log/rep
endif

ifndef SAFPLUS_UTILS_LIB
$(LIB_DIR)/libclUtils.so: $(wildcard $(SAFPLUS_SRC_DIR)/utils/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx)
	make -C $(SAFPLUS_SRC_DIR)/utils
endif

$(INSTALL_DIR)/lib/libxml2.so:
	make -C $(SAFPLUS_SRC_DIR)/3rdparty/base

#ifndef SAFPLUS_IOC_LIB
#$(LIB_DIR)/libclIoc.so $(LIB_DIR)/libclTIPC.so  $(LIB_DIR)/libclUDP.so:
#	make -C $(SAFPLUS_SRC_DIR)/ioc/client
#endif

ifndef SAFPLUS_RPC_LIB
$(SAFPLUS_TARGET)/bin/protoc-gen-rpc: $(wildcard $(SAFPLUS_SRC_DIR)/rpc/protoc/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/rpc/protoc/*.hxx)
	make -C $(SAFPLUS_SRC_DIR)/rpc/protoc

$(LIB_DIR)/libclRpc.so: $(wildcard $(SAFPLUS_SRC_DIR)/rpc/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx) $(wildcard $(SAFPLUS_SRC_DIR)/rpc/protoc/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/rpc/protoc/*.hxx)
	make -C $(SAFPLUS_SRC_DIR)/rpc
endif

ifndef SAFPLUS_OSAL_LIB
$(LIB_DIR)/libclOsal.so: $(wildcard $(SAFPLUS_SRC_DIR)/osal/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx)
	make -C $(SAFPLUS_SRC_DIR)/osal
endif

ifndef SAFPLUS_MGT_LIB
$(LIB_DIR)/libclMgt.so:
	make -C $(SAFPLUS_SRC_DIR)/mgt
endif

ifndef SAFPLUS_DBAL_LIB
$(LIB_DIR)/libclDbal.so:
	make -C $(SAFPLUS_SRC_DIR)/dbal
endif

ifndef SAFPLUS_PY_LIB
$(LIB_DIR)/pySAFplus.so $(LIB_DIR)/amfctrl.py $(LIB_DIR)/safplus.py:
	make -C $(SAFPLUS_SRC_DIR)/python
endif


ifndef SAFPLUS_DBAL_PYLIB
$(LIB_DIR)/pyDbal.so:
	make -C $(SAFPLUS_SRC_DIR)/mgt/pylib
endif

ifndef SAFPLUS_DBAL_PLUGIN

ifndef SAFPLUS_DBAL_BERKELEY_PLUGIN
$(PLUGIN_DIR)/libclBerkeleyDB.so:
	make -C $(SAFPLUS_SRC_DIR)/dbal/plugins/berkeley
endif

ifndef SAFPLUS_DBAL_GDBM_PLUGIN
$(PLUGIN_DIR)/libclGDBM.so:
	make -C $(SAFPLUS_SRC_DIR)/dbal/plugins/gdbm
endif

ifndef SAFPLUS_DBAL_SQLITE_PLUGIN
$(PLUGIN_DIR)/libclSQLiteDB.so:
	make -C $(SAFPLUS_SRC_DIR)/dbal/plugins/sqlite
endif

endif

ifndef SAFPLUS_CKPT_LIB
$(LIB_DIR)/libclCkpt.so: $(wildcard $(SAFPLUS_SRC_DIR)/ckpt/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx) 
	make -C $(SAFPLUS_SRC_DIR)/ckpt
endif

ifndef SAFPLUS_GROUP_LIB
$(LIB_DIR)/libclGroup.so: 
	make -C $(SAFPLUS_SRC_DIR)/group
endif

ifndef SAFPLUS_NAME_LIB
$(LIB_DIR)/libclName.so: 
	make -C $(SAFPLUS_SRC_DIR)/name
endif

ifndef SAFPLUS_MSG_LIB

$(LIB_DIR)/libclMsg.so: $(wildcard $(SAFPLUS_SRC_DIR)/msg/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx)
	make -C $(SAFPLUS_SRC_DIR)/msg
endif

ifndef SAFPLUS_FAULT_LIB
$(LIB_DIR)/libclFault.so: $(wildcard $(SAFPLUS_SRC_DIR)/fault/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx) 
	make -C $(SAFPLUS_SRC_DIR)/fault
endif

ifndef SAFPLUS_FAULT_SERVER
$(LIB_DIR)/libclFaultServer.so $(PLUGIN_DIR)/AmfFaultPolicy.so $(PLUGIN_DIR)/CustomFaultPolicy.so: $(wildcard $(SAFPLUS_SRC_DIR)/fault/server/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx) 
	make -C $(SAFPLUS_SRC_DIR)/fault/server
endif


ifndef SAFPLUS_AMF_LIB
$(LIB_DIR)/libclAmf.so: $(wildcard $(SAFPLUS_SRC_DIR)/amf/*.cxx)
	make -C $(SAFPLUS_SRC_DIR)/amf
endif

$(LIB_DIR)/libezxml.so:
	make -C $(SAFPLUS_SRC_DIR)/3rdparty/ezxml/ezxml-0.8.6/

# ordered by dependency
SAFplusSOs := $(LIB_DIR)/libclUtils.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclOsal.so  $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclMsg.so $(LIB_DIR)/libclRpc.so $(LIB_DIR)/libclName.so $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclMgt.so $(LIB_DIR)/libclFault.so $(LIB_DIR)/libclDbal.so $(LIB_DIR)/libclAmf.so $(LIB_DIR)/pyDbal.so

ifndef SAFPLUS_MSG_PLUGIN
# .PHONY: $(LIB_DIR)/clMsgUdp.so
$(LIB_DIR)/clMsgUdp.so: $(wildcard $(SAFPLUS_SRC_DIR)/msg/transports/*.cxx)
	make -C $(SAFPLUS_SRC_DIR)/msg/transports
endif

SAFplusMsgTransports := $(LIB_DIR)/clMsgUdp.so

ifndef SAFPLUS_MSG_TEST
$(TEST_DIR)/testTransport $(TEST_DIR)/testMsgPerf $(TEST_DIR)/msgReflector:
	make -C $(SAFPLUS_SRC_DIR)/msg/test
endif

ifndef SAFPLUS_LOG_TEST
$(TEST_DIR)/testLog:
	make -C $(SAFPLUS_SRC_DIR)/log/test
endif

ifndef SAFPLUS_LOG_SERVER
$(SAFPLUS_TARGET)/bin/splogd:
	make -C $(SAFPLUS_SRC_DIR)/log/server
endif

ifndef SAFPLUS_CKPT_TEST
$(TEST_DIR)/testCkpt:
	make -C $(SAFPLUS_SRC_DIR)/ckpt/test
endif

#ifndef SAFPLUS_IOC_TEST
#$(TEST_DIR)/TestSendMsg $(TEST_DIR)/TestReceiveMsg:
#	make -C $(SAFPLUS_SRC_DIR)/ioc/test
#endif

ifndef SAFPLUS_RPC_TEST
$(TEST_DIR)/TestClient $(TEST_DIR)/TestServer $(TEST_DIR)/TestCombine:
	make -C $(SAFPLUS_SRC_DIR)/rpc/test
endif

ifndef SAFPLUS_MGT_TEST
$(TEST_DIR)/testmgt:
	make -C $(SAFPLUS_SRC_DIR)/mgt/test
endif

ifndef SAFPLUS_GROUP_TEST
$(TEST_DIR)/testGroup $(TEST_DIR)/testMultiGroup :
	make -C $(SAFPLUS_SRC_DIR)/group/test
endif

ifndef SAFPLUS_NAME_TEST
$(TEST_DIR)/testName:
	make -C $(SAFPLUS_SRC_DIR)/name/test
endif

ifndef SAFPLUS_AMF_TEST
$(TEST_DIR)/exampleSafApp:
	make -C $(SAFPLUS_SRC_DIR)/amf/test
endif


ifndef SAFPLUS_AMF_SERVER
$(BIN_DIR)/safplus_amf $(PLUGIN_DIR)/customAmfPolicy.so $(PLUGIN_DIR)/nPlusmAmfPolicy.so:
	make -C $(SAFPLUS_SRC_DIR)/amf/server
endif

#SAFplusTests := $(TEST_DIR)/testLog $(TEST_DIR)/testmgt   $(TEST_DIR)/TestClient $(TEST_DIR)/TestServer $(TEST_DIR)/TestCombine $(TEST_DIR)/testCkpt $(TEST_DIR)/testGroup $(TEST_DIR)/exampleSafApp $(TEST_DIR)/testTransport $(TEST_DIR)/testMsgPerf

SAFplusTests := $(TEST_DIR)/testLog $(TEST_DIR)/testmgt   $(TEST_DIR)/testCkpt $(TEST_DIR)/testGroup $(TEST_DIR)/exampleSafApp $(TEST_DIR)/testTransport $(TEST_DIR)/testMsgPerf

# $(TEST_DIR)/TestSendMsg $(TEST_DIR)/TestReceiveMsg
#  $(SAFPLUS_TARGET)/bin/splogd $(TEST_DIR)/testGroup $(TEST_DIR)/testGroupServer

SAFplusServices :=  $(SAFPLUS_TARGET)/bin/splogd $(SAFPLUS_TARGET)/bin/safplus_amf

SAFplusTools := $(SAFPLUS_TARGET)/bin/protoc-gen-rpc

SAFplusPlugins := $(PLUGIN_DIR)/libclBerkeleyDB.so $(PLUGIN_DIR)/libclGDBM.so $(PLUGIN_DIR)/libclSQLiteDB.so $(SAFplusMsgTransports)

ThirdPartySOs := $(LIB_DIR)/libezxml.so

Languages := $(LIB_DIR)/pySAFplus.so

cleanSAFplus:
	rm -rf $(SAFplusTests) $(SAFplusSOs) $(SAFplusServices) $(LIB_DIR)/* $(MWOBJ_DIR)/* $(OBJ_DIR)/* $(TEST_DIR)/*
