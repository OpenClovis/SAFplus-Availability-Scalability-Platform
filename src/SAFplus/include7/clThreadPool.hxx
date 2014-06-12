#include <clThreadApi.hxx>
#include <vector>

namespace SAFplus
{
  /* Definition of user's own task: he can run his own task by defining this function and pass it to 
     an object derived from Poolable OR he can add his codes to virtual function wake() of a class
     also derived from Poolable or Wakeable
  */
  typedef uint32_t (*UserCallbackT) (void* invocation);

  class Poolable: public Wakeable
  {
  public:    
    struct timespec startTime;
    struct timespec endTime;
    uint32_t executionTimeLimit;    
    bool deleteWhenComplete;
    UserCallbackT fn;

    Poolable(UserCallbackT _fn=NULL, uint32_t timeLimit=30000, bool _deleteWhenComplete=false);
    void calculateStartTime();
    void calculateEndTime();
    void calculateExeTime();
    virtual ~Poolable();
  };

  struct TaskData
  {
    Wakeable* p;
    void* arg;
    TaskData(Wakeable* _p, void* _arg): p(_p), arg(_arg){}
  };

  // This queue holds a number of tasks to be executed concurrently
  typedef std::vector<TaskData*> TaskQueue;

  class ThreadPool
  {
  protected:
    static TaskQueue* taskQueue;
    bool isStopped;
    Mutex mutex;
    ThreadCondition cond;
    TaskData* dequeue();
    void enqueue(Wakeable* p, void* arg);
    void startThread();
    static void runTask(void* arg);    
    
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
    /** Starts the thread pool running */
    void start();
    /** Stops the thread pool.  All currently running threads complete their job and then exit */
    void stop();
    /** Adds a job into the thread pool.  Poolable is NOT COPIED; do not delete or let it fall out of scope. 
        A Poolable object is more sophisticated than a Wakeable; it measures how long it was run for, can track
        whether it should be deleted, and will ASSERT if it is running for longer than a configured limit (deadlock detector)
     */
    //void run(Poolable* p, void* arg);
    /** Adds a job into the thread pool.  Wakeable is NOT COPIED; do not delete or let it fall out of scope
        Parameters:
          p: is of any derived from Wakeable, so, the virtual function wake() must be implemented by the user
             to do his own task OR the can create a class derived from Poolable and pass his function to the
             constructor. See the testThreadPool.cpp as an usage example.
          arg: optional argument for wake()
    */
    void run(Wakeable* p, void* arg);

    ~ThreadPool(); /* Finalize pool */
    
  };

 
}
