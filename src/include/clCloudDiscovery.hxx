#pragma once

#include <clClusterNodes.hxx>
#include <clMsgHandler.hxx>
#include <clSafplusMsgServer.hxx>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <clMsgPortsAndTypes.hxx>
#include <clGroup.hxx>

namespace SAFplusI
{
  class CloudDiscovery:public SAFplus::MsgHandler
  {
  public:

    enum Messages
      {
        MSG_ID=0x58,
        HELLO_MSG_ID_VERSION_1 = 0x5811,
        NODELIST_MSG_ID_VERSION_1 = 0x5821
      };

    CloudDiscovery();
    CloudDiscovery(bool wellKnown=false, SAFplus::MsgServer* svr = NULL, int msgType = SAFplusI::CLOUD_DISCOVERY_TYPE);
    CloudDiscovery(SAFplus::Group* grp, SAFplus::MsgServer* svr=NULL, int msgType = SAFplusI::CLOUD_DISCOVERY_TYPE);

    //? Called by the constructors.  If you use 2-phase initialization, you must have initialized the group and iAmWellKnown variables before calling this function
    void init(SAFplus::MsgServer* svr, int msgType);

    //? Send out the initial hello message to let everybody know this node is here, and receive the existing node list
    void start(void);

    virtual void msgHandler(SAFplus::MsgServer* svr, SAFplus::Message* msg, ClPtrT cookie);


    //? Typically only used internally.  Broadcast the hello message to all other known nodes.
    void sendHello();

  protected:

    //boost::interprocess::shared_memory_object mem;
    //boost::interprocess::mapped_region        memRegion;
    //ClusterNodeSharedMemory* data;
    ClusterNodes nodes;      

    SAFplus::MsgServer*        msgServer;
    SAFplus::Group* group;

    // If the system restarts, the generation is different so restarts can be distinguished by the rest of the cluster from a momentary lapse in communication.
    uint64_t gen;
    int      myMsgType;
    bool     iAmWellKnown;
  };
 

};
