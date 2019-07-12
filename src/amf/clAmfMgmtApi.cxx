#include <clAmfMgmtApi.hxx>
#include <amfMgmtRpc.hxx>
#include <clRpcChannel.hxx>
#include <clSafplusMsgServer.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clHandleApi.hxx>
#include <clNameApi.hxx>

using namespace SAFplusI;


namespace SAFplus
{
extern Handle myHandle;
SAFplus::Rpc::RpcChannel *amfMgmtRpcChannel=NULL;
SAFplus::Rpc::amfMgmtRpc::amfMgmtRpc_Stub *amfMgmtRpc=NULL;
ClBoolT gAmfMgmtInitialized = CL_FALSE;
ClBoolT rpcInitialized = CL_FALSE;
void rpcInit()
{
  amfMgmtRpcChannel=new SAFplus::Rpc::RpcChannel(&safplusMsgServer, NULL);
  amfMgmtRpcChannel->setMsgType(AMF_MGMT_REQ_HANDLER_TYPE,AMF_MGMT_REPLY_HANDLER_TYPE);
  amfMgmtRpc=new SAFplus::Rpc::amfMgmtRpc::amfMgmtRpc_Stub(amfMgmtRpcChannel);
  rpcInitialized = CL_TRUE;
}

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


ClRcT amfMgmtComponentCreate(const Handle& mgmtHandle,SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp)
{   
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::CreateComponentRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_componentconfig(comp);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::CreateComponentResponse resp; 
      amfMgmtRpc->createComponent(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtComponentConfigSet(const Handle& mgmtHandle,SAFplus::Rpc::amfMgmtRpc::ComponentConfig* comp)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::UpdateComponentRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_componentconfig(comp);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::UpdateComponentResponse resp;
      amfMgmtRpc->updateComponent(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}


ClRcT amfMgmtComponentDelete(const Handle& mgmtHandle,const std::string& compName)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteComponentRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_name(compName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteComponentResponse resp;
      amfMgmtRpc->deleteComponent(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtCommit(const Handle& amfMgmtHandle)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::CommitRequest request;
   request.add_amfmgmthandle((const char*) &amfMgmtHandle, sizeof(Handle));   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::CommitResponse resp;
      amfMgmtRpc->commit(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtFinalize(const Handle& amfMgmtHandle)
{
  if (!gAmfMgmtInitialized)
  {
    return CL_ERR_NOT_INITIALIZED;
  }
  ClRcT rc;
  SAFplus::Rpc::amfMgmtRpc::FinalizeRequest request;
  request.add_amfmgmthandle((const char*) &amfMgmtHandle, sizeof(Handle));   
  try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::FinalizeResponse resp;
      amfMgmtRpc->finalize(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
  catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
  return rc; 
}

}
