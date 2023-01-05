#include <clCommon.hxx>
#include <clThreadApi.hxx>

namespace SAFplusI
{

    // Note that this class is currently single-threaded.  TODO: if this data was put into shared memory, it would survive AMF restart and non-AMF-started programs could use it to allocate a port for themselves.
class PortAllocator
  {
  public:
  enum 
    {
      PortArrayLen = SAFplus::MsgDynamicPortEnd-SAFplus::MsgDynamicPortStart
    };

    pid_t pid[PortArrayLen];
    PortAllocator();
    int allocPort();
    void assignPort(int port, pid_t pid);
    void releasePort(int port);

    void releasePortByPid(pid_t pid);
    SAFplus::ProcSem mutex;
  };

extern PortAllocator portAllocator;  // Its a singleton class
};
