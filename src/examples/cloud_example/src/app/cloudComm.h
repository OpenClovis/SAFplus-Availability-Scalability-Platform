#define CLOUD_MGT_PORT 5743

enum
  {
    CLOUD_MSG_HDR_LEN = sizeof(uint16_t) + sizeof(uint16_t),  // magic + command

    CLOUD_MSG_HDR_ID = 0x4386,
    CLOUD_MSG_HDR_ID_SWAP = 0x8643,

    CLOUD_MSG_ADD_NODE = 1, // + Node name
    CLOUD_MSG_REM_NODE = 2, // + Node name

    CLOUD_MSG_ADD_APP = 3,  // + App binary , node names.  If the app does not exist, create it
    CLOUD_MSG_REM_APP = 4,   // + App binary , node names.  If this removes all nodes, delete the app

    CLOUD_MSG_OK = 1,
    CLOUD_MSG_NOT_EXISTING = 2,    
    
  };
