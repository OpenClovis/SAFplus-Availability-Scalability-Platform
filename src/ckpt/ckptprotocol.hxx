#include <clHandleApi.hxx>

namespace SAFplusI
{

  enum
  {
  CKPT_MSG_TYPE = 0x5873,  // Upper 16 of msgType
  CKPT_MSG_TYPE_BACKWARDS = 0x7358,  // Upper 16 of msgType

  CKPT_MSG_TYPE_SYNC_REQUEST_1 = 0x1,

  CKPT_MSG_TYPE_SYNC_MSG_1 = 0x100,
  CKPT_MSG_TYPE_SYNC_COMPLETE_1 = 0x101,
  CKPT_MSG_TYPE_SYNC_RESPONSE_1 = 0x102,

  CKPT_MSG_TYPE_UPDATE_MSG_1 = 0x200,

  CKPT_MSG_TYPE_ERROR_RESPONSE_1 = 0x800,
  };

  class CkptMsgHdr
    {
    public:
    SAFplus::Handle    checkpoint;
    uint32_t           msgType;
    };

  class CkptSyncRequest_1:public CkptMsgHdr
    {
    public:
    uint32_t  generation;
    uint32_t  changeNum;
    uint32_t  cookie;
    SAFplus::Handle    response;
    };

  class CkptSyncMsg:public CkptMsgHdr
    {
    public:
    uint32_t count;
    uint32_t  cookie;
    };

  class CkptSyncCompleteMsg:public CkptMsgHdr
    {
    public:
    uint32_t finalCount;
    uint32_t finalGeneration;
    uint32_t finalChangeNum;

    };


  class CkptErrorResponse_1:public CkptMsgHdr
    {
    public:
    uint32_t  cookie;
    uint32_t  error;
    };

  typedef CkptSyncRequest_1 CkptSyncRequest;
  typedef CkptErrorResponse_1 CkptErrorResponse;  

}
