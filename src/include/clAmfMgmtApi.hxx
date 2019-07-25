#include <amfMgmtRpc.pb.hxx>
#include <clCommon6.h>
#include <clHandleApi.hxx>
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
ClRcT amfMgmtNodeConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::NodeConfig* node);

}
