#pragma once
#include <clDbg.hxx>
#include <clIocApi.h>
#include <clMsgHandler.hxx>
#include <clSafplusMsgServer.hxx>

namespace SAFplus
{

/** Resolve a handle to a messaging address */
    ClIocAddressT getAddress(SAFplus::Handle h)
      {
      if (h.getType() == SAFplus::TransientHandle)
        {
          ClIocAddressT ret;
          ret.iocPhyAddress.nodeAddress = h.getNode();
          ret.iocPhyAddress.portId = h.getProcess();
          return ret;
        }
      else
        {
        clDbgNotImplemented("Getting the address of some handle types");
        }
      }
};
