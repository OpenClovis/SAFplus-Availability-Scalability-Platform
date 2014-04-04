#include "groupMessageHandler.hxx"

using namespace SAFplus;

extern void nodeJoinFromMaster(GroupMessageProtocolT *msg);
extern void roleChangeFromMaster(GroupMessageProtocolT *msg);
extern void nodeJoin(ClIocNodeAddressT nAddress);
extern void nodeLeave(ClIocNodeAddressT nAddress);
extern void componentJoin(ClIocAddressT *pAddress);
extern void componentLeave(ClIocAddressT *pAddress);
extern void elect();
#ifdef __TEST
extern ClRcT initializeClusterNodeGroup();
extern ClUint32T clAspLocalId;
#define logEmergency(area, context,M, ...)  fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logAlert(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logCritical(area, context,M,...)    fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logError(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logWarning(area, context,M,...)     fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logNotice(area, context,M, ...)     fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logInfo(area, context, M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logDebug(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logTrace(area, context,M,...)       fprintf(stderr, "GMS::%s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif // __TEST

void GroupMessageHandler::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
#ifdef __TEST
    /* Emulate message node join */
    if(from.iocPhyAddress.portId == 71)
    {
      initializeClusterNodeGroup();
      return;
    }
    /* Emulate message node leave */
    if(from.iocPhyAddress.portId == 72)
    {
      nodeLeave(3);
      return;
    }
    /* Emulate elect request */
    if(from.iocPhyAddress.portId == 73)
    {
      elect();
      return;
    }
#endif // __TEST

#ifndef __TEST
    if(from.iocPhyAddress.nodeAddress == clIocLocalAddressGet())
#else
    if(from.iocPhyAddress.nodeAddress == clAspLocalId)
#endif
    {
      logDebug("GMS","MSGHDL","Local message. Ignored");
      return;
    }
    GroupMessageProtocolT *rxMsg = (GroupMessageProtocolT *)msg;
    if(rxMsg == NULL)
    {
      logError("GMS","MSGHDL","Received NULL message. Ignored");
      return;
    }
    switch(rxMsg->messageType)
    {
      case NODE_JOIN_FROM_SC:
        logDebug("GMS","MSGHDL","Node join message from Master");
        nodeJoinFromMaster(rxMsg);
        break;
      case CLUSTER_NODE_ROLE_NOTIFY:
        logDebug("GMS","MSGHDL","Role change message from Master");
        roleChangeFromMaster(rxMsg);
        break;
      case NODE_JOIN_FROM_CACHE:
        logDebug("GMS","MSGHDL","Node join message from Cache");
        nodeJoin(*(ClIocNodeAddressT *)rxMsg->data);
      default:
        logDebug("GMS","MSGHDL","Unknown message type [%d] from %d",rxMsg->messageType,from.iocPhyAddress.nodeAddress);
        break;
    }

}
