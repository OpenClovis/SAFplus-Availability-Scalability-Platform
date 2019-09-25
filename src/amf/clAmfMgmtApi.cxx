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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
   ClRcT rc;
   SAFplus::Rpc::amfMgmtRpc::RepairComponentRequest request;
   request.add_amfmgmthandle((const char*) &mgmtHandle, sizeof(Handle));
   request.set_compname(compName);   
   try
    {
      Handle& remoteAmfHdl = name.getHandle(AMF_MASTER_HANDLE, 2000);
      SAFplus::Rpc::amfMgmtRpc::RepairComponentResponse resp;
      amfMgmtRpc->repairComponent(remoteAmfHdl,&request,&resp);
      rc = (ClRcT)resp.err();
    }
   catch(NameException& ex)
    {
      logError("MGMT","INI","getHandle got exception [%s]", ex.what());
      rc = CL_ERR_NOT_EXIST;
    }
   return rc;
}

ClRcT amfMgmtSURepair(const Handle& mgmtHandle, const std::string& suName)
{
  if (!gAmfMgmtInitialized)
   {
     return CL_ERR_NOT_INITIALIZED;
   }
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
