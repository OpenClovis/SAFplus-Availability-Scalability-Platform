#if 0
#include <cloudDiscovery.hxx>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <clGroup.hxx>


  CloudDiscovery::CloudDiscovery()
    : group(NULL), msgServer(NULL), data(NULL), iAmWellKnown(false),
      mem(boost::interprocess::open_or_create, ClusterNodesSharedMemoryName, boost::interprocess::read_write)
  {
  
  }

  CloudDiscovery::CloudDiscovery(bool wellKnown, MsgServer* svr, int msgType)
  : mem(boost::interprocess::open_or_create, ClusterNodesSharedMemoryName, boost::interprocess::read_write), msgServer(svr)
  {
    iAmWellKnown = wellKnown;
    group = NULL;
    init(svr,msgType);
  }
 
  CloudDiscovery::CloudDiscovery(Group* grp, MsgServer* svr, int msgType)
  : mem(boost::interprocess::open_or_create, ClusterNodesSharedMemoryName, boost::interprocess::read_write), msgServer(svr)
  {
    group = grp;
    // We can use the ACCEPT_ACTIVE capability of the group to decide if this entity is "well-known" or not.  If the application did not set up the group in this way, its a bug.  In practice the AMF will almost always be running cloud discovery and the system controllers will be the "well-known" nodes so it all works out.
    iAmWellKnown = ((group->myInformation.capabilities & Group::ACCEPT_ACTIVE)!=0);  
    init(svr,msgType);
  }

  void CloudDiscovery::init(MsgServer* svr, int msgType)
  {
    gen = 0;  // 0 means system was reset so tell me my generation... or should the node persistently keep trace of its own generation (this code would get it from env var)?

    //? <cfg name="SAFPLUS_CLOUD_NODES">[OPTIONAL] A comma, colon or space separated list specifying the addresses of well-known nodes in this cloud-based cluster.  All nodes will contact one of these nodes to join the cluster and communicate with it to discover all other members.  The existence of this variable also selects cloud mode rather than LAN mode in transports that support both.  This variable must be the same for all nodes in the cloud because the order assigns node Ids.  The Nth address in this list will be assigned the Nth id (starting at 1).  To skip a node id, use an empty entry, i.e. "A , , B" will get you A as id 1 and B as id 3. </cfg>  
    char* cnVar = std::getenv("SAFPLUS_CLOUD_NODES");
    if (!cnVar)
      {
        throw ;  // Not using cloud mode
      }

   
    // Now let's hook up with messaging
    if(!msgServer)
      {
        msgServer = &safplusMsgServer;
      }
 
    // Callbacks can happen at any time after this is called so I better be ready!
    msgServer->RegisterHandler(msgType, this, (void*) 0);

    // Now let's contact known cluster nodes

    if (cnVar)
      {
        std::string wellKnownNodes(cnVar);
        char_separator<char> sep(",: ");
        tokenizer< char_separator<char> > tokens(wellKnownNodes, sep);
        int wkn=1;
        BOOST_FOREACH (const std::string& t, tokens) 
          {
            if (!t.empty())
              {         
                nodeArray.insert(wkn,t,ClusterNodeSharedMemory::UnknownGeneration);
                // We are passing in whether this node should behave as a well known node or not.  if (t == me) iAmWellKnown=true;
              }
            wkn++;
          }
      }

    // Join CloudDiscovery group.  This is a sticky group so I won't win the election unless maybe everyone is coming up at once.
    // If the user pre-initialized this object with a group, it is assumed that we have already registered, etc... an application would do this if it is reusing an existing group to also manage CloudDiscovery (the AMF CLUSTER_GROUP, for example)
    if (!group)
      {
        group = new SAFplus::Group();
        assert(group);
        group->init(CLOUD_DISCOVERY_GROUP,SAFplusI::GMS_IOC_PORT); //,execSemantics);
        Handle hdl = wellKnownEntity(CLOUD_DISCOVERY_GROUP, msgServer->handle.getNode(),msgServer->handle.getPort());
        // Register myself.  If I am well known, set the ACCEPT_STANDBY, ACCEPT_ACTIVE bits.
        int flags = Group::STICKY;
        if (iAmWellKnown) flags |= Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE;          
        group->registerEntity(hdl,SAFplus::ASP_NODEADDR,flags);
        
      }
    
  }


  void CloudDiscovery::sendHello()
  {
    Message*  m = msgServer->getMsgPool().allocMsg();
    MsgFragment* frag = m->append(sizeof(uint16_t));

    if (1)
      {
      MessageOStream  mos(m);
      mos << ((uint16_t) HELLO_MSG_ID_VERSION_1);
      mos << ((uint16_t) preferredId);
      mos << myAddress;
      }
    // Assuming that all the cluster cloud discovery entities are listening on the same port that I am configured to listen on...
    // Also, I can use the AllNodes broadcast mechanism because I stuck the well known nodes into shared memory during construction so the message transport will see them (provided that the caller handed a ClusterNodes object to the transport between construction and start).
    Handle broadcastDest = getProcessHandle(msgServer->port,Handle::AllNodes);  
    msgServer->SendMsg(broadcastDest, m,myMsgType);
  }

  void CloudDiscovery::start()
  {
    Handle active =  group->getActive();
    
    // Even if I'm well known, I shout a hello to the master so I will get the complete node list in response
    // But if I'm the active, I'd just be shouting to myself so what's the point?
    if (active != group->myInformation.id)
      {
        sendHello();
      }
  }

  void CloudDiscovery::msgHandler(MsgServer* svr, Message* msgHead, ClPtrT cookie)
  {
    Message* msg = msgHead;
    while(msg)
      {
        MessageIStream  mos(msg);

        uint16_t msgId;
        mos >> msgId;

        if ((msgId&255) == MSG_ID) // Oops the MSG_ID should be in the HIGH byte.  The sender is opposite endian!
          {
            mos.eswap = true;  // So swap the rest of the objects as I pull them out
            msgId = __builtin_bswap16(msgId);
          }

        switch (msgId)
          {
          case HELLO_MSG_ID_VERSION_1:
            {
              uint16_t preferredId;
              string s;
              mos >> preferredId;
              mos >> s;
              if (preferredId >= MaxNodes) preferredId = 0;  // ignore bad preferences.
              // enter this sender into our node database
              
              // reply to sender with all nodes if I am active
              // broadcast this sender's info to all nodes
            } break;
          case NODELIST_MSG_ID_VERSION_1:
            {

            } break;
          }
        msg = msg->nextMsg;
      }

  }


};
#endif
