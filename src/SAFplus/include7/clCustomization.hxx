/* This file contains configurable defaults for the entire set of SAFplus services */
#pragma once
#ifndef CL_CUSTOMIZATION_HXX
#define CL_CUSTOMIZATION_HXX
/* Configuration parameters that are part of the API */
namespace SAFplus
  {
    enum
    {
    Log2MaxNodes = 10,  // 2^10 = 1024 total nodes.
    MaxNodes = (1<<Log2MaxNodes)
    };
  };


/* Configuration parameters that are used internally */
namespace SAFplusI
  {

  /* LOGGING */
  enum
    {
    LogDefaultFileBufferSize = 16*1024,
    LogDefaultMessageBufferSize = 16*1024,
    };

  /* GROUP */
  enum
    {
    GroupSharedMemSize = 4 * 1024*1024,
    GroupMax           = 1024,  // Maximum number of groups
    GroupMaxMembers    = 1024,   // Maximum number of members in a group
    GroupElectionTimeMs = 5000  // Default Group election time in milliseconds
    };

  /* CHECKPOINT */
  enum
    {
    CkptMinSize = 4*1024,
    CkptDefaultSize = 64*1024,

    CkptMinRows = 2,
    CkptDefaultRows = 256,

    CkptSyncMsgStride = 128,   // Checkpoint sync messages will be either < this length OR have only one record.  That is, if a record is > this amount the message can be bigger.

    };



  };

#endif

