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
  //logDebug("MGMT","RPC.INI","rpc initialize");
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
    request.add_amfmgmthandle((myHandle != SAFplus::INVALID_HDL)?((const char*) &myHandle):((const char*) &amfMgmtHandle), sizeof(Handle));
    try 
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::InitializeResponse resp;
      amfMgmtRpc->initialize(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      if (rc == CL_OK)
      {
        gAmfMgmtInitialized = CL_TRUE;    
        if (amfMgmtHandle == SAFplus::INVALID_HDL)
        {
            amfMgmtHandle = myHandle;
        }
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

ClRcT amfMgmtServiceUnitCreate(const Handle& mgmtHandle,SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig* su)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::CreateSURequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_serviceunitconfig(su);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::CreateSUResponse resp;
      amfMgmtRpc->createSU(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtServiceUnitConfigSet(const Handle& mgmtHandle,SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig* su)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::UpdateSURequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_serviceunitconfig(su);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::UpdateSUResponse resp;
      amfMgmtRpc->updateSU(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtServiceUnitDelete(const Handle& mgmtHandle,const std::string& suName)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteSURequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_name(suName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteSUResponse resp;
      amfMgmtRpc->deleteSU(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtServiceGroupCreate(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig* sg)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::CreateSGRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_servicegroupconfig(sg);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::CreateSGResponse resp;
      amfMgmtRpc->createSG(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtServiceGroupConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig* sg)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::UpdateSGRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_servicegroupconfig(sg);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::UpdateSGResponse resp;
      amfMgmtRpc->updateSG(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtServiceGroupDelete(const Handle& mgmtHandle, const std::string& sgName)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteSGRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_name(sgName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteSGResponse resp;
      amfMgmtRpc->deleteSG(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtNodeCreate(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::NodeConfig* node)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::CreateNodeRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_nodeconfig(node);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::CreateNodeResponse resp;
      amfMgmtRpc->createNode(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtNodeConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::NodeConfig* node)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::UpdateNodeRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_nodeconfig(node);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::UpdateNodeResponse resp;
      amfMgmtRpc->updateNode(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtNodeDelete(const Handle& mgmtHandle, const std::string& nodeName)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteNodeRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_name(nodeName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteNodeResponse resp;
      amfMgmtRpc->deleteNode(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtServiceInstanceCreate(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig* si)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::CreateSIRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_serviceinstanceconfig(si);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::CreateSIResponse resp;
      amfMgmtRpc->createSI(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}
ClRcT amfMgmtServiceInstanceConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig* si)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::UpdateSIRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_serviceinstanceconfig(si);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::UpdateSIResponse resp;
      amfMgmtRpc->updateSI(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}
ClRcT amfMgmtServiceInstanceDelete(const Handle& mgmtHandle, const std::string& siName)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteSIRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_name(siName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteSIResponse resp;
      amfMgmtRpc->deleteSI(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}


ClRcT amfMgmtComponentServiceInstanceCreate(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig* csi)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::CreateCSIRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_componentserviceinstanceconfig(csi);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::CreateCSIResponse resp;
      amfMgmtRpc->createCSI(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtComponentServiceInstanceConfigSet(const Handle& mgmtHandle, SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig* csi)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::UpdateCSIRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_allocated_componentserviceinstanceconfig(csi);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::UpdateCSIResponse resp;
      amfMgmtRpc->updateCSI(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtComponentServiceInstanceDelete(const Handle& mgmtHandle, const std::string& csiName)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteCSIRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_name(csiName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteCSIResponse resp;
      amfMgmtRpc->deleteCSI(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtCSINVPDelete(const Handle& mgmtHandle, const std::string& csiName)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteCSINVPRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_name(csiName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteCSINVPResponse resp;
      amfMgmtRpc->deleteCSINVP(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtNodeSUListDelete(const Handle& mgmtHandle, const std::string& nodeName, const std::vector<std::string>& suNames)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteNodeSUListRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_nodename(nodeName);
   std::vector<std::string>::const_iterator it = suNames.begin();
   for(; it!=suNames.end();it++)
   {
     request.add_sulist(*it);
   }
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteNodeSUListResponse resp;
      amfMgmtRpc->deleteNodeSUList(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSGSUListDelete(const Handle& mgmtHandle, const std::string& sgName, const std::vector<std::string>& suNames)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteSGSUListRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_sgname(sgName);
   std::vector<std::string>::const_iterator it = suNames.begin();
   for(; it!=suNames.end();it++)
   {
     request.add_sulist(*it);
   }
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteSGSUListResponse resp;
      amfMgmtRpc->deleteSGSUList(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSGSIListDelete(const Handle& mgmtHandle, const std::string& sgName, const std::vector<std::string>& siNames)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteSGSIListRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_sgname(sgName);
   std::vector<std::string>::const_iterator it = siNames.begin();
   for(; it!=siNames.end();it++)
   {
     request.add_silist(*it);
   }
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteSGSIListResponse resp;
      amfMgmtRpc->deleteSGSIList(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSUCompListDelete(const Handle& mgmtHandle, const std::string& suName, const std::vector<std::string>& compNames)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteSUCompListRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_suname(suName);
   std::vector<std::string>::const_iterator it = compNames.begin();
   for(; it!=compNames.end();it++)
   {
     request.add_complist(*it);
   }
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteSUCompListResponse resp;
      amfMgmtRpc->deleteSUCompList(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSICSIListDelete(const Handle& mgmtHandle, const std::string& siName, const std::vector<std::string>& csiNames)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::DeleteSICSIListRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_siname(siName);
   std::vector<std::string>::const_iterator it = csiNames.begin();
   for(; it!=csiNames.end();it++)
   {
     request.add_csilist(*it);
   }
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::DeleteSICSIListResponse resp;
      amfMgmtRpc->deleteSICSIList(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtNodeLockAssignment(const Handle& mgmtHandle, const std::string& nodeName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::LockNodeAssignmentRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_nodename(nodeName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::LockNodeAssignmentResponse resp;
      amfMgmtRpc->lockNodeAssignment(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSGLockAssignment(const Handle& mgmtHandle, const std::string& sgName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::LockSGAssignmentRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_sgname(sgName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::LockSGAssignmentResponse resp;
      amfMgmtRpc->lockSGAssignment(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSULockAssignment(const Handle& mgmtHandle, const std::string& suName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::LockSUAssignmentRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_suname(suName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::LockSUAssignmentResponse resp;
      amfMgmtRpc->lockSUAssignment(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSILockAssignment(const Handle& mgmtHandle, const std::string& siName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::LockSIAssignmentRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_siname(siName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::LockSIAssignmentResponse resp;
      amfMgmtRpc->lockSIAssignment(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtNodeLockInstantiation(const Handle& mgmtHandle, const std::string& nodeName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::LockNodeInstantiationRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_nodename(nodeName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::LockNodeInstantiationResponse resp;
      amfMgmtRpc->lockNodeInstantiation(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSGLockInstantiation(const Handle& mgmtHandle, const std::string& sgName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::LockSGInstantiationRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_sgname(sgName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::LockSGInstantiationResponse resp;
      amfMgmtRpc->lockSGInstantiation(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSULockInstantiation(const Handle& mgmtHandle, const std::string& suName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::LockSUInstantiationRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_suname(suName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::LockSUInstantiationResponse resp;
      amfMgmtRpc->lockSUInstantiation(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtNodeUnlock(const Handle& mgmtHandle, const std::string& nodeName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::UnlockNodeRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_nodename(nodeName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::UnlockNodeResponse resp;
      amfMgmtRpc->unlockNode(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSGUnlock(const Handle& mgmtHandle, const std::string& sgName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::UnlockSGRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_sgname(sgName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::UnlockSGResponse resp;
      amfMgmtRpc->unlockSG(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSUUnlock(const Handle& mgmtHandle, const std::string& suName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::UnlockSURequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_suname(suName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::UnlockSUResponse resp;
      amfMgmtRpc->unlockSU(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSIUnlock(const Handle& mgmtHandle, const std::string& siName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::UnlockSIRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_siname(siName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::UnlockSIResponse resp;
      amfMgmtRpc->unlockSI(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtNodeRepair(const Handle& mgmtHandle, const std::string& nodeName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::RepairNodeRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_nodename(nodeName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::RepairNodeResponse resp;
      amfMgmtRpc->repairNode(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtCompRepair(const Handle& mgmtHandle, const std::string& compName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::RepairComponentRequest request;
   logDebug("MGMT","XXX","adding handle [%" PRIx64 ":%" PRIx64 "]", mgmtHandle.id[0],mgmtHandle.id[1]);
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   logDebug("MGMT","XXX","set comp name [%s]", compName.c_str());
   request.set_compname(compName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::RepairComponentResponse resp;
      logDebug("MGMT","INI","invoke repairComponent");
      amfMgmtRpc->repairComponent(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   logDebug("MGMT","XXX","exit [%s]", __FUNCTION__);
   return rc;
}

ClRcT amfMgmtSURepair(const Handle& mgmtHandle, const std::string& suName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::RepairSURequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_suname(suName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::RepairSUResponse resp;
      amfMgmtRpc->repairSU(remoteAmfHdl,&request,&resp);
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

ClRcT amfMgmtFinalize(const Handle& amfMgmtHandle, bool finalizeRpc)
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
      if(rc == CL_OK)
      {
          gAmfMgmtInitialized = CL_FALSE;
      }
      if (finalizeRpc)
      {
          delete amfMgmtRpc;
          amfMgmtRpc = NULL;
          delete amfMgmtRpcChannel;
          amfMgmtRpcChannel = NULL;
          rpcInitialized = CL_FALSE;
      }
    }
  catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
  return rc; 
}

ClRcT amfMgmtComponentGetConfig(const Handle& mgmtHandle,const std::string& compName, SAFplus::Rpc::amfMgmtRpc::ComponentConfig** compConfig)
{   
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetComponentConfigRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_compname(compName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetComponentConfigResponse resp; 
      amfMgmtRpc->getComponentConfig(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","COMP.GET.CONFIG","getComponentConfig [0x%x]", rc);
      if (rc == CL_OK)
      {
        *compConfig = resp.release_componentconfig();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtNodeGetConfig(const Handle& mgmtHandle,const std::string& nodeName, SAFplus::Rpc::amfMgmtRpc::NodeConfig** nodeConfig)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetNodeConfigRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_nodename(nodeName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetNodeConfigResponse resp; 
      amfMgmtRpc->getNodeConfig(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","GET.CFG","getNodeConfig [0x%x]", rc);
      if (rc == CL_OK)
      {
        *nodeConfig = resp.release_nodeconfig();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtSGGetConfig(const Handle& mgmtHandle,const std::string& sgName, SAFplus::Rpc::amfMgmtRpc::ServiceGroupConfig** sgConfig)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetSGConfigRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_sgname(sgName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetSGConfigResponse resp; 
      amfMgmtRpc->getSGConfig(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","GET.CFG","getSGConfig [0x%x]", rc);
      if (rc == CL_OK)
      {
        *sgConfig = resp.release_servicegroupconfig();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtSUGetConfig(const Handle& mgmtHandle,const std::string& suName, SAFplus::Rpc::amfMgmtRpc::ServiceUnitConfig** suConfig)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetSUConfigRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_suname(suName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetSUConfigResponse resp; 
      amfMgmtRpc->getSUConfig(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","GET.CFG","getSUConfig [0x%x]", rc);
      if (rc == CL_OK)
      {
        *suConfig = resp.release_serviceunitconfig();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtSIGetConfig(const Handle& mgmtHandle,const std::string& siName, SAFplus::Rpc::amfMgmtRpc::ServiceInstanceConfig** siConfig)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetSIConfigRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_siname(siName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetSIConfigResponse resp; 
      amfMgmtRpc->getSIConfig(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","GET.CFG","getSIConfig [0x%x]", rc);
      if (rc == CL_OK)
      {
        *siConfig = resp.release_serviceinstanceconfig();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtCSIGetConfig(const Handle& mgmtHandle,const std::string& csiName, SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceConfig** csiConfig)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetCSIConfigRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_csiname(csiName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetCSIConfigResponse resp; 
      amfMgmtRpc->getCSIConfig(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","GET.CFG","getCSIConfig [0x%x]", rc);
      if (rc == CL_OK)
      {
        *csiConfig = resp.release_componentserviceinstanceconfig();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtComponentGetStatus(const Handle& mgmtHandle,const std::string& compName, SAFplus::Rpc::amfMgmtRpc::ComponentStatus** compStatus)
{   
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetComponentStatusRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_compname(compName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetComponentStatusResponse resp; 
      amfMgmtRpc->getComponentStatus(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","COMP.GET.STATUS","getComponentStatus [0x%x]", rc);
      if (rc == CL_OK)
      {
        *compStatus = resp.release_componentstatus();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtNodeGetStatus(const Handle& mgmtHandle,const std::string& nodeName, SAFplus::Rpc::amfMgmtRpc::NodeStatus** nodeStatus)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetNodeStatusRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_nodename(nodeName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetNodeStatusResponse resp; 
      amfMgmtRpc->getNodeStatus(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","GET.CFG","getNodeStatus [0x%x]", rc);
      if (rc == CL_OK)
      {
        *nodeStatus = resp.release_nodestatus();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtSGGetStatus(const Handle& mgmtHandle,const std::string& sgName, SAFplus::Rpc::amfMgmtRpc::ServiceGroupStatus** sgStatus)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetSGStatusRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_sgname(sgName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetSGStatusResponse resp; 
      amfMgmtRpc->getSGStatus(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","GET.CFG","getSGStatus [0x%x]", rc);
      if (rc == CL_OK)
      {
        *sgStatus = resp.release_servicegroupstatus();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtSUGetStatus(const Handle& mgmtHandle,const std::string& suName, SAFplus::Rpc::amfMgmtRpc::ServiceUnitStatus** suStatus)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetSUStatusRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_suname(suName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetSUStatusResponse resp; 
      amfMgmtRpc->getSUStatus(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","GET.CFG","getSUStatus [0x%x]", rc);
      if (rc == CL_OK)
      {
        *suStatus = resp.release_serviceunitstatus();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtSIGetStatus(const Handle& mgmtHandle,const std::string& siName, SAFplus::Rpc::amfMgmtRpc::ServiceInstanceStatus** siStatus)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetSIStatusRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_siname(siName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetSIStatusResponse resp; 
      amfMgmtRpc->getSIStatus(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","GET.CFG","getSIStatus [0x%x]", rc);
      if (rc == CL_OK)
      {
        *siStatus = resp.release_serviceinstancestatus();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtCSIGetStatus(const Handle& mgmtHandle,const std::string& csiName, SAFplus::Rpc::amfMgmtRpc::ComponentServiceInstanceStatus** csiStatus)
{
   if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::GetCSIStatusRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_csiname(csiName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::GetCSIStatusResponse resp; 
      amfMgmtRpc->getCSIStatus(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
      logInfo("MGMT.CLI","GET.CFG","getCSIStatus [0x%x]", rc);
      if (rc == CL_OK)
      {
        *csiStatus = resp.release_componentserviceinstancestatus();
      }
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    } 
   return rc;
}

ClRcT amfMgmtNodeRestart(const Handle& mgmtHandle, const std::string& nodeName)

{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::NodeRestartRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_nodename(nodeName);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::NodeRestartResponse resp;
        amfMgmtRpc->nodeRestart(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

ClRcT amfMgmtSURestart(const Handle& mgmtHandle, const std::string& suName)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::ServiceUnitRestartRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_suname(suName);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::ServiceUnitRestartResponse resp;
        amfMgmtRpc->serviceUnitRestart(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

ClRcT amfMgmtCompRestart(const Handle& mgmtHandle, const std::string& compName)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::ComponentRestartRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_compname(compName);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::ComponentRestartResponse resp;
        amfMgmtRpc->componentRestart(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

ClRcT amfMgmtSGAdjust(const Handle& mgmtHandle, const std::string& sgName, bool enabled)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::AdjustSGRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_sgname(sgName);
    request.set_enabled(enabled);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::AdjustSGResponse resp;
        amfMgmtRpc->adjustSG(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

ClRcT amfMgmtSISwap(const Handle& mgmtHandle, const std::string& siName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::SwapSIRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_siname(siName);
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::SwapSIResponse resp;
      amfMgmtRpc->swapSI(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtCompErrorReport(const Handle& mgmtHandle, const std::string& compName, SAFplus::Rpc::amfMgmtRpc::Recovery recommendedRecovery)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::CompErrorReportRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_compname(compName);
    request.set_recommendedrecovery(recommendedRecovery);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::CompErrorReportResponse resp;
        amfMgmtRpc->compErrorReport(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

ClRcT amfMgmtNodeErrorReport(const Handle& mgmtHandle, const std::string& nodeName)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::NodeErrorReportRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_nodename(nodeName);
    request.set_shutdownamf(false);
    request.set_rebootnode(false);
    request.set_gracefulswitchover(true);
    request.set_restartamf(false);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::NodeErrorReportResponse resp;
        amfMgmtRpc->nodeErrorReport(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

ClRcT amfMgmtNodeErrorClear(const Handle& mgmtHandle, const std::string& nodeName)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::NodeErrorClearRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_nodename(nodeName);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::NodeErrorClearResponse resp;
        amfMgmtRpc->nodeErrorClear(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

ClRcT amfNodeJoin(const Handle& mgmtHandle, const std::string& nodeName)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc = amfMgmtNodeErrorClear(mgmtHandle, nodeName);
    return rc;
}

ClRcT amfMgmtNodeShutdown(const Handle& mgmtHandle, const std::string& nodeName)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::NodeErrorReportRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_nodename(nodeName);
    request.set_shutdownamf(true);
    request.set_rebootnode(false);
    request.set_gracefulswitchover(true);
    request.set_restartamf(false);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::NodeErrorReportResponse resp;
        resp.set_err(-1);
        amfMgmtRpc->nodeErrorReport(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

ClRcT amfNodeRestart(const Handle& mgmtHandle, const std::string& nodeName, bool graceful)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::NodeErrorReportRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_nodename(nodeName);
    request.set_shutdownamf(false);
    request.set_rebootnode(true);
    request.set_gracefulswitchover(graceful);
    request.set_restartamf(false);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::NodeErrorReportResponse resp;
        amfMgmtRpc->nodeErrorReport(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

ClRcT amfMiddlewareRestart(const Handle& mgmtHandle, const std::string& nodeName, bool graceful, bool nodeReset)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::NodeErrorReportRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_nodename(nodeName);
    request.set_shutdownamf(true);
    request.set_rebootnode(nodeReset);
    request.set_gracefulswitchover(graceful);
    request.set_restartamf(true);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::NodeErrorReportResponse resp;
        amfMgmtRpc->nodeErrorReport(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

ClRcT amfMgmtAssignSUtoSI(const Handle& mgmtHandle, const std::string& siName, const std::string& activeSUName, const std::string& standbySUName)
{
#ifdef HANDLE_VALIDATE
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
#endif
   ClRcT rc;

   const std::string& stringToCheckNULL = "objectIsNULL_";
   std::string getActiveSUNameDiffNull = "";
   std::string getStandbySUNameDiffNull = "";
   if(activeSUName.compare(0, stringToCheckNULL.size(), stringToCheckNULL) == 0)
   {
     getActiveSUNameDiffNull = activeSUName;
     getActiveSUNameDiffNull.erase(0, stringToCheckNULL.size());
     logDebug("MGMT","ASUI","getActiveSUNameDiffNull [%s]", getActiveSUNameDiffNull.c_str());

     SAFplus::Rpc::amfMgmtRpc::LockSUAssignmentRequest requestLock;
     requestLock.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
     requestLock.set_suname(getActiveSUNameDiffNull);
     Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
     try
     {
         SAFplus::Rpc::amfMgmtRpc::LockSUAssignmentResponse respLock;
         amfMgmtRpc->lockSUAssignment(remoteAmfHdl,&requestLock,&respLock);
         rc = (ClRcT)respLock.err();
     }
     catch(NameException& ex)
     {
         logError("MGMT","INI","getHandle got exception [%s]", ex.what());
         rc = CL_ERR_NOT_EXIST;
     }
     if(rc == CL_OK)
     {
         while(1)
         {
             // wait for 1 s
             boost::this_thread::sleep(boost::posix_time::milliseconds(2000));

             SAFplus::Rpc::amfMgmtRpc::ServiceUnitStatus* suStatus = NULL;
             SAFplus::Rpc::amfMgmtRpc::GetSUStatusRequest requestGet;
             requestGet.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
             requestGet.set_suname(getActiveSUNameDiffNull);
             try
             {
                 SAFplus::Rpc::amfMgmtRpc::GetSUStatusResponse respGet;
                 amfMgmtRpc->getSUStatus(remoteAmfHdl,&requestGet,&respGet);
                 rc = (ClRcT)respGet.err();
                 if (rc == CL_OK)
                 {
                     suStatus = respGet.release_serviceunitstatus();
                     if (suStatus->hastate() == ::SAFplus::Rpc::amfMgmtRpc::HighAvailabilityState::HighAvailabilityState_idle
                             && suStatus->presencestate() == ::SAFplus::Rpc::amfMgmtRpc::PresenceState::PresenceState_instantiated
                             && suStatus->hareadinessstate() == ::SAFplus::Rpc::amfMgmtRpc::HighAvailabilityReadinessState::HighAvailabilityReadinessState_notReadyForAssignment
                             && suStatus->readinessstate() == ::SAFplus::Rpc::amfMgmtRpc::ReadinessState::ReadinessState_outOfService)
                     {
                         SAFplus::Rpc::amfMgmtRpc::UnlockSURequest requestUnlock;
                         requestUnlock.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
                         requestUnlock.set_suname(getActiveSUNameDiffNull);
                         try
                         {
                             SAFplus::Rpc::amfMgmtRpc::UnlockSUResponse respUnlock;
                             amfMgmtRpc->unlockSU(remoteAmfHdl,&requestUnlock,&respUnlock);
                             rc = (ClRcT)respUnlock.err();
                         }
                         catch(NameException& ex)
                         {
                             logError("MGMT","INI","getHandle got exception [%s]", ex.what());
                             rc = CL_ERR_NOT_EXIST;
                         }
                         break;
                     }
                     else
                     {
                         logDebug("MGMT","ASUI","getActiveSUNameDiffNull [%s] ---- State is incorrect ----", getActiveSUNameDiffNull.c_str());
                     }
                 }
                 else
                 {
                     logDebug("MGMT","ASUI","getActiveSUNameDiffNull [%s] ---- Can not getSUStatus ----", getActiveSUNameDiffNull.c_str());
                 }
             }
             catch(NameException& ex)
             {
                 logError("MGMT","INI","getHandle got exception [%s]", ex.what());
                 rc = CL_ERR_NOT_EXIST;
             }
         }
     }
     else
     {
         logDebug("MGMT","ASUI","getActiveSUNameDiffNull [%s] ---- CL_ERR_INVALID_STATE ----", getActiveSUNameDiffNull.c_str());
     }
   }
   if(standbySUName.compare(0, stringToCheckNULL.size(), stringToCheckNULL) == 0)
   {
       getStandbySUNameDiffNull = standbySUName;
       getStandbySUNameDiffNull.erase(0, stringToCheckNULL.size());
       logDebug("MGMT","ASUI","getStandbySUNameDiffNull [%s]", getStandbySUNameDiffNull.c_str());

       SAFplus::Rpc::amfMgmtRpc::LockSUAssignmentRequest requestLock;
       requestLock.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
       requestLock.set_suname(getStandbySUNameDiffNull);
       Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
       try
       {
           SAFplus::Rpc::amfMgmtRpc::LockSUAssignmentResponse respLock;
           amfMgmtRpc->lockSUAssignment(remoteAmfHdl,&requestLock,&respLock);
           rc = (ClRcT)respLock.err();
       }
       catch(NameException& ex)
       {
           logError("MGMT","INI","getHandle got exception [%s]", ex.what());
           rc = CL_ERR_NOT_EXIST;
       }
       if(rc == CL_OK)
       {
           while(1)
           {
               // wait for 1 s
               boost::this_thread::sleep(boost::posix_time::milliseconds(2000));

               SAFplus::Rpc::amfMgmtRpc::ServiceUnitStatus* suStatus = NULL;
               SAFplus::Rpc::amfMgmtRpc::GetSUStatusRequest requestGet;
               requestGet.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
               requestGet.set_suname(getStandbySUNameDiffNull);
               try
               {
                   SAFplus::Rpc::amfMgmtRpc::GetSUStatusResponse respGet;
                   amfMgmtRpc->getSUStatus(remoteAmfHdl,&requestGet,&respGet);
                   rc = (ClRcT)respGet.err();
                   if (rc == CL_OK)
                   {
                       suStatus = respGet.release_serviceunitstatus();
                       if (suStatus->hastate() == ::SAFplus::Rpc::amfMgmtRpc::HighAvailabilityState::HighAvailabilityState_idle
                               && suStatus->presencestate() == ::SAFplus::Rpc::amfMgmtRpc::PresenceState::PresenceState_instantiated
                               && suStatus->hareadinessstate() == ::SAFplus::Rpc::amfMgmtRpc::HighAvailabilityReadinessState::HighAvailabilityReadinessState_notReadyForAssignment
                               && suStatus->readinessstate() == ::SAFplus::Rpc::amfMgmtRpc::ReadinessState::ReadinessState_outOfService)
                       {
                           SAFplus::Rpc::amfMgmtRpc::UnlockSURequest requestUnlock;
                           requestUnlock.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
                           requestUnlock.set_suname(getStandbySUNameDiffNull);
                           try
                           {
                               SAFplus::Rpc::amfMgmtRpc::UnlockSUResponse respUnlock;
                               amfMgmtRpc->unlockSU(remoteAmfHdl,&requestUnlock,&respUnlock);
                               rc = (ClRcT)respUnlock.err();
                           }
                           catch(NameException& ex)
                           {
                               logError("MGMT","INI","getHandle got exception [%s]", ex.what());
                               rc = CL_ERR_NOT_EXIST;
                           }
                           break;
                       }
                       else
                       {
                           logDebug("MGMT","ASUI","getStandbySUNameDiffNull [%s] ---- State is incorrect ----", getStandbySUNameDiffNull.c_str());
                       }
                   }
                   else
                   {
                       logDebug("MGMT","ASUI","getStandbySUNameDiffNull [%s] ---- Can not getSUStatus ----", getStandbySUNameDiffNull.c_str());
                   }
               }
               catch(NameException& ex)
               {
                   logError("MGMT","INI","getHandle got exception [%s]", ex.what());
                   rc = CL_ERR_NOT_EXIST;
               }
           }
       }
       else
       {
           logDebug("MGMT","ASUI","getStandbySUNameDiffNull [%s] ---- CL_ERR_INVALID_STATE ----", getStandbySUNameDiffNull.c_str());
       }
   }

   SAFplus::Rpc::amfMgmtRpc::AssignSUtoSIRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_siname(siName);


   if(getActiveSUNameDiffNull == "" && getStandbySUNameDiffNull == "")
   {
       request.set_activesuname(activeSUName);
       request.set_standbysuname(standbySUName);
   }
   else if(getActiveSUNameDiffNull == "" && getStandbySUNameDiffNull != "")
   {
       request.set_activesuname(activeSUName);
       request.set_standbysuname("");
   }
   else if(getActiveSUNameDiffNull != "" && getStandbySUNameDiffNull == "")
   {

       request.set_activesuname("");
       request.set_standbysuname(standbySUName);
   }
   else
   {
       return rc;
   }

   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::AssignSUtoSIResponse resp;
      amfMgmtRpc->assignSUtoSI(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtForceLockInstantiation(const Handle& mgmtHandle, const std::string& suName)
{
#ifdef HANDLE_VALIDATE
    if (!gAmfMgmtInitialized)
    {
        return CL_ERR_NOT_INITIALIZED;
    }
#endif
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::ForceLockInstantiationRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_suname(suName);
    Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
    try
    {
        SAFplus::Rpc::amfMgmtRpc::ForceLockInstantiationResponse resp;
        amfMgmtRpc->forceLockInstantiation(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

Handle amfMgmtCompAddressGet(const std::string& entityName)
{
    return name.getHandle(entityName, 2000);
}

ClRcT setSafplusInstallInfo(const Handle& mgmtHandle, const std::string& nodeName, const std::string& safplusInstallInfo)
{
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::SetSafplusInstallInfoRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_nodename(nodeName);
    request.set_safplusinstallinfo(safplusInstallInfo);
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::SetSafplusInstallInfoResponse resp;
        amfMgmtRpc->setSafplusInstallInfo(remoteAmfHdl,&request,&resp);
        rc = (ClRcT)resp.err();
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}
ClRcT getSafplusInstallInfo(const Handle& mgmtHandle, const std::string& nodeName, std::string& safplusInstallInfo)
{
    ClRcT rc;
    SAFplus::Rpc::amfMgmtRpc::GetSafplusInstallInfoRequest request;
    request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
    request.set_nodename(nodeName);    
    try
    {
        Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
        SAFplus::Rpc::amfMgmtRpc::GetSafplusInstallInfoResponse resp;
        amfMgmtRpc->getSafplusInstallInfo(remoteAmfHdl,&request,&resp);        
        rc = (ClRcT)resp.err();
        if (rc == CL_OK)
        {
           safplusInstallInfo = resp.safplusinstallinfo();
        }
    }
    catch(NameException& ex)
    {
        logError("MGMT","INI","getHandle got exception [%s]", ex.what());
        rc = CL_ERR_NOT_EXIST;
    }
    return rc;
}

}
