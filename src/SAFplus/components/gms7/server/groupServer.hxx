#ifndef groupServer_hxx
#define groupServer_hxx

// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clNameApi.hxx>
#include <clGroup.hxx>
using namespace SAFplus;
  typedef enum
  {
      CLUSTER_NODE_ARRIVAL,
      CLUSTER_NODE_LEAVE,
      CLUSTER_NODE_ELECT,
  } messageTypeT;

  typedef struct
  {
    GroupIdentity    grpIdentity;
    //bool             isLeader;
  } notificationData;

  typedef struct
  {
    messageTypeT messageType; //Component dead,join...Cluster dead/join/elect
    int groupId;
    int numberOfItems;
    notificationData *data;
  } messageProtocol;

  void entityJoinHandle(messageProtocol *rxMsg);
  void entityLeaveHandle(messageProtocol *rxMsg);
  void entityElectHandle();
#endif
