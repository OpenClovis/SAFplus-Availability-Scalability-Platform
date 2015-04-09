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
 

  class Poolable: public Wakeable // , public boost::intrusive::list_base_hook<>
  {
  protected:
     struct timespec startTime;
    struct timespec endTime;
    uint32_t executionTimeLimit;        
    UserCallbackT fn;


  public:
    boost::intrusive::list_member_hook<> listHook;  
    bool deleteWhenComplete;
    uint_t structId;
    
    enum 
      {
        STRUCT_ID = 0x12345678,
      };

    void* arg;

    Poolable(UserCallbackT _fn=NULL, void* _arg=NULL, uint32_t timeLimit=30000, bool _deleteWhenComplete=false);
    bool isDeleteWhenComplete();
    void calculateStartTime();
    void calculateEndTime();
    void calculateExecTime();
    virtual void complete(void);  // derived class free function
    virtual ~Poolable();
  };

  typedef boost::intrusive::member_hook<Poolable,boost::intrusive::list_member_hook<>,&Poolable::listHook> PoolableMemberHookOption;
  typedef boost::intrusive::list<Poolable,PoolableMemberHookOption> PoolableList;

  class WakeableHelper: public Poolable
  {
  public:
    Wakeable* wk;
    WakeableHelper* next;
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
    bool zombie; // thread has quit and is waiting for join()
    struct timespec idleTimestamp;
    ThreadState(bool wk, bool qa): working(wk), quitAllowed(qa), zombie(false) {}
  };
 
  typedef std::pair<const pthread_t,ThreadState> ThreadMapPair; 
  typedef boost::unordered_map <pthread_t, ThreadState> ThreadHashMap;
  //typedef std::list<WakeableHelper*> WHList;

  class ThreadPool
  {
  protected:
    pthread_t checker;  // handle to the deadlock checker thread
    bool isStopped;
    Mutex mutex;
    ThreadCondition checkerCond;
    ThreadCondition cond;
    ThreadHashMap threadMap;
    int numCurrentThreads;
    int numIdleThreads;
    //WHList whList;
    PoolableList poolableList;    
  
    Poolable* dequeue();
    void enqueue(Poolable* p);
    void startThread();
    static void runTask(void* arg);
    /** Starts the thread pool running */
    void start();

    WakeableHelper* unusedWakeableHelperList;
    WakeableHelper* allocWakeableHelper(Wakeable* wk, void* arg);
    void deleteWakeableHelper(WakeableHelper* wh);
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
    //? <ctor>  This 2 phase constructor requires that you call Init() to actually use the created object </ctor>
    ThreadPool();
    //? <ctor> Initializes and starts this thread pool </ctor>
    ThreadPool(short _minThreads, short _maxThreads); 

    //? 2nd phase of the 2 phase constructor.  Initializes and starts the thread pool
    void init(uint_t _minThreads, uint_t _maxThreads); /* Initialize pool*/    

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
 
    //? [Internal] Runs the passed object within the context of the calling thread
    void immediatelyRun(Poolable*p);
   
    //? [GDB only] verify that all items in the queue are Poolable objects
    bool dbgValidateQueue();
  };
#if 0
  inline std::size_t hash_value(pthread_t id)
  {
     boost::hash<uint32_t> hasher;        
     return hasher(id);
  }
#endif
}
