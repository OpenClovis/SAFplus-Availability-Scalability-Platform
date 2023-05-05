#pragma once
#ifndef CL_AMF_IPI_HXX
#define CL_AMF_IPI_HXX
#include <clCommon.hxx>
#include <saAmf.h>
#include <clThreadApi.hxx>
#include <clHandleApi.hxx>

namespace SAFplusI
  {
  class AmfSession
    {
    public:
    enum
    {
    STRUCT_ID = 0xA3f5e55
    };
    uint32_t structId;  // Make sure we get a valid object from the app layer
    int readFd;
    int writeFd;
    SAFplus::Handle handle;
    bool finalize;
    SAFplus::ThreadSem dispatchCount;
    SaAmfCallbacksT callbacks;
    SaAmfCallbacksT_4 callbacks4;
    int safVersion;

    AmfSession()
      {
      finalize = false;
      dispatchCount.init(0);  // Number of threads dispatching
      readFd = -1;
      writeFd = -1;
      handle = SAFplus::INVALID_HDL;
      }
    };

  enum
    {
      AMF_CSI_REMOVE_ONE = 0x8,
      AMF_HA_OPERATION_REMOVE = 5, // HighAvailabilityState.hxx:HighAvailabilityState::quiescing + 1
    };
  
  enum CompAnnoucementType
  {
    ARRIVAL,
    DEPARTURE
  };

  struct CompAnnouncementPayload
  {
    char compName[256];
    char nodeName[256];
    SAFplus::Handle compHdl;
    SAFplus::Handle nodeHdl;
    CompAnnoucementType type;
  };

  extern AmfSession* amfSession;
  }

#endif
