#pragma once
#include <clDbg.hxx>
#include <clIocApi.h>
#include <clMsgHandler.hxx>
#include <clSafplusMsgServer.hxx>

namespace SAFplus
  {

    /** Resolve a handle to a messaging address */
    inline ClIocAddressT getAddress(SAFplus::Handle h)
      {
        ClIocAddressT ret;
        if (h.getType() == SAFplus::TransientHandle)
          {
            ret.iocPhyAddress.nodeAddress = h.getNode();
            ret.iocPhyAddress.portId = h.getProcess();
          }
        else
          {
            clDbgNotImplemented("Getting the address of some handle types");
          }
        return ret;
      }

  }
;
