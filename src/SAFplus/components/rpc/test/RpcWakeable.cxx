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

#include "RpcWakeable.hxx"
#include "SAFplusRpc.pb.h"
#include "SAFplusPBExt.pb.h"
#include "rpcTest.hxx"
#include <iostream>

namespace SAFplus
  {
    namespace Rpc
      {
        void FooDone1(SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse* response)
          {
            if (response->has_dataresult())
              {
                const SAFplus::Rpc::rpcTest::DataResult& dr = response->dataresult();
                logInfo("RPC", "TEST", "Response is name='%s': status='%d'", dr.name().c_str(), dr.status());
              }
          }

        void FooDone2(SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response* response)
          {
            if (response->has_dataresult())
              {
                const SAFplus::Rpc::rpcTest::DataResult& dr = response->dataresult();
                logInfo("RPC", "TEST", "Response is name='%s': status='%d'", dr.name().c_str(), dr.status());
              }
          }

        void FooDone3(SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response* response)
          {
            if (response->has_dataresult())
              {
                const SAFplus::Rpc::rpcTest::DataResult& dr = response->dataresult();
                logInfo("RPC", "TEST", "Response is name='%s': status='%d'", dr.name().c_str(), dr.status());
              }
          }

        RpcWakeable::RpcWakeable(int method) :
            method(method)
          {
          }

        RpcWakeable::~RpcWakeable()
          {
          }

        void RpcWakeable::wake(int amt, void* cookie)
          {
            switch (method)
              {
              case 1:
                FooDone1((SAFplus::Rpc::rpcTest::TestGetRpcMethodResponse*) cookie);
                break;
              case 2:
                FooDone2((SAFplus::Rpc::rpcTest::TestGetRpcMethod2Response*) cookie);
                break;
              case 3:
                FooDone3((SAFplus::Rpc::rpcTest::TestGetRpcMethod3Response*) cookie);
                break;
              default:
                break;
              }
          }

      } /* namespace Rpc */
  } /* namespace SAFplus */
