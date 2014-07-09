#ifndef clGroup_hxx
#define clGroup_hxx

// Standard includes
#include <string>
#include <boost/unordered_map.hpp>
#include <functional>
// SAFplus includes
#include <clHandleApi.hxx>
#include <clThreadApi.hxx>
#include <clNameApi.hxx>
#include <clIocApi.h>
#include <clIocIpi.h>
#include <clLogApi.hxx>
#include <clCkptApi.hxx>
#include <clCustomization.hxx>
#include <clSafplusMsgServer.hxx>
#include <clGroupIpi.hxx>
#include <clCpmApi.h>

namespace SAFplusI
{
  class GroupMessageProtocol;
  enum class GroupMessageTypeT;
  enum class GroupRoleNotifyTypeT;
  enum class GroupMessageSendModeT;
};

namespace SAFplus
{
  class GroupIdentity;
  typedef SAFplus::Handle EntityIdentifier;
  typedef EntityIdentifier  GroupMapKey;
  typedef GroupIdentity     GroupMapValue;
  typedef void*             DataMapValue;

  typedef std::pair<const GroupMapKey,GroupMapValue> GroupMapPair;
  typedef std::pair<const GroupMapKey,DataMapValue> DataMapPair;
  typedef boost::unordered_map < GroupMapKey, GroupMapValue> GroupHashMap;
  typedef boost::unordered_map < GroupMapKey, DataMapValue> DataHashMap;

  class GroupIdentity
  {
    public:
      EntityIdentifier id;
      uint64_t credentials;
      uint capabilities;
      uint dataLen;
      GroupIdentity& operator=(const GroupIdentity & c)
      {
        id            = c.id;
        credentials   = c.credentials;
        capabilities  = c.capabilities;
        dataLen       = c.dataLen;
        return *this;
      }
      GroupIdentity()
      {
        id = INVALID_HDL;
        credentials = 0;
        capabilities = 0;
        dataLen = 0;
      }
      GroupIdentity(EntityIdentifier me,uint64_t credentials,uint datalen,uint capabilities)
      {
        this->id = me;
        this->credentials = credentials;
        this->capabilities = capabilities;
        this->dataLen = datalen;
      }
      void dumpInfo()
      {
        logInfo("GMS", "---","Dumping GroupIdentity at %x",this);
        logInfo("GMS", "---","ID: 0x%x 0x%x",id.id[0],id.id[1]);
        logInfo("GMS", "---","CREDENTIALS: 0x%x ",credentials);
        logInfo("GMS", "---","CAPABILITY: 0x%x ",capabilities);
        logInfo("GMS", "---","DATA LENGTH: 0x%x ",dataLen);
      }
  };

  class Group
  {
    public:
      enum
      {
        ACCEPT_STANDBY = 1,  // Can this entity become standby?
        ACCEPT_ACTIVE  = 2,  // Can this entity become active?
        IS_ACTIVE      = 4,
        IS_STANDBY     = 8
      };
      enum
      {
        NODE_JOIN_SIG,    // Signal for node join
        NODE_LEAVE_SIG,   // Signal for node leave
        ELECTION_FINISH_SIG
      };
      enum
      {
        ELECTION_TYPE_BOTH = 3,     // Elect both active/standby roles
        ELECTION_TYPE_ACTIVE  = 2,  // Elect active role only
        ELECTION_TYPE_STANDBY  = 1, // Elect standby role only
      };
      enum
      {
        DATA_IN_CHECKPOINT = 1,   // Group database is in checkpoint
        DATA_IN_MEMORY     = 2    // Group database is in memory
      };
      enum
      {
        CRED_ACTIVE_BIT  = 1<<11,   // Extra credentials for active entity
        CRED_STANDBY_BIT = 1 << 10  // Extra credentials for standby entity
      };

      Group(SAFplus::Handle groupHandle) { automaticElection=true; init(groupHandle); }
      Group(int dataStoreMode = DATA_IN_CHECKPOINT, int comPort = CL_IOC_GMS_PORT); // Deferred initialization

      void init(SAFplus::Handle groupHandle,int dataStoreMode = DATA_IN_CHECKPOINT, int comPort = CL_IOC_GMS_PORT);

      // Named group uses the name service to resolve the name to a handle
      Group(const std::string& name,int dataStoreMode = DATA_IN_CHECKPOINT, int comPort = CL_IOC_GMS_PORT);

      // register a member of the group.  This is separate from the constructor so someone can iterate through members of the group without being a member.  Caller owns data when register returns.
      // @param: credentials: a number that must be unique across all members of the group.  Highest credential wins the election.
      // @param: data, dataLength:  a group member can provide some arbitrary data that other group members can see
      // @param: capabilities: whether this entity can become active/standby and IS this entity currently active/standby (latter should always be 0, Group will elect)
      // @param: needNotify: Callback whenever group membership changes?
      void registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities,bool needNotify = true);

      void registerEntity(GroupIdentity grpIdentity,bool needNotify = true);

      // If me=0 (default), use the group identifier the last call to "register" was called with.
      void deregister(EntityIdentifier me = INVALID_HDL,bool needNotify = true);

      // If default me=0, use the group identifier the last call to "register" was called with.
      void setCapabilities(uint capabilities, EntityIdentifier me = INVALID_HDL);

      // This also returns the current active/standby state of the entity since that is part of the capabilities bitmap.
      uint getCapabilities(EntityIdentifier id);

      // This also returns the current active/standby state of the entity since that is part of the capabilities bitmap.
      void* getData(EntityIdentifier id);

      // Calls for an election with specified role
      std::pair<EntityIdentifier,EntityIdentifier>  elect(SAFplus::Wakeable& wake = SAFplus::Wakeable::Synchronous);

      // Get the current active entity
      EntityIdentifier getActive(void) const;

      // Get the current standby entity
      EntityIdentifier getStandby(void) const;

      // Utility functions
      bool isMember(EntityIdentifier id);

      void setNotification(SAFplus::Wakeable& w);  // call w.wake when someone enters/leaves the group or an active or standby assignment or transition occurs.  Pass what happened into the wakeable.

      // Enable/disable auto election whenever join/leave/fail occurs
      bool                              automaticElection;

      // Whether election should be run again
      bool                              needReElect;

      // Time (second) to delay from boot until auto election
      int                               minimumElectionTime;

      // My information
      GroupIdentity                     myInformation;

      // Communication port
      int                               groupCommunicationPort;

      // Allow active/standby to be active/standby again
      bool                              stickyMode;

      typedef SAFplus::DataMapPair KeyValuePair;
      // std template like iterator
      class Iterator
      {
      protected:

      public:
        Iterator(Group* _group);
        ~Iterator();

        // comparison
        bool operator !=(const Iterator& otherValue) const;

        // increment the pointer to the next value
        Iterator& operator++();
        Iterator& operator++(int);

        KeyValuePair& operator*() { return *curval; }
        const KeyValuePair& operator*() const { return *curval; }
        KeyValuePair* operator->() { return curval; }
        const KeyValuePair* operator->() const { return curval; }

        Group* group;
        SAFplus::DataMapPair* curval;

        SAFplus::DataHashMap::iterator iter;
      };
      // Group iterator
      Iterator begin();
      Iterator end();

    public: //private:
      // Handle node join message
      void nodeJoinHandle(SAFplusI::GroupMessageProtocol *rxMsg);

      // Handle node leave message
      void nodeLeaveHandle(SAFplusI::GroupMessageProtocol *rxMsg);

      // Handle role notification message
      void roleNotificationHandle(SAFplusI::GroupMessageProtocol *rxMsg);

      // Handle election request message
      void electionRequestHandle(SAFplusI::GroupMessageProtocol *rxMsg);

      std::pair<EntityIdentifier,EntityIdentifier> electForRoles();

      // Election utility
      EntityIdentifier electARole(EntityIdentifier ignoreMe = INVALID_HDL);

      // Initialize/Finalize libraries
      ClRcT initializeLibraries();
      void finalizeLibraries();

      // Set an entity become standby or active
      void setActive(EntityIdentifier id);
      void setStandby(EntityIdentifier id);

      // Read/write to group database
      void* readFromDatabase(Buffer *key);
      void  writeToDatabase(Buffer *key, void *);
      void  removeFromDatabase(Buffer* key);

      // Message server
      void startMessageServer();
      void fillSendMessage(void* data, SAFplusI::GroupMessageTypeT msgType,SAFplusI::GroupMessageSendModeT msgSendMode, SAFplusI::GroupRoleNotifyTypeT roleType, bool forcing=false);
      void sendNotification(void* data, int dataLength, SAFplusI::GroupMessageSendModeT messageMode);
      SAFplus::SafplusMsgServer   *groupMsgServer;

      // Whether there is an running election
      static  bool isElectionRunning;

      // Whether current election was done
      static  bool isElectionFinished;

      // Whether boot time election had done
      static  bool isBootTimeElectionDone;

      // Timers handle
      static  ClTimerHandleT electionRequestTHandle;
      static  ClTimerHandleT roleWaitingTHandle;

      // Election timer callback
      static ClRcT electionRequest(void *arg);

      // Role change timer callback
      static ClRcT roleChangeRequest(void *arg);

      // IOC notification callback
      static ClRcT iocNotificationCallback(ClIocNotificationT *notification, ClPtrT cookie);

      // Class to handle peer to peer messages between group services
      class GroupMessageHandler:public SAFplus::MsgHandler
      {
        public:
          Group* mGroup;
          GroupMessageHandler(SAFplus::Group *grp=nullptr);
          void msgHandler(ClIocAddressT from, SAFplus::MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);
      };

    protected:
      static SAFplus::Checkpoint        mGroupCkpt;             // The checkpoint where storing entity information if mode is CHECKPOINT
      SAFplus::GroupHashMap             mGroupMap;              // The map where store entity information if mode is MEMORY
      SAFplus::DataHashMap              groupDataMap;           // The map where store entity associated data
      SAFplus::Handle                   handle;                 // The handle of this group, store/retrieve from name
      SAFplus::Wakeable*                wakeable;               // Wakeable object for async communication
      EntityIdentifier                  activeEntity;           // Current active entity
      EntityIdentifier                  standbyEntity;          // Current standby entity
      EntityIdentifier                  lastRegisteredEntity;   // Last entity come to registerEntity
      int                               dataStoringMode;        // Where entity information is stored
  };
  // Boost require hash_value to be implemented
  std::size_t hash_value(SAFplus::Handle const& h);
}

#endif
