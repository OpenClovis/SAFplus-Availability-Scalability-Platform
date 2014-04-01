#include "groupMessageHandler.hxx"

using namespace SAFplus;

extern void nodeJoinFromMaster(GroupMessageProtocolT *msg);
extern void roleChangeFromMaster(GroupMessageProtocolT *msg);
extern void nodeJoin(ClIocAddressT *pAddress);
extern void nodeLeave(ClIocAddressT *pAddress);
extern void componentJoin(ClIocAddressT *pAddress);
extern void componentLeave(ClIocAddressT *pAddress);

void GroupMessageHandler::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
  int messageScope = *(int *)cookie;
  if(messageScope == GMS_MESSAGE)
  {
    GroupMessageProtocolT *rxMsg = (GroupMessageProtocolT *)msg;
    if(from.iocPhyAddress.nodeAddress == clIocLocalAddressGet())
    {
      logInfo("GMS","MSGHDL","Local message, ignore");
      return;
    }
    switch(rxMsg->messageType)
    {
      case CLUSTER_NODE_ARRIVAL:
        logInfo("GMS","MSGHDL","Node join message from Master");
        nodeJoinFromMaster(rxMsg);
        break;
      case CLUSTER_NODE_ROLE_NOTIFY:
        logInfo("GMS","MSGHDL","Role change message from Master");
        roleChangeFromMaster(rxMsg);
        break;
      default:
        logDebug("GMS","MSGHDL","Unsupported message");
        break;
    }
  }
  else if(messageScope == AMF_MESSAGE)
  {
    /* TODO: handle notification (node join/leave, component membership) from AMF server */
  }
  else
  {
    logError("GMS","MSGHDL","Unknown message scope");
  }

}
