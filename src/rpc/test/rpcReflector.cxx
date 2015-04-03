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
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include "clSafplusMsgServer.hxx"
#include "clRpcApi.hxx"
#include "reflection/msgReflection.hxx"

using namespace SAFplus;

#define SERVER_MSG_PORT 65

unsigned int count=0;

int main(void)
{
    logSeverity = LOG_SEV_DEBUG;

    clMsgInitialize();

    //Msg server listening
    SAFplus::SafplusMsgServer safplusMsgServer(SERVER_MSG_PORT, 1000, 1);

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
