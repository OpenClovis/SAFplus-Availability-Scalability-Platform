
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <clTestApi.hxx>
#include <clMgtApi.hxx>
#include <clNameApi.hxx>
#include <boost/filesystem.hpp>

using namespace SAFplus;
using namespace boost::filesystem;

#ifdef CL_NO_TEST
#error This test only makes sense to be compiled with test macros on
#endif

void TestLog_basic(void)
{
  logSeverity = LOG_SEV_DEBUG;
  //logEchoToFd = 1;
  testVerbosity = TestVerbosity::TEST_PRINT_ALL;
  clTestCaseStart(("LOG-BAS-FNC.TC001: issue some logs"));
  logEmergency("LOG","TST","Test Emergency Log");
  logEmergency("LOG","TST","Test Emergency Log2");
  logAlert("LOG","TST","Test Alert Log");

  logSeverity = LOG_SEV_CRITICAL;
  logCritical("LOG","TST","Test critical Log");
  logError("LOG","TST","this log should not appear");
  logSeverity = LOG_SEV_ERROR;
  logError("LOG","TST","this log should appear");
  logDebug("LOG","TST","this log should not appear");

  logSeverity = LOG_SEV_TRACE;
  logError("LOG","TST","this log should appear");
  logDebug("LOG","TST","this log should appear");
  clTestSuccess(("logs issued without failure"));
  clTestCaseEnd((" "));
}

void TestLog_mgt(void)
{
  int TEST_FILE_SIZE=4096;
  std::string logFileName("testLog");
  char cwd[256];
  mgtCreate("/safplusLog/streamConfig/stream/test");
  std::string logname = mgtGet("/safplusLog/streamConfig/stream/test/name");
  std::string fileName = mgtGet("/safplusLog/streamConfig/stream/test/fileName");
  std::string fileLocation = mgtGet("/safplusLog/streamConfig/stream/test/fileLocation");
  printf("name:%s file:%s loc:%s\n",logname.c_str(),fileName.c_str(),fileLocation.c_str());

  mgtSet("/safplusLog/streamConfig/stream/test/fileUnitSize",std::to_string(TEST_FILE_SIZE));
  mgtSet("/safplusLog/streamConfig/stream/test/maximumFilesRotated","2");
  mgtSet("/safplusLog/streamConfig/stream/test/fileName",logFileName);

  clTestCaseMalfunction(("no current directory -- you deleted the directory from underneath this program (or gdb)"), getcwd(cwd,256)==NULL, return);

  std::string logdir(cwd);
  assert(logdir.size() > 10);  // Sanity check so we don't accidentally remove everything on the disk.
  logdir.append("/tmplogs");
  boost::filesystem::remove_all(logdir);

  mgtSet("/safplusLog/streamConfig/stream/test/fileLocation",logdir);

  int fus = mgtGetInt("/safplusLog/streamConfig/stream/test/fileUnitSize");
  clTest(("Set and read fileUnitSize"),(fus == TEST_FILE_SIZE),("received file size was: %d, expected %d",fus,TEST_FILE_SIZE));

  Handle testLogHdl;
  while(1)
    {
    try
      {
      testLogHdl = name.getHandle("test");
      break;
      }
    catch (NameException& e)  
      {
        usleep(10000);
      }
    }

  printf("test stream handle is [%" PRIx64 ":%" PRIx64 "]\n", testLogHdl.id[0], testLogHdl.id[1]);
  clTest(("new stream has a handle"), testLogHdl != INVALID_HDL,("handle is invalid"));
  for(int loop=0;loop<2;loop++)
    {
    logStrWrite(testLogHdl,LOG_SEV_CRITICAL,1,"TST","LOG",__FILE__,__LINE__,"This is a test of a dynamically created log stream");
    logMsgWrite(testLogHdl,LOG_SEV_CRITICAL,1,"TST","LOG",__FILE__,__LINE__,"loop %d", loop);
    usleep(1000*100);
    }

  std::string logfile = logdir + "/" + logFileName + "0.log";
  FILE* fp = fopen(logfile.c_str(),"r");
  if (fp)
    {
    char buffer[256];
    fgets(buffer,256,fp);
    printf("%s\n", buffer);
    clTest(("was log printed?"),strstr(buffer,"This is a test of a dynamically created log stream") != NULL,("log was: %s",buffer));
    fclose(fp);
    }
  else
    {
      clTestFailed(("Log file was not created"));
    }


  // Make sure just 2 files are created:
  for(int loop=0;loop<100;loop++)
    {
    logStrWrite(testLogHdl,LOG_SEV_CRITICAL,1,"TST","LOG",__FILE__,__LINE__,"This is a test of a dynamically created log stream");
    logMsgWrite(testLogHdl,LOG_SEV_CRITICAL,1,"TST","LOG",__FILE__,__LINE__,"loop %d", loop);
    usleep(1000*10);
    }

  boost::filesystem::directory_iterator end;
  int numfiles = 0;
  for (directory_iterator it(logdir); it != end; ++it)
    {
      numfiles++;
    }
  clTest(("file rotation"), numfiles == 2, ("too many files %d",numfiles));

  // Now expand # of rotated files
  mgtSet("/safplusLog/streamConfig/stream/test/maximumFilesRotated","10");
  // Make sure just 2 files are created:
  for(int loop=0;loop<1000;loop++)
    {
    logStrWrite(testLogHdl,LOG_SEV_CRITICAL,1,"TST","LOG",__FILE__,__LINE__,"This is a test of a dynamically created log stream");
    logMsgWrite(testLogHdl,LOG_SEV_CRITICAL,1,"TST","LOG",__FILE__,__LINE__,"loop %d", loop);
    usleep(1000);
    }
  
  numfiles = 0;
  for (directory_iterator it(logdir); it != end; ++it)
    {
      numfiles++;
    }
  clTest(("file rotation increased"), numfiles == 10, ("too many files %d",numfiles));

  mgtSet("/safplusLog/streamConfig/stream/test/maximumFilesRotated","3");
  // Make sure just 2 files are created:
  for(int loop=0;loop<500;loop++)
    {
    logStrWrite(testLogHdl,LOG_SEV_CRITICAL,1,"TST","LOG",__FILE__,__LINE__,"This is a test of a dynamically created log stream");
    logMsgWrite(testLogHdl,LOG_SEV_CRITICAL,1,"TST","LOG",__FILE__,__LINE__,"loop %d", loop);
    usleep(1000);
    }

  numfiles = 0;
  for (directory_iterator it(logdir); it != end; ++it)
    {
      numfiles++;
    }
  clTest(("file rotation decreased"), numfiles == 3, ("too many files %d",numfiles));


  const int STRM_CNT = 20;

  for (int i=0;i<STRM_CNT;i++)
    {
      std::string loc("/safplusLog/streamConfig/stream/");
      std::string streamname = "lg" + std::to_string(i) + "test";
      loc.append(streamname);
      mgtCreate(loc);
      loc.append("/");
      mgtSet(loc + "fileLocation",logdir);
      std::string filename = "lg" + std::to_string(i) + "test";
      mgtSet(loc + "fileName",filename);
      
    }

  Handle logHdl[STRM_CNT];
  for (int i=0;i<STRM_CNT;i++)
    {
      std::string filename = "lg" + std::to_string(i) + "test";
      while(1)
        {
          try
            {
              logHdl[i] = name.getHandle(filename);
              break;
            }
          catch (NameException& e)  
            {
              clTestFailed(("Need blocking name API"));
            }
        }
    }

  for (int j=0;j<20;j++)
   for (int i=0;i<STRM_CNT;i++)
    {
    for(int loop=0;loop<5;loop++)
      {
      logStrWrite(logHdl[i],LOG_SEV_CRITICAL,1,"TST","LOG",__FILE__,__LINE__,"This is a test of a dynamically created log stream");
      logMsgWrite(logHdl[i],LOG_SEV_CRITICAL,1,"TST","LOG",__FILE__,__LINE__,"loop %d", loop);
      }
    }

}

int main(int argc, char* argv[])
{
  SAFplus::logCompName = "testLog";
  clTestGroupInitialize(("log"));
  logInitialize();
  TestLog_basic();

  SafplusInitializationConfiguration sic;
  sic.iocPort     = 86;

  safplusInitialize(SAFplus::LibDep::NAME | SAFplus::LibDep::MGT_ACCESS,sic);
  TestLog_mgt();
  clTestGroupFinalize(); 
  safplusFinalize();
}


