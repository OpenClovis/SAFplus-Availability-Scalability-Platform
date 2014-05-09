
#ifndef GroupServer_hxx
#define GroupServer_hxx

// Standard includes
#include <string>
#include <boost/thread.hpp>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clGroup.hxx>
#include <clCpmApi.h>
#include "groupMessageHandler.hxx"
#include "clGroupIpi.hxx"
#include "clNodeCache.h"
#include <clIocIpi.h>
#include <clOsalApi.h>
#include <clHeapApi.h>

namespace SAFplusI
{

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
      void fillSendMessage(void* data, GroupMessageTypeT msgType,GroupMessageSendModeT msgSendMode = GroupMessageSendModeT::SEND_BROADCAST, GroupRoleNotifyTypeT roleType = GroupRoleNotifyTypeT::ROLE_UNDEFINED);
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
      boost::thread serviceThread;
      static GroupServer* instance;
      GroupServer();
      ~GroupServer();
    private:
      ClRcT getNodeInfo(ClIocNodeAddressT nAddress, SAFplus::GroupIdentity *grpIdentity, int pid = 0);
      bool  isMasterNode();
      bool  isActiveNode();
      void  sendNotification(void* data, int dataLength, GroupMessageSendModeT messageMode =  GroupMessageSendModeT::SEND_BROADCAST);
      SAFplus::EntityIdentifier createHandleFromAddress(ClIocNodeAddressT nAddress, int pid = 0);
  };

}
ClRcT roleNotificationTimer(void *arg);
ClRcT electRequestTimer(void *arg);
ClRcT iocNotificationCallback(ClIocNotificationT *notification, ClPtrT cookie);
void* gmsServiceThread(void *arg);
#endif

