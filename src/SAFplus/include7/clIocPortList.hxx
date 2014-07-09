#pragma once

namespace SAFplusI
  {
  enum
    {
    AMF_IOC_PORT = 1,
    LOG_IOC_PORT = 2,
    };

  enum
    {
    AMF_REQ_HANDLER_TYPE = 17,
    AMF_REPLY_HANDLER_TYPE = 18,
    HEARTBEAT_MSG_TYPE = 3, // must be == CL_IOC_PROTO_HB
    GRP_MSG_TYPE = 4,
    CKPT_SYNC_MSG_TYPE = 5,
    };
  }
