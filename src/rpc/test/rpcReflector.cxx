/*
 * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 *
 * The source code for  this program is not published  or otherwise
 * divested of  its trade secrets, irrespective  of  what  has been
 * deposited with the U.S. Copyright office.
 *
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
#include <boost/program_options.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>

#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include "clSafplusMsgServer.hxx"
#include "clRpcApi.hxx"
#include "reflection/msgReflection.hxx"

using namespace SAFplus;

int SERVER_MSG_PORT=65;

unsigned int count=0;

boost::program_options::variables_map parseOptions(int argc,char *argv[])
{
  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("help", "display help message")
    ("port", boost::program_options::value<int>()->default_value(SERVER_MSG_PORT), "my port number")
    ("loglevel", boost::program_options::value<std::string>(), "logging cutoff level")
    ;

  boost::program_options::variables_map vm;        
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);    
  
  if (vm.count("port")) SERVER_MSG_PORT = vm["port"].as<int>();
  if (vm.count("loglevel")) SAFplus::logSeverity = logSeverityGet(vm["loglevel"].as<std::string>().c_str());
  if (vm.count("help")) 
    {
      std::cout << desc << "\n";
    }
  return vm;
}



int main(int argc, char* argv[])
{
  //logSeverity = LOG_SEV_DEBUG;

    boost::program_options::variables_map vm = parseOptions(argc,argv);
    if (vm.count("help")) return 0;  // Help already printed by parseOptions

    SafplusInitializationConfiguration sic;
    sic.iocPort     = SERVER_MSG_PORT;
    sic.msgQueueLen = 1000;
    sic.msgThreads  = 30;
    safplusInitialize( SAFplus::LibDep::MSG, sic);


    // Handle RPC
    SAFplus::Rpc::RpcChannel *channel = new SAFplus::Rpc::RpcChannel(&safplusMsgServer, new SAFplus::Rpc::msgReflection::msgReflectionImpl());
    channel->setMsgType(100, 101);

    safplusMsgServer.Start();

    // Loop forever
    while(1)
      {
        sleep(5);
        printf("received [%d]\n", count);
      }

}


namespace SAFplus {
namespace Rpc {
namespace msgReflection {

  msgReflectionImpl::msgReflectionImpl()
  {
    //TODO: Auto-generated constructor stub
  }

  msgReflectionImpl::~msgReflectionImpl()
  {
    //TODO: Auto-generated destructor stub
  }

  void msgReflectionImpl::call(const ::SAFplus::Rpc::msgReflection::CallRequest* request,
                                ::SAFplus::Rpc::msgReflection::CallResponse* response)
  {
    if (request->has_data())
      response->set_data(request->data());
    //printf("Message RECEIVED!\n");
    count++;
  }

}  // namespace msgReflection
}  // namespace Rpc
}  // namespace SAFplus
