S7 := 1
include ../../mk/preface.mk

MGT_SRC_DIR ?= $(SAFPLUS_SRC_DIR)/../../mgt

YANG_MODULES_STANDARD_PATH=$(SAFPLUS_SRC_DIR)/../../YangModels/standard/ietf/RFC
YANG_MODULES_STANDARD=ietf-netconf-monitoring ietf-system
YANG_MODULES_RPCS=ietfNetconfMonitoring ietfSystem

Release all: ietfNetconfMonitoring.so ietfSystem.so

ietfNetconfMonitoring.so:
#TODO
	

ietfSystem.so:
#TODO
	
gen:
	$(foreach m, $(YANG_MODULES_STANDARD), \
	PYTHONPATH=$$PYTHONPATH:$(MGT_SRC_DIR)/3rdparty/pyang:/usr/local/lib PYANG_PLUGINPATH=$$PYANG_PLUGINPATH:$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang:$(YANG_MODULES_STANDARD_PATH) -f y2cpp $(YANG_MODULES_STANDARD_PATH)/$(strip $m).yang --y2cpp-output=`pwd` --y2cpp-sdkdir=$(SAFPLUS_SRC_DIR); \
	)
	$(foreach m, $(YANG_MODULES_STANDARD), \
	PYTHONPATH=$$PYTHONPATH:$(MGT_SRC_DIR)/3rdparty/pyang:/usr/local/lib PYANG_PLUGINPATH=$$PYANG_PLUGINPATH:$(MGT_SRC_DIR)/pyplugin $(MGT_SRC_DIR)/3rdparty/pyang/bin/pyang --path=$(SAFPLUS_SRC_DIR)/yang:$(YANG_MODULES_STANDARD_PATH) -f y2cpp $(YANG_MODULES_STANDARD_PATH)/$(strip $m).yang --y2cpp-output=`pwd` --y2cpp-sdkdir=$(SAFPLUS_SRC_DIR) --y2cpp-rpc; \
	)

genrpc: gen
	$(foreach m, $(YANG_MODULES_RPCS), \
	$(call SAFPLUS_RPC_GEN, $(m),.,true); \
	)

clean:
# Mgt data
	rm -rf ianaCryptHash ietfInetTypes ietfNetconfAcm ietfNetconfMonitoring ietfNetconfMonitoringRpc ietfNetconfMonitorRpc ietfSystem ietfYangTypes

# Rpc impl
	rm -rf *.proto *.hxx *.cxx server
