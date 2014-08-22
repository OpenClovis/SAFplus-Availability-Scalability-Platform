#ifndef clGroup_hxx
#define clGroup_hxx

// Standard includes
#include <string>
#include <boost/unordered_map.hpp>
#include <boost/intrusive/list.hpp>
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
#include <clMsgApi.hxx>
#include <clIocPortList.hxx>
//#include <clGroupIpi.hxx>
#include <clCpmApi.h>

namespace SAFplusI
{
  class GroupMessageProtocol;
  enum class GroupMessageTypeT;
  enum class GroupRoleNotifyTypeT;
  enum class GroupMessageSendModeT;
  class GroupSharedMem;
};

namespace SAFplus
{
  class GroupIdentity;
  typedef SAFplus::Handle EntityIdentifier;
  typedef EntityIdentifier  GroupMapKey;
  typedef GroupIdentity     GroupMapValue;
  //typedef void*             DataMapValue;

  typedef std::pair<GroupMapKey,GroupMapValue> GroupMapPair;
  //typedef std::pair<const GroupMapKey,DataMapValue> DataMapPair;
  //typedef boost::unordered_map < GroupMapKey, GroupMapValue> GroupHashMap;
  //typedef boost::unordered_map < GroupMapKey, DataMapValue> DataHashMap;

  // Call once before creating any Group objects
  void groupInitialize(void);

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
      void init(EntityIdentifier me,uint64_t creds,uint caps,uint datasz=0)
      {
        id = me;
        credentials = creds;
        capabilities = caps;
        dataLen = datasz;
      }
      GroupIdentity(EntityIdentifier me,uint64_t credentials,uint datalen,uint capabilities)
      {
        this->id = me;
        this->credentials = credentials;
        this->capabilities = capabilities;
        this->dataLen = datalen;
      }

      //? Change this entity's info if the values are different.  Return whether changes needed to be made.
      bool override(uint64_t new_credentials,uint new_capabilities)
      {
      bool changed = false;
      if (new_credentials != credentials) {credentials = new_credentials; changed = true; }
      if (new_capabilities != capabilities) { capabilities = new_capabilities; changed = true; }
      return changed;
      }

      void dumpInfo()
      {
        logInfo("GMS", "---","Dumping GroupIdentity at [%p]",this);
        logInfo("GMS", "---","ID: 0x%lx 0x%lx",id.id[0],id.id[1]);
        logInfo("GMS", "---","CREDENTIALS: 0x%lx ",credentials);
        logInfo("GMS", "---","CAPABILITY: 0x%x ",capabilities);
        logInfo("GMS", "---","DATA LENGTH: 0x%x ",dataLen);
      }
  };

  class Group
    {
    public:
    enum
      {
        ACCEPT_STANDBY = 1,    // Can this entity become standby?
        ACCEPT_ACTIVE  = 2,    // Can this entity become active?
        IS_ACTIVE      = 4,    // Is this Group currently active?
        IS_STANDBY     = 8,    // Is this Group currently standby?
        STICKY         = 0x10, // Active/standby assignments will stay with this entity even if an entity joins with higher credential, unless that entity is also Active/Standby (split).
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

      Group(); // Deferred initialization
      Group(SAFplus::Handle groupHandle, int comPort = SAFplusI::GMS_IOC_PORT);
      // Named group uses the name service to resolve the name to a handle
      //Group(const std::string& name, int comPort = SAFplusI::GMS_IOC_PORT);
      ~Group();

      void init(SAFplus::Handle groupHandle, int comPort = SAFplusI::GMS_IOC_PORT);
      //void init(const std::string& name, int comPort = SAFplusI::GMS_IOC_PORT);

      // register a member of the group.  This is separate from the constructor so someone can iterate through members of the group without being a member.  Caller owns data when register returns.
      // @param: credentials: a number that must be unique across all members of the group.  Highest credential wins the election.
      // @param: data, dataLength:  a group member can provide some arbitrary data that other group members can see
      // @param: capabilities: whether this entity can become active/standby and IS this entity currently active/standby (latter should always be 0, Group will elect)
      // @param: needNotify: Callback whenever group membership changes?
      void registerEntity(EntityIdentifier me, uint64_t credentials, uint capabilities)
        {
        assert(myInformation.id == INVALID_HDL);  // You can only register 1 entity per group object
        myInformation.id = me;
        myInformation.credentials = credentials;
        myInformation.capabilities = capabilities;
        _registerEntity(me, credentials, NULL, 0, capabilities);

        }

      void registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities) 
        {
        assert(myInformation.id == INVALID_HDL);  // You can only register 1 entity per group object
        myInformation.id = me;
        myInformation.credentials = credentials;
        myInformation.capabilities = capabilities;
        _registerEntity(me, credentials, data, dataLength, capabilities);
        }

      // common internal registration function that handles both application registrations and remote announcements.
      void _registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities);

      // If me=0 (default), use the group identifier the last call to "register" was called with.
      void deregister(EntityIdentifier me = INVALID_HDL);


      // Get the current active entity.  If an active entity is not determined this call will block until the election is complete.  Therefore it will only return INVALID_HDL if there is no entity with active capability
      EntityIdentifier getActive(void);

      // Get the current standby entity.  If a standby entity is not determined this call will block until the election is complete.  Therefore it will only return INVALID_HDL if there is no entity with standby capability
      EntityIdentifier getStandby(void);

      // Get the current active/standby.  If a standby entity is not determined this call will block until the election is complete.  Therefore it will only return INVALID_HDL if there is no entity with standby capability
      std::pair<EntityIdentifier,EntityIdentifier> getRoles();

      // Utility functions
      bool isMember(EntityIdentifier id);


      void setNotification(SAFplus::Wakeable& w);  //? call w.wake when someone enters/leaves the group or an active or standby assignment or transition occurs.  Pass what happened into the wakeable.  There can be only one registered notification per Group object.
      uint64_t lastChange(); //?  Return the time of the last change to this group in monotonically increasing system ticks.

      // triggers an election if not already running and returns (mostly for internal use)
      void startElection(void);


      // variables:

      // My information
      GroupIdentity                     myInformation;

      SAFplus::Handle                   handle;                 // The handle of this group, store/retrieve from name


      // std template like iterator
      typedef SAFplus::GroupMapPair KeyValuePair;
      class Iterator
      {
      protected:
        void load(void);  // helper function that actually loads the current value from the shared memory
      public:
        Iterator() { group=NULL; curIdx=0xffff; locked = false; }  // note that the END iterator object is initialized with this (so curIdx MUST be inited to 0xffff).
        Iterator(Group* _group,bool lock=false);
        ~Iterator();

        // comparison
        bool operator !=(const Iterator& otherValue) { return curIdx != otherValue.curIdx; }

        // increment the pointer to the next value
        Iterator& operator++();
        Iterator& operator++(int);

        //KeyValuePair& operator*() { return curval; }
        const KeyValuePair& operator*() const { return curval; }
        //KeyValuePair* operator->() { return &curval; }
        const KeyValuePair* operator->() const { return &curval; }

        Group* group;
        int curIdx;
        bool locked;
        KeyValuePair curval;
        static Iterator END;
      };
      // Group iterator
      Iterator begin() { return Iterator(this); }
      Iterator& end() { return Iterator::END; }

      //?  Returns a string describing the passed capabilities, pass in a 100 byte string as the fill buffer
      static char* capStr(uint cap, char* buf);

    protected:
      // Functions that send various announcement messages
      void sendGroupAnnounceMessage();
      void sendRoleMessage(EntityIdentifier active,EntityIdentifier standby,bool forcing);
      void fillAndSendMessage(void* data, SAFplusI::GroupMessageTypeT msgType,SAFplusI::GroupMessageSendModeT msgSendMode, SAFplusI::GroupRoleNotifyTypeT roleType, bool forcing=false);
      void sendNotification(void* data, int dataLength, SAFplusI::GroupMessageSendModeT messageMode);
 
     // Communication port
      int                               groupCommunicationPort;
      SAFplus::SafplusMsgServer*        groupMsgServer;
      SAFplus::Wakeable*                wakeable;               // Wakeable object for change notification
      friend class SAFplusI::GroupSharedMem;
    };

}


#endif
