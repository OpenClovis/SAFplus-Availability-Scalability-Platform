#include <clGlobals.hxx>
#include <clMsgPortsAndTypes.hxx>
#include <clMgtApi.hxx>
#include <clGroupIpi.hxx>

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

  safplusInitialize( SAFplus::LibDep::LOG | SAFplus::LibDep::MSG, sic);
  SAFplusI::gsm.init();
}

/*
 * SAFplus Node attribute
 */
int main(int argc, char *argv[])
{
    ClRcT ret;
    initSafLibraries();
    std::string getValue, setValue;

    //Test mgtGet function
    getValue = SAFplus::MgtFunction::mgtGet("/SAFplusAmf/Component/c0");
    printf("Get /SAFplusAmf/Component/c0 return: %s\n\n", getValue.c_str());

    //Test mgtSet function
    setValue.assign("False");
    printf("Set /SAFplusAmf/ServiceGroup/sg0/autoRepair = %s\n", setValue.c_str());
    SAFplus::MgtFunction::mgtSet("/SAFplusAmf/ServiceGroup/sg0/autoRepair", setValue);
    getValue = SAFplus::MgtFunction::mgtGet("/SAFplusAmf/ServiceGroup/sg0/autoRepair");
    printf("Get /SAFplusAmf/ServiceGroup/sg0/autoRepair return: %s\n\n", getValue.c_str());

    //Test mgtSet function
    setValue.assign("True");
    printf("Set /SAFplusAmf/ServiceGroup/sg0/autoRepair = %s\n", setValue.c_str());
    SAFplus::MgtFunction::mgtSet("/SAFplusAmf/ServiceGroup/sg0/autoRepair", setValue);
    getValue = SAFplus::MgtFunction::mgtGet("/SAFplusAmf/ServiceGroup/sg0/autoRepair");
    printf("Get /SAFplusAmf/ServiceGroup/sg0/autoRepair return: %s\n\n", getValue.c_str());

    //Test mgtCreate function
    ret = SAFplus::MgtFunction::mgtCreate("/SAFplusAmf/ServiceGroup/sgTest");
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
    getValue = SAFplus::MgtFunction::mgtGet("/SAFplusAmf/ServiceGroup");
    printf("Get /SAFplusAmf/ServiceGroup return: %s\n\n", getValue.c_str());

    //Test mgtDelete function
    ret = SAFplus::MgtFunction::mgtDelete("/SAFplusAmf/ServiceGroup/sgTest");
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
    getValue = SAFplus::MgtFunction::mgtGet("/SAFplusAmf/ServiceGroup");
    printf("Get /SAFplusAmf/ServiceGroup return: %s\n\n", getValue.c_str());

    return 0;
}
