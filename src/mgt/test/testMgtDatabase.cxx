#include <clGlobals.hxx>

#include "clTransaction.hxx"
#include "clMgtProv.hxx"
#include "clMgtProvList.hxx"
#include "MgtFactory.hxx"
#include "clMgtList.hxx"
#include <stdio.h>
#include <string>
#include <clLogApi.hxx>
#include "clMgtApi.hxx"
#include "boost/functional/hash.hpp"
#include <clMgtModule.hxx>
#include <clSafplusMsgServer.hxx>
#include <clMgtDatabase.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clGroupIpi.hxx>
#include <clMgtApi.hxx>

using namespace SAFplus;

#define MGT_API_TEST_PORT (SAFplusI::END_IOC_PORT + 1)

namespace SAFplusI
  {
  extern GroupSharedMem gsm;
  };

void initSafLibraries()
{
  ClRcT rc = CL_OK;

  SafplusInitializationConfiguration sic;
  sic.iocPort     = MGT_API_TEST_PORT;
  sic.msgQueueLen = 25;
  sic.msgThreads  = 1;
  SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;
  SAFplus::logEchoToFd = 1;  // echo logs to stdout for debugging

  safplusInitialize( SAFplus::LibDep::FAULT | SAFplus::LibDep::GRP | SAFplus::LibDep::LOG | SAFplus::LibDep::MSG, sic);
  mgtAccessInitialize();
  //SAFplusI::gsm.init();
}

/*
 * SAFplus Node attribute
 */
int main(int argc, char *argv[]) // testmgtapi /safplusAmf/Component Component11 /safplusAmf/Component[@name=\"Component11\"]
{
    ClRcT ret;
    initSafLibraries();
    std::string val;
    std::vector<std::string> child, child2;
    const std::string dbName("safplusAmf");//, xpath(argv[1]);
    MgtDatabase amfDb;
    amfDb.initialize(dbName);
    std::string strXpath(argv[3]), entityAttrXpath, strRootXpath(argv[1]);

    //print("Iterating %s\n", strRootXpath.c_str());
#if 0
    std::vector<std::string> _childs;
    std::string root("/");
    amfDb.iterate(root,_childs);
    printf("========== Iterating ROOT ==========================================================\n");
    for(std::vector<std::string>::const_iterator cit = _childs.cbegin(); cit!=_childs.cend();cit++)
    {
       printf("%s\n", (*cit).c_str());
    }
    printf("====================================================================================\n");
#endif
    std::string pval;
    std::vector<std::string> childs;
    std::stringstream keypart;
    keypart << "[@name" << "=\"" << argv[2] <<"\"" << "]";
    printf("keypart [%s]\n", keypart.str().c_str());    

    printf("getting record of xpath [%s]\n", strRootXpath.c_str());
    ret = amfDb.getRecord(strRootXpath, pval, &childs);
    if (ret == CL_OK)
    {
       std::vector<std::string>::iterator delIter = std::find(childs.begin(), childs.end(), keypart.str());
       if (delIter != childs.end())
       {
          childs.erase(delIter);
          ret = amfDb.setRecord(strRootXpath, pval, &childs);
       }
    }
    if (ret!=CL_OK)
    {
       printf("set record of key [%s] failed rc [0x%x]\n", strRootXpath.c_str(), ret);
       return ret;
    }
#if 0
    printf("getting children of key [%s]\n", strXpath.c_str());
    ClRcT rc = amfDb.getRecord(strXpath, val, &child);
    for(std::vector<std::string>::const_iterator it = child.cbegin();it!=child.cend();it++)
    {
       entityAttrXpath = strXpath;
       entityAttrXpath.append("/");
       entityAttrXpath.append(*it);
       printf("xpath of attribute is [%s]\n", entityAttrXpath.c_str());
       //printf("%s\n", (*it).c_str());
       rc = amfDb.deleteRecord(entityAttrXpath);
       //printf("value of [%s] is [%s]; num of child [%d]\n", entityAttrXpath.c_str(), val.c_str(), (int)child2.size());
       printf("delete [%s] rc [0x%x]\n", entityAttrXpath.c_str(), rc);
    }
#endif
    ClRcT rc = amfDb.deleteAllRecordsContainKey(keypart.str());
    if (rc != CL_OK)
       logError("MGMT","DELL.ENT", "delete record FAILED for xpath containing [%s], rc=[0x%x]", keypart.str().c_str(), rc);
    rc = amfDb.deleteRecord(strXpath);
    if (rc != CL_OK)
    {
       logError("MGMT","DELL.ENT", "delete record with key [%s] FAILED rc=[0x%x]", strXpath.c_str(),rc);
    }


    return 0;
}
