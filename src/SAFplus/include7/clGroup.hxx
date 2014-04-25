#ifndef clGroup_hxx
#define clGroup_hxx

// Standard includes
#include <string>
#include <boost/interprocess/errors.hpp>
#include <boost/unordered_map.hpp>
#include <functional>
// SAFplus includes
#include <clHandleApi.hxx>
#include <clThreadApi.hxx>
#include <clNameApi.hxx>
#include <clIocApi.h>
#include <clLogApi.hxx>
#include <clCkptApi.hxx>
#include <clCustomization.hxx>


namespace SAFplus
{
  typedef SAFplusI::BufferPtr  GroupMapKey;
  typedef SAFplusI::BufferPtr  GroupMapValue;
  typedef SAFplus::Handle EntityIdentifier;

  typedef std::pair<const GroupMapKey,GroupMapValue> GroupMapPair;
  typedef boost::unordered_map < GroupMapKey, GroupMapValue> GroupHashMap;

  class GroupIdentity
  {
    public:
      EntityIdentifier id;
      uint64_t credentials;
      SAFplus::Buffer* data;
      uint capabilities;
      uint dataLen;
      GroupIdentity& operator=(GroupIdentity const& c)
      {
        id            = c.id;
        credentials   = c.credentials;
        capabilities  = c.capabilities;
        dataLen       = c.dataLen;
      }
      GroupIdentity()
      {
        credentials = 0;
        capabilities = 0;
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
      std::pair<EntityIdentifier,EntityIdentifier>  elect(int electionType = (int)SAFplus::Group::ELECTION_TYPE_BOTH );

      typedef SAFplus::GroupMapPair KeyValuePair;

    private:
      EntityIdentifier electLeader();
      EntityIdentifier electDeputy(EntityIdentifier highestCreEntity);
      std::pair<EntityIdentifier,EntityIdentifier> electForRoles(int electionType);

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
        SAFplus::GroupMapPair* curval;

        SAFplus::GroupHashMap::iterator iter;
      };

      Iterator begin();
      Iterator end();

      bool isMember(EntityIdentifier id);

      void setNotification(SAFplus::Wakeable& w);  // call w.wake when someone enters/leaves the group or an active or standby assignment or transition occurs.  Pass what happened into the wakeable.

      EntityIdentifier getActive(void) const;
      void setActive(EntityIdentifier id);
      EntityIdentifier getStandby(void) const;
      void setStandby(EntityIdentifier id);

    protected:
      static SAFplus::Checkpoint        mCheckpoint;
      SAFplus::GroupHashMap*            map;
      SAFplus::Handle                   handle;
      SAFplus::Wakeable*                wakeable;
      EntityIdentifier                  activeEntity;
      EntityIdentifier                  standbyEntity;
      EntityIdentifier                  lastRegisteredEntity;
  };
}

#endif
