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

#ifndef CLRPCGENERATOR_HXX_
#define CLRPCGENERATOR_HXX_

#include <google/protobuf/compiler/code_generator.h>
#include <iostream>

namespace SAFplus
  {

    namespace Rpc
      {
        /*
         *
         */
        class RpcGenerator : public google::protobuf::compiler::CodeGenerator
          {
          public:
            RpcGenerator(const std::string &dir);
            ~RpcGenerator();
            bool Generate(const google::protobuf::FileDescriptor* file, const std::string& parameter,
                google::protobuf::compiler::GeneratorContext* generator_context, std::string* error) const;
          private:
           std::string dir;
          };
      } /* namespace Rpc */
  } /* namespace SAFplus */
#endif /* CLRPCGENERATOR_HXX_ */
