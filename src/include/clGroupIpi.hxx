#ifndef clGroupIpi_hxx
#define clGroupIpi_hxx

#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/unordered_map.hpp>     //boost::unordered_map
#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash

#include <clCustomization.hxx>
#include <clMsgApi.hxx>
#include <clGroupApi.hxx>
#include <clGlobals.hxx>
#include <clLogIpi.hxx>


namespace SAFplusI
{
  enum
  {
  CL_GROUP_BUFFER_HEADER_STRUCT_ID_7 = 0x284ad233
  };

  enum class GroupMessageTypeT
  {
    MSG_ENTITY_JOIN = 1,     // Happens first time the entity joins the group
    MSG_ENTITY_LEAVE,
    MSG_HELLO,         // Just like the NODE_JOIN but happens at any time -- whenever an election request happens for example.
    MSG_GROUP_ANNOUNCE,
    MSG_ROLE_NOTIFY,
    MSG_ELECT_REQUEST,
    MSG_UNDEFINED
  };

  enum class GroupRoleNotifyTypeT
  {
    ROLE_ACTIVE,
    ROLE_STANDBY,
    ROLE_UNDEFINED
  };


  class GroupMessageProtocol
  {
    public:
      SAFplus::Handle       group;
      GroupMessageTypeT     messageType;
      GroupRoleNotifyTypeT  roleType;
      bool                  force; //When role type change, force receiver to apply new roles without checking
      char                  data[1]; //Not really 1, it will be place on larger memory
      GroupMessageProtocol()
      {
        messageType = GroupMessageTypeT::MSG_UNDEFINED;
        roleType = GroupRoleNotifyTypeT::ROLE_UNDEFINED;
        force  = false;
      }
  };

  class GroupShmHeader
    {
    public:
    uint64_t       structId;
    pid_t          rep;         // This is the node representative
    uint32_t       repPort;     // Node representative communications port
    uint64_t       repWatchdog; // Node representative periodically writes the current time to this variable so other apps are sure it is still active
    };

  class GroupData
    {
    public:
    enum // Flags
      {
      AUTOMATIC_ELECTION = 1,
      ELECTION_IN_PROGRESS = 2,
      };

    SAFplus::Handle hdl;  // Should be part of the GroupSharedMemEntry not GroupData because it is not changed
    uint32_t flags;
    uint32_t electionTimeMs; //? Election time in milliseconds
    uint16_t activeIdx;  // Index into the members array to find the active member
    uint16_t standbyIdx; // Index into the members array to find the standby member
    uint64_t lastChanged;
    uint64_t retentionTime; // How long to hold the group open even after all members have left
    uint64_t changeCount;
    uint64_t numMembers;
    SAFplus::GroupIdentity members[::SAFplusI::GroupMaxMembers];

    const SAFplus::GroupIdentity* find(SAFplus::Handle entity) const
    {
    for (int i=0;i<numMembers;i++)
      {
      if (entity == members[i].id)
        {
        return &members[i];
        }
      }
    return NULL;
    }

    SAFplus::GroupIdentity* find(SAFplus::Handle entity)
    {
    for (int i=0;i<numMembers;i++)
      {
      if (entity == members[i].id)
        {
        return &members[i];
        }
      }
    return NULL;
    }

    bool setActive(SAFplus::Handle entity)
    {
    if (activeIdx != 0xffff)
      {
      members[activeIdx].capabilities &= ~SAFplus::Group::IS_ACTIVE;
      activeIdx = 0xffff;
      }
    for (int i=0;i<numMembers;i++)
      {
      if (entity == members[i].id)
        {
        activeIdx = i;
        if (standbyIdx == i) standbyIdx = 0xffff;  // Standby is being promoted to active...
        members[activeIdx].capabilities &= ~SAFplus::Group::IS_STANDBY;
        members[activeIdx].capabilities |= SAFplus::Group::IS_ACTIVE;
        return true;
        }
      }
    return false;   // We didn't have knowledge of the member that took active...
    }

    bool setStandby(SAFplus::Handle entity)
    {
    if (standbyIdx != 0xffff)
      {
      members[standbyIdx].capabilities &= ~SAFplus::Group::IS_STANDBY;
      standbyIdx = 0xffff;
      }
    for (int i=0;i<numMembers;i++)
      {
      if (entity == members[i].id)
        {
        standbyIdx = i;
        if (activeIdx == i) activeIdx = 0xffff;  // its an abnormal case but if dual active due to netsplit when healed active can be demoted to standby
        members[standbyIdx].capabilities &= ~SAFplus::Group::IS_ACTIVE;
        members[standbyIdx].capabilities |= SAFplus::Group::IS_STANDBY;
        return true;
        }
      }
    return false;   // We didn't have knowledge of the member that took active...
    }



    void init(SAFplus::Handle handle)
      {
      hdl = handle;
      flags = AUTOMATIC_ELECTION;
      electionTimeMs = SAFplusI::GroupElectionTimeMs;
      activeIdx = 0xffff;
      standbyIdx = 0xffff;
      retentionTime = 0;
      lastChanged = 0;
      changeCount = 0;
      numMembers = 0;
      for (int i=0;i< ::SAFplusI::GroupMaxMembers;i++)
        {
        assert(members[i].id == SAFplus::INVALID_HDL);  // Constructor should have set it
        }
      }
    };

  class GroupShmEntry
    {
    public:
    enum
    {
      GROUP_NAME_LEN = 256
    };
    char     name[GROUP_NAME_LEN];  // This name is just for display purposes, use the Name service for lookup
    uint32_t  which;   // which data is the valid (reading) copy
    GroupData data[2];
    const GroupData& read() const { return data[which&1]; }
    GroupData& write() { return data[!(which&1)]; }
    void flip() { which = !which; }

    void clearElectionFlag()
      {
      data[0].flags &= ~GroupData::ELECTION_IN_PROGRESS;
      data[1].flags &= ~GroupData::ELECTION_IN_PROGRESS;
      }
    void setElectionFlag()
      {
      data[0].flags |= GroupData::ELECTION_IN_PROGRESS;
      data[1].flags |= GroupData::ELECTION_IN_PROGRESS;
      }

    void init(SAFplus::Handle grp)
      {
        which = 0;
        name[0] = 0;
        data[0].init(grp);
        memcpy(&data[1],&data[0],sizeof(GroupData));
      }
    std::pair<SAFplus::EntityIdentifier,SAFplus::EntityIdentifier> getRoles() const
      {
      std::pair<SAFplus::EntityIdentifier,SAFplus::EntityIdentifier> ret;
      const GroupData& gd = read();
      if (gd.activeIdx == 0xffff) ret.first = SAFplus::INVALID_HDL;
      else ret.first = gd.members[gd.activeIdx].id;
      if (gd.standbyIdx == 0xffff) ret.second = SAFplus::INVALID_HDL;
      else ret.second = gd.members[gd.standbyIdx].id;
      return ret;
      }
    };

  struct HandleEqual
  {
    bool operator() (const SAFplus::Handle& x, const SAFplus::Handle& y) const;
    typedef SAFplus::Handle first_argument_type;
    typedef SAFplus::Handle second_argument_type;
    typedef bool result_type;
  };

  // Group data stored in shared memory
  typedef std::pair<const SAFplus::Handle,GroupShmEntry> GroupShmMapPair;
  typedef boost::interprocess::allocator<GroupShmEntry, boost::interprocess::managed_shared_memory::segment_manager> GroupEntryAllocator;
  typedef boost::unordered_map < SAFplus::Handle, GroupShmEntry, boost::hash<SAFplus::Handle>, std::equal_to<SAFplus::Handle>,GroupEntryAllocator> GroupShmHashMap;

  // Some local data about the groups -- what the last shared memory change is that we handled, and the list of group objects per shared memory group.
  typedef std::pair<uint64_t,std::list<SAFplus::Group*> > GroupChangeMapValue;
  typedef std::pair<const SAFplus::Handle,GroupChangeMapValue > GroupChangeMapPair;
  typedef boost::unordered_map < SAFplus::Handle, GroupChangeMapValue, boost::hash<SAFplus::Handle>, std::equal_to<SAFplus::Handle> > GroupChangeMap;

  class GroupSharedMem
    {
    public:
      bool quit;
      boost::thread grpDispatchThread;
    SAFplus::ProcSem mutex;  // protects the shared memory region from simultaneous access
    SAFplus::Mutex  localMutex;
    GroupChangeMap lastChange;
    boost::interprocess::managed_shared_memory groupMsm;
    SAFplusI::GroupShmHashMap* groupMap;
    SAFplusI::GroupShmHeader* groupHdr;
    void init();
    void finalize();
    ~GroupSharedMem();
    void clear(); // Remove all group's entities from shared memory
    void claim(int pid, int port); // Claim that the provided pid and port is the node representative.  Overwrites an existing claim
    void dispatcher(void);
    GroupShmEntry* createGroup(SAFplus::Handle grp);

    void registerGroupObject(SAFplus::Group* grp);
    void deregisterGroupObject(SAFplus::Group* grp);

    unsigned int dbgCountGroups(void);  //? Counts the number of groups -- intended for unit tests.
    unsigned int dbgCountEntities(void);  //? Counts the total number of entities in all groups -- intended for unit tests.

    void dbgDump(void);

    //? typically you'd never call this... it deletes the shared memory segment out of under any running applications which can cause crashes, etc.
    static void deleteSharedMemory();
    };

class GroupServer:public SAFplus::MsgHandler
  {
  public:
  GroupServer();
    ~GroupServer();
  GroupSharedMem gsm;
  SAFplus::SafplusMsgServer   *groupMsgServer;
  boost::thread faultHandler;
    bool quit;
      // Communication port
  int groupCommunicationPort;
  bool m_reelect;

  void operator()(void);  // fault polling thread

  virtual void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);

    // ? Elects the entity that will be updating the group shared memory segment on this node.  Returns true if this process is the representative
  bool electNodeRepresentative();

  void init();
  void registerEntity (GroupShmEntry* grp, SAFplus::Handle me, uint64_t credentials, const void* data, int dataLength, uint capabilities,bool needNotify);
  void registerEntityEx (const SAFplus::Handle& grpHdl, SAFplus::Handle me, uint64_t credentials, const void* data, int dataLength, uint capabilities,bool needNotify);
  void deregisterEntity (GroupShmEntry* grp, SAFplus::Handle me,bool needNotify);
  void _deregister (GroupShmEntry* grp, unsigned int node, unsigned int port=0);

  void handleRoleNotification(GroupShmEntry* ge, SAFplusI::GroupMessageProtocol *rxMsg);
  void handleElectionRequest(SAFplus::Handle grpHandle);
  void startElection(SAFplus::Handle grpHandle);
  void sendHelloMessage(SAFplus::Handle grpHandle,const SAFplus::GroupIdentity& entityData);
  void sendRoleAssignmentMessage(SAFplus::Handle grpHandle,const std::pair<SAFplus::Handle,SAFplus::Handle>& results);

  std::pair<SAFplus::Handle,SAFplus::Handle> _electRoles(const GroupData& gd);

  void removeEntities(SAFplus::Handle faultEntity);
  };

  // Group associated data
  extern char groupSharedMemoryObjectName[256]; // Separate memory object name for a nodeID to support multi-node running

}
#endif // clGroupIpi_hxx
