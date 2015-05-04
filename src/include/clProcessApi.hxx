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

    //? Returns the command line that was used to start this process
    std::string getCmdline(void);
    };



  Process forkProcess(Func f, void* arg, uint_t flags=0);


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
