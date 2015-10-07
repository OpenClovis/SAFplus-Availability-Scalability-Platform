//? <section name="Group Membership">
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
//#include <clIocApi.h>
//#include <clIocIpi.h>
//#include <clLogApi.hxx>
#include <clCkptApi.hxx>
#include <clCustomization.hxx>
#include <clSafplusMsgServer.hxx>
#include <clMsgApi.hxx>
#include <clMsgPortsAndTypes.hxx>
//#include <clGroupIpi.hxx>
//#include <clCpmApi.h>

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

  //? Initialize the group subsystem.  Typically this is called during execution of safplusInitialize(), so applications would only call this if they are using the group service standalone.
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

    //? Change this entity's info if the configured values are different.  Return whether changes needed to be made.
    bool override(uint64_t new_credentials,uint new_capabilities);
   
    void dumpInfo();
#if 0
    void dumpInfo()
    {
      logInfo("GMS", "---","Dumping GroupIdentity at [%p]",this);
      logInfo("GMS", "---","ID: 0x%lx 0x%lx",id.id[0],id.id[1]);
      logInfo("GMS", "---","CREDENTIALS: 0x%lx ",credentials);
      logInfo("GMS", "---","CAPABILITY: 0x%x ",capabilities);
      logInfo("GMS", "---","DATA LENGTH: 0x%x ",dataLen);
    }
#endif
  };

  //? When using the message send capabilities of a group, this determines the semantics of the message send.
  enum class GroupMessageSendMode
  {
    SEND_BROADCAST,    //? Send to all members in the group
    SEND_TO_ACTIVE,    //? Send to the active member of the group
    SEND_TO_STANDBY,   //? Send to the standby member of the group
    SEND_LOCAL_ROUND_ROBIN, //? Send using a locally-calculated round robin heuristic algorithm.  In other words, this Group instance will send the message to a single member of the group.  Each subsequent call will send to a different member until every member has been sent a message.  Then repeat.  It is not guaranteed that messages will be sent to destinations in a particular order, or that all destinations will receive at least one message before any destination receives two.  This is a simple algorithm intended for load balancing. 
  };

  //? <class> The group class allows a set of entities find eachother, elect leader, elect standby, and talk to eachother.  Entities can be on any/multiple processes and nodes in the cluster.  Entities who instantiate this group class can list all group members, can join the group as a member specifying capabilities such as the ability to become the leader, and can send messages to all or individual members of the group.
  class Group
    {
    public:
    enum
      {
        ACCEPT_STANDBY = 1,    //? Group entity capability: Can this entity become standby?
        ACCEPT_ACTIVE  = 2,    //? Group entity capability: Can this entity become active?
        IS_ACTIVE      = 4,    //? Group entity capability: Is this Group currently active?
        IS_STANDBY     = 8,    //? Group entity capability: Is this Group currently standby?
        STICKY         = 0x10, //? Group entity capability: Active/standby assignments will stay with this entity even if an entity joins with higher credential, unless that entity is also Active/Standby (split).

        SOURCE_CAPABILITIES = ACCEPT_STANDBY |ACCEPT_ACTIVE|STICKY
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

      Group(); //? <ctor> 2 phase (deferred initialization) constructor.  You MUST call init() before using this object.</ctor>

      /*? <ctor>  Construct a group identified by a particular handle.
       <arg name="groupHandle">The identity of this group</arg>
       <arg name="comPort" default="discover">[Optional]  What port the group system is using.  By default, this will use the correct SAFplus group port discovered by looking into the shared memory segment</arg>
       </ctor> */
      Group(SAFplus::Handle groupHandle, int comPort = 0);
      // Named group uses the name service to resolve the name to a handle
      //Group(const std::string& name, int comPort = SAFplusI::GMS_IOC_PORT);

      /* <ctor>This creates a named group.  A "named" group associates the string name with the handle in the Name service and in the Group shared memory.  The handle is the authorative identifier, but the name can be used by other applications to get to this group.
       <arg name="groupHandle">The identity of this group</arg>
       <arg name="name">A null-terminated string which is the name of this group</arg>
       <arg name="comPort" default="SAFplusI::GMS_IOC_PORT">[Optional]  What port the group system is using.  By default, this will use the correct SAFplus group port.</arg>         
         </ctor>
       */
      Group(SAFplus::Handle groupHandle,const char* name, int comPort = 0);
      ~Group();

      /*? <ctor>  Construct a group identified by a particular handle.
       <arg name="groupHandle">The identity of this group</arg>
       <arg name="comPort" default="SAFplusI::GMS_IOC_PORT">[Optional]  What port the group system is using.  By default, this will use the correct SAFplus group port.</arg>
       <arg name="execSemantics" default="BLOCK">[Optional]  How to respond when the group is initialized.  By default, this will block but you can also provide a callback or thread semaphore (see <ref type="class">SAFplus::Wakeable</ref> for details).</arg>
       </ctor> */
      void init(SAFplus::Handle groupHandle, int comPort = 0,SAFplus::Wakeable& execSemantics = BLOCK);

      /*? <ctor>This creates a named group.  A "named" group associates the string name with the handle in the Name service and in the Group shared memory.  The handle is the authorative identifier, but the name can be used by other applications to get to this group.
       <arg name="groupHandle">The identity of this group</arg>
       <arg name="name">A null-terminated string which is the name of this group</arg>
       <arg name="comPort" default="SAFplusI::GMS_IOC_PORT">[Optional]  What port the group system is using.  By default, this will use the correct SAFplus group port.</arg>         
         </ctor>
       */
      void init(SAFplus::Handle groupHandle, const char* name, int comPort = 0);
      //void init(const std::string& name, int comPort = SAFplusI::GMS_IOC_PORT);

      //? register a member of the group.  This is separate from the constructor so someone can iterate through members of the group without being a member.  Caller owns data when register returns.
      // <arg name="me">The SAFplus::Handle that uniquely identifies your entity</arg>
      // <arg name="credentials"> a number that must be unique across all members of the group.  Highest credential wins the election.</arg>
      // <arg name="capabilities"> whether this entity can become active/standby and IS this entity currently active/standby (latter should always be 0, Group will elect).</arg>
      void registerEntity(EntityIdentifier me, uint64_t credentials, uint capabilities)
        {
        assert(myInformation.id == INVALID_HDL);  // You can only register 1 entity per group object
        myInformation.id = me;
        myInformation.credentials = credentials;
        myInformation.capabilities = capabilities;
        _registerEntity(me, credentials, NULL, 0, capabilities);
        }


      //? register a member of the group.  This is separate from the constructor so someone can iterate through members of the group without being a member.  Caller owns data when register returns.
      // <arg name="me">The SAFplus::Handle that uniquely identifies your entity</arg>
      // <arg name="credentials">A number that must be unique across all members of the group.  Highest credential wins the election.</arg>
      // <arg name="capabilities">Whether this entity can become active/standby and IS this entity currently active/standby (latter should always be 0, Group will elect).</arg>
      // <arg name="data, dataLength">A group member can provide some arbitrary data that other group members can see.  Pass via a buffer and a length.  This data will be copied so you may free it when this function returns.</arg>
      void registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities) 
        {
       // [not implemented args] <arg name="needNotify: Callback whenever group membership changes?</arg>
        assert(myInformation.id == INVALID_HDL);  // You can only register 1 entity per group object
        myInformation.id = me;
        myInformation.credentials = credentials;
        myInformation.capabilities = capabilities;
        _registerEntity(me, credentials, data, dataLength, capabilities);
        }

      // common internal registration function that handles both application registrations and remote announcements.
      void _registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities);

      //? Stop being a member of the group.  You can still use this object to list members, etc.
      // <arg name="me">[OPTIONAL: default is last entity to register] What entity should be deregistered.  If me=INVALID_HDL, use the group identifier the last call to "register" was called with.</arg>
      void deregister(EntityIdentifier me = INVALID_HDL);

      //? Sets the name for this group.  Stores the name in shared memory.  TODO: And adds this name to the Name server.
      // <arg name="name">A null-terminated string which names this group</arg>
      void setName(const char* name);

      //? Get the current active entity.  If an active entity has not been determined and execSemantics == BLOCK, this call will block until the election is complete.  Therefore, in the blocking case it will only return INVALID_HDL if there is no entity with active capability
      // <arg name="execSemantics">[OPTIONAL: default is BLOCK]  You can also choose ABORT which will return INVALID_HDL if the election is not complete.</arg>
      EntityIdentifier getActive(SAFplus::Wakeable& execSemantics = BLOCK);

      //? Get the current standby entity.  If a standby entity has not been determined and execSemantics == BLOCK, this call will block until the election is complete.  Therefore, in the blocking case it will only return INVALID_HDL if there is no entity with standby capability
      // <arg name="execSemantics">[OPTIONAL: default is BLOCK]  You can also choose ABORT which will return INVALID_HDL if the election is not complete.</arg>
      EntityIdentifier getStandby(SAFplus::Wakeable& execSemantics = BLOCK);

      //? Blocking convenience function to get the current active/standby.  If the active or standby entity is not determined this call will block until the election is complete.  Therefore it will only return INVALID_HDL if there is no entity with the required capability
      std::pair<EntityIdentifier,EntityIdentifier> getRoles();

      // Utility functions

      //? Return true if the passed Handle is a member of this group.
      bool isMember(EntityIdentifier id);

      void setNotification(SAFplus::Wakeable& w);  //? call w.wake when someone enters/leaves the group or an active or standby assignment or transition occurs.  Pass what happened into the wakeable.  There can be only one registered notification per Group object.
      uint64_t lastChange(); //?  Return the time of the last change to this group in monotonically increasing system ticks.

      //? triggers an election if not already running and returns right away.  This function is mostly for internal use, but you could use it to force a re-election.
      void startElection(void);

      // variables:

      //? This member's information.  If you register multiple entities, only the last one registered will be remembered.
      GroupIdentity                     myInformation;

      SAFplus::Handle                   handle;                 //? The handle of this group


      // std template like iterator
      typedef SAFplus::GroupMapPair KeyValuePair;

      //? <class> Iterator to view all members of this group.  Like STL iterators, this object acts like a pointer where the obj->first gets the member's Handle, and obj->second gets information about that member (an instance of <ref type="class">GroupIdentity</ref>?)
      class Iterator
      {
      protected:
        void load(void);  // helper function that actually loads the current value from the shared memory
      public:
        Iterator() { group=NULL; curIdx=0xffff; locked = false; }  // note that the END iterator object is initialized with this (so curIdx MUST be inited to 0xffff).
        Iterator(Group* _group,bool lock=false);
        ~Iterator();

        //? comparison
        bool operator !=(const Iterator& otherValue) { return curIdx != otherValue.curIdx; }
        //? comparison
        bool operator ==(const Iterator& otherValue) { return curIdx == otherValue.curIdx; }

        //? Go to the next member in this group
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
      }; //? </class>

      //? Similar to STL, starts iteration through the members of this group.
      Iterator begin() { return Iterator(this); }
      //? Similar to STL, this returns the termination sentinel -- that is, an invalid object that can be compared to an iterator to determine that it is at the end of the list of members.
      Iterator& end() { return Iterator::END; }

      //?  Returns a string describing the passed capabilities in English, pass in a 100 byte string as the fill buffer
      static char* capStr(uint cap, char* buf);

      //? Send a message to the members in this group.  Currently all messages are sent via the <ref type="class">ObjectMessager</ref> facility.  [TODO: analyze the Handle and send it to node/port if that is what the handle specifies]
      //  <arg name="data">A buffer of the data to be sent</arg>
      //  <arg name="dataLength">Length of the data to be sent</arg>
      //  <arg name="messageMode">Who to send this message to?  Everyone, the active, the standby or any member in a round robin fashion?</arg>
      void send(void* data, int dataLength, SAFplus::GroupMessageSendMode messageMode);

    protected:
      // Functions that send various announcement messages
      void sendGroupAnnounceMessage();
      void sendRoleMessage(EntityIdentifier active,EntityIdentifier standby,bool forcing);
      void fillAndSendMessage(void* data, SAFplusI::GroupMessageTypeT msgType,SAFplus::GroupMessageSendMode msgSendMode, SAFplusI::GroupRoleNotifyTypeT roleType, bool forcing=false);
      void sendNotification(void* data, int dataLength, SAFplus::GroupMessageSendMode messageMode);
 
     // Communication port
      int                               groupCommunicationPort;
      SAFplus::SafplusMsgServer*        groupMsgServer;
      SAFplus::Wakeable*                wakeable;               // Wakeable object for change notification
      friend class SAFplusI::GroupSharedMem;
      Iterator roundRobin;
  };  //? </class>

}


#endif
//? </section>
