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
    };
  };


/* Configuration parameters that are used internally */
namespace SAFplusI
  {
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

  /* AMF */
  enum
    {
    AmfMaxComponentServiceInstanceKeyValuePairs = 1024
    };



  /* UDP message transport */
  enum
    {
    UdpTransportMaxMsgSize = 65507,  // 65,535 - 8 byte UDP header - 20 byte IP header  (http://en.wikipedia.org/wiki/User_Datagram_Protocol).  This is defined here so you can artifically limit the packet size.
    UdpTransportNumPorts = 2048,  // Limit the ports to a range for no particular reason
    UdpTransportStartPort = 7000,  // Pick a random spot in the UDP port range so our ports don't overlap common services
    UdpTransportMaxMsg = 1024,
    UdpTransportMaxFragments = 1024,
    };

  /* SCTP message transport */
  enum
    {
    SctpTransportMaxMsgSize = 65503,  // 65,535 - 12 bytes SCTP header - 20 bytes IP header. This is defined here so you can artifically limit the packet size.
    SctpTransportNumPorts = 2048,  // Limit the ports to a range for no particular reason
    SctpTransportStartPort = 8000,  // Pick a random spot in the UDP port range so our ports don't overlap common services
    SctpTransportMaxMsg = 1024,
    SctpTransportMaxFragments = 1024,
    SctpMaxStream = 64,
    }; 
  
  enum
    {
    TcpTransportMaxMsgSize = 65475,  // 65,535 - 40 bytes TCP header - 20 bytes IP header. This is defined here so you can artifically limit the packet size.
    TcpTransportNumPorts = 2048,  // Limit the ports to a range for no particular reason
    TcpTransportStartPort = 9000,  // Pick a random spot in the TCP port range so our ports don't overlap common services
    TcpTransportMaxMsg = 1024,
    TcpTransportMaxFragments = 1024,
    TcpMaxStream = 64,
    }; 

  extern const char* defaultMsgTransport;  //? Specifies the default messaging transport plugin filename.  This can be overridden by an environment variable.

  };

#endif

