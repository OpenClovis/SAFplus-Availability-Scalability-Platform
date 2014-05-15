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
  };
