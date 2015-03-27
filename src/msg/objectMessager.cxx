#include <clObjectMessager.hxx>
#include <clMsgPortsAndTypes.hxx>

namespace SAFplus
  {
  ObjectMessager objectMessager;

  void objectMessagerInitialize()
    {
    safplusMsgServer.registerHandler(SAFplusI::OBJECT_MSG_TYPE,&objectMessager,0);
    }

  void ObjectMessager::msgHandler(Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
    {
    Handle* h = (Handle*) msg;
    ObjectMessagerMap::iterator objref = omap.find(*h);
    if (objref == omap.end())
      {
      logWarning("MSG","OBJ","Object message request to handle [%" PRIx64 ":%" PRIx64 "] failed.  No mapping exists.",h->id[0],h->id[1]);
      return;
      }
    SAFplus::MsgHandler* obj = objref->second;
    obj->msgHandler(from, svr, ((char*)msg)+sizeof(Handle), msglen-sizeof(Handle),  (ClPtrT) SAFplusI::OBJECT_MSG_TYPE);  // TODO: cookie should identify this as an object message but not use SAFplusI namespace b/c app should not use SAFplusI

    }

  void ObjectMessager::insert(SAFplus::Handle h, SAFplus::MsgHandler* obj)
    {
    omap[h] = obj;
    }

  void ObjectMessager::remove(SAFplus::Handle h)
    {
    omap.erase(h);
    }

  };
