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
      PROCESS_MAX_ERROR
      };
    ProcessError(int err):Error(NULL)
      {
      }
    };


  class Process
    {
    int pid;
    public:
    Process(int pid);

    //? Returns the command line that was used to start this process
    std::string getCmdline(void);
    };

  };

#endif
