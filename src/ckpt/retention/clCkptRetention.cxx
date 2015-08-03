#include <clCustomization.hxx>
#include <clThreadApi.hxx>
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
#define SharedMemPath "/dev/shm/"


namespace fs = boost::filesystem;

typedef boost::container::multimap<std::time_t, fs::path> file_result_set_t;

/* The callback function declaration for timer handler */
class RetentionTimer;

typedef void (*TimeoutCb) (const boost::system::error_code&, void*, RetentionTimer*);


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
    timer = new boost::asio::deadline_timer(_iosvc, boost::posix_time::seconds(wd));
  }
  /* wait on a duration */
  void wait(void* arg, RetentionTimer* rt=NULL)
  {   
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
  //uint64_t retentionDuration; // in second (obsolete)
  // And maybe a ProcGate associated with this checkpoint used to check its status
  SAFplus::ProcGate gate;
  bool isRunning; // states that if this timer is running or not 
  RetentionTimerData(Timer t, SAFplus::ProcGate g, bool running): timer(t), gate(g), isRunning(running)
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
    logTrace("CKPRET", "START", "Enter startTimer");
    Timer& timer = iter->second.timer;
    if (!iter->second.isRunning)
    {
      logTrace("CKPRET", "START", "[startTimer] starting timer of ckpt handle [%" PRIx64 ":%" PRIx64 "]", iter->first.id[0], iter->first.id[1]);
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
    logTrace("CKPRET", "STOP", "Enter stopTimckpt shared mem fileer");
    Timer& timer = iter->second.timer;
    if (iter->second.isRunning) 
    {
      logTrace("CKPRET", "STOP", "[stopTimer] canceling timer of ckpt handle [%" PRIx64 ":%" PRIx64 "]", iter->first.id[0], iter->first.id[1]);
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
    logTrace("CKPRET","UDT", "[shm] Enter updateRetentionTimerMap");
    int lastSlashPos, ckptShmPos;
    fs::directory_iterator end_iter;
    for( fs::directory_iterator dir_iter(SharedMemPath) ; dir_iter != end_iter ; ++dir_iter)
    {
      if (!fs::is_regular_file(dir_iter->status())) continue;  // skip any sym links, etc.
      // Extract just the file name by bracketing it between the preceding / and the file extension
      std::string filePath = dir_iter->path().string();
      logTrace("CKPRET","UDT", "[shm-updateRetentionTimerMap] parsing file [%s]", filePath.c_str());
      // skip ahead to the last / so a name like /dev/shm/ckpt_ works
      lastSlashPos = filePath.rfind("/");
      ckptShmPos = filePath.find("ckpt_", lastSlashPos);
      int flen = filePath.length();
      if (ckptShmPos >= 0 && ckptShmPos < flen)
      {
        int hdlStartPos = filePath.find("_", ckptShmPos);
        hdlStartPos++;        
        if (hdlStartPos >= 0 && hdlStartPos < flen)
        {
          int hdlEndPos = filePath.rfind(":");          
          if (hdlEndPos >= 0 && hdlEndPos < flen)
          {
            std::string strHdl = filePath.substr(hdlStartPos, hdlEndPos-hdlStartPos);
            std::string strRetentionDuration = filePath.substr(hdlEndPos+1, flen-hdlEndPos-1);
            logTrace("CKPRET","UDT", "[shm-updateRetentionTimerMap] strHdl [%s], strRetentionDuration [%s]", strHdl.c_str(), strRetentionDuration.c_str());
            uint64_t retentionDuration = 0;
            try 
            {
              retentionDuration = boost::lexical_cast<uint64_t>(strRetentionDuration);
            }
            catch (...)
            {
              logError("CKPRET", "UDT", "retention duration [%s] is invalid from the shared memory file", strRetentionDuration.c_str());
              continue;
            }
            SAFplus::Handle ckptHdl;
            ckptHdl.fromStr(strHdl);
            if (ckptHdl == SAFplus::INVALID_HDL) continue; // it's not a valid handle, so bypass it and go to another
            //check if this handle exists in the map
            logTrace("CKPRET","UDT", "[shm-updateRetentionTimerMap] Got {CkptHandle,time}:{%" PRIx64 ":%" PRIx64 "%lu}", ckptHdl.id[0], ckptHdl.id[1], retentionDuration);
            CkptTimerMap::iterator contents = ckptTimerMap.find(ckptHdl);
            if (contents == ckptTimerMap.end()) // not exist
            {     
              // Add new item to map
              logTrace("CKPRET","UDT", "[shm-updateRetentionTimerMap] insert item {%" PRIx64 ":%" PRIx64 ", %lu} to map", ckptHdl.id[0], ckptHdl.id[1], retentionDuration);
              Timer timer(retentionDuration, iosvc, &onSharedMemRetentionTimeout);
              SAFplus::ProcGate gate(ckptHdl.id[1]);
              RetentionTimerData rt(timer, gate, false);
              CkptTimerMapPair kv(ckptHdl, rt);
              ckptTimerMap.insert(kv);          
            }
          } 
          else
          {
            logWarning("CKPRET", "UDT", "Malformed ckpt shared mem file [%s] for retention duration", filePath.c_str());
          }
        }
        else
        {
          logWarning("CKPRET", "UDT", "Malformed ckpt shared mem file [%s] for the handle", filePath.c_str());
        }
      }
      else
      {
        logTrace("CKPRET", "UDT", "[%s] is not a ckpt shared mem file", filePath.c_str());
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
    uint64_t persistentRetentionDuration;
    Timer timer(persistentRetentionDuration, iosvc, &onPersistentRetentionTimeout);
    SAFplus::Handle ckptHandle;
    SAFplus::ProcGate gate(ckptHandle.id[1]);
    RetentionTimerData rt(timer, gate, false);
    CkptTimerMapPair value(ckptHandle, rt);
    ckptTimerMap.insert(value);
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

boost::asio::io_service io; // io service to associate with one or many deadline timers. In this case, it associates with ckptUseStatusCheckTimer and ckptRetentionTimer
// the onTimeout function for ckptUseStatusCheckTimer
void onCkptUseStatusCheckTimeout(const boost::system::error_code& e, void* arg, RetentionTimer* rt); 
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
  // start the ckpt use status checking
  Timer ckptUseStatusCheckTimer(SAFplusI::CkptUseCheckDuration, io, onCkptUseStatusCheckTimeout);
  //TimerArg timerArg(&ckptUseStatusCheckTimer, &sharedMemFilesTimer, &persistentFilesTimer);
  TimerArg timerArg(&ckptUseStatusCheckTimer, &sharedMemFilesTimer, NULL);
  ckptUseStatusCheckTimer.wait(&timerArg);
  // run the io service
  runIoService();  // The program control blocks here
}

void onCkptUseStatusCheckTimeout(const boost::system::error_code& e, void* arg, RetentionTimer* rt)
{
  logTrace("CKPRET", "STACHK", "Enter onCkptUseStatusCheckTimeout");
  // get the RetentionTimer objects from arg (both sharedMemfiles and persistent files)  
  TimerArg* tArg = (TimerArg*) arg;
  updateTimer(tArg->shmTimer, io);
  updateTimer(tArg->perstTimer, io);
  // start a new status check cycle
  tArg->hostTimer->expire();
  tArg->hostTimer->wait(arg);
}

void updateTimer(RetentionTimer* timer, boost::asio::io_service& iosvc)
{
  logTrace("CKPRET", "UPTTMR", "Enter updateTimer");
  if (!timer)
  {
    logNotice("CKPRET", "UPTTMR", "[updateTimer] timer argument is NULL. Do not handle it");
    return;
  }
  timer->updateRetentionTimerMap(SharedMemPath, iosvc);
  // Loop thru the map for each ckpt: check to see if there is any process opening this checkpoint
  for(CkptTimerMap::iterator iter = timer->ckptTimerMap.begin(); iter != timer->ckptTimerMap.end(); iter++)
  {    
    SAFplus::ProcGate& gate = iter->second.gate;
    if (gate.try_lock()) // there is a process opening this ckpt
    {
      timer->stopTimer(iter);
    }
    else
    {
      timer->startTimer(iter);
    }
  }
}

void runIoService()
{
  io.run();
}

void onSharedMemRetentionTimeout(const boost::system::error_code& e, void* arg, RetentionTimer* rt)
{
  logTrace("CKPRET", "TIMEOUT", "Enter onSharedMemRetentionTimeout");
  if (e != boost::asio::error::operation_aborted)
  {
    // Timer was not cancelled, the timer has expired within retentionDuration, so delete data    
    SAFplus::Handle* ckptHandle = (SAFplus::Handle*)arg;
    SharedMemFileTimer* timer = (SharedMemFileTimer*)rt;        
    CkptTimerMap::iterator contents = timer->ckptTimerMap.find(*ckptHandle);
    if (contents != timer->ckptTimerMap.end())
    { 
      char sharedMemFile[256];
      Timer& tmr = contents->second.timer;    
      // TODO: construct the checkpoint name based on its handle and retention duration
      strcpy(sharedMemFile,"ckpt_");
      ckptHandle->toStr(&sharedMemFile[5]);
      std::string strTime = boost::lexical_cast<std::string>(tmr.waitDuration);
      strcat(sharedMemFile, ":");
      strcat(sharedMemFile, strTime.c_str());
      logTrace("CKPRET", "TIMEOUT", "[onSharedMemRetentionTimeout] deleting shared mem file [%s]", sharedMemFile);
      boost::interprocess::shared_memory_object::remove(sharedMemFile);
      delete tmr.timer;
      // Remove this item from the map, too
      timer->ckptTimerMap.erase(contents);
    }
  }
  else
  {
    logDebug("CKPRET", "TIMEOUT", "timer was cancelled because the specified checkpoint is opened");
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
