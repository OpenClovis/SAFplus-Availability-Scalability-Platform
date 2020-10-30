#include <boost/program_options.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <clGlobals.hxx>
#include <clGroupCliApi.hxx>
#include <iostream>
#include <clMsgPortsAndTypes.hxx>
#include <clLogApi.hxx>

#define MGT_API_TEST_PORT (SAFplusI::END_IOC_PORT + 1)

using namespace SAFplus;

boost::program_options::variables_map parseOptions(int argc,char *argv[])
{
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("zap", boost::program_options::value<uint>()->implicit_value(0), "delete the shared memory segment")
    ("list", boost::program_options::value<uint>()->implicit_value(SAFplus::MaxNodes), "show all existing groups and entities ")
    ("clear", boost::program_options::value<std::string>()->implicit_value("*all*"), "delete all entities from a group (or all groups with no argument)")
    ;

  boost::program_options::variables_map vm;        
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);    
  if (vm.count("help")) 
    {
      std::cout << desc << "\n";
    }
  return vm;
}

int main(int argc,char *argv[])
  {
  //SAFplus::utilsInitialize();  // Needed to grab the env vars like ASP_NODENAME

  boost::program_options::variables_map vm = parseOptions(argc,argv);
  if (vm.count("help")) return 0;  // Help already printed by parseOptions

  /*if (vm.count("zap")) 
    {
      SAFplusI::GroupSharedMem::deleteSharedMemory();
    }

  //SAFplusI::GroupSharedMem gsm;
  //gsm.init();

  if (vm.count("clear")) 
    {
      std::string group = vm["clear"].as<std::string>();
      if (group == "*all*") gsm.clear();
      else
        {
          printf("not implemented\n");
        }
    }*/
  SafplusInitializationConfiguration sic;
  sic.iocPort     = MGT_API_TEST_PORT;
  sic.msgQueueLen = 25;
  sic.msgThreads  = 1;
  SAFplus::logSeverity = SAFplus::LOG_SEV_DEBUG;
  SAFplus::logEchoToFd = 1;  // echo logs to stdout for debugging

  safplusInitialize(/* SAFplus::LibDep::FAULT | SAFplus::LibDep::GRP | */SAFplus::LibDep::LOG | SAFplus::LibDep::MSG, sic);
  if (vm.count("list")) 
    {
    //std::string view = SAFplus::getClusterView();
    printf("calling grpCliClusterViewGet\n");
    std::string view = SAFplus::grpCliClusterViewGet();
    printf("Cluster view:\n%s", view.c_str());
    }

  return 0;
  }
