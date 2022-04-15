#include <clGlobals.hxx>

#include <stdio.h>
#include <string>
#include <clLogApi.hxx>
#include "boost/functional/hash.hpp"
//#include <clSafplusMsgServer.hxx>
#include <clMsgPortsAndTypes.hxx>
//#include <clGroupIpi.hxx>
#include <clAmfMgmtApi.hxx>

using namespace SAFplus;

void initSafLibraries()
{
  ClRcT rc = CL_OK;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = 33;
  sic.msgQueueLen = 25;
  sic.msgThreads  = 1;
  SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;
  SAFplus::logEchoToFd = 1;  // echo logs to stdout for debugging

  safplusInitialize(SAFplus::LibDep::NAME | SAFplus::LibDep::FAULT | SAFplus::LibDep::GRP | SAFplus::LibDep::LOG | SAFplus::LibDep::MSG, sic);
  //mgtAccessInitialize();
  //SAFplusI::gsm.init();    
}

/*
 * SAFplus Node attribute
 */
int main(int argc, char *argv[]) // testmgtapi /safplusAmf/Component Component11 /safplusAmf/Component[@name=\"Component11\"]
{
    ClRcT ret;
    initSafLibraries();
    const char* method = argv[1];  //set or get
    const char* nodeName = argv[2];
    std::string info;
    Handle dummyHdl;
    ClRcT rc = amfMgmtInitialize(dummyHdl);
    if (rc != CL_OK)
    {
       printf("mgmt init fail rc [0x%x]", rc);
       return rc;
    }
    if (strcmp(method, "get")==0)
       rc = getSafplusInstallInfo(dummyHdl, std::string(nodeName), info);
    else if (strcmp(method, "set")==0)
    {   
       info = "SAFplusInstallInfoz";
       rc = setSafplusInstallInfo(dummyHdl, std::string(nodeName), info);       
    }
    if (rc == CL_OK)
    {
       printf("[%s] Safplus install info:%s\n", method, info.c_str());
    }
    else
       printf("[%s] FAILED rc=0x%x\n", method, rc);
    amfMgmtFinalize(dummyHdl);

    return 0;
}
