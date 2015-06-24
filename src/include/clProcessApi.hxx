#pragma once
#ifndef CLPROCESSAPI_HXX_
#define CLPROCESSAPI_HXX_

#include <string>
#include <clCommon.hxx>

namespace SAFplus
  {
  class ProcessError:public Error
    {
    public:
    enum
      {
      PROCESS_NOT_FOUND=Error::PROCESS_ERRORS,
      PROCESS_FORK_FAILURE,
      PROCESS_MAX_ERROR
      };
    ProcessError(int err):Error(NULL)
      {
      }
    };


  class Process
    {
    public:
    int pid;

    Process(int pid);

    enum 
      {
      NoFlags = 0,
      CreateNewSession = 1,
      CreateNewGroup = 2,
      InheritEnvironment = 4,
      };

    //? Returns true if the process exists
      bool alive();

    //? Returns the command line that was used to start this process
    std::string getCmdline(void);
    };


  //? Create a new process and execute the passed function.  This is extremely dangerous in a multi-threaded environment because other threads could be holding mutexes which will remain held forever in the new process context.  This will cause the new process to hang if it calls a function that takes the mutex.  Therefore use of this function is discouraged.
  Process forkProcess(Func f, void* arg, uint_t flags=0);

    /*? Execute a program.  
        <arg name='command'>The command-line to run the program.  May contain space-separated arguments</arg>
        <arg name='env'>Environment variables to set before starting the new program</arg>
        <arg name='flags'>Flags that control various process creation options: NoFlags, CreateNewSession, CreateNewGroup, InheritEnvironment</arg>
     */
  Process executeProgram(const std::string& command, const std::vector<std::string>& env,uint_t flags=0);


  class ProcessList
  {
    public:
    typedef std::vector<Process *> ProcList;

    // Vector which gives the list of process
    ProcList pList;

    ProcessList();
    ~ProcessList();

    protected:
    // This funtion is to obtain the list of all currently running 
    // process by checking the directories present in /proc. 
    // This function will fill the member 'pList'
    void findProcessList();
    bool isDirNamePID(std::string &fileName);

  };

  }; /* namespace SAFplus */
#endif
