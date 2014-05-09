#include "groupMessageHandler.hxx"
#include "GroupServer.hxx"
#include "clGroupIpi.hxx"
using namespace SAFplus;
using namespace SAFplusI;


#ifdef __TEST
extern ClRcT initializeClusterNodeGroup();
extern ClUint32T clAspLocalId;
#endif
#ifdef __LOGPRINT
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

    if(from.iocPhyAddress.nodeAddress == clIocLocalAddressGet())
    {
      logDebug("GMS","MSGHDL","Local message. Ignored");
      return;
    }
    GroupMessageProtocol *rxMsg = (GroupMessageProtocol *)msg;
    if(rxMsg == NULL)
    {
      logError("GMS","MSGHDL","Received NULL message. Ignored");
      return;
    }
    switch(rxMsg->messageType)
    {
      case GroupMessageTypeT::MSG_NODE_JOIN:
        logDebug("GMS","MSGHDL","Node join message from Master");
        GroupServer::getInstance()->nodeJoinFromMaster(rxMsg);
        break;
      case GroupMessageTypeT::MSG_ROLE_NOTIFY:
        logDebug("GMS","MSGHDL","Role change message from Master");
        GroupServer::getInstance()->roleChangeFromMaster(rxMsg);
        break;
      case GroupMessageTypeT::MSG_ELECT_REQUEST:
        logDebug("GMS","MSGHDL","Election information request");
        GroupServer::getInstance()->elect(CL_FALSE,rxMsg);
        break;
      default:
        logDebug("GMS","MSGHDL","Unknown message type [%d] from %d",rxMsg->messageType,from.iocPhyAddress.nodeAddress);
        break;
    }

}
