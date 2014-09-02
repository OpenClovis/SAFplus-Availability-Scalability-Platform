#pragma once
#include "clCustomization.hxx"

namespace SAFplusI
  {
  enum
    {
    AMF_IOC_PORT = 1,
    LOG_IOC_PORT = 2,
    MGT_IOC_PORT = 3,
#ifdef AMF_GRP_NODE_REPRESENTATIVE  // If the AMF is handling the group server functionality, then the GMS port will be the same as the AMF port
    GMS_IOC_PORT = AMF_IOC_PORT,
#else  // otherwise give it a unique port
    GMS_IOC_PORT = 4,
#endif
    };

  enum
    {
    // DO NOT USE MSG TYPES 1 and 2, 7, 10 these are IOC internal...
    AMF_REQ_HANDLER_TYPE = 17,
    AMF_REPLY_HANDLER_TYPE = 18,
    HEARTBEAT_MSG_TYPE = 3, // must be == CL_IOC_PROTO_HB
    GRP_MSG_TYPE = 4,
    CL_MGT_MSG_TYPE = 5,
    CKPT_SYNC_MSG_TYPE = 6,
    OBJECT_MSG_TYPE = 19,
    };
  }
