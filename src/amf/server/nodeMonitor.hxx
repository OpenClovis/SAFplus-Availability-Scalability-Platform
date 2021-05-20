#pragma once

#include <clMsgApi.hxx>
#include <clThreadApi.hxx>
#include <clGroupApi.hxx>

enum
  {
  NODENAME_LEN = 16*8,              // Make sure it is a multiple of 64 bits
  SC_ELECTION_BIT = 1<<8,           // This bit is set in the credential if the node is a system controller, making SCs preferred 
  STARTUP_ELECTION_DELAY_MS = 2000,  // Wait for 5 seconds during startup if nobody is active so that other nodes can arrive, making the initial election not dependent on small timing issues. 

  REEVALUATION_DELAY = 1000, //? How long to wait (max) before reevaluating the cluster
  };

class NodeMonitor:public SAFplus::MsgHandler
{
public:
  SAFplus::Handle currentActive;
  NodeMonitor(): active(false), standby(false),quit(true) {};  // two step constructor
  ~NodeMonitor();
  void initialize(void);
  void finalize(void);
  void becomeActive(void);
  void becomeStandby(void);
  void monitorThread(void);
  void operator() () { monitorThread(); }
  virtual void msgHandler(SAFplus::MsgServer* svr, SAFplus::Message* msg, ClPtrT cookie);
protected:

  SAFplus::Mutex exclusion;
  int64_t lastHeard[SAFplus::MaxNodes];
  int64_t lastHbRequest;
  SAFplus::Handle lastHbHandle;
  boost::thread thread;
  bool active;
  bool standby;
  bool quit;
  std::vector<const SAFplus::GroupIdentity*> members;
};
