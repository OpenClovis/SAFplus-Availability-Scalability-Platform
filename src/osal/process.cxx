
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/types.h>  // for kill()
#include <signal.h>  // for kill()
#include <sys/wait.h>
#include <clProcessApi.hxx>

extern char **environ;

using namespace std;

namespace SAFplus
{

  Process::Process(int processId): pid(processId)
  {
  }

  bool Process::alive()
  {
    if (pid == 0) return false;  // pid 0 is impossible
    int result = kill(pid,0);  // signal 0 means don't send a signal but get error code back
    if (result == 0) return true;
    if (errno == EPERM) { assert (0); } // permissions problem
    assert(errno == ESRCH);  // there can be no other error coming from kill
    return false;
  }

  void Process::signal(int sig)
  {
    assert(pid > 1);  // Use something else to access the broadcast versions of kill
    assert(sig > 0);
    int result = kill(pid, sig);
    if (result == 0) return; // worked
    if (errno == EPERM) { assert (0); } // software error: permissions problem
    if (errno == EINVAL) { assert (0); } // software error: bad signal
    assert(errno == ESRCH); // (there can be no other error coming from kill) The pid or process group does not exist. Note that an existing process might be a zombie...    
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
#if 0 // the code is not going to work in some linux distro versions e.g. ubuntu 16.04
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
    if(flags & Process::InheritEnvironment)
        {
        /* Set the process group id to its own pid */
        for (char** envvar = environ; *envvar != 0; envvar++)
          {
          envpchars.push_back(*envvar);
          }
          
        }
     for (int i=0;i<env.size(); i++)
      {
        envpchars.push_back(const_cast<char*>(env[i].c_str()));
      }
    envpchars.push_back(0);
#endif

    int n = CL_MAX_NAME_LENGTH; // maximum arguments for a function
    char* args[CL_MAX_NAME_LENGTH];
    int i=0;
    for(;i<n;i++)
    {
      args[i] = new char[CL_MAX_NAME_LENGTH];
    }
    i=0;
    while (ss >> arg)
    {
      strcpy(args[i++],arg.c_str());      
    }
    args[i]=(char*)0;

    char* envs[CL_MAX_NAME_LENGTH];
    if(flags & Process::InheritEnvironment)
    {
        for(i=0;i<n;i++)
        {
          envs[i] = new char[CL_MAX_NAME_LENGTH];
        }
        i=0;
        /* Set the process group id to its own pid */
        for (char** envvar = environ; *envvar != 0; envvar++)
        {
          strcpy(envs[i++],*envvar);
          //logInfo("PROC","EXE","envs[%d]=%s",i-1,envs[i-1]);
        }
     }
     for (int j=0;j<env.size(); j++)
      {
        strcpy(envs[i++],env[j].c_str());
        //logInfo("PROC","EXE","2. envs[%d]=%s",i-1,envs[i-1]);
      }
    envs[i]=(char*)0;

    pid = fork();

    if(pid < 0)
      {
      throw ProcessError(ProcessError::PROCESS_FORK_FAILURE);
      }

    if(0 == pid) // In the new process
      {
      SAFplus::pid = pid = getpid();  // Set the global pid variable briefly in case there are logs before spawning

      if(flags & Process::CreateNewSession)
        {
        sid = setsid ();
        }
      if(flags & Process::CreateNewGroup)
        {
        /* Set the process group id to its own pid */
        setpgid (pid, 0);
        }
#if 0
      execvpe(charstrs[0], &charstrs[0], &envpchars[0]);  // if works will not return
#endif
      execvpe(args[0], args, envs);  // if works will not return
      int err = errno;
      char temp[CL_MAX_NAME_LENGTH];
      SAFplus::logCompName = "SPN"; // change the log name of the child so the source of this log isn't confusing
      logAlert("OS","PRO","Program [%s] execution failed with error [%s (%d)].  Working directory [%s]",args[0], strerror(err),err, getcwd(temp,CL_MAX_NAME_LENGTH));
      char fname[CL_MAX_NAME_LENGTH];
      snprintf(fname,CL_MAX_NAME_LENGTH,"%s/%d.error",(ASP_RUNDIR[0] != 0) ? ASP_RUNDIR : ".", pid);
      FILE* fp = fopen(fname,"w");
      if (fp)
        {
          fprintf(fp,"Program [%s] execution failed with error [%s (%d)].  Working directory is [%s].", args[0], strerror(err),err, getcwd(temp,CL_MAX_NAME_LENGTH));
          fclose(fp);
        }
      // If the error is understood, exit.  Otherwise assert
      // We need to _exit() so destructors aren't run.  We don't want to run destructors because the original process is still running
      if (err == ENOENT) _exit(err); // Expected error; user did not give a valid executable
      if (err == EACCES) _exit(err); // Expected error; permissions are not correct
      assert(0);
      }

    // In the parent
    return Process(pid);
    }

  int executeProgramWithTimeout(const std::string& command, const std::vector<std::string>& env, ClUint64T timeout, uint_t flags)
      {
      ClInt32T pid = -1;
      ClInt32T sid = -1;
      int status;
      std::istringstream ss(command);
      std::string arg;
#if 0 // the code is not going to work in some linux distro versions e.g. ubuntu 16.04
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
      if(flags & Process::InheritEnvironment)
          {
          /* Set the process group id to its own pid */
          for (char** envvar = environ; *envvar != 0; envvar++)
            {
            envpchars.push_back(*envvar);
            }

          }
       for (int i=0;i<env.size(); i++)
        {
          envpchars.push_back(const_cast<char*>(env[i].c_str()));
        }
      envpchars.push_back(0);
#endif
      int n = CL_MAX_NAME_LENGTH; // maximum arguments for a function
      char* args[CL_MAX_NAME_LENGTH];
      int i=0;
      for(;i<n;i++)
      {
        args[i] = new char[CL_MAX_NAME_LENGTH];
      }
      i=0;
      while (ss >> arg)
      {
        strcpy(args[i++],arg.c_str());      
      }
      args[i]=(char*)0;

      char* envs[CL_MAX_NAME_LENGTH];
      if(flags & Process::InheritEnvironment)
      {
        for(i=0;i<n;i++)
        {
          envs[i] = new char[CL_MAX_NAME_LENGTH];
        }
        i=0;
        /* Set the process group id to its own pid */
        for (char** envvar = environ; *envvar != 0; envvar++)
        {
          strcpy(envs[i++],*envvar);          
        }
      }
      for (int j=0;j<env.size(); j++)
      {
        strcpy(envs[i++],env[j].c_str());        
      }
      envs[i]=(char*)0;
      pid = fork();

      if(pid < 0)
      {
        throw ProcessError(ProcessError::PROCESS_FORK_FAILURE);
      }

      if(0 == pid) // In the new process
      {
        SAFplus::pid = pid = getpid();  // Set the global pid variable briefly in case there are logs before spawning

        if(flags & Process::CreateNewSession)
          {
          sid = setsid ();
          }
        if(flags & Process::CreateNewGroup)
          {
          /* Set the process group id to its own pid */
          setpgid (pid, 0);
          }
        execvpe(args[0], args, envs);  // if works will not return
        logError("PRO","EXEC","Program [%s] execution failed with error [%s (%d)]", args[0], strerror(errno),errno);
        _exit(EXIT_FAILURE);
      }
      else
      {
         /* This is the parent process.  Wait for the child to complete.  */
    	 ClUint32T waittime = 0;
    	 pid_t wpid;
    	 do
    	 {
    	    wpid = waitpid(pid, &status, WNOHANG);
    	    if (wpid == 0)
    	    {
    	       if (waittime < timeout)
    	       {
    	          usleep(1000);
    	          waittime++;
    	       }
    	       else
    	       {
    	          // Kill if timeout happened
    	          kill(pid, SIGKILL);
    	       }
    	     }
    	   }
    	   while (wpid == 0 && waittime <= timeout);

    	   if (status == -1) /* system command failed */
    	   {
    	        //TODO:
    	   }
    	   else if (WEXITSTATUS(status) == 127) /* system command could not find your script */
    	   {
    	       status = -1;
    	   }
    	   else if (WEXITSTATUS(status)) /* user script failed */
    	   {
    	      status = -1;
    	   }
    	   else
    	   {
    	      // successful
    	      status = 0;
    	   }
      }
      return status;

      }

ProcessList::ProcessList()
{
    findProcessList();
}

ProcessList::~ProcessList()
{
    ProcList::iterator it;
    for (it = pList.begin(); it != pList.end(); ++it)
    {
        delete *it;
    }
}

bool ProcessList::isDirNamePID(std::string &fileName)
{
    if (fileName.empty())
    {
        return false;
    }

    std::string::const_iterator it = fileName.begin();
    while (it != fileName.end())
    {
        if (!(std::isdigit(*it)))
        {
            return false;
        }
        ++it;
    }

    return true;
}

void ProcessList::findProcessList()
{
    DIR *dir;

    dir = opendir("/proc");
    if (!dir)
    {
        throw ProcessError(ProcessError::PROCESS_NOT_FOUND);
    }

    struct dirent *entry;
    std::string dirName;
    int32_t pid;

    while (entry = readdir(dir))
    {
        dirName.clear();
        dirName = entry->d_name;

        // /proc directory contains each sub directory for each process
        // and the sub directory's name will be the process ID of the 
        // process. /proc file may contain other files too. So we need
        //  to check whether the fileName is an integer number or not.
        if (!(isDirNamePID(dirName)))
        {
            continue;
        }

        pid = atoi((const_cast<const char*>(dirName.c_str())));
        Process *proc = new Process(pid);

        // Lining up the process entry into the vector.
        // It will be deleted in the destructor
        pList.push_back(proc);
    }

    closedir(dir);
}
  };
