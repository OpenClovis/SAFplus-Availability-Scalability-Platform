#include <clGlobals.hxx>
#include <boost/thread/thread.hpp>
extern void ckptServer(void);
extern bool quit;

int main(int argc, char* argv[])
{
  SAFplus::logSeverity = SAFplus::LOG_SEV_TRACE;
  //SAFplus::SafplusInitializationConfiguration sic;
  //sic.iocPort = 70;
  //SAFplus::safplusInitialize(SAFplus::LibDep::MSG|SAFplus::LibDep::LOG|SAFplus::LibDep::UTILS, sic);
  //ckptServer();
  SAFplus::logEchoToFd = 1; 
  /*SAFplus::SafplusInitializationConfiguration sic;
  sic.iocPort = 1;
  SAFplus::safplusInitialize(SAFplus::LibDep::MSG, sic);
  */
  ckptServer();
  while (!quit)
  {
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
  }
}
