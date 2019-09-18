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

}
