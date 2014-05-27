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
      execvpe(charstrs[0], &charstrs[0],NULL);  // should never return
      assert(0);
      }

    // In the parent
    return Process(pid);
    }
  };
