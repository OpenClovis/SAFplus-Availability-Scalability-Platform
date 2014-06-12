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
#pragma once

#ifndef CLRPCSERVICE_HXX_
#define CLRPCSERVICE_HXX_

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <clCommon.hxx>
#include <clHandleApi.hxx>

namespace SAFplus
  {
    namespace Rpc
      {

        /*
         *
         */
        class RpcService
          {
          public:
            RpcService() {} ;
            virtual ~RpcService() {};

            // Return the descriptor for this service.
            virtual const google::protobuf::ServiceDescriptor* GetDescriptor() = 0;

            // Invoke a method.
            virtual void CallMethod(const google::protobuf::MethodDescriptor *method, SAFplus::Handle destination,
                const google::protobuf::Message *request, google::protobuf::Message *response, SAFplus::Wakeable &wakeable) = 0;

            virtual const google::protobuf::Message& GetRequestPrototype(const google::protobuf::MethodDescriptor *method) const = 0;
            virtual const google::protobuf::Message& GetResponsePrototype(const google::protobuf::MethodDescriptor *method) const = 0;
          };

      } /* namespace Rpc */
  } /* namespace SAFplus */
#endif /* CLRPCSERVICE_HXX_ */
