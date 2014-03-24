#ifndef groupServer_hxx
#define groupServer_hxx

// Standard includes
#include <string>

// SAFplus includes
#include <clHandleApi.hxx>
#include <clGroup.hxx>

typedef enum
{
  CLUSTER_NODE_ARRIVAL,
  CLUSTER_NODE_LEAVE,
  CLUSTER_NODE_ELECT,
} messageTypeT;

typedef struct
{
  messageTypeT  messageType;
  int           groupId;
  int           numberOfItems;
  SAFplus::GroupIdentity  grpIdentity;
} messageProtocol;

void entityJoinHandle(messageProtocol *rxMsg);
void entityLeaveHandle(messageProtocol *rxMsg);
void entityElectHandle();


#endif
