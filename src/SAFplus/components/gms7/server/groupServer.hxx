#ifndef groupServer_hxx
#define groupServer_hxx

// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clNameApi.hxx>
#include <clGroup.hxx>

namespace SAFplus
{

  typedef enum
  {
      CLUSTER_NODE_ARRIVAL,
      CLUSTER_NODE_LEAVE,
      CLUSTER_NODE_ELECT_LEADER_CHANGE,
      CLUSTER_NODE_ELECT_DEPUTY_CHANGE,
      CLUSTER_COMP_ARRIVAL,
      CLUSTER_COMP_LEAVE,
      CLUSTER_COMP_FAILED
  } messageTypeT;

  typedef struct
  {
    EntityIdentifier entity;
    bool             isLeader;
    bool             capabilities;
  } notificationData;

  typedef struct
  {
    messageTypeT messageType; //Component dead,join...Cluster dead/join/elect
    int groupId;
    int numberOfItems;
    notificationData *data;
  } messageProtocol;

  void nodeJoinHandle(ClIocAddressT *pAddress);
  void nodeLeaveHandle(ClIocAddressT *pAddress);
  int getDataFromNodeCache(ClIocNodeAddressT nodeAddress, int groupId,GroupIdentity &grpIdentity);
  EntityIdentifier getEntityIdentifierFromAddress(ClIocNodeAddressT nodeAddress);
  void registerCpmNotificationCallback();
  static void sendBroadcast(void* data, int dataLength);
  void registerAndStartMessageHandler();
}

#endif
