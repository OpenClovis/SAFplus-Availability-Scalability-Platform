#pragma once

namespace SAFplusI
  {
  enum
    {
    AMF_IOC_PORT = 1,
    LOG_IOC_PORT = 2,
    MGT_IOC_PORT = 3,
    };

  enum
    {
    AMF_REQ_HANDLER_TYPE = 17,
    AMF_REPLY_HANDLER_TYPE = 18,
    HEARTBEAT_MSG_TYPE = 3, // must be == CL_IOC_PROTO_HB
    GRP_MSG_TYPE = 4,
    CL_MGT_MSG_TYPE = 5,
    CKPT_SYNC_MSG_TYPE = 6,
    //CKPT_MSG_TYPE = 7,
    };
  }
