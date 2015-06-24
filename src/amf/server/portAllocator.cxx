#include "clPortAllocator.hxx"

namespace SAFplusI 
{
  PortAllocator::PortAllocator()
  {
    for (int i=0; i < PortArrayLen; i++) pid[i] = 0; // Set to available: TODO, actually check to see if the port is used or unused.    
  }

  int PortAllocator::allocPort()
  {
    // These linear searches are not expected to matter performance-wise because processes are started rarely
    for (int i=0; i < PortArrayLen; i++)
      {
        if (pid[i] == 0)
          {
            pid[i] = 1; // a temporary indication that we've handed this port out.
            return i+SAFplus::MsgDynamicPortStart;
          }
      }

    return 0;  // No ports available, return invalid port
  }

  void PortAllocator::assignPort(int port, pid_t thepid)
  {
    assert(port >= SAFplus::MsgDynamicPortStart);
    assert(port < SAFplus::MsgDynamicPortEnd);
    port -= SAFplus::MsgDynamicPortStart;
    assert(pid[port] == 1);  // We must have set it to 1 when allocPort was called.
    pid[port] = thepid;
  }

  void PortAllocator::releasePort(int port)
  {
    assert(port >= SAFplus::MsgDynamicPortStart);
    assert(port < SAFplus::MsgDynamicPortEnd);
    port -= SAFplus::MsgDynamicPortStart;
    pid[port] = 0;
  }

  void PortAllocator::releasePortByPid(pid_t thepid)
  {
    // These linear searches are not expected to matter performance-wise because processes fail rarely
    for (int i=0; i < PortArrayLen; i++)
      {
        if (pid[i] == thepid)
          {
            pid[i] = 0;
            // return;  // This pid should only have one port but just clean up the whole array to be certain
          }
      }
  }

  PortAllocator portAllocator;

};
