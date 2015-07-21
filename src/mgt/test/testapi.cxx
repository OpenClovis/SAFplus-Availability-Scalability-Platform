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
#include "unitTest/Ut.hxx"

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
  safplusInitialize( SAFplus::LibDep::FAULT | SAFplus::LibDep::GRP | SAFplus::LibDep::LOG | SAFplus::LibDep::MSG, sic);
  mgtAccessInitialize();
  //SAFplusI::gsm.init();
}

/*
 * SAFplus Node attribute
 */
int main(int argc, char *argv[])
{
    ClRcT ret;
    initSafLibraries();
    std::string getValue, setValue;
    char buf[200];
    //Test mgtGet function
    snprintf(buf,200,"{%d}/SAFplusAmf/Component[name='c0']",1000);
    getValue = SAFplus::mgtGet(buf);
    printf("Get /SAFplusAmf/Component[name='c0'] = %s\n", getValue.c_str());

    snprintf(buf,200,"{%d}/SAFplusAmf/Component[name='c1']",1000);
    getValue = SAFplus::mgtGet(buf);
    printf("Get /SAFplusAmf/Component[name='c1'] = %s\n", getValue.c_str());


    for (int i=0;i<2;i++)
      {
      snprintf(buf,200,"{%d}/SAFplusAmf/Component/c0",i);
      getValue = SAFplus::mgtGet(buf);
      printf("Get %d /SAFplusAmf/Component/c0 return: %s\n\n", i, getValue.c_str());
      if (getValue.size() == 0) 
        {
          assert(0);
        }
      }
    //Test mgtSet function
    setValue.assign("False");
    printf("Set /SAFplusAmf/ServiceGroup/sg0/autoRepair = %s\n", setValue.c_str());
    ret = SAFplus::mgtSet("/SAFplusAmf/ServiceGroup/sg0/autoRepair", setValue);
    if (ret == CL_OK)
      {
        printf("Set /SAFplusAmf/ServiceGroup/sg0/autoRepair successfully!\n");
      }
    else if (ret == CL_ERR_DOESNT_EXIST)
      {
        printf("/SAFplusAmf/ServiceGroup/sg0/autoRepair does not exist!\n");
      }
    else
      {
        printf("Failed to set /SAFplusAmf/ServiceGroup/sg0/autoRepair, error [%#x]\n", ret);
      }

    getValue = SAFplus::mgtGet("/SAFplusAmf/ServiceGroup/sg0/autoRepair");
    printf("Get /SAFplusAmf/ServiceGroup/sg0/autoRepair return: %s\n\n", getValue.c_str());

    //Test mgtSet function
    setValue.assign("True");
    printf("Set /SAFplusAmf/ServiceGroup/sg0/autoRepair = %s\n", setValue.c_str());
    ret = SAFplus::mgtSet("/SAFplusAmf/ServiceGroup/sg0/autoRepair", setValue);
    if (ret == CL_OK)
      {
        printf("Set /SAFplusAmf/ServiceGroup/sg0/autoRepair successfully!\n");
      }
    else if (ret == CL_ERR_DOESNT_EXIST)
      {
        printf("/SAFplusAmf/ServiceGroup/sg0/autoRepair does not exist!\n");
      }
    else
      {
        printf("Failed to set /SAFplusAmf/ServiceGroup/sg0/autoRepair, error [%#x]\n", ret);
      }

    getValue = SAFplus::mgtGet("/SAFplusAmf/ServiceGroup/sg0/autoRepair");
    printf("Get /SAFplusAmf/ServiceGroup/sg0/autoRepair return: %s\n\n", getValue.c_str());

    //Test mgtCreate function
    ret = SAFplus::mgtCreate("/SAFplusAmf/ServiceGroup/sgTest");
    if (ret == CL_OK)
      {
        printf("Create /SAFplusAmf/ServiceGroup/sgTest successfully!\n");
      }
    else if (ret == CL_ERR_ALREADY_EXIST)
      {
        printf("/SAFplusAmf/ServiceGroup/sgTest already exists!\n");
      }
    else
      {
        printf("Failed to create /SAFplusAmf/ServiceGroup/sgTest, error [%#x]\n", ret);
      }
    getValue = SAFplus::mgtGet("/SAFplusAmf/ServiceGroup");
    printf("Get /SAFplusAmf/ServiceGroup return: %s\n\n", getValue.c_str());

    //Test mgtDelete function
    ret = SAFplus::mgtDelete("/SAFplusAmf/ServiceGroup/sgTest");
    if (ret == CL_OK)
      {
        printf("Delete /SAFplusAmf/ServiceGroup/sgTest successfully!\n");
      }
    else if (ret == CL_ERR_DOESNT_EXIST)
      {
        printf("/SAFplusAmf/ServiceGroup/sgTest does not exist!\n");
      }
    else
      {
        printf("Failed to delete /SAFplusAmf/ServiceGroup/sgTest, error [%#x]\n", ret);
      }
    getValue = SAFplus::mgtGet("/SAFplusAmf/ServiceGroup");
    printf("Get /SAFplusAmf/ServiceGroup return: %s\n\n", getValue.c_str());

    return 0;
}
