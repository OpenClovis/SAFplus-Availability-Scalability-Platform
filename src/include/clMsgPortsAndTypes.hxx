#pragma once
#include "clCustomization.hxx"

namespace SAFplusI
  {
  enum
    {
    START_IOC_PORT = 1,
    AMF_IOC_PORT = 1,
    LOG_IOC_PORT = 2,
    MGT_IOC_PORT = 3,

#ifdef SAFPLUS_AMF_GRP_NODE_REPRESENTATIVE  // If the AMF is handling the group server functionality, then the GMS port will be the same as the AMF port
    GMS_IOC_PORT = AMF_IOC_PORT,
#else  // otherwise give it a unique port
    GMS_IOC_PORT = 4,
#endif
    GMS_STANDALONE_IOC_PORT = 4, // GMS_IOC_PORT,
#ifdef SAFPLUS_AMF_FAULT_NODE_REPRESENTATIVE  // If the AMF is handling the fault server functionality, then the GMS port will be the same as the AMF port
    FAULT_IOC_PORT = AMF_IOC_PORT,
#else  // otherwise give it a unique port
    FAULT_IOC_PORT = 5,
#endif
    END_IOC_PORT = 5,

    };

  enum
    {
    HEARTBEAT_MSG_TYPE = 3, // must be == CL_IOC_PROTO_HB
    GRP_MSG_TYPE = 4,
    CL_MGT_MSG_TYPE = 5,
    CKPT_SYNC_MSG_TYPE = 6,
    CL_MGT_RPC_TYPE = 7,

    AMF_REQ_HANDLER_TYPE = 17,
    AMF_REPLY_HANDLER_TYPE = 18,
    OBJECT_MSG_TYPE = 19,
    CLOUD_DISCOVERY_TYPE = 20,
    AMF_APP_REQ_HANDLER_TYPE = 21,
    AMF_APP_REPLY_HANDLER_TYPE = 22,
    FAULT_MSG_TYPE = 23,

    // TODO: possibly misused:
    CL_IOC_SAF_MSG_REPLY_PROTO = 24

    };
  }
