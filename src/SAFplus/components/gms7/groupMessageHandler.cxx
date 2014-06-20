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
      logInfo("GMS","MSG","Local message. Ignored");
      return;
    }
    /* Parse the message and process if it is valid */
    GroupMessageProtocol *rxMsg = (GroupMessageProtocol *)msg;
    if(rxMsg == NULL)
    {
      logError("GMS","MSG","Received NULL message. Ignored");
      return;
    }
    switch(rxMsg->messageType)
    {
      case GroupMessageTypeT::MSG_NODE_JOIN:
        logDebug("GMS","MSG","Node JOIN message");
        mGroup->nodeJoinHandle(rxMsg);
        break;
      case GroupMessageTypeT::MSG_NODE_LEAVE:
        logDebug("GMS","MSG","Node LEAVE message");
        mGroup->nodeLeaveHandle(rxMsg);
        break;
      case GroupMessageTypeT::MSG_ROLE_NOTIFY:
        logDebug("GMS","MSG","Role CHANGE message");
        mGroup->roleNotificationHandle(rxMsg);
        break;
      case GroupMessageTypeT::MSG_ELECT_REQUEST:
        logDebug("GMS","MSG","Election REQUEST message");
        mGroup->electionRequestHandle(rxMsg);
        break;
      default:
        logDebug("GMS","MSG","Unknown message type [%d] from %d",rxMsg->messageType,from.iocPhyAddress.nodeAddress);
        break;
    }

}
