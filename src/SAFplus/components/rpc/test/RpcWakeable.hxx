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

#include <clHandleApi.hxx>
#include <clCommon.hxx>

#ifndef RPCWAKEABLE_HXX_
#define RPCWAKEABLE_HXX_

namespace SAFplus
  {
    namespace Rpc
      {

        /*
         *
         */
        class RpcWakeable : public SAFplus::Wakeable
          {
          public:
            RpcWakeable() : method(0) {};
            RpcWakeable(int method);
            virtual ~RpcWakeable();
            void wake(int amt, void* cookie = NULL);

          public:
            int method;
          };

      } /* namespace Rpc */
  } /* namespace SAFplus */
#endif /* CLRPCWAKEABLE_HXX_ */
