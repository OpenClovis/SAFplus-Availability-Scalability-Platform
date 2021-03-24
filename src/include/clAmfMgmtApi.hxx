#include <amfMgmtRpc.pb.hxx>
#include <clCommon6.h>
#include <clHandleApi.hxx>
//#include <vector>

namespace SAFplus {

ClRcT amfMgmtInitialize(Handle& amfMgmtHandle);
ClRcT amfMgmtCommit(const Handle& amfMgmtHandle);
ClRcT amfMgmtFinalize(const Handle& amfMgmtHandle);

ClRcT amfMgmtComponentCreate(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp);
ClRcT amfMgmtComponentConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp);
ClRcT amfMgmtComponentDelete(const Handle& mgmtHandle,const std::string& compName);

ClRcT amfMgmtServiceUnitConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig* su);
ClRcT amfMgmtServiceUnitCreate(const Handle& mgmtHandle,SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig* su);
ClRcT amfMgmtServiceUnitDelete(const Handle& mgmtHandle,const std::string& suName);

ClRcT amfMgmtServiceGroupConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig* sg);
ClRcT amfMgmtServiceGroupCreate(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig* sg);
ClRcT amfMgmtServiceGroupDelete(const Handle& mgmtHandle, const std::string& sgName);

ClRcT amfMgmtNodeCreate(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::NodeConfig* node);
ClRcT amfMgmtNodeConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::NodeConfig* node);
ClRcT amfMgmtNodeDelete(const Handle& mgmtHandle, const std::string& nodeName);

ClRcT amfMgmtServiceInstanceCreate(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig* si);
ClRcT amfMgmtServiceInstanceConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig* si);
ClRcT amfMgmtServiceInstanceDelete(const Handle& mgmtHandle, const std::string& siName);

ClRcT amfMgmtComponentServiceInstanceCreate(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig* csi);
ClRcT amfMgmtComponentServiceInstanceConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig* csi);
ClRcT amfMgmtComponentServiceInstanceDelete(const Handle& mgmtHandle, const std::string& csiName);

ClRcT amfMgmtCSINVPDelete(const Handle& mgmtHandle, const std::string& csiName);
ClRcT amfMgmtNodeSUListDelete(const Handle& mgmtHandle, const std::string& nodeName, const std::vector<std::string>& suNames);
ClRcT amfMgmtSGSUListDelete(const Handle& mgmtHandle, const std::string& sgName, const std::vector<std::string>& suNames);
ClRcT amfMgmtSGSIListDelete(const Handle& mgmtHandle, const std::string& sgName, const std::vector<std::string>& siNames);
ClRcT amfMgmtSUCompListDelete(const Handle& mgmtHandle, const std::string& suName, const std::vector<std::string>& compNames);
ClRcT amfMgmtSICSIListDelete(const Handle& mgmtHandle, const std::string& siName, const std::vector<std::string>& csiNames);

ClRcT amfMgmtNodeLockAssignment(const Handle& mgmtHandle, const std::string& nodeName);
ClRcT amfMgmtSGLockAssignment(const Handle& mgmtHandle, const std::string& sgName);
ClRcT amfMgmtSULockAssignment(const Handle& mgmtHandle, const std::string& suName);
ClRcT amfMgmtSILockAssignment(const Handle& mgmtHandle, const std::string& siName);
ClRcT amfMgmtNodeLockInstantiation(const Handle& mgmtHandle, const std::string& nodeName);
ClRcT amfMgmtSGLockInstantiation(const Handle& mgmtHandle, const std::string& sgName);
ClRcT amfMgmtSULockInstantiation(const Handle& mgmtHandle, const std::string& suName);
ClRcT amfMgmtNodeUnlock(const Handle& mgmtHandle, const std::string& nodeName);
ClRcT amfMgmtSGUnlock(const Handle& mgmtHandle, const std::string& sgName);
ClRcT amfMgmtSUUnlock(const Handle& mgmtHandle, const std::string& suName);
ClRcT amfMgmtSIUnlock(const Handle& mgmtHandle, const std::string& siName);
ClRcT amfMgmtNodeRepair(const Handle& mgmtHandle, const std::string& nodeName);
ClRcT amfMgmtCompRepair(const Handle& mgmtHandle, const std::string& compName);
ClRcT amfMgmtSURepair(const Handle& mgmtHandle, const std::string& suName);

ClRcT amfMgmtNodeRestart(const Handle& mgmtHandle, const std::string& nodeName);
ClRcT amfMgmtSURestart(const Handle& mgmtHandle, const std::string& suName);
ClRcT amfMgmtCompRestart(const Handle& mgmtHandle, const std::string& compName);

ClRcT amfMgmtComponentGetConfig(const Handle& mgmtHandle,const std::string& compName, SAFplus::Rpc::amfMgmtRpc::ComponentConfig** compConfig);
ClRcT amfMgmtNodeGetConfig(const Handle& mgmtHandle,const std::string& nodeName, SAFplus::Rpc::amfMgmtRpc::NodeConfig** nodeConfig);
ClRcT amfMgmtSGGetConfig(const Handle& mgmtHandle,const std::string& sgName, SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig** sgConfig);
ClRcT amfMgmtSUGetConfig(const Handle& mgmtHandle,const std::string& suName, SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig** suConfig);
ClRcT amfMgmtSIGetConfig(const Handle& mgmtHandle,const std::string& siName, SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig** siConfig);
ClRcT amfMgmtCSIGetConfig(const Handle& mgmtHandle,const std::string& csiName, SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig** csiConfig);
      
ClRcT amfMgmtComponentGetStatus(const Handle& mgmtHandle,const std::string& compName, SAFplus::Rpc::amfMgmtRpc::ComponentStatus** compStatus);
ClRcT amfMgmtNodeGetStatus(const Handle& mgmtHandle,const std::string& nodeName, SAFplus::Rpc::amfMgmtRpc::NodeStatus** nodeStatus);
ClRcT amfMgmtSGGetStatus(const Handle& mgmtHandle,const std::string& sgName, SAFplus::Rpc::amfMgmtRpc::ServiceGroupStatus** sgStatus);
ClRcT amfMgmtSUGetStatus(const Handle& mgmtHandle,const std::string& suName, SAFplus::Rpc::amfMgmtRpc::ServiceUnitStatus** suStatus);
ClRcT amfMgmtSIGetStatus(const Handle& mgmtHandle,const std::string& siName, SAFplus::Rpc::amfMgmtRpc::ServiceInstanceStatus** siStatus);
ClRcT amfMgmtCSIGetStatus(const Handle& mgmtHandle,const std::string& csiName, SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceStatus** csiStatus);

ClRcT amfMgmtSGAdjust(const Handle& mgmtHandle, const std::string& sgName, bool enabled);

ClRcT amfMgmtSISwap(const Handle& mgmtHandle, const std::string& siName);

ClRcT amfMgmtCompErrorReport(const Handle& mgmtHandle, const std::string& compName, SAFplus::Rpc::amfMgmtRpc::Recovery recommendedRecovery);

}
