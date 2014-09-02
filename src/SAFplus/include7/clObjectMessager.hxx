#include <clMsgHandler.hxx>
#include <clSafplusMsgServer.hxx>
#include <boost/unordered_map.hpp>     //boost::unordered_map

namespace SAFplus
  {

  // Some local data about the groups -- what the last shared memory change is that we handled, and the list of group objects per shared memory group.
  typedef boost::unordered_map < SAFplus::Handle, SAFplus::MsgHandler*, boost::hash<SAFplus::Handle>, std::equal_to<SAFplus::Handle> > ObjectMessagerMap;


  class ObjectMessager:public SAFplus::MsgHandler
    {
    public:
    virtual void msgHandler(ClIocAddressT from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);

    void insert(SAFplus::Handle h, SAFplus::MsgHandler* obj);
    void remove(SAFplus::Handle h);

    protected:
    ObjectMessagerMap omap;
    };

  extern ObjectMessager objectMessager;

  };
