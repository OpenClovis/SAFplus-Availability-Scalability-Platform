#ifndef GroupServer_hxx
#define GroupServer_hxx

// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clGroup.hxx>
#include <clCpmApi.h>
#include "groupMessageHandler.hxx"
#include "clNodeCache.h"
#include <clIocIpi.h>
#include <clOsalApi.h>
#include <clHeapApi.h>

namespace SAFplusI
{
  typedef enum
  {
    MSG_NODE_JOIN,
    MSG_ROLE_NOTIFY,
    MSG_ELECT_REQUEST,
    MSG_UNDEFINED
  } GroupMessageTypeT;

  typedef enum
  {
    ROLE_ACTIVE,
    ROLE_STANDBY,
    ROLE_UNDEFINED
  } GroupRoleNotifyTypeT;

  typedef enum
  {
    SEND_BROADCAST,
    SEND_TO_MASTER,
    SEND_LOCAL_RR, //Round Robin
    SEND_UNDEFINED
  } GroupMessageSendModeT;

  class GroupMessageProtocol
  {
    public:
      GroupMessageTypeT     messageType;
      GroupRoleNotifyTypeT  roleType;
      char                  data[1]; //Not really 1, it will be place on larger memory
      GroupMessageProtocol()
      {
        messageType = MSG_UNDEFINED;
        roleType = ROLE_UNDEFINED;
      }
  };
  class GroupServer
  {
    public:
      static GroupServer* getInstance();
      void clGrpStartServer();
      void clGrpStopServer();
      SAFplus::Group *clusterNodeGrp;
      SAFplus::Group *clusterCompGrp;
      void nodeJoinFromMaster(GroupMessageProtocol *msg);
      void roleChangeFromMaster(GroupMessageProtocol *msg);
      void elect(ClBoolT isRequest = CL_TRUE,GroupMessageProtocol *msg = NULL);
      void fillSendMessage(void* data, GroupMessageTypeT msgType,GroupMessageSendModeT msgSendMode = SEND_BROADCAST, GroupRoleNotifyTypeT roleType = ROLE_UNDEFINED);
      void nodeJoin(ClIocNodeAddressT nAddress);
      void nodeLeave(ClIocNodeAddressT nAddress);
      void componentJoin(ClIocAddressT address);
      void componentLeave(ClIocAddressT address);
      void dumpClusterNodeGroup();
      static ClRcT initializeLibraries();
      static ClRcT finalizeLibraries();
      static SAFplus::SafplusMsgServer   *groupMsgServer;
      ClTimerHandleT               electTimerHandle;
      ClTimerHandleT               roleNotiTimerHandle;
      ClBoolT                      isElectTimerRunning;
      ClBoolT                      finished;
    protected:
      static GroupServer* instance;
      GroupServer();
      ~GroupServer();
    private:
      ClRcT getNodeInfo(ClIocNodeAddressT nAddress, SAFplus::GroupIdentity *grpIdentity, int pid = 0);
      bool  isMasterNode();
      bool  isActiveNode();
      void  sendNotification(void* data, int dataLength, GroupMessageSendModeT messageMode =  SEND_BROADCAST);
      SAFplus::EntityIdentifier createHandleFromAddress(ClIocNodeAddressT nAddress, int pid = 0);
  };

}
ClRcT roleNotificationTimer(void *arg);
ClRcT electRequestTimer(void *arg);
ClRcT iocNotificationCallback(ClIocNotificationT *notification, ClPtrT cookie);
void* gmsServiceThread(void *arg);
#endif
