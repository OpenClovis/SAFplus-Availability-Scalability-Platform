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

#include "clRpcGenerator.hxx"
#include "clRpcImplGenerator.hxx"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/zero_copy_stream.h>

namespace SAFplus
  {

    namespace Rpc
      {
        RpcGenerator::RpcGenerator()
          {
          }

        RpcGenerator::~RpcGenerator()
          {
          }

        bool RpcGenerator::Generate(const google::protobuf::FileDescriptor* file, const std::string& parameter,
            google::protobuf::compiler::GeneratorContext* generator_context, std::string* error) const
          {
            std::string basename = google::protobuf::StripSuffixString(file->name(), ".proto") + "Impl";

            SAFplus::Rpc::RpcImplGenerator file_generator(file, basename);

            std::string header_name = basename + ".hxx";
            std::string code_name = basename + ".cxx";

            google::protobuf::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> header_output(generator_context->Open(header_name));
            google::protobuf::io::Printer header_printer(header_output.get(), '$');
            file_generator.GenerateHeader(&header_printer);

            google::protobuf::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> code_output(generator_context->Open(code_name));
            google::protobuf::io::Printer code_printer(code_output.get(), '$');
            file_generator.GenerateImplementation(&code_printer);

            return true;
          }
      } /* namespace Rpc */
  } /* namespace SAFplus */
