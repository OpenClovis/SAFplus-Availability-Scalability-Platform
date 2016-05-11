#include <clClusterNodes.hxx>
#include <clCustomization.hxx>
#include <clMsgApi.hxx>
#include <boost/program_options.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

using namespace SAFplus;

/*? <section name="Command Line Programs">
<command name="safplus_cloud">
<html>
<h2>safplus_cloud</h2>
<p>This command allows you to see and modify the cluster membership on a node, 
if the message transport is using "cloud" mode.  You must use "Cloud" mode if your
selected message transport or network does not support broadcast functionality.
</p><p>
Without broadcast, it is impossible for SAFplus to discover what nodes are part of 
the cluster.  Therefore, it is necessary to use this tool to configure well known
addresses for at least two cluster nodes (in case one fails).
</p>

commands are:
<pre>
./safplus_cloud --[help,add,remove,zap,list,reload]
</pre>
<p>
Adding nodes:
<pre>
./safplus_cloud --add (address)
</pre>
</p>
<p>
Removing nodes:
<pre>
./safplus_cloud --remove (index)
</pre>
Pass in the index of the node to remove, as discovered by ./safplus_cloud --list
</p>
<p>
Listing all known nodes:
<pre>
./safplus_cloud --list
</pre>
</p>
<p>
Delete the entire cloud table
<pre>
./safplus_cloud --zap
</pre>
</p>
<p>
Load the cloud table from the SAFPLUS_CLOUD_NODES environment variable
<pre>
./safplus_cloud --reload
</pre>
</p>
</html>
</command>

</section> */

int main(int argc, char* argv[])
{
  // Explicitly create the shared memory segment if it does not exist
  SAFplus::defaultClusterNodes = new ClusterNodes(true);
  // I need the transport initialized so I can transform a string address into the transport's underlying address
  try
    {
    safplusInitialize( SAFplus::LibDep::IOC);
    }
  catch (SAFplus::Error& e)  // IOC initialize cloud mode will throw an error because my own node is not in the cloud list.  This is a problem for everyone EXCEPT this program, because it ADDS to the list.
    {
    }

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
    ("reload", boost::program_options::value<std::string>()->implicit_value("*env*"), "clear current entries and load from argument or SAFPLUS_CLOUD_NODES variable if no argument")
    ("load", boost::program_options::value<std::string>(), "load from argument or SAFPLUS_CLOUD_NODES variable if no argument, do not clear current entries")
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
  if (vm.count("reload") || vm.count("load")) 
    {
      bool erase = false;
      std::string nodes;
      if (vm.count("reload"))
	{
        nodes = vm["reload"].as<std::string>();
        erase = true;
	}
      else
        nodes = vm["reload"].as<std::string>();

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
      if (erase)
        ClusterNodes::deleteSharedMemory();
      if (nodes != "1")
	{
	  ClusterNodes cn(true);
	  int id = 0;
	  boost::char_separator<char> sep(", ");
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
    }


  try
    {

    if (vm.count("list")) 
      {
  // I need the transport initialized so I can transform a string address into the transport's underlying address
  safplusInitialize( SAFplus::LibDep::IOC);

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
