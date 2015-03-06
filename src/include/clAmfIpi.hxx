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
    SaAmfCallbacksT_3 callbacks;

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
    AMF_CSI_REMOVE_ONE = 0x8
    };
  
  extern AmfSession* amfSession;
  }
