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

#ifndef AMFRPCSERVICEIMPL_HXX_
#define AMFRPCSERVICEIMPL_HXX_
#include <iostream>

namespace SAFplus
  {
    namespace Rpc
      {

        /*
         *
         */
        class AMFRpcServiceImpl : public SAFplus::Rpc::amfRpc::amfRpc
          {
            void startComponent(::google::protobuf::RpcController* controller, const ::SAFplus::Rpc::amfRpc::StartComponentRequest* request,
                ::SAFplus::Rpc::amfRpc::StartComponentResponse* response, ::google::protobuf::Closure* done)
              {
                std::cout << "Implementation and put response data!" << std::endl;
                done->Run();
              }
            void stopComponent(::google::protobuf::RpcController* controller, const ::SAFplus::Rpc::amfRpc::StopComponentRequest* request,
                ::SAFplus::Rpc::amfRpc::StopComponentResponse* response, ::google::protobuf::Closure* done)
              {
                std::cout << "Implementation and put response data!" << std::endl;
                done->Run();
              }
            AMFRpcServiceImpl();
            virtual ~AMFRpcServiceImpl();
          };

      } /* namespace Rpc */
  } /* namespace SAFplus */
#endif /* AMFRPCSERVICEIMPL_HXX_ */
