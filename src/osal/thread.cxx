#include <clCommon.hxx>
#include <clThreadApi.hxx>
#include <cltypes.h>
//#include <clCksmApi.h>
//#include <clCommon.h>
//#include <clCommonErrors.h>
#include <errno.h>
#include <stdio.h>

namespace SAFplus
{

Wakeable& BLOCK = *((Wakeable*) NULL);
Wakeable& ABORT = *((Wakeable*) 1);
WakeableNoop IGNORE;

  /* from semctl man page, calling program needs to define this struct */
typedef union CosSemCtl_u
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo  *__buf; 
} CosSemCtl_t;

  
  ProcSem::ProcSem(unsigned int key,int initialValue)
  {
    init(key,initialValue);   
  }

  ProcSem::ProcSem(const char* key,int initialValue)
  {
    ClUint32T realKey;
    int keyLen = strlen(key);
    realKey = computeCrc32((ClUint8T*) key, keyLen);
    init(realKey,initialValue);
  }
  
  void ProcSem::init(const char* key,int initialValue)
    {
    ClUint32T realKey;
    int keyLen = strlen(key);
    realKey = computeCrc32((ClUint8T*) key, keyLen);
    init(realKey,initialValue);
    }

  void ProcSem::init(unsigned int key,int initialValue)
  {
    uint_t retry;
    uint_t flags = 0666;
    do  // open the semaphore, creating it if it does not exist.
      {
      retry = 0;
      semId = semget(key, 1, flags);
      if(semId < 0 )
        {
        int err = errno;
        if(err == EINTR) retry = 1;
        // The IPC_EXCL is used because I need to KNOW whether I created this sem or not so that 
        // the creator is the only entity that initializes it
        else if (err == ENOENT) { flags |= IPC_CREAT | IPC_EXCL; retry = 1; }
        else if (err == EEXIST) { flags ^= IPC_CREAT | IPC_EXCL; retry = 1; }
        else
          {
            assert(0);  // Later raise exception
          }
        }
      } while(retry);

    if( (flags & IPC_CREAT) )
      {
        int err;
        CosSemCtl_t arg = {0};
        arg.val = initialValue;
        retry = 0;
        do
          {
            err = semctl(semId,0,SETVAL,arg);
            if(err < 0 )
              {
                if(errno == EINTR) retry = 1;
                else
                  {
                    assert(0);
                  }
        
              }
          } while(retry);
      }    
  }
  
  void ProcSem::lock(int amt)
    {
      struct sembuf sembuf = {0,(short int)(-1*amt),SEM_UNDO};
      int err;
      do
        {        
          err = semop(semId,&sembuf,1);
        } while ((err<0)&&(errno==EINTR));
      if (err<0)
        {
          int err = errno;
          assert(err<0);
        }
    }

  void ProcSem::wake(int amt,void* cookie) { unlock(amt); }  
  
  void ProcSem::unlock(int amt)
    {
      struct sembuf sembuf = {0,((short int)amt),SEM_UNDO};
      int err;
      do
        {        
          err = semop(semId,&sembuf,1);
        } while ((err<0)&&(errno==EINTR));
      if (err<0)
        {
          int err = errno;
          assert(err<0);
        }
    }
  
  bool ProcSem::try_lock(int amt)
  {
    struct sembuf sembuf = {0, (short int)(-1*amt),SEM_UNDO | IPC_NOWAIT};
    int err;
    do
      {
        err = semop(semId,&sembuf,1);
      } while ((err<0)&&(errno==EINTR));
    if (err<0)
      {
        int err = errno;
        if (err == EAGAIN) return false;
      }
    return true;
  }
  
  bool ProcSem::timed_lock(uint64_t mSec,int amt)
    {
      struct sembuf sembuf = {0,(short int)(-1*amt),SEM_UNDO};
      long int temp = (((long)mSec)%1000L)*1000L*1000L;
      struct timespec timeout = {(time_t) (mSec/1000), temp};  // tv_sec, tv_nsec
      int err;
      do
        {
          err = semtimedop(semId,&sembuf,1,&timeout);
        } while ((err<0)&&(errno==EINTR));
      if (err<0)
        {
          int err = errno;
          if (err == EAGAIN) return false;
          else
            {
              perror("timed_lock");
            assert(0);
            }
        }
      return true;
    }

  template<class bstMutT> void tMutex<bstMutT>::wake(int amt,void* cookie)
  {
    unlock();
  };

  // Instantiate these templates
  template class tMutex<boost::timed_mutex>;
  template class tMutex<boost::recursive_timed_mutex>;
  
  /**
   * Implement class ThreadCondition
   */
  ThreadCondition::ThreadCondition() {};

   void ThreadCondition::notify_one()
   {
       waitCondition.notify_one();
   }

   void ThreadCondition::notify_all()
   {
       waitCondition.notify_all();
   }

  void ThreadCondition::wake(int amt,void* cookie)
   {
       waitCondition.notify_all();
   }

   void ThreadCondition::wait(SAFplus::Mutex &mutex)
   {
       waitCondition.wait(mutex);
   }

   bool ThreadCondition::timed_wait(SAFplus::Mutex &mutex, int duration)
   {
     //boost::system_time const timeout = boost::get_system_time() + boost::posix_time::milliseconds(duration);
     // return waitCondition.timed_wait(mutex, timeout);
     auto timeout = boost::posix_time::milliseconds(duration);
     return waitCondition.timed_wait(mutex, timeout);
   }

  ThreadSem::ThreadSem(int initialValue)
  {
  count = initialValue;
  }

#if 0
  ThreadSem::ThreadSem()
  {
  count = 0;
  }
#endif

  void ThreadSem::init(int initialValue)
  {
  count = initialValue;
  }

  void ThreadSem::lock(int amt)
  {
  mutex.lock();
  while (count<amt)
    {
    cond.wait(mutex);
    }
  count -= amt;
  mutex.unlock();
  }

  void ThreadSem::wake(int amt,void* cookie) { unlock(amt); }  

  void ThreadSem::unlock(int amt)
  {
  mutex.lock();
  count += amt;
  cond.notify_all();
  mutex.unlock();
  }

bool ThreadSem::try_lock(int amt)
  {
  mutex.lock();
  if (count<amt)
    {
    mutex.unlock();
    return false;
    }
  count -= amt;
  cond.notify_all();  // Notifying here because the count was decreased, and some people may be blocking until zero
  mutex.unlock();
  return true;
  }

bool ThreadSem::timed_lock(uint64_t mSec,int amt)
  {
  ScopedLock<> lock(mutex);
  while (count<amt)
    {
    // TODO: take into account elapsed time going around the while loop more than once
    if (!cond.timed_wait(mutex,mSec)) return false;
    }
  count -= amt;
  cond.notify_all(); // Notifying here because the count was decreased, and some people may be blocking until zero
  return true;
  }

bool ThreadSem::blockUntil(uint amt,uint mSec)
  {
  ScopedLock<> lock(mutex);
  while (count > amt)
    {
    // TODO: take into account elapsed time going around the while loop more than once
    if (!cond.timed_wait(mutex,mSec)) return false;
    }
  mutex.unlock();
  }

  ThreadSem::~ThreadSem()
  {
  count = -1;  // Indicate that this object was deleted by putting an impossible value in count
  }


    ProcGate::ProcGate(unsigned int key,int initialValue)
      {
      init(key,initialValue);
      }

ProcGate::ProcGate(const char* key,int initialValue)
  {
  ClUint32T realKey;
  int keyLen = strlen(key);
  realKey = computeCrc32((ClUint8T*) key, keyLen);
  init(realKey,initialValue);
  }

  void ProcGate::init(unsigned int key, int initialValue)
    {
    if (initialValue != 0) initialValue = 1;  // Make it boolean
    uint_t retry;
    uint_t flags = 0666;
    do  // open the semaphore, creating it if it does not exist.
      {
      retry = 0;
      semId = semget(key, 2, flags);
      if(semId < 0 )
        {
        int err = errno;
        if(err == EINTR) retry = 1;
        // The IPC_EXCL is used because I need to KNOW whether I created this sem or not so that 
        // the creator is the only entity that initializes it
        else if (err == ENOENT) { flags |= IPC_CREAT | IPC_EXCL; retry = 1; }
        else if (err == EEXIST) { flags ^= IPC_CREAT | IPC_EXCL; retry = 1; }
        else
          {
          assert(0);  // Later raise exception
          }
        }
      } while(retry);

    if( (flags & IPC_CREAT) )
      {
      int err;
      CosSemCtl_t arg = {0};
      arg.val = 0; // initialValue;
      retry = 0;
      do
        {
        err = semctl(semId,0,SETVAL,0);
        if(err < 0 )
          {
          if(errno == EINTR) retry = 1;
          else
            {
            assert(0);
            }
          }
        arg.val = initialValue;  // The gate starts open or closed?
        err = semctl(semId,1,SETVAL,arg);
        if(err < 0 )
          {
          if(errno == EINTR) retry = 1;
          else
            {
            assert(0);
            }
        
          }
        } while(retry);
      }    
    }
  

    void ProcGate::wake(int amt,void* cookie) { open(); }
    void ProcGate::lock(int amt)   // This is not exclusive -- multiple entities can hold the lock at the same time.
      {
      // Block until the gate is open (= zero), then this lock will increment the semaphore
      struct sembuf sembuf[] = {{1, 0,SEM_UNDO},{0, (short int)(amt),SEM_UNDO }};
      int err;
      do
        {
          err = semop(semId,sembuf,2);
        } while ((err<0)&&(errno==EINTR));
      if (err<0)
        {
          int err = errno;
          assert(err<0);
        }

      }
    void ProcGate::unlock(int amt)
      {
      struct sembuf sembuf = {0, (short int)(-1*amt),SEM_UNDO };
      int err;
      do
        {
          err = semop(semId,&sembuf,1);
        } while ((err<0)&&(errno==EINTR));
      if (err<0)
        {
          int err = errno;
          assert(err<0);
        }

      }

    bool ProcGate::try_lock(int amt)
    {
    assert(0);
    }

    bool ProcGate::timed_lock(uint64_t mSec,int amt)
      {
      assert(0);
      }

    void ProcGate::close()  // close the gate so all lockers block on lock, returns when no entity has a lock.
      {
      if (1)  // First, close the gate
        {
        struct sembuf sembuf[] = { {1, (short int)1,SEM_UNDO }  };
        int err;
        do
          {
          err = semop(semId,sembuf,1);
          } while ((err<0)&&(errno==EINTR));
        if (err<0)
          {
          int err = errno;
          assert(err<0);
          }
        }

      if (1)  // now wait for all racers to leave or be blocked by the gate (wait for zero)
        {
        struct sembuf sembuf =  {0, 0,SEM_UNDO };
        int err;
        do
          {
          err = semop(semId,&sembuf,1);
          } while ((err<0)&&(errno==EINTR));
        if (err<0)
          {
          int err = errno;
          assert(err<0);
          }
        }
      }

void ProcGate::open()   // open the gate to allow lockers to proceed.
  {
  // open the gate
        
  struct sembuf sembuf[] = { {1, (short int) -1,SEM_UNDO }  };
  int err;
  do
    {
    err = semop(semId,sembuf,1);
    } while ((err<0)&&(errno==EINTR));
  if (err<0)
    {
    int err = errno;
    assert(err<0);
    }
  }

  };

