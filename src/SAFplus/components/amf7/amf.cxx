#include <cltypes.h>
#include <clGroup.hxx>

#include <saAis.h>
#include <saAmf.h>

namespace SAFplus
  {
  uint_t amfInitCount;
  Group            clusterGroup(SAFplus::Group::DATA_IN_CHECKPOINT);

  Handle           myHandle;  // This handle resolves to THIS process.
  };

using namespace SAFplus;

// IOC related globals
ClUint32T clAspLocalId = 0x1;
ClUint32T chassisId = 0x0;
ClBoolT   gIsNodeRepresentative = CL_FALSE;

extern "C"
{
  SaAisErrorT saAmfInitialize(SaAmfHandleT *amfHandle, const SaAmfCallbacksT *amfCallbacks, SaVersionT *version)
    {
    ClRcT rc;
    uint_t tmp = amfInitCount;
    amfInitCount++;
    if (tmp==0)
      {
      logInitialize();
      utilsInitialize();
      clAspLocalId  = SAFplus::ASP_NODEADDR;  // remove clAspLocalId
      // TODO: What about the IOC address/port?
      rc = clIocLibInitialize(NULL);
      assert(rc==CL_OK);
      logError("AMF","INI","Messaging initialization failed with SAFplus error [0x%x].  Most likely the messaging port is in use.", rc);
      if (rc != CL_OK) return SA_AIS_ERR_NO_RESOURCES;  // port is already taken
      // Register this component with the name service
      myHandle = Handle::create();
      name.set(SAFplus::ASP_COMPNAME,myHandle,NameRegistrar::MODE_NO_CHANGE);

      clusterGroup.init(CLUSTER_GROUP);
      // TODO: clusterGroup.setNotification(somethingChanged);
      }

    return SA_AIS_OK;
    }

SaAisErrorT saAmfFinalize(SaAmfHandleT amfHandle)
  {
  return SA_AIS_OK;
  }

SaAisErrorT saAmfComponentRegister(SaAmfHandleT amfHandle,const SaNameT *compName,const SaNameT *proxyCompName)
  {
  // TODO: Compare compName with ASP_COMPNAME if ASP_COMPNAME!=0

  // Send a message to AMF telling it that I am a particular component

  if (1) // (reply is ok)
    {
    if (SAFplus::ASP_COMPNAME[0]==0)  // compname is not set; did not come from AMF, so we'll set it now.
      saNameGet(SAFplus::ASP_COMPNAME, compName,CL_MAX_NAME_LENGTH);
    }
  }

SaAisErrorT saAmfComponentUnregister(SaAmfHandleT amfHandle, const SaNameT *compName, const SaNameT *proxyCompName)
  {
  // Send a message to AMF telling it that I am no longer the component
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
  return SA_AIS_OK;
  }

SaAisErrorT saAmfCSIQuiescingComplete(SaAmfHandleT amfHandle, SaInvocationT invocation, SaAisErrorT error)
  {
  return SA_AIS_OK;
  }



}
