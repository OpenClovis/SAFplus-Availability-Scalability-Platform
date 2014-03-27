#ifndef clGroup_hxx
#define clGroup_hxx

// Standard includes
#include <string>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/errors.hpp>
#include <boost/unordered_map.hpp>
#include <functional>
// SAFplus includes
#include <clHandleApi.hxx>
#include <clThreadApi.hxx>
#include <clNameApi.hxx>
#include <clIocApi.h>
#include <clLogApi.hxx>


namespace SAFplus
{
  typedef SAFplus::Handle EntityIdentifier;

  class GroupIdentity
  {
    public:
      EntityIdentifier id;
      uint64_t credentials;
      SAFplus::Buffer* data;
      uint capabilities; //Capabilities is a bitmap. It also contains state of group member
      uint dataLen;
      GroupIdentity& operator=(GroupIdentity const& c)
      {
        id            = c.id;
        credentials   = c.credentials;
        capabilities  = c.capabilities;
        dataLen       = c.dataLen;
        //data          = new(c.data->data) Buffer(sizeof(c.dataLen));
      }
      GroupIdentity()
      {
        credentials = 0;
      }
      GroupIdentity(EntityIdentifier me,uint64_t credentials,SAFplus::Buffer *dat,uint datalen,uint capabilities)
      {
        id = me;
        this->credentials = credentials;
        this->capabilities = capabilities;
        this->dataLen = datalen;
        char tmpData[sizeof(SAFplus::Buffer)-1+datalen];
        data = new(tmpData) Buffer(datalen);
        memcpy((char *)data->data,(char *)dat->data,datalen);
      }
  };
}
namespace SAFplusI
{
  class GroupBufferHeader;

  typedef SAFplusI::BufferPtr  GroupMapKey;
  typedef SAFplusI::BufferPtr  GroupMapValue;

  typedef std::pair<const GroupMapKey,GroupMapValue> GroupMapPair;
  typedef boost::interprocess::allocator<GroupMapValue, boost::interprocess::managed_shared_memory::segment_manager> GroupAllocator;
  typedef boost::unordered_map < GroupMapKey, GroupMapValue, boost::hash<GroupMapKey>, BufferPtrContentsEqual, GroupAllocator> GroupHashMap;

  class GroupBufferHeader
  {
    public:
      uint64_t structId;
      pid_t    serverPid;  // This is used to ensure that 2 servers don't fight for the memory
      SAFplus::Handle handle;
  };

}

namespace SAFplus
{
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
        ELECTION_TYPE_BOTH = 1,     // Elect both active/standby roles
        ELECTION_TYPE_ACTIVE  = 2,  // Elect active role only
        ELECTION_TYPE_STANDBY  = 4, // Elect standby role only
      };
      enum
      {
        GROUP_SEGMENT_SIZE = 65536, // Which size is enough to contain group map ?
        GROUP_SEGMENT_ROWS = 256,
        CL_GROUP_BUFFER_HEADER_STRUCT_ID_7 = 0x5941234,
        GROUP_CAPABILITY_MASK = 0xfffff0
      };

      Group(SAFplus::Handle groupHandle) { init(groupHandle); }
      Group(); // Deferred initialization

      void init(SAFplus::Handle groupHandle);

      // Named group uses the name service to resolve the name to a handle
      Group(std::string name);

      // register a member of the group.  This is separate from the constructor so someone can iterate through members of the group without being a member.  Caller owns data when register returns.
      void registerEntity(EntityIdentifier me, uint64_t credentials, const void* data, int dataLength, uint capabilities);

      void registerEntity(GroupIdentity grpIdentity);

      // If me=0 (default), use the group identifier the last call to "register" was called with.
      void deregister(EntityIdentifier me = INVALID_HDL);

      // If default me=0, use the group identifier the last call to "register" was called with.
      void setCapabilities(uint capabilities, EntityIdentifier me = INVALID_HDL);

      // This also returns the current active/standby state of the entity since that is part of the capabilities bitmap.
      uint getCapabilities(EntityIdentifier id);

      // This also returns the current active/standby state of the entity since that is part of the capabilities bitmap.
      SAFplus::Buffer& getData(EntityIdentifier id);

      // Calls for an election with specified role
      int elect(std::pair<EntityIdentifier,EntityIdentifier> &res,int electionType = (int)SAFplus::Group::ELECTION_TYPE_BOTH );

      typedef SAFplusI::GroupMapPair KeyValuePair;

    private:
      EntityIdentifier electLeader();
      EntityIdentifier electDeputy(EntityIdentifier highestCreEntity);
      void initializeSharedMemory(char sharedMemName[]);
      std::pair<EntityIdentifier,EntityIdentifier> electForRoles(int electionType);
      void sendNotification(int notificationType, void* data); //Used to notify other nodes or members about group membership/role changed

    public:
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
        SAFplusI::GroupMapPair* curval;

        SAFplusI::GroupHashMap::iterator iter;
      };

      Iterator begin();
      Iterator end();

      bool isMember(EntityIdentifier id);

      void setNotification(SAFplus::Wakeable& w);  // call w.wake when someone enters/leaves the group or an active or standby assignment or transition occurs.  Pass what happened into the wakeable.

      EntityIdentifier getActive(void) const;
      EntityIdentifier getStandby(void) const;

    protected:
      boost::interprocess::managed_shared_memory msm;
      SAFplusI::GroupHashMap*           map;
      SAFplusI::GroupBufferHeader*      hdr;
      SAFplus::Handle                   handle;
      SAFplus::Wakeable*                wakeable;
      EntityIdentifier                  activeEntity;
      EntityIdentifier                  standbyEntity;
      EntityIdentifier                  lastRegisteredEntity;

  };

}

#endif
