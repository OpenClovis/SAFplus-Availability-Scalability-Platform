#include <clGroupCliApi.hxx>
#include <groupCliRpc.hxx>
#include <clRpcChannel.hxx>
#include <clSafplusMsgServer.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clHandleApi.hxx>
#include <clNameApi.hxx>

using namespace SAFplusI;


namespace SAFplus
{
SAFplus::Rpc::RpcChannel *grpCliRpcChannel=NULL;
SAFplus::Rpc::groupCliRpc::groupCliRpc_Stub *grpCliRpc=NULL;
bool grpRpcInitialized = false;
void grpRpcInit()
{
  SafplusInitializationConfiguration sic;
  sic.iocPort     = 36;  // TODO: auto assign port
  sic.msgQueueLen = 10;
  sic.msgThreads  = 1;
  safplusInitialize( SAFplus::LibDep::NAME | SAFplus::LibDep::LOCAL,sic);

  grpCliRpcChannel=new SAFplus::Rpc::RpcChannel(&safplusMsgServer, NULL);
  grpCliRpcChannel->setMsgType(AMF_GROUP_CLI_REQ_HANDLER_TYPE,AMF_GROUP_CLI_REPLY_HANDLER_TYPE);
  grpCliRpc=new SAFplus::Rpc::groupCliRpc::groupCliRpc_Stub(grpCliRpcChannel);
  grpRpcInitialized = true;
}
#if 0
ClRcT amfMgmtInitialize(Handle& amfMgmtHandle) 
{
  ClRcT rc = CL_OK;
  if (!gAmfMgmtInitialized)
  {
    if (!rpcInitialized) rpcInit();
    SAFplus::Rpc::amfMgmtRpc::InitializeRequest request;
    request.add_amfmgmthandle((const char*) &myHandle, sizeof(Handle));    
    try 
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::InitializeResponse resp;
      amfMgmtRpc->initialize(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      if (rc == CL_OK)
      {
        gAmfMgmtInitialized = CL_TRUE;    
        amfMgmtHandle = myHandle;
      }
      else
      {
        amfMgmtHandle = INVALID_HDL;
      }
    }
    catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }   
  }
  return rc;
}
#endif

std::string grpCliClusterViewGet()
{   
   if (!grpRpcInitialized)
   {
     grpRpcInit();
   }   
   SAFplus::Rpc::groupCliRpc::GetClusterViewRequest request;
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::groupCliRpc::GetClusterViewResponse resp; 
      grpCliRpc->getClusterView(remoteAmfHdl,&request,&resp);
      return resp.clusterview();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());      
    } 
   return std::string();
}

}
