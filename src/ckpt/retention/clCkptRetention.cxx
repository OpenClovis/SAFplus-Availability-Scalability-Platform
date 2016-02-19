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
using namespace SAFplusI;
namespace fs = boost::filesystem;

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
  SAFplus::DbalPlugin* pDbal; // database handle to manipulate with persistent checkpoint. With non-persistent checkpoint, this variable is always NULL
  std::string name; // Represents the physical file name of specified checkpoint type. For example, if this retention is based on share memory, its value is the name of shared memory (not included path). If this retention is persistence, its value is the full path to the database file name on disk
  RetentionTimerData(Timer t, boost::posix_time::ptime _lastUsed, bool running, std::string _name, SAFplus::DbalPlugin* _pDbal=NULL): timer(t), lastUsed(_lastUsed), isRunning(running), name(_name), pDbal(_pDbal)
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
protected:
  SAFplus::ThreadSem sem;

public:  
  CkptTimerMap ckptTimerMap;
    
  RetentionTimer()
  {
    sem.init(1);
  }
  void lock()
  {
    sem.lock();
  }
  void unlock()
  {
    sem.unlock();
  }
  // an entry for starting retention timer
  void startTimer(CkptTimerMap::iterator& iter)
  {    
    logTrace("CKPRET", "START", "Enter [%s]", __FUNCTION__);
    Timer& timer = iter->second.timer;
    if (!iter->second.isRunning)
    {
      logTrace("CKPRET", "START", "starting timer of ckpt handle [%" PRIx64 ":%" PRIx64 "]; Duration [%" PRIx64 "]", iter->first.id[0], iter->first.id[1], timer.waitDuration);
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
  virtual void updateRetentionTimerMap(boost::asio::io_service& iosvc)=0;  
};

class SharedMemFileTimer: public RetentionTimer
{
public:
  
  virtual void updateRetentionTimerMap(boost::asio::io_service& iosvc)
  {
    /* Using boost library to walk thru the /dev/shm/ to get all the checkpoint shared memory files following the format: SAFplusCkpt__handle.id[0]:handle.id[1] */
    logTrace("CKPRET","UDT", "[shm] Enter [%s]", __FUNCTION__);    
    int lastSlashPos, ckptShmPos;
    fs::directory_iterator end_iter;
    for( fs::directory_iterator dir_iter(SharedMemPath) ; dir_iter != end_iter ; ++dir_iter)
    {
      if (!fs::is_regular_file(dir_iter->status())) continue;  // skip any sym links, etc.
      // Extract just the file name by bracketing it between the preceding / and the file extension
      std::string filePath = dir_iter->path().string();
      logTrace("CKPRET","UDT", "[shm] parsing file [%s]", filePath.c_str());
      // skip ahead to the last / so a name like /dev/shm/SAFplusCkpt_ works
      lastSlashPos = filePath.rfind("/");
      ckptShmPos = filePath.find("SAFplusCkpt_", lastSlashPos);
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
            logTrace("CKPRET","UDT", "[shm] insert item {%" PRIx64 ":%" PRIx64 ", %" PRIx64 "} to map", ckptHdl.id[0], ckptHdl.id[1], retentionDuration);
            Timer timer(retentionDuration, iosvc, &onSharedMemRetentionTimeout);
            RetentionTimerData rt(timer, ckptHdr.first->lastUsed, false, filePath.substr(ckptShmPos));
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
  bool getHdrValue(SAFplus::DbalPlugin* dbal, int nkey, void* val)
  {
    if (!dbal) return false;
    ClDBKeyHandleT key;
    ClUint32T keySize;
    ClDBRecordHandleT rec;
    ClUint32T recSize;
    ClRcT rc = dbal->getFirstRecord(&key, &keySize, &rec, &recSize);  
    while (rc == CL_OK)
    {    
      if (!memcmp(key, ckptHeader(nkey), keySize))
      {
        memcpy(val, rec, recSize);
        dbal->freeKey(key);
        dbal->freeRecord(rec);
        return true;
      }
      dbal->freeRecord(rec);
      ClDBKeyHandleT curKey = key;
      ClUint32T curKeySize = keySize;    
      rc = dbal->getNextRecord(curKey, curKeySize, &key, &keySize, &rec, &recSize);     
      dbal->freeKey(curKey);        
    }
    return false;
  }  
  
  virtual void updateRetentionTimerMap(boost::asio::io_service& iosvc)
  {
    /* TODO: using boost library to walk thru the path to get all the checkpoint persistent files following the format: SAFplusCkpt_handle.id[0]:handle.id[1].db     
    */     
    logTrace("CKPRET","UDT", "[persist] Enter [%s]", __FUNCTION__);
    ClRcT rc;
    SAFplus::DbalPlugin* dbal;
    const char* path=SAFplus::ASP_RUNDIR;
    if (!path || !strlen(SAFplus::ASP_RUNDIR))
    {
      path = "./";
    }
    int lastSlashPos, ckptPersistPos;
    fs::directory_iterator end_iter;
    for( fs::directory_iterator dir_iter(path) ; dir_iter != end_iter ; ++dir_iter)
    {
      if (!fs::is_regular_file(dir_iter->status())) continue;  // skip any sym links, etc.
      // Extract just the file name by bracketing it between the preceding / and the file extension
      std::string filePath = dir_iter->path().string();
      logTrace("CKPRET","UDT", "[persist] parsing file [%s]", filePath.c_str());
      // skip ahead to the last / so a name like .../SAFplusCkpt_ works
      lastSlashPos = filePath.rfind("/");
      ckptPersistPos = filePath.find("SAFplusCkpt_", lastSlashPos);
      int flen = filePath.length();
      if (ckptPersistPos >= 0 && ckptPersistPos < flen)
      {
        int hdlStartPos = filePath.find("_", ckptPersistPos);
        int hdlEndPos = filePath.find(".db", ckptPersistPos);
        if (hdlStartPos >= 0 && hdlStartPos < flen && hdlEndPos>=0 && hdlEndPos<flen)
        {
          hdlStartPos++;
          std::string strHdl = filePath.substr(hdlStartPos, hdlEndPos-hdlStartPos);
          logTrace("CKPRET","UDT", "[persist] strHdl [%s]", strHdl.c_str());
          SAFplus::Handle ckptHdl;
          ckptHdl.fromStr(strHdl);          
          CkptTimerMap::iterator contents = ckptTimerMap.find(ckptHdl);
          if (contents != ckptTimerMap.end()) // exist
          {
            logTrace("CKPRET","UDT", "[persist] CkptHandle [%" PRIx64 ":%" PRIx64 "] exists in the map. Update the lastUsed field only", ckptHdl.id[0], ckptHdl.id[1]);
            // update the lastUsed only
            boost::posix_time::ptime lUsed;
            if (getHdrValue(contents->second.pDbal, lastUsed, &lUsed))
               contents->second.lastUsed = lUsed;
          }
          else
          {
            dbal = SAFplus::clDbalObjCreate();
            rc = dbal->open(filePath.c_str(), filePath.c_str(), CL_DB_OPEN, CkptMaxKeySize, CkptMaxRecordSize);
            if (rc == CL_OK)
            {
              // Read database to get the checkpoint handle
              SAFplus::Handle tmpHdl;
              if (!(getHdrValue(dbal, hdl, &tmpHdl)))
              {
                logNotice("CKPRET","UDT", "[persist] No checkpoint handle from header found for checkpoint [%s]", strHdl.c_str());
                continue; // no header found so bypass this checkpoint 
              }
              if (ckptHdl != tmpHdl) 
              {
                logNotice("CKPRET","UDT", "[persist] Handle from header found for checkpoint [%s] but its handle is invalid", strHdl.c_str());
                continue; // it's not a valid handle, so bypass it and go to another
              }  
              uint64_t retDuration;          
              boost::posix_time::ptime lUsed;
              if (getHdrValue(dbal, retentionDuration, &retDuration) && getHdrValue(dbal, lastUsed, &lUsed))
              {
                // Add new item to map
                logTrace("CKPRET","UDT", "[persist] insert item {%" PRIx64 ":%" PRIx64 ", %" PRIx64 "} to map", ckptHdl.id[0], ckptHdl.id[1], retDuration);
                Timer timer(retDuration, iosvc, &onPersistentRetentionTimeout);
                RetentionTimerData rt(timer, lUsed, false, filePath, dbal);
                CkptTimerMapPair kv(ckptHdl, rt);
                ckptTimerMap.insert(kv);                              
              }
            }
          }
        }
        else
        {
          logWarning("CKPRET", "UDT", "[persist] Malformed ckpt peristent file [%s] for the handle", filePath.c_str());
        }
      }
      else
      {
        logTrace("CKPRET", "UDT", "[persist] [%s] is not a ckpt persistent file", filePath.c_str());
      }
    }
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
  TimerArg timerArg(&ckptUpdateTimer, &sharedMemFilesTimer, &persistentFilesTimer);
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
  timer->lock();
  timer->updateRetentionTimerMap(iosvc);
  // Loop thru the map for each ckpt, start the timer for each checkpoint
  for(CkptTimerMap::iterator iter = timer->ckptTimerMap.begin(); iter != timer->ckptTimerMap.end(); iter++)
  {    
    timer->startTimer(iter);    
  }
  timer->unlock();
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
    SAFplus::Handle* ckptHandle = (SAFplus::Handle*)arg;
    SharedMemFileTimer* timer = (SharedMemFileTimer*)rt;
    timer->lock();   
    CkptTimerMap::iterator contents = timer->ckptTimerMap.find(*ckptHandle);
    if (contents != timer->ckptTimerMap.end())
    { 
      // Get the checkpoint header to see whether the lastUsed changed or not
      std::string& ckptShmName = contents->second.name;
      logTrace("CKPRET", "TIMEOUT", "[%s] opening shm [%s]", __FUNCTION__, ckptShmName.c_str());     
      managed_shared_memory managed_shm(open_only, ckptShmName.c_str());
      std::pair<SAFplusI::CkptBufferHeader*, std::size_t> ckptHdr = managed_shm.find<SAFplusI::CkptBufferHeader>("header");
      if (!ckptHdr.first)
      {
        logWarning("CKPRET", "TIMEOUT", "[%s] header for checkpoint [%" PRIx64 ":%" PRIx64 "] not found", __FUNCTION__, ckptHandle->id[0], ckptHandle->id[1]);
        timer->unlock();
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
        logTrace("CKPRET", "TIMEOUT", "[%s] deleting shared mem file [%s]", __FUNCTION__, ckptShmName.c_str());
        shared_memory_object::remove(ckptShmName.c_str());
        delete tmr.timer;
        // Remove this item from the map, too
        timer->ckptTimerMap.erase(contents);
      }      
    }
    timer->unlock();
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
    // Timer was not cancelled, the timer has expired within retentionDuration  
    SAFplus::Handle* ckptHandle = (SAFplus::Handle*)arg;
    PersistentFileTimer* timer = (PersistentFileTimer*)rt;        
    timer->lock();
    CkptTimerMap::iterator contents = timer->ckptTimerMap.find(*ckptHandle);    
    if (contents != timer->ckptTimerMap.end())
    { 
      // Get the checkpoint header to see whether the lastUsed changed or not
      std::string& ckptPersistName = contents->second.name;   
      boost::posix_time::ptime lUsed;
      if (!timer->getHdrValue(contents->second.pDbal, lastUsed, &lUsed))      
      {
        logWarning("CKPRET", "TIMEOUT", "[%s] lastUsed from header for checkpoint [%" PRIx64 ":%" PRIx64 "] not found", __FUNCTION__, ckptHandle->id[0], ckptHandle->id[1]);
        timer->unlock();
        return;
      }
      contents->second.isRunning = false;
      if (contents->second.lastUsed != lUsed)
      {
        // don't delete the checkpoint because it's still accessed by a process or app. So, restart the timer
        logTrace("CKPRET", "TIMEOUT", "[%s] Restart timer for checkpoint [%" PRIx64 ":%" PRIx64 "]", __FUNCTION__, ckptHandle->id[0], ckptHandle->id[1]);
        timer->startTimer(contents);
      }
      else
      {
        // delete the persistent checkpoint        
        Timer& tmr = contents->second.timer;
        logTrace("CKPRET", "TIMEOUT", "[%s] deleting peristent checkpoint file [%s]", __FUNCTION__, ckptPersistName.c_str());
        // close the database handle
        delete contents->second.pDbal;
        unlink(ckptPersistName.c_str());
        delete tmr.timer;
        // Remove this item from the map, too
        timer->ckptTimerMap.erase(contents);
      }      
    }
    timer->unlock();
  }
  else
  {
    logDebug("CKPRET", "TIMEOUT", "timer was cancelled because the specified checkpoint is opened");
  }
}
