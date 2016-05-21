
# If we don't want to spawn off any submakes then set this variable.  This is needed when
# we are doing multi-threaded builds because we don't want multiple submakes attempting to build the same subdirectory
ifndef NO_SUB_BUILD  
$(info sub-building is enabled)

ifndef SAFPLUS_LOG_LIB
$(LIB_DIR)/libclLog.so:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/log
endif

ifndef SAFPLUS_LOG_SERVER
$(LIB_DIR)/libclLogServer.so:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/log/server
endif

ifndef SAFPLUS_LOGREP_LIB
$(LIB_DIR)/libclLogRep.so:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/log/rep
endif

ifndef SAFPLUS_UTILS_LIB
$(LIB_DIR)/libclUtils.so: $(wildcard $(SAFPLUS_SRC_DIR)/utils/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx)
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/utils
endif

# We expect this to be installed by default -- 3rd party is just in case this does not work for you
#$(INSTALL_DIR)/lib/libxml2.so:
#	if [ $(DISTRIBUTION_LIB) -eq 0 ]; then \
#            $(MAKE) -C $(SAFPLUS_SRC_DIR)/3rdparty/base; \
#	fi

#ifndef SAFPLUS_IOC_LIB
#$(LIB_DIR)/libclIoc.so $(LIB_DIR)/libclTIPC.so  $(LIB_DIR)/libclUDP.so:
#	$(MAKE) -C $(SAFPLUS_SRC_DIR)/ioc/client
#endif

ifndef SAFPLUS_RPC_LIB
$(SAFPLUS_TOOL_TARGET)/bin/protoc-gen-rpc: $(wildcard $(SAFPLUS_SRC_DIR)/rpc/protoc/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/rpc/protoc/*.hxx)
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/rpc/protoc

$(LIB_DIR)/libclRpc.so: $(wildcard $(SAFPLUS_SRC_DIR)/rpc/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx) $(wildcard $(SAFPLUS_SRC_DIR)/rpc/protoc/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/rpc/protoc/*.hxx)
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/rpc

$(SAFPLUS_SRC_DIR)/rpc/SAFplusPBExt.pb.hxx:
	ln -s $(PROTOBUFVER)/SAFplusPBExt.pb.hxx $@

endif

ifndef SAFPLUS_OSAL_LIB
$(LIB_DIR)/libclOsal.so: $(wildcard $(SAFPLUS_SRC_DIR)/osal/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx)
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/osal
endif

ifndef SAFPLUS_MGT_LIB
$(LIB_DIR)/libclMgt.so:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/mgt

$(SAFPLUS_SRC_DIR)/mgt/MgtMsg.pb.hxx:
	ln -s $(PROTOBUFVER)/MgtMsg.pb.hxx $@

endif

ifndef SAFPLUS_DBAL_LIB
$(LIB_DIR)/libclDbal.so:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/dbal
endif

ifndef SAFPLUS_TIMER_LIB
$(LIB_DIR)/libclTimer.so:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/timer
endif

ifndef SAFPLUS_PY_LIB
$(LIB_DIR)/pySAFplus.so $(LIB_DIR)/amfctrl.py $(LIB_DIR)/safplus.py:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/python
endif


ifndef SAFPLUS_DBAL_PYLIB
$(LIB_DIR)/pyDbal.so $(BIN_DIR)/safplus_db:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/mgt/pylib

endif

ifndef SAFPLUS_DBAL_PLUGIN
$(PLUGIN_DIR)/libclSQLiteDB.so $(PLUGIN_DIR)/libclGDBM.so $(PLUGIN_DIR)/libclBerkeleyDB.so $(PLUGIN_DIR)/libclCkptDB.so:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/dbal/plugins
endif

ifndef SAFPLUS_CKPT_LIB
$(LIB_DIR)/libclCkpt.so: $(wildcard $(SAFPLUS_SRC_DIR)/ckpt/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx) 
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/ckpt
endif

ifndef SAFPLUS_CKPT_RET
$(BIN_DIR)/ckptretention: $(wildcard $(SAFPLUS_SRC_DIR)/ckpt/retention/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx) 
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/ckpt/retention
endif

ifndef SAFPLUS_GROUP_LIB
$(LIB_DIR)/libclGroup.so: 
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/group

$(BIN_DIR)/spgroupd $(BIN_DIR)/safplus_group: 
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/group
endif

ifndef SAFPLUS_NAME_LIB
$(LIB_DIR)/libclName.so $(BIN_DIR)/safplus_name: 
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/name
endif

ifndef SAFPLUS_MSG_LIB

$(LIB_DIR)/libclMsg.so: $(wildcard $(SAFPLUS_SRC_DIR)/msg/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx)
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/msg
endif

ifndef SAFPLUS_FAULT_LIB
$(LIB_DIR)/libclFault.so: $(SAFPLUS_SRC_DIR)/mgt/MgtMsg.pb.hxx $(wildcard $(SAFPLUS_SRC_DIR)/fault/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx) 
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/fault
endif

ifndef SAFPLUS_FAULT_SERVER
$(LIB_DIR)/libclFaultServer.so $(PLUGIN_DIR)/AmfFaultPolicy.so $(PLUGIN_DIR)/CustomFaultPolicy.so: $(wildcard $(SAFPLUS_SRC_DIR)/fault/server/*.cxx) $(wildcard $(SAFPLUS_SRC_DIR)/include/*.hxx) 
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/fault/server
endif


ifndef SAFPLUS_AMF_LIB
$(LIB_DIR)/libclAmf.so: $(wildcard $(SAFPLUS_SRC_DIR)/amf/*.cxx)
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/amf
endif

ifndef SAFPLUS_MSG_PLUGIN
# .PHONY: $(LIB_DIR)/clMsgUdp.so
$(PLUGIN_DIR)/clMsgUdp.so $(PLUGIN_DIR)/clMsgTcp.so $(PLUGIN_DIR)/clMsgSctp.so $(PLUGIN_DIR)/clMsgTipc.so: $(wildcard $(SAFPLUS_SRC_DIR)/msg/transports/*.cxx)
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/msg/transports
endif

ifndef SAFPLUS_MSG_TEST
$(TEST_DIR)/testTransport $(TEST_DIR)/testMsgPerf $(TEST_DIR)/msgReflector:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/msg/test
endif

ifndef SAFPLUS_LOG_TEST
$(TEST_DIR)/testLog:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/log/test
endif

ifndef SAFPLUS_LOG_SERVER
$(SAFPLUS_TARGET)/bin/splogd:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/log/server
endif

ifndef SAFPLUS_CKPT_TEST
$(TEST_DIR)/testCkpt:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/ckpt/test
endif

#ifndef SAFPLUS_IOC_TEST
#$(TEST_DIR)/TestSendMsg $(TEST_DIR)/TestReceiveMsg:
#	$(MAKE) -C $(SAFPLUS_SRC_DIR)/ioc/test
#endif

ifndef SAFPLUS_RPC_TEST
$(TEST_DIR)/TestClient $(TEST_DIR)/TestServer $(TEST_DIR)/TestCombine:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/rpc/test
endif

ifndef SAFPLUS_MGT_TEST
$(TEST_DIR)/testmgt:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/mgt/test
endif

ifndef SAFPLUS_GROUP_TEST
$(TEST_DIR)/testGroup $(TEST_DIR)/testMultiGroup :
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/group/test
endif

ifndef SAFPLUS_NAME_TEST
$(TEST_DIR)/testName:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/name/test
endif

ifndef SAFPLUS_AMF_TEST
$(TEST_DIR)/exampleSafApp:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/amf/test
endif


ifndef SAFPLUS_AMF_SERVER
$(BIN_DIR)/safplus_amf $(PLUGIN_DIR)/customAmfPolicy.so $(PLUGIN_DIR)/nPlusmAmfPolicy.so:
	$(MAKE) -C $(SAFPLUS_SRC_DIR)/amf/server
endif

endif

#SAFplusTests := $(TEST_DIR)/testLog $(TEST_DIR)/testmgt   $(TEST_DIR)/TestClient $(TEST_DIR)/TestServer $(TEST_DIR)/TestCombine $(TEST_DIR)/testCkpt $(TEST_DIR)/testGroup $(TEST_DIR)/exampleSafApp $(TEST_DIR)/testTransport $(TEST_DIR)/testMsgPerf

SAFplusMsgTransports := $(PLUGIN_DIR)/clMsgUdp.so $(PLUGIN_DIR)/clMsgTipc.so $(PLUGIN_DIR)/clMsgSctp.so $(PLUGIN_DIR)/clMsgTcp.so
SAFplusDbalPlugins := $(PLUGIN_DIR)/libclBerkeleyDB.so $(PLUGIN_DIR)/libclSQLiteDB.so $(PLUGIN_DIR)/libclGDBM.so $(PLUGIN_DIR)/libclCkptDB.so

# ordered by dependency
SAFplusSOs := $(LIB_DIR)/libclUtils.so $(LIB_DIR)/libclTimer.so $(LIB_DIR)/libclLog.so $(LIB_DIR)/libclOsal.so  $(LIB_DIR)/libclCkpt.so $(LIB_DIR)/libclMsg.so $(LIB_DIR)/libclRpc.so $(LIB_DIR)/libclName.so $(LIB_DIR)/libclGroup.so $(LIB_DIR)/libclMgt.so $(LIB_DIR)/libclFault.so $(LIB_DIR)/libclDbal.so $(LIB_DIR)/libclAmf.so $(LIB_DIR)/pyDbal.so


SAFplusTests := $(TEST_DIR)/testLog $(TEST_DIR)/testmgt   $(TEST_DIR)/testCkpt $(TEST_DIR)/testGroup $(TEST_DIR)/exampleSafApp $(TEST_DIR)/testTransport $(TEST_DIR)/testMsgPerf

# $(TEST_DIR)/TestSendMsg $(TEST_DIR)/TestReceiveMsg
#  $(SAFPLUS_TARGET)/bin/splogd $(TEST_DIR)/testGroup $(TEST_DIR)/testGroupServer

SAFplusServices :=  $(SAFPLUS_TARGET)/bin/splogd $(SAFPLUS_TARGET)/bin/spgroupd $(SAFPLUS_TARGET)/bin/safplus_amf $(BIN_DIR)/ckptretention
SAFplusBin :=  $(SAFPLUS_TARGET)/bin/safplus_amf $(BIN_DIR)/safplus_db $(BIN_DIR)/safplus_name $(BIN_DIR)/safplus_cloud $(BIN_DIR)/safplus_cleanup $(BIN_DIR)/safplus_group 

SAFplusTools := $(SAFplusRpcGen)

SAFplusPlugins := $(SAFplusDbalPlugins) $(SAFplusMsgTransports)

ThirdPartySOs :=

Languages ?= $(LIB_DIR)/pySAFplus.so

cleanSAFplus:
	rm -rf $(SAFplusBin) $(SAFplusTests) $(SAFplusSOs) $(SAFplusServices) $(LIB_DIR)/* $(MWOBJ_DIR)/* $(OBJ_DIR)/* $(TEST_DIR)/*
