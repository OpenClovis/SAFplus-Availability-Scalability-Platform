/* This file contains configurable defaults for the entire set of SAFplus services */
#pragma once
#ifndef CL_CUSTOMIZATION_HXX
#define CL_CUSTOMIZATION_HXX
/* Configuration parameters that are part of the API */

//? Indicates that the AMF will act as the one-per-node group membership shared memory maintainer, rather than using a standalone safplus_group process
#define SAFPLUS_AMF_GRP_NODE_REPRESENTATIVE

//? Indicates that the AMF will act as the one-per-node fault manager, rather than using a standalone safplus_fault process
#define SAFPLUS_AMF_FAULT_NODE_REPRESENTATIVE

//? Define this if you want the logging system to be cluster wide.  Comment it out to make the logging system local to the node.
#define SAFPLUS_CLUSTERWIDE_LOG

namespace SAFplus
  {
    enum
    {
    Log2MaxNodes = 10,  // 2^10 = 1024 total nodes.
    MaxNodes = (1<<Log2MaxNodes),
    CL_MAX_NAME_LENGTH=256  //? Maximum length of names in the system
    };

  /* Messaging */
  enum
    {
    MsgAppMaxThreads = 2,   //? Default maximum number of message processing threads for applications
    MsgAppQueueLen   = 25   //? Default maximum queue size for message processing
    };

  /* Messaging */
  enum
    {
      MsgTransportAddressMaxLen = 64, //? The maximum number of bytes allowed for any message transport's physical address (i.e. IP address, TIPC address, etc).  This is needed to store node addresses in shared memory when using cloud mode clustering.

      MsgSafplusReservedPorts  = 32,  //? The number of ports reserved for use by SAFplus applications.  Ports reserved are: 0 -> SAFplusReservedPorts-1.  DO NOT CHANGE
      MsgApplicationPortStart  = MsgSafplusReservedPorts,  //? The "well-known" ports reserved for your use.
      MsgApplicationPortEnd    = MsgApplicationPortStart + 100,  //? The last "well-known" port reserved for your use
      MsgDynamicPortStart      = MsgApplicationPortEnd+1,  //? The AMF will dynamically allocate ports when it starts up an application from this group.
      MsgDynamicPortEnd        = MsgDynamicPortStart+512,  //? The AMF will dynamically allocate ports when it starts up an application from this group.  This is the largest port in the dynamic group.
    };
  };


/* Configuration parameters that are used internally */
namespace SAFplusI
  {

  /* Messaging */
  enum
    {
    MsgSafplusSendReplyRetryInterval = 4000,   //? Default time to wait before retrying a message that expects a reply (uses sendReply API)
    RpcRetryInterval = 4000,  //? Default time to wait before retrying a RPC that expects a reply
    };

  /* THREADS */
  enum
    {
    ThreadPoolTimerInterval = 60, //? Seconds between checks that threads are not hung
    ThreadPoolIdleTimeLimit = 50, //? If a thread has had no tasks for this long, let it quit
    };

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
    GroupElectionTimeMs = 3000  // Default Group election time in milliseconds
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

  /* FAULT */
  enum
    {
    MAX_FAULT_BUFFER_SIZE = 1024*1024,
    MAX_FAULT_DEPENDENCIES = 5,
    FaultSharedMemSize = 4 * 1024*1024,
    FaultMaxMembers    = 1024,   // Maximum number of fault entity
    };

  /* MGT */
  enum
    {
      MgtToStringRecursionDepth = 64,  //? When converting an object with children to XML this the maximum child depth.  By default this is set to a depth that should exceed any reasonable management hierarchy, but not be infinite in case the MGT tree has a cycle.
      MgtHistoryArrayLen = 1000 //? How many samples should be kept for the history of MgtHistoryStat statistic objects
    };


  /* AMF */
  enum
    {
    AmfMaxComponentServiceInstanceKeyValuePairs = 1024
    };



  /* UDP message transport */
  enum
    {
    UdpTransportMaxMsgSize = 65507,  // 65,535 - 8 byte UDP header - 20 byte IP header  (http://en.wikipedia.org/wiki/User_Datagram_Protocol).  This is defined here so you can artifically limit the packet size.
    UdpTransportNumPorts = 256,  // Limit the ports to a range for no particular reason
    UdpTransportStartPort = 21500,  // Pick a random spot in the UDP port range so our ports don't overlap common services
    UdpTransportMaxMsg = 1024,
    UdpTransportMaxFragments = 1024,
    };

  /* SCTP message transport */
  enum
    {
    SctpTransportMaxMsgSize = 65503,  // 65,535 - 12 bytes SCTP header - 20 bytes IP header. This is defined here so you can artifically limit the packet size.
    SctpTransportNumPorts = 2048,  // Limit the ports to a range for no particular reason
    SctpTransportStartPort = 18000,  // Pick a random spot in the UDP port range so our ports don't overlap common services
    SctpTransportMaxMsg = 1024,
    SctpTransportMaxFragments = 1024,
    SctpMaxStream = 64,
    }; 
  
  enum
    {
    TcpTransportMaxMsgSize = 10*1024*1024,  // This is defined here so you can artifically limit the packet size.
    TcpTransportNumPorts = 2048,  // Limit the ports to a range for no particular reason
    TcpTransportStartPort = 19000,  // Pick a random spot in the TCP port range so our ports don't overlap common services
    TcpTransportMaxMsg = 1024,
    TcpTransportMaxFragments = 2048,
    TcpMaxStream = 64,
    }; 

  /* TIPC message transport */
    enum
    {
    TipcTransportMaxMsgSize = 65996, // TIPC_MAX_USER_MSG_SIZE  66000U - 4 bytes reserved for IP header
    TipcTransportNumPorts = 2048,  // Limit the ports to a range for no particular reason
    TipcTransportStartPort = 20000,  // Pick a random spot in the TIPC port range so our ports don't overlap common services
    TipcTransportMaxMsg = 1024,
    TipcTransportMaxFragments = 2048,
    };

  extern const char* defaultMsgTransport;  //? Specifies the default messaging transport plugin filename.  This can be overridden by an environment variable.
  extern const char* defaultDbalPluginFile;  //? Specifies the default dbal plugin filename.  This can be overridden by an environment variable.


  /* ckpt default retention duration */
  enum 
  {
    CkptUpdateDuration = 60, /* This is the configured duration in second for which the program update the checkpoint, means to get changed checkpoint parameters such as last used time or there is any new checkpoint added */
    CkptRetentionDurationDefault = 28800, /* This is the default retention duration in second (8*60*60) for the retention timer to decide if a checkpoint data is deleted from memory */
  };

  /* ckpt peristent parameters */
  enum 
  {
    CkptMaxKeySize = 1000, /* Maximum size of the key (in bytes) to be stored in the database. This parameter is ignored in case of Berkeley and GDBM databases */
    CkptMaxRecordSize = 40000, /* Maximum size of the record (in bytes) to be stored in the database. This parameter is ignored in case of Berkeley and GDBM databases */
  };

  };

#endif

