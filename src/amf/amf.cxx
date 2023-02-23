#include <cltypes.h>
#include <clGroupApi.hxx>

#include <saAis.h>
#include <saAmf.h>

#include <amfAppRpc.hxx>
#include <clRpcChannel.hxx>

#include <clAmfIpi.hxx>
#include <clLogIpi.hxx>

namespace SAFplus
  {
  uint_t           amfInitCount=0;
  Group            clusterGroup;
  Handle           myHandle;  // This handle resolves to THIS process.

  SafplusInitializationConfiguration   serviceConfig;
  SAFplus::LibDep::Bits                requiredServices = (SAFplus::LibDep::Bits) (SAFplus::LibDep::GRP | SAFplus::LibDep::NAME | SAFplus::LibDep::CKPT | SAFplus::LibDep::LOG);


  SAFplus::Rpc::amfAppRpc::amfAppRpcImpl amfAppRpcServer;
  SAFplus::Rpc::RpcChannel *amfRpcChannel=NULL;
  SAFplus::Rpc::amfAppRpc::amfAppRpc_Stub *amfAppRpc=NULL;
  };


SAFplusI::AmfSession* SAFplusI::amfSession = NULL;

using namespace SAFplus;
using namespace SAFplusI;


extern "C"
{

  SaAmfCallbacksT_3& set (SaAmfCallbacksT_3& lhs, const SaAmfCallbacksT& rhs)
  {
    lhs.saAmfHealthcheckCallback = rhs.saAmfHealthcheckCallback;
    lhs.saAmfComponentTerminateCallback = rhs.saAmfComponentTerminateCallback;
    lhs.saAmfCSISetCallback = rhs.saAmfCSISetCallback;
    lhs.saAmfCSIRemoveCallback = rhs.saAmfCSIRemoveCallback;
    lhs.saAmfProtectionGroupTrackCallback = rhs.saAmfProtectionGroupTrackCallback;
    lhs.saAmfProxiedComponentInstantiateCallback = rhs.saAmfProxiedComponentInstantiateCallback;
    lhs.saAmfProxiedComponentCleanupCallback = rhs.saAmfProxiedComponentCleanupCallback;

    lhs.saAmfContaintedComponentInstantiateCallback = NULL;
    lhs.saAmfContaintedComponentCleanupCallback = NULL;

  }


  static SaAisErrorT  commonInitialize(SaAmfHandleT *amfHandle,AmfSession* ret)
    {
    uint_t tmp = amfInitCount;
    amfInitCount++;

    if (tmp==0)
      {
      safplusInitialize(requiredServices, serviceConfig);
      myHandle = Handle::create();
      amfSession = ret;
      ret->handle = myHandle; // first AmfSession created is going to be the default one
      clusterGroup.init(CLUSTER_GROUP,"safplusCluster");
      // TODO: clusterGroup.setNotification(somethingChanged);

      // Connect the AMF RPC server to the messaging port so that this component can start processing incoming AMF requests
      amfRpcChannel = new SAFplus::Rpc::RpcChannel(&safplusMsgServer, &amfAppRpcServer);
      amfRpcChannel->setMsgType(AMF_APP_REQ_HANDLER_TYPE,AMF_APP_REPLY_HANDLER_TYPE);
      amfAppRpc = new SAFplus::Rpc::amfAppRpc::amfAppRpc_Stub(amfRpcChannel);
      }
    else
      {
      assert(!"Right now, only 1 session is supported per process");  // need to figure out the session in the amfAppRpc call before we can do multiple sessions 
      ret->handle = Handle::create();
      }
    *amfHandle = (SaAmfHandleT) ret;  // Cast the session pointer into the handle to make it opaque as per SAF specs
    ret->structId = AmfSession::STRUCT_ID;
    return SA_AIS_OK;
    }

  SaAisErrorT saAmfInitialize_4(SaAmfHandleT *amfHandle, const SaAmfCallbacksT_4 *amfCallbacks, SaVersionT *ver)
    {
    assert(amfHandle);
    assert(amfCallbacks);
    ClRcT rc;
    AmfSession* ret = new AmfSession;
    
    ret->callbacks4 = *amfCallbacks;
    ret->safVersion = 4;
    return commonInitialize(amfHandle,ret);
    }

  SaAisErrorT saAmfInitialize(SaAmfHandleT *amfHandle, const SaAmfCallbacksT *amfCallbacks, SaVersionT *ver)
    {
    assert(amfHandle);
    assert(amfCallbacks);
    ClRcT rc;
    AmfSession* ret = new AmfSession;
    
    ret->callbacks = *amfCallbacks;
    ret->safVersion = 3;
    return commonInitialize(amfHandle, ret);
    }


SaAisErrorT saAmfFinalize(SaAmfHandleT amfHandle)
  {
  assert(amfHandle);
  assert(amfInitCount==1);
  AmfSession* sess = (AmfSession*) amfHandle;
  assert(sess->structId ==  AmfSession::STRUCT_ID);
  sess->finalize=1;
  if (sess->readFd != -1)
    {
    close(sess->readFd);
    sess->readFd = -1;
    }
  if (sess->writeFd != -1)
    {
    close(sess->writeFd);
    sess->writeFd = -1;
    }
  sess->dispatchCount.blockUntil(0);

  if (amfInitCount == 1)
    safplusFinalize();
  sess->structId = 0xde1e1ed;
  delete sess;

  amfInitCount--;
  return SA_AIS_OK;
  }

SaAisErrorT saAmfSelectionObjectGet(SaAmfHandleT amfHandle, SaSelectionObjectT *selectionObject)
{
  assert(selectionObject);
  assert(amfHandle);
  AmfSession* sess = (AmfSession*) amfHandle;
  assert(sess->structId ==  AmfSession::STRUCT_ID);

  if (sess->readFd == -1)
    {
    int fds[2];
    int err;
    err = pipe(fds);
    assert(err == 0);
    sess->readFd = fds[0];
    sess->writeFd = fds[1];

    int flags = fcntl(fds[0], F_GETFL, 0);
    fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);
    }

  *selectionObject = sess->readFd;
  return SA_AIS_OK;
}

SaAisErrorT saAmfDispatch(SaAmfHandleT amfHandle, SaDispatchFlagsT dispatchFlags)
  {
  assert(amfHandle);
  AmfSession* sess = (AmfSession*) amfHandle;
  assert(sess->structId == AmfSession::STRUCT_ID);

  sess->dispatchCount.unlock();
  do
    {
    int readSomething;
    do
      {
      char c;
      readSomething = read(sess->readFd, (void *) &c, 1);
      if (readSomething)
        {
        }
      } while ((dispatchFlags == SA_DISPATCH_ALL)&&(!sess->finalize)&&(readSomething));

    } while ((dispatchFlags == SA_DISPATCH_BLOCKING)&&(!sess->finalize));  // DISPATCH_BLOCKING means to handle all incoming reqs until AMF finalized

  sess->dispatchCount.lock();
  }


SaAisErrorT saAmfComponentRegister(SaAmfHandleT amfHandle,const SaNameT *compName,const SaNameT *proxyCompName)
  {
  // Compare compName with ASP_COMPNAME if ASP_COMPNAME!=0.  If they are not equal, big problem AMF thinks it is starting a different comp than the app.
    Handle compHdl;
    if ((compName != 0) && (SAFplus::ASP_COMPNAME[0]!=0) && (strncmp((const char*) compName->value,SAFplus::ASP_COMPNAME,std::min((unsigned int) SA_MAX_NAME_LENGTH,(unsigned int) sizeof(SAFplus::ASP_COMPNAME)))!=0))
    {
       try
       {
          compHdl = name.getHandle(SAFplus::ASP_COMPNAME);
          compHdl = Handle::create();
       }
       catch (SAFplus::NameException& n)
       {       
          logWarning("AMF","INI","Component name [%s] does not match AMF expectation [%s].  Using passed name.", (const char*) compName->value, SAFplus::ASP_COMPNAME);
          saNameGet(SAFplus::ASP_COMPNAME, compName,CL_MAX_NAME_LENGTH);          
       }
    }

  // Maybe send a message to AMF telling it that I am a particular component, for now name registration is used.

  if (1) // (reply is ok)
    {
    if (SAFplus::ASP_COMPNAME[0]==0)  // compname is not set; did not come from AMF, so we'll set it now.
      {
        if (compName == 0)
          {
          logCritical("AMF","INI","Component name was not supplied by AMF or passed into saAmfComponentRegister.  Cannot register as a component.\n");
          return SA_AIS_ERR_INVALID_PARAM;
          }
      saNameGet(SAFplus::ASP_COMPNAME, compName,CL_MAX_NAME_LENGTH);
      }

    // TODO: read the name and log a warning if it is not INVALID_HDL
    if (compHdl == INVALID_HDL)
    {
      logInfo("AMF","INI","Registering component name [%s] as handle [%" PRIx64 ":%" PRIx64 "]",      SAFplus::ASP_COMPNAME, myHandle.id[0],myHandle.id[1]);
      name.set(SAFplus::ASP_COMPNAME,myHandle,NameRegistrar::MODE_NO_CHANGE);
      name.setLocalObject(myHandle,(void*) amfHandle);
      name.setLocalObject(SAFplus::ASP_COMPNAME,(void*) amfHandle);
    }
    else
    {
      logInfo("AMF","INI","Registering component name [%s] as handle [%" PRIx64 ":%" PRIx64 "]", compName->value, compHdl.id[0],compHdl.id[1]);
      name.set((const char*)compName->value,compHdl,NameRegistrar::MODE_NO_CHANGE);
      name.setLocalObject(compHdl,(void*) amfHandle);
    }    
  }
  return SA_AIS_OK;
  }

SaAisErrorT saAmfComponentUnregister(SaAmfHandleT amfHandle, const SaNameT *compName, const SaNameT *proxyCompName)
  {
  // Maybe send a message to AMF telling it that I am no longer the component, for now just remove the name from the name server
  logInfo("AMF","INI","Unregistering component name [%s] as invalid handle", SAFplus::ASP_COMPNAME);
  name.set(SAFplus::ASP_COMPNAME,INVALID_HDL,NameRegistrar::MODE_NO_CHANGE);
  return SA_AIS_OK;
  }

SaAisErrorT saAmfComponentNameGet(SaAmfHandleT amfHandle,SaNameT *compName)
  {
  assert(compName);

  if (SAFplus::ASP_COMPNAME[0])
    {
    SAFplus::saNameSet(compName,SAFplus::ASP_COMPNAME);
    return SA_AIS_OK;
    }
  else  // Application was run outside of AMF, and has not yet been initialized.  You need to supply the name.
    {
    compName->length = 0;
    compName->value[0] = 0;
    return SA_AIS_ERR_UNAVAILABLE;
    }
  }

SaAisErrorT saAmfResponse(SaAmfHandleT amfHandle, SaInvocationT invocation, SaAisErrorT error)
  {
  SAFplus::Rpc::amfAppRpc::WorkOperationResponseRequest resp;
  assert(amfAppRpc);
  resp.set_invocation(invocation);
  resp.set_result(error);

  // Invocation is coded with the node address and ioc port
  uint nodeAddress = (invocation >> (32+16));
  uint portId      = (invocation >> 32) & 0xFFFF;
  Handle amfHdl(SAFplus::TransientHandle,0,portId,nodeAddress);  // Ok, cheating here a little.  I know that the workOperationResponse function does not really need a handle.  It just needs the port and node from the handle.  So I create a fake handle with the correct fields

  amfAppRpc->workOperationResponse(amfHdl,&resp);
  return SA_AIS_OK;
  }

SaAisErrorT saAmfCSIQuiescingComplete(SaAmfHandleT amfHandle, SaInvocationT invocation, SaAisErrorT error)
{
    saAmfResponse(amfHandle, invocation, error);
    return SA_AIS_OK;
}



}
