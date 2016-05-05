#include <boost/program_options.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <clGlobals.hxx>
#include <clGroupIpi.hxx>


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
  SAFplus::utilsInitialize();  // Needed to grab the env vars like ASP_NODENAME

  boost::program_options::variables_map vm = parseOptions(argc,argv);
  if (vm.count("help")) return 0;  // Help already printed by parseOptions

  if (vm.count("zap")) 
    {
      SAFplusI::GroupSharedMem::deleteSharedMemory();
    }

  SAFplusI::GroupSharedMem gsm;
  gsm.init();

  if (vm.count("clear")) 
    {
      std::string group = vm["clear"].as<std::string>();
      if (group == "*all*") gsm.clear();
      else
        {
          printf("not implemented\n");
        }
    }
  if (vm.count("list")) 
    {
    gsm.dbgDump();
    }

  return 0;
  }
