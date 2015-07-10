#include <clCommon.hxx>
#include <clGlobals.hxx>
#include <clSafplusMsgServer.hxx>
#include <clMsgHandler.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <ckptprotocol.hxx>
#include <boost/asio/deadline_timer.hpp>
#include <boost/unordered_map.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

//typedef std::pair<const SAFplus::Handle, boost::asio::deadline_timer> CkptTimerMapPair;

/* the boost deadline timer plays as a timer role. Each checkpoint has its own timer
   It counts time within retention duration
   If the timer expires, the data of the checkpoint will be deleted from memory
*/
typedef boost::unordered_map<SAFplus::Handle, boost::asio::deadline_timer*> CkptTimerMap;

using namespace SAFplus;

//SAFplus::SafplusMsgServer safplusMsgServer(ComponentId::Checkpoint);

static SAFplus::SafplusMsgServer ckptMsgServer;
static CkptTimerMap ckptTimerMap;
// io service to work with the deadline timer
static boost::asio::io_service io;
static bool ioRun = false;

bool quit=false;


class CkptServerMsgHandler:public MsgHandler
  {
  public:
    virtual ~CkptServerMsgHandler();
    virtual void msgHandler(SAFplus::Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie);    
    //virtual void msgHandler(MsgServer* svr, Message* msg, ClPtrT cookie);
  };


CkptServerMsgHandler* handler;

void ckptServer(void)
{ 
  logTrace("CKPSVR", "SVR", "Enter ckptServer()");  
  // Initialize this ckptMsgServer on a well-known port, so that ckpt can send notification msg
  // stating its use status on that port
  ckptMsgServer.init(SAFplusI::CKPT_USE_NOTIFIER_PORT,20,2);
  ckptMsgServer.Start();
  handler = new CkptServerMsgHandler();
  logDebug("CKPSVR", "SVR", "Register ckptMsgServer  node [%d]; port [%d] with a msgHandler", ckptMsgServer.handle.getNode(), ckptMsgServer.handle.getPort());
  // Register this ckptMsgServer with a handler on CKPT_MSG_TYPE, so that ckpt can send notification msg
  // with that type
  ckptMsgServer.registerHandler(SAFplusI::CKPT_MSG_TYPE, handler, 0);
  // Add work to io service so that the deadline timer continues working after it restarts
  boost::asio::io_service::work work(io);
}


CkptServerMsgHandler::~CkptServerMsgHandler()
{  
  ckptMsgServer.removeHandler(SAFplusI::CKPT_MSG_TYPE);
}

void onTimeout(const boost::system::error_code& e, const SAFplus::Handle& ckptHandle)
{
  if (e != boost::asio::error::operation_aborted)
  {
    // Timer was not cancelled, the timer has expired within retentionDuration, so delete data    
    logDebug("CKPSVR", "TIMEOUT", "timer was expired. Delete the data of specified checkpoint from memory");
    char sharedMemName[256];
    strcpy(sharedMemName,"ckpt_");
    ckptHandle.toStr(&sharedMemName[5]);
    boost::interprocess::shared_memory_object::remove(sharedMemName);
  }
  else
  {
    logDebug("CKPSVR", "TIMEOUT", "timer was cancelled because the specified checkpoint is opened");
  }
}

void startTimer(const SAFplus::Handle& ckptHandle, uint32_t retentionDuration)
{
  CkptTimerMap::iterator contents = ckptTimerMap.find(ckptHandle);
  if (contents != ckptTimerMap.end())
  {
    boost::asio::deadline_timer* timer = contents->second;
    boost::posix_time::time_duration duration;
    duration = boost::posix_time::seconds(retentionDuration);
    boost::posix_time::ptime startTime = boost::posix_time::second_clock::universal_time();
    boost::posix_time::ptime deadline = startTime + duration;    
    timer->expires_at(deadline);
    //timer->async_wait(onTimeout);
    timer->async_wait(boost::bind(onTimeout, boost::asio::placeholders::error, ckptHandle));
  }
  else
  {
    boost::asio::deadline_timer* timer = new boost::asio::deadline_timer(io, boost::posix_time::seconds(retentionDuration));
    ckptTimerMap[ckptHandle] = timer;    
    //timer->async_wait(onTimeout);
    timer->async_wait(boost::bind(onTimeout, boost::asio::placeholders::error, ckptHandle));
  }
}

void stopTimer(const SAFplus::Handle& ckptHandle)
{
  CkptTimerMap::iterator contents = ckptTimerMap.find(ckptHandle);
  if (contents != ckptTimerMap.end())
  {
    boost::asio::deadline_timer* timer = contents->second;
    timer->cancel();
  }
  else
  {
    logDebug("CKPSVR", "TMRSTOP", "There is no timer for checkpoint [%" PRIx64 ":%" PRIx64 "]. Nothing to do", ckptHandle.id[0], ckptHandle.id[1]);
  }
}

void runIoService()
{
  ioRun = true;
  io.run();
}

void CkptServerMsgHandler::msgHandler(SAFplus::Handle from, MsgServer* svr, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{    
  logInfo("CKPSVR", "RCV", "received message of length [%d] from [%" PRIx64 ":%" PRIx64 "]", (int)msglen, from.id[0], from.id[1]);
  SAFplusI::CkptUseNotificationHdr* hdr = (SAFplusI::CkptUseNotificationHdr*) msg;
  assert((hdr->msgType>>16) == SAFplusI::CKPT_MSG_TYPE);
  switch (hdr->msgType&0xffff)
  {
  case SAFplusI::CKPT_MSG_TYPE_CKPT_OPEN_NOTIFICATION:
    {
      // There is a process opening the checkpoint, stop its timer immediately
      stopTimer(hdr->checkpoint);
      break;
    }      
  case SAFplusI::CKPT_MSG_TYPE_CKPT_CLOSE_NOTIFICATION:
    {
      // There is a process closing the checkpoint, start or restart its timer immediately
      startTimer(hdr->checkpoint, hdr->retentionDuration);
      // this io service can only be called 'run' once
      if (!ioRun)
      {
        boost::thread(runIoService);
      }
      break;
    }
  default:
    {
      logWarning("CKPSVR", "RCV", "Ckpt server received unknown message type [%x]", hdr->msgType&0xffff);
      break;
    }
  }
}

/*void CkptServerMsgHandler::msgHandler(MsgServer* svr, Message* msg, ClPtrT cookie)
{
  logInfo("CKPT", "RCV", "received message from [%" PRIx64 ":%" PRIx64 "]", svr.handle.id[0], svr.handle.id[1]);
  
}*/


void ckptServerQuit(void)
{
  quit = true;
  ckptMsgServer.removeHandler(SAFplusI::CKPT_MSG_TYPE);
  CkptServerMsgHandler* tmp = handler;
  handler = nullptr;
  delete tmp;
  // Stop io service
  io.stop();
}
