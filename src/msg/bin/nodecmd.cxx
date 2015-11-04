#include <clClusterNodes.hxx>
#include <clCustomization.hxx>
#include <clMsgApi.hxx>
#include <boost/program_options.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

using namespace SAFplus;

int main(int argc, char* argv[])
{
  // Explicitly create the shared memory segment if it does not exist
  SAFplus::defaultClusterNodes = new ClusterNodes(true);
  // I need the transport initialized so I can transform a string address into the transport's underlying address
  safplusInitialize( SAFplus::LibDep::IOC);

  //SAFplus::logCompName = "RFL";
  //SAFplus::logSeverity = SAFplus::LOG_SEV_INFO;
  //logEchoToFd = 1; // stdout
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("add", boost::program_options::value<std::string>(), "add nodes")
    ("remove", boost::program_options::value<uint>(), "remove node by id")
    ("zap", boost::program_options::value<uint>()->implicit_value(0), "delete the shared memory segment")
    ("list", boost::program_options::value<uint>()->implicit_value(SAFplus::MaxNodes), "show all existing cluster nodes ")
    ("id", boost::program_options::value<uint>(), "supply a node id if needed")
    ("reload", boost::program_options::value<std::string>()->implicit_value("*env*"), "clear current entries and load from argument or SAFPLUS_CLOUD_NODES variable")
    ;

  boost::program_options::variables_map vm;        
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);    
  if (vm.count("help")) 
    {
      std::cout << desc << "\n";
      return 0;
    }
  if (vm.count("add")) 
    {
      std::string nodename = vm["add"].as<std::string>();
      ClusterNodes cn(true);
      int id = 0;
      if (vm.count("id")) { id = vm["id"].as<uint>(); }
      printf("Adding '%s'\n",nodename.c_str());
      cn.add(nodename,1,id);      
    }
  if (vm.count("remove")) 
    {
    uint id = vm["remove"].as<uint>();  
    ClusterNodes cn(true);
    cn.mark(id, NodeStatus::NonExistent);
    }
  if (vm.count("zap")) 
    {
    ClusterNodes::deleteSharedMemory();
    }
  if (vm.count("reload")) 
    {
      std::string nodes = vm["reload"].as<std::string>();
      if (nodes == "*env*")
      {
        char* cnVar = std::getenv("SAFPLUS_CLOUD_NODES");
        if (!cnVar)
          {
          printf("SAFPLUS_CLOUD_NODES environment variable is not defined\n");
          return -1;
          }
        nodes = cnVar;
      }
      ClusterNodes::deleteSharedMemory();

      ClusterNodes cn(true);
      int id = 0;
      boost::char_separator<char> sep(",: ");
      boost::tokenizer< boost::char_separator<char> > tokens(nodes, sep);
      int wkn=1;
      BOOST_FOREACH (const std::string& t, tokens) 
          {
            if (!t.empty())
              { 
              cn.set(wkn,t,1,NodeStatus::Unknown);
              }
            wkn++;
            printf("Adding '%s'\n",t.c_str());
          }
    }


  try
    {
    if (vm.count("list")) 
      {
     ClusterNodes cn;
    uint id = 1;
    if (vm.count("id")) { id = vm["id"].as<uint>(); }  
    int showAmt = vm["list"].as<uint>();
    int count = 0;
    for(ClusterNodes::Iterator it = cn.begin(id); (it != cn.end()) && (count < showAmt); it++,count++)
      {
      std::string s = it.transportAddressString();
      printf("%4d: %s %s \n", it.nodeId(), s.c_str(), nodeStatus2CString(it.status()));
      }
    
      }
   } 
  catch(boost::interprocess::interprocess_exception& e)
    {
    printf("Exception '%s': possibly segment is not created\n",e.what());
    }
  catch(SAFplus::Error &e)
    {
      if (e.clError == SAFplus::Error::DOES_NOT_EXIST)
        {
          // section does not exist so nothing to list
        }
      else throw;
    }
    //SAFplus::logSeverity = logSeverityGet(vm["loglevel"].as<std::string>().c_str());
  //printf("done\n");

  return 0;
  }
