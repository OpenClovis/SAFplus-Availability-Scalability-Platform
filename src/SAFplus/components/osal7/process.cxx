#include <fstream>
#include <clProcessApi.hxx>

using namespace std;

namespace SAFplus
  {

  Process::Process(int processId): pid(processId)
    {
    }

  std::string Process::getCmdline(void)
    {
    char cmdLineFilename[80];
    snprintf(cmdLineFilename,80,"/proc/%d/cmdline",pid);
    std::string ret;
    ifstream file;
    file.open(cmdLineFilename);
    if (file.is_open())
      {
      getline(file, ret);
      file.close();
      }
    else
      {
      throw ProcessError(ProcessError::PROCESS_NOT_FOUND);
      }
    return ret;
    }

  Process forkProcess(Func f, void* arg, uint_t flags)
    {
    ClInt32T pid = -1;
    ClInt32T sid = -1;
    ClRcT retCode = 0;

    pid = fork();
    if(pid < 0)
      {
      throw ProcessError(ProcessError::PROCESS_FORK_FAILURE);
      }

    if(0 == pid) // In the new process
      {
      if(flags & Process::CreateNewSession)
        {
        sid = setsid ();
        }
      if(flags & Process::CreateNewGroup)
        {
        pid = getpid();
        /* Set the process group id to its own pid */
        setpgid (pid, 0);
        }
      f(arg);
      exit(0);
      }
    
    // In the parent
    return Process(pid);
    }

  Process executeProgram(const std::string& command, const std::vector<std::string>& env, uint_t flags)
    {
    ClInt32T pid = -1;
    ClInt32T sid = -1;

    std::istringstream ss(command);
    std::string arg;
    std::vector<std::string> v1;
    std::vector<char*> charstrs;
    while (ss >> arg)
      {
      v1.push_back(arg);
      charstrs.push_back(const_cast<char*>(v1.back().c_str()));
      }
    charstrs.push_back(0);  // need terminating null pointer

    /*
     * Setting variable environment for process
     */
    std::vector<char*> envpchars;
    for (int i=0;i<env.size(); i++)
      {
        envpchars.push_back(const_cast<char*>(env[i].c_str()));
      }
    envpchars.push_back(0);

    pid = fork();

    if(pid < 0)
      {
      throw ProcessError(ProcessError::PROCESS_FORK_FAILURE);
      }

    if(0 == pid) // In the new process
      {
      if(flags & Process::CreateNewSession)
        {
        sid = setsid ();
        }
      if(flags & Process::CreateNewGroup)
        {
        pid = getpid();
        /* Set the process group id to its own pid */
        setpgid (pid, 0);
        }
      execvpe(charstrs[0], &charstrs[0], &envpchars[0]);  // if works will not return
      int err = errno;
      logAlert("OS","PRO","Program [%s] execution failed with error [%s (%d)]",charstrs[0], strerror(err),err);
      // If the error is understood, exit.  Otherwise assert
      if (err == ENOENT) exit(0); // Expected error; user did not give a valid executable
      if (err == EACCES) exit(0); // Expected error; permissions are not correct
      assert(0);
      }

    // In the parent
    return Process(pid);
    }
  };
