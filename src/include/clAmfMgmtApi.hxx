#include <amfMgmtRpc.pb.hxx>
#include <clCommon6.h>
#include <clHandleApi.hxx>
namespace SAFplus {

ClRcT amfMgmtInitialize(Handle& amfMgmtHandle);
ClRcT amfMgmtCommit(const Handle& amfMgmtHandle);
ClRcT amfMgmtFinalize(const Handle& amfMgmtHandle);
ClRcT amfMgmtComponentCreate(const Handle& mgmtHandle,SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp);
//ClRcT amfMgmtComponentConfigSet(const Handle& mgmtHandle,SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp);



}
