#include "groupMessageHandler.hxx"

using namespace SAFplus;

extern void NodeJoinFromMaster(GroupMessageProtocolT *msg);
extern void RoleChangeFromMaster(GroupMessageProtocolT *msg);
extern void NodeJoin(ClIocAddressT *pAddress);
extern void NodeLeave(ClIocAddressT *pAddress);
extern void ComponentJoin(ClIocAddressT *pAddress);
extern void ComponentLeave(ClIocAddressT *pAddress);

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
        NodeJoinFromMaster(rxMsg);
        break;
      case CLUSTER_NODE_ROLE_NOTIFY:
        logInfo("GMS","MSGHDL","Role change message from Master");
        RoleChangeFromMaster(rxMsg);
        break;
      default:
        logDebug("GMS","MSGHDL","Unsupported message");
        break;
    }
  }
  else if(messageScope == AMF_MESSAGE)
  {

  }
  else
  {
    logError("GMS","MSGHDL","Unknown message scope");
  }

}
