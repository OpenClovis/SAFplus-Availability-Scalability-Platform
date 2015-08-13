#include <clCustomization.hxx>
#include <clThreadApi.hxx>
#include <clCkptApi.hxx>
#include <clCkptIpi.hxx>
#include <boost/asio/deadline_timer.hpp>
#include <boost/unordered_map.hpp>
#include <boost/container/map.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <string>

using namespace boost::interprocess;
namespace fs = boost::filesystem;

/* The callback function declaration for timer handler */
class RetentionTimer;

typedef void (*TimeoutCb) (const boost::system::error_code&, void*, RetentionTimer*);

/* ThreadSem to prevent access map simultanceously */
SAFplus::ThreadSem shmSem(1);


/* this class wraps the boost deadline timer so that we can operate boost deadline timer via an object */
class Timer
{
protected:    
  boost::asio::io_service& iosvc;
  TimeoutCb fpOnTimeout;

public:  
  boost::asio::deadline_timer* timer;
  uint64_t waitDuration;

public:
  Timer(uint64_t wd, boost::asio::io_service& _iosvc, TimeoutCb cb): waitDuration(wd), iosvc(_iosvc), fpOnTimeout(cb)
  {
    timer = new boost::asio::deadline_timer(_iosvc);
  }
  /* wait on a duration */
  void wait(void* arg, RetentionTimer* rt=NULL)
  {   
    expire();
    timer->async_wait(boost::bind(fpOnTimeout,boost::asio::placeholders::error, arg, rt));    
  }
  /* Force the timer to expire immediately */
  void expire()
  {
    timer->expires_from_now(boost::posix_time::seconds(waitDuration));
  }
  /* Cancel the timer immediately, the timer no longer runs */
  void cancel()
  {
    timer->cancel();
  }
};

/* this struct contains the timer pointer and retention duration for the timer */
struct RetentionTimerData 
{
  Timer timer;
  boost::posix_time::ptime lastUsed; // in seconds 
  bool isRunning; // states that if this timer is running or not 
  RetentionTimerData(Timer t, boost::posix_time::ptime _lastUsed, bool running): timer(t), lastUsed(_lastUsed), isRunning(running)
  {
  }
};


typedef std::pair<const SAFplus::Handle, RetentionTimerData> CkptTimerMapPair;
typedef boost::unordered_map<SAFplus::Handle, RetentionTimerData> CkptTimerMap; // a map contains ckptHandle as a key and struct RetentionTimerData as a value

void onSharedMemRetentionTimeout(const boost::system::error_code& e, void* arg, RetentionTimer* rt);
void onPersistentRetentionTimeout(const boost::system::error_code& e, void* arg, RetentionTimer* rt);

/* Retention timer class: use this class for retention timer for both shared memory files and persistent files on disk */
class RetentionTimer
{
public:
  CkptTimerMap ckptTimerMap;
    
  // an entry for starting retention timer
  void startTimer(CkptTimerMap::iterator& iter)
  {    
    logTrace("CKPRET", "START", "Enter [%s]", __FUNCTION__);
    Timer& timer = iter->second.timer;
    if (!iter->second.isRunning)
    {
      logTrace("CKPRET", "START", "starting timer of ckpt handle [%" PRIx64 ":%" PRIx64 "]; Duration [%lu]", iter->first.id[0], iter->first.id[1], timer.waitDuration);
      timer.wait((void*)&iter->first, this);    
      iter->second.isRunning = true;
    }
    else
    {
      logTrace("CKPRET", "START", "Timer for handle [%" PRIx64 ":%" PRIx64 "] is running. Do not start again", iter->first.id[0], iter->first.id[1]);
    }
  }
  // an entry for stopping retention timer
  void stopTimer(CkptTimerMap::iterator& iter)
  {
    logTrace("CKPRET", "STOP", "Enter [%s]", __FUNCTION__);
    Timer& timer = iter->second.timer;
    if (iter->second.isRunning) 
    {
      logTrace("CKPRET", "STOP", "canceling timer of ckpt handle [%" PRIx64 ":%" PRIx64 "]", iter->first.id[0], iter->first.id[1]);
      timer.cancel();
      iter->second.isRunning = false;
    }  
    else
    {
      logTrace("CKPRET", "STOP", "Timer for handle [%" PRIx64 ":%" PRIx64 "] is not running. Do nothing", iter->first.id[0], iter->first.id[1]);
    }
  }
  // Fill out the retentionTimerMap, also update in case there is a new checkpoint created
  virtual void updateRetentionTimerMap(const char* path, boost::asio::io_service& iosvc)=0;  
};

class SharedMemFileTimer: public RetentionTimer
{
public:uint64_t waitDuration;
  virtual void updateRetentionTimerMap(const char* path, boost::asio::io_service& iosvc)
  {
    /* Using boost library to walk thru the /dev/shm/ to get all the checkpoint shared memory files following the format: cktp_handle.id[0]:handle.id[1]:retentionDuration */
    logTrace("CKPRET","UDT", "[shm] Enter [%s]", __FUNCTION__);
    int lastSlashPos, ckptShmPos;
    fs::directory_iterator end_iter;
    for( fs::directory_iterator dir_iter(SharedMemPath) ; dir_iter != end_iter ; ++dir_iter)
    {
      if (!fs::is_regular_file(dir_iter->status())) continue;  // skip any sym links, etc.
      // Extract just the file name by bracketing it between the preceding / and the file extension
      std::string filePath = dir_iter->path().string();
      logTrace("CKPRET","UDT", "[shm] parsing file [%s]", filePath.c_str());
      // skip ahead to the last / so a name like /dev/shm/ckpt_ works
      lastSlashPos = filePath.rfind("/");
      ckptShmPos = filePath.find("ckpt_", lastSlashPos);
      int flen = filePath.length();
      if (ckptShmPos >= 0 && ckptShmPos < flen)
      {
        int hdlStartPos = filePath.find("_", ckptShmPos);                
        if (hdlStartPos >= 0 && hdlStartPos < flen)
        {
          hdlStartPos++;
          std::string strHdl = filePath.substr(hdlStartPos);
          logTrace("CKPRET","UDT", "[shm] strHdl [%s]", strHdl.c_str());
          SAFplus::Handle ckptHdl;
          ckptHdl.fromStr(strHdl);          
          // get existing checkpoint header
          std::string tempStr = filePath.substr(ckptShmPos);
          logTrace("CKPRET","UDT", "[shm] Opening shm [%s]", tempStr.c_str());
          managed_shared_memory managed_shm(open_only, tempStr.c_str());
          std::pair<SAFplusI::CkptBufferHeader*, std::size_t> ckptHdr = managed_shm.find<SAFplusI::CkptBufferHeader>("header");
          if (!ckptHdr.first)
          {
            logNotice("CKPRET","UDT", "[shm] No header found for checkpoint [%s]", strHdl.c_str());
            continue; // no header found so bypass this checkpoint 
          }                
          if (ckptHdl != ckptHdr.first->handle) 
          {
            logNotice("CKPRET","UDT", "[shm] Header found for checkpoint [%s] but its handle is invalid", strHdl.c_str());
            continue; // it's not a valid handle, so bypass it and go to another
          }
          //check if this handle exists in the map
          CkptTimerMap::iterator contents = ckptTimerMap.find(ckptHdl);
          if (contents != ckptTimerMap.end()) // exist
          {
            logTrace("CKPRET","UDT", "[shm] CkptHandle [%" PRIx64 ":%" PRIx64 "] exists in the map. Update the lastUsed field only", ckptHdl.id[0], ckptHdl.id[1]);
            // update the lastUsed only
            contents->second.lastUsed = ckptHdr.first->lastUsed;
          }
          else
          {
            uint64_t retentionDuration = ckptHdr.first->retentionDuration;          
            // Add new item to map
            logTrace("CKPRET","UDT", "[shm] insert item {%" PRIx64 ":%" PRIx64 ", %lu} to map", ckptHdl.id[0], ckptHdl.id[1], retentionDuration);
            Timer timer(retentionDuration, iosvc, &onSharedMemRetentionTimeout);
            RetentionTimerData rt(timer, ckptHdr.first->lastUsed, false);
            CkptTimerMapPair kv(ckptHdl, rt);
            ckptTimerMap.insert(kv);                              
          }
        }
        else
        {
          logWarning("CKPRET", "UDT", "[shm] Malformed ckpt shared mem file [%s] for the handle", filePath.c_str());
        }
      }
      else
      {
        logTrace("CKPRET", "UDT", "[shm] [%s] is not a ckpt shared mem file", filePath.c_str());
      }
    }   
  }  
};

class PersistentFileTimer: public RetentionTimer
{
public:
  virtual void updateRetentionTimerMap(const char* path, boost::asio::io_service& iosvc)
  {
    /* TODO: using boost library to walk thru the path to get all the checkpoint persistent files following the format: cktp_handle.id[0]:handle.id[1]:retentionDuration.db
     Loop thru all files to get the ckpt handle, retention duration, handle.id[1] (as a ProcGate semId)
    */
    // Add item to map
#if 0
    uint64_t persistentRetentionDuration;
    Timer timer(persistentRetentionDuration, iosvc, &onPersistentRetentionTimeout);
    SAFplus::Handle ckptHandle;
    SAFplus::ProcSem gate(ckptHandle.id[1]);
    RetentionTimerData rt(timer, gate, false);
    CkptTimerMapPair value(ckptHandle, rt);
    ckptTimerMap.insert(value);
#endif
  } 
};

/* struct definition for argument for timer handler */
struct TimerArg
{
  Timer* hostTimer;
  SharedMemFileTimer* shmTimer;
  PersistentFileTimer* perstTimer;
  TimerArg(Timer* _hostTimer, SharedMemFileTimer* _shmTimer, PersistentFileTimer* _perstTimer): hostTimer(_hostTimer), shmTimer(_shmTimer), perstTimer(_perstTimer)
  {
  }
};


/* Global variables and functions */

boost::asio::io_service io; // io service to associate with one or many deadline timers. In this case, it associates with ckptUpdateTimer and ckptRetentionTimer
// the onTimeout function for onCkptUpdateTimeout
void onCkptUpdateTimeout(const boost::system::error_code& e, void* arg, RetentionTimer* rt); 
// update timer ckpt retention data and start or stop timer accordingly
void updateTimer(RetentionTimer* timer, boost::asio::io_service& iosvc);
// Run the io service
void runIoService();

int main()
{  
  SAFplus::logSeverity = SAFplus::LOG_SEV_TRACE;
  SAFplus::logEchoToFd = 1;  // echo logs to stdout for debugging 
  SAFplus::safplusInitialize(SAFplus::LibDep::CKPT | SAFplus::LibDep::LOG);
  // Add work to io service so that the deadline timer continues working after it restarts
  boost::asio::io_service::work work(io);
  
  SharedMemFileTimer sharedMemFilesTimer;
  PersistentFileTimer persistentFilesTimer;  
  // start the ckpt update
  Timer ckptUpdateTimer(SAFplusI::CkptUpdateDuration, io, onCkptUpdateTimeout);
  //TimerArg timerArg(&ckptUpdateTimer, &sharedMemFilesTimer, &persistentFilesTimer);
  TimerArg timerArg(&ckptUpdateTimer, &sharedMemFilesTimer, NULL);
  ckptUpdateTimer.wait(&timerArg);
  // run the io service
  runIoService();  // The program control blocks here
  return 1;
}

void onCkptUpdateTimeout(const boost::system::error_code& e, void* arg, RetentionTimer* rt)
{
  logTrace("CKPRET", "STACHK", "Enter [%s]", __FUNCTION__);
  // get the RetentionTimer objects from arg (both sharedMemfiles and persistent files)  
  TimerArg* tArg = (TimerArg*) arg;
  updateTimer(tArg->shmTimer, io);
  updateTimer(tArg->perstTimer, io);
  // start a new ckpt update cycle  
  tArg->hostTimer->wait(arg);
}

void updateTimer(RetentionTimer* timer, boost::asio::io_service& iosvc)
{
  logTrace("CKPRET", "UPTTMR", "Enter [%s]", __FUNCTION__);
  if (!timer)
  {
    logNotice("CKPRET", "UPTTMR", "timer argument is NULL. Do not handle it");
    return;
  }
  shmSem.lock();
  timer->updateRetentionTimerMap(SharedMemPath, iosvc);
  // Loop thru the map for each ckpt, start the timer for each checkpoint
  for(CkptTimerMap::iterator iter = timer->ckptTimerMap.begin(); iter != timer->ckptTimerMap.end(); iter++)
  {    
    timer->startTimer(iter);    
  }
  shmSem.unlock();
}

void runIoService()
{
  io.run();
}

void onSharedMemRetentionTimeout(const boost::system::error_code& e, void* arg, RetentionTimer* rt)
{
  logTrace("CKPRET", "TIMEOUT", "Enter [%s]", __FUNCTION__);
  if (e != boost::asio::error::operation_aborted)
  {
    // Timer was not cancelled, the timer has expired within retentionDuration  
    shmSem.lock();
    SAFplus::Handle* ckptHandle = (SAFplus::Handle*)arg;
    SharedMemFileTimer* timer = (SharedMemFileTimer*)rt;        
    CkptTimerMap::iterator contents = timer->ckptTimerMap.find(*ckptHandle);
    if (contents != timer->ckptTimerMap.end())
    { 
      char sharedMemFile[256];
      strcpy(sharedMemFile,"ckpt_");
      ckptHandle->toStr(&sharedMemFile[5]);
      // Get the checkpoint header to see whether the lastUsed changed or not      
      managed_shared_memory managed_shm(open_only, sharedMemFile);
      std::pair<SAFplusI::CkptBufferHeader*, std::size_t> ckptHdr = managed_shm.find<SAFplusI::CkptBufferHeader>("header");
      if (!ckptHdr.first)
      {
        logWarning("CKPRET", "TIMEOUT", "[%s] header for checkpoint [%" PRIx64 ":%" PRIx64 "] not found", __FUNCTION__, ckptHandle->id[0], ckptHandle->id[1]);
        shmSem.unlock();
        return;
      }
      contents->second.isRunning = false;
      if (contents->second.lastUsed != ckptHdr.first->lastUsed)
      {
        // don't delete the checkpoint because it's still accessed by a process or app. So, restart the timer
        logTrace("CKPRET", "TIMEOUT", "[%s] Restart timer for checkpoint [%" PRIx64 ":%" PRIx64 "]", __FUNCTION__, ckptHandle->id[0], ckptHandle->id[1]);
        timer->startTimer(contents);
      }
      else
      {
        // delete the checkpoint        
        Timer& tmr = contents->second.timer;
        logTrace("CKPRET", "TIMEOUT", "[%s] deleting shared mem file [%s]", __FUNCTION__, sharedMemFile);
        shared_memory_object::remove(sharedMemFile);
        delete tmr.timer;
        // Remove this item from the map, too
        timer->ckptTimerMap.erase(contents);
      }
      shmSem.unlock();
    }
  }
  else
  {
    logDebug("CKPRET", "TIMEOUT", "[%s] timer was cancelled because the specified checkpoint is opened", __FUNCTION__);
  }
}

void onPersistentRetentionTimeout(const boost::system::error_code& e, void* arg, RetentionTimer* rt)
{
  if (e != boost::asio::error::operation_aborted)
  {
    // Timer was not cancelled, the timer has expired within retentionDuration, so delete data    
    SAFplus::Handle* ckptHandle = (SAFplus::Handle*)arg;
    PersistentFileTimer* timer = (PersistentFileTimer*)rt;
    char path[256];
    // TODO: construct the checkpoint persistent file based on its handle and persistent retention duration   
    unlink(path); // remove file on disk
    // Remove this item from the map, too
    CkptTimerMap::iterator contents = timer->ckptTimerMap.find(*ckptHandle);
    if (contents != timer->ckptTimerMap.end())
    {     
      delete contents->second.timer.timer;
      timer->ckptTimerMap.erase(contents);
    }
  }
  else
  {
    logDebug("CKPRET", "TIMEOUT", "timer was cancelled because the specified checkpoint is opened");
  }
}
