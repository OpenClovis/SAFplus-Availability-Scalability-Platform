#include <clThreadPool.hxx>

namespace SAFplus
{
class MsgServer;

class MakePrimary:public Poolable
  {
  public:
  MakePrimary():Poolable(nullptr, nullptr) {}
  MsgServer* msgSvr;

  virtual void wake(int amt, void* cookie);
  };

};



