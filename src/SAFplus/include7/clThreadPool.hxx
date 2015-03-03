#pragma once

#include <clThreadApi.hxx>
#include <vector>
#include <boost/intrusive/list.hpp>
#include <boost/unordered_map.hpp>

namespace SAFplus
{
  /* Definition of user's own task: he can run his own task by defining this function and pass it to 
     an object derived from Poolable OR he can add his codes to virtual function wake() of a class
     also derived from Poolable or Wakeable
  */

  typedef uint32_t (*UserCallbackT) (void* invocation);

  class Poolable: public Wakeable, public boost::intrusive::list_base_hook<>
  {
  protected:    
    struct timespec startTime;
    struct timespec endTime;
    uint32_t executionTimeLimit;        
    bool deleteWhenComplete;
    UserCallbackT fn;

  public:    
    void* arg;
    typedef boost::intrusive::list<Poolable> PoolableList;
    static PoolableList poolableList;    

    Poolable(UserCallbackT _fn=NULL, void* _arg=NULL, uint32_t timeLimit=30000, bool _deleteWhenComplete=false);
    bool isDeleteWhenComplete();
    void calculateStartTime();
    void calculateEndTime();
    void calculateExecTime();
    virtual ~Poolable();
  };

  class WakeableHelper: public Poolable
  {
  public:
    Wakeable* wk;
    WakeableHelper(Wakeable* _wk, void* _arg): Poolable(NULL,_arg)
    {
      wk=_wk;
    }    
    void wake(int amt, void* cookie){} 
  };

#if 0
  struct TaskData
  {
    Wakeable* p;
    void* arg;
    TaskData(Wakeable* _p, void* _arg): p(_p), arg(_arg){}
  };

  // This queue holds a number of tasks to be executed concurrently
  typedef std::vector<TaskData*> TaskQueue;
#endif
  struct ThreadState
  {    
    bool working;
    bool quitAllowed;
    struct timespec idleTimestamp;
    ThreadState(bool wk, bool qa): working(wk), quitAllowed(qa){}
  };
 
  typedef std::pair<const pthread_t,ThreadState> ThreadMapPair; 
  typedef boost::unordered_map <pthread_t, ThreadState> ThreadHashMap;
  typedef std::list<WakeableHelper*> WHList;

  class ThreadPool
  {
  protected:
    bool isStopped;
    Mutex mutex;
    ThreadCondition cond;
    ThreadHashMap threadMap;
    int numCurrentThreads;
    int numIdleThreads;
    WHList whList;
  
    Poolable* dequeue();
    void enqueue(Poolable* p);
    void startThread();
    static void runTask(void* arg);
    /** Starts the thread pool running */
    void start();
    WakeableHelper* getUnusedWHElement();
    //int getNumIdleThreads();
    void checkAndReleaseThread();
    static void* timerThreadFunc(void* arg);
    
#if 0
    void createTask();
    void startNewTask();
    void taskEntry();
#endif

  public:
    short minThreads;
    short maxThreads;
//    short numIdleTasks;
//    short flags;    
#if 0
    UserCallbackT m_preIdleFn;
    void* m_preIdleCookie;
    UserCallbackT m_onDeckFn;
    void* m_onDeckCookie;        
    uint32_t m_pendingJobs;
#endif
    ThreadPool(short _minThreads, short _maxThreads); /* Initialize pool*/    

    /** Stops the thread pool.  All currently running threads complete their job and then exit */
    void stop();
    /** Adds a job into the thread pool.  Poolable is NOT COPIED; do not delete or let it fall out of scope. 
        A Poolable object is more sophisticated than a Wakeable; it measures how long it was run for, can track
        whether it should be deleted, and will ASSERT if it is running for longer than a configured limit (deadlock detector)
     */
    void run(Poolable* p);
    /** Adds a job into the thread pool.  Wakeable is NOT COPIED; do not delete or let it fall out of scope
        Parameters:
          p: is of any derived from Wakeable, so, the virtual function wake() must be implemented by the user
             to do his own task OR the can create a class derived from Poolable and pass his function to the
             constructor. See the testThreadPool.cpp as an usage example.
          arg: optional argument for wake()
    */
    void run(Wakeable* wk, void* arg);

    ~ThreadPool(); /* Finalize pool */
    
  };
#if 0
  inline std::size_t hash_value(pthread_t id)
  {
     boost::hash<uint32_t> hasher;        
     return hasher(id);
  }
#endif
}
