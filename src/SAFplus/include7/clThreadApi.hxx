#pragma once
#ifndef CLTHREADAPI_HXX_
#define CLTHREADAPI_HXX_
#include <sys/syscall.h>

#include <boost/thread/mutex.hpp> 
#include <boost/thread/recursive_mutex.hpp> 
#include <boost/thread/locks.hpp> 
#include <boost/thread/condition_variable.hpp>

#include <clCommon.hxx>

namespace SAFplus
{

  inline long gettid()
    {
    return syscall(SYS_gettid);
    }

  class SemI:public Wakeable
  {
  public:
    virtual void lock(int amt=1) = 0;
    virtual void unlock(int amt=1) = 0;
    virtual bool try_lock(int amt=1) = 0;
    virtual bool timed_lock(uint64_t mSec,int amt=1) = 0;
  };

  /* Interprocess semaphore must use SYS-V semaphores because they can be automatically released on process death.  Api signatures are very similar to c++ boost library. */
  class ProcSem:public SemI
  {
  protected:
    int semId;
  public:
    ProcSem() { semId = -1; }  // deferred initialization
    ProcSem(unsigned int key,int initialValue=0);
    ProcSem(const char* key,int initialValue=0);  // the actual key will be the CRC32 of this string
    void init(unsigned int key,int initialValue);
    void init(const char* key,int initialValue); // the actual key will be the CRC32 of this string
    void wake(int amt,void* cookie=NULL);
    void lock(int amt=1);
    void unlock(int amt=1);
    bool try_lock(int amt=1);
    bool timed_lock(uint64_t mSec,int amt=1);
  };


  /* Interprocess gate must use SYS-V semaphores because they can be automatically released on process death.  Api signatures are very similar to c++ boost library. */
  /* A gate is an abstraction that allows multiple threads/processes
   * access until the gate is "shut".  Once all processes are out of
   * the critical region, the entity that shut the gate is allowed to run */
  class ProcGate:public SemI
  {
  protected:
    int semId;
  public:
    ProcGate() { semId = -1; }
    ProcGate(unsigned int key,int initialValue=0);
    ProcGate(const char* key,int initialValue=0);
    void init(unsigned int key,int initialValue=0);
    void wake(int amt,void* cookie=NULL);
    void lock(int amt=1);   // This is not exclusive -- multiple entities can hold the lock at the same time.
    void unlock(int amt=1);
    bool try_lock(int amt=1);
    bool timed_lock(uint64_t mSec,int amt=1);

    void close();  // close the gate so all lockers block on lock, returns when no entity has a lock.
    void open();   // open the gate to allow lockers to proceed.
  };

  
  template<class bstMutT> class tMutex: public Wakeable
  {
  public:
    bstMutT mutex;
  public:
    void wake(int amt,void* cookie=NULL);
    void lock(void) { mutex.lock(); }
    void unlock(void) { mutex.unlock(); }
    bool try_lock(void) { return mutex.try_lock(); }
  };

  typedef class tMutex<boost::timed_mutex> Mutex;
  typedef class tMutex<boost::recursive_timed_mutex> RecursiveMutex;

  template<class Mut = Mutex> class ScopedLock
  {
  protected:
    Mut& mutex;
  public:
    ScopedLock(Mut& m):mutex(m) { mutex.lock(); }
    ~ScopedLock() { mutex.unlock(); }
   
  };

  /**
   * Class derive from boost::condition_variable_any
   */
  class ThreadCondition: public Wakeable
  {
  public:
    ThreadCondition();

    void wake(int amt,void* cookie=NULL);
    /**
     * Signal to wakeup
     */
    void notify_one();

    /**
     * Signal to wakeup
     */
    void notify_all();

    /*
     * Do not encourage, wait forever
     */
    void wait(SAFplus::Mutex &mutex);

    /**
     * Wait duration to wake up
     */
    bool timed_wait(SAFplus::Mutex &mutex, int duration);

  protected:
    boost::condition_variable_any waitCondition;
  };


  /* thread semaphore */
  class ThreadSem:public SemI
  {
  protected:
    ThreadCondition cond;
    Mutex           mutex;
    int count;
  public:
    ThreadSem():count(0) {}
    ThreadSem(int initialValue);
    ~ThreadSem();
    void init(int initialValue);
    void wake(int amt,void* cookie=NULL);
    void lock(int amt=1);    // subtracts one from the semaphore, blocks if that would cause the value to become < 0
    void unlock(int amt=1);  // Adds one to the semaphore
    bool blockUntil(uint val = 0, uint mSec=0xffffffff);   // wake me when the count is val or below. 
    bool try_lock(int amt=1);
    bool timed_lock(uint64_t mSec,int amt=1);
  };
};


#endif //CLTHREADAPI_HXX_
