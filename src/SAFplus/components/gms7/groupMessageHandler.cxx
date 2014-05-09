#include "clGroupIpi.hxx"

using namespace SAFplus;
using namespace SAFplusI;

GroupMessageHandler::GroupMessageHandler(SAFplus::Group *grp)
{
  mGroup = grp;
}
void GroupMessageHandler::msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
    /* If no communication needed, just ignore */
    if(mGroup == NULL)
    {
      return;
    }
    /* If message from me, just ignore */
    if(from.iocPhyAddress.nodeAddress == clIocLocalAddressGet())
    {
      logInfo("GMS","MSGHDL","Local message. Ignored");
      return;
    }
    /* Parse the message and process if it is valid */
    GroupMessageProtocol *rxMsg = (GroupMessageProtocol *)msg;
    if(rxMsg == NULL)
    {
      logError("GMS","MSGHDL","Received NULL message. Ignored");
      return;
    }
    switch(rxMsg->messageType)
    {
      case GroupMessageTypeT::MSG_NODE_JOIN:
        logDebug("GMS","MSGHDL","Node JOIN message");
        mGroup->nodeJoinHandle(rxMsg);
        break;
      case GroupMessageTypeT::MSG_NODE_LEAVE:
        logDebug("GMS","MSGHDL","Node LEAVE message");
        mGroup->nodeLeaveHandle(rxMsg);
        break;
      case GroupMessageTypeT::MSG_ROLE_NOTIFY:
        logDebug("GMS","MSGHDL","Role CHANGE message");
        mGroup->roleNotificationHandle(rxMsg);
        break;
      case GroupMessageTypeT::MSG_ELECT_REQUEST:
        logDebug("GMS","MSGHDL","Election REQUEST message");
        mGroup->electionRequestHandle(rxMsg);
        break;
      default:
        logDebug("GMS","MSGHDL","Unknown message type [%d] from %d",rxMsg->messageType,from.iocPhyAddress.nodeAddress);
        break;
    }

}
