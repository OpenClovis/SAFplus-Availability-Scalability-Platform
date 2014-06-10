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

#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include "clRpcGenerator.hxx"
#include "clRpcImplGenerator.hxx"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/zero_copy_stream.h>

using namespace std;
using namespace google::protobuf::compiler;

namespace SAFplus
  {

    namespace Rpc
      {
        RpcGenerator::RpcGenerator(const std::string &dir)
          {
            if( dir.c_str()[0] != '/' )
              {
                char cwd[1024] = {0};
                if (getcwd(cwd, 1024) != NULL)
                  {
                    this->dir.assign(cwd).append("/").append(dir);
                  }
              }
            else
              {
                this->dir.assign(dir);
              }
          }

        RpcGenerator::~RpcGenerator()
          {
          }

        bool RpcGenerator::Generate(const google::protobuf::FileDescriptor* file, const std::string& parameter,
            google::protobuf::compiler::GeneratorContext* generator_context, std::string* error) const
          {
            std::string basename = google::protobuf::StripSuffixString(file->name(), ".proto") + "Impl";

            SAFplus::Rpc::RpcImplGenerator file_generator(file, basename);

            std::string headerFileName = basename + ".hxx";
            std::string headerFileNameFull = dir + "/" + basename + ".hxx";
            std::string backUpHeaderName = dir + "/" + basename + ".hxx.obs";

            std::string codeFileName = basename + ".cxx";
            std::string codeFileNameFull = dir + "/" + basename + ".cxx";
            std::string backUpCodeFileName = dir + "/" + basename + ".cxx.obs";

            bool mergeHeader = false;
            bool mergeImpl = false;

            std::ifstream ifs(headerFileNameFull.c_str(), std::ios::binary);
            if (ifs)
              {
                mergeHeader = true;

                std::ofstream ofs(backUpHeaderName.c_str(), std::ios::binary);

                if (ofs)
                  {
                    ofs << ifs.rdbuf();
                    ofs.close();
                  }
                ifs.close();
              }

            //Check to backup implementation file
            ifs.open(codeFileNameFull.c_str(), std::ios::binary);
            if (ifs)
              {
                mergeImpl = true;

                std::ofstream ofs(backUpCodeFileName.c_str(), std::ios::binary);

                if (ofs)
                  {
                    ofs << ifs.rdbuf();
                    ofs.close();
                  }
                ifs.close();
              }

            google::protobuf::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> header_output(generator_context->Open(headerFileName));

            google::protobuf::io::Printer header_printer(header_output.get(), '$');
            file_generator.GenerateHeader(&header_printer);

            google::protobuf::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> code_output(generator_context->Open(codeFileName));
            google::protobuf::io::Printer code_printer(code_output.get(), '$');
            file_generator.GenerateImplementation(&code_printer);

            //Calling diff and patch
            if (mergeHeader)
              {
                //Do merge header
              }

            if (mergeImpl)
              {
                //Do merge implementation
              }

            return true;
          }
      } /* namespace Rpc */
  } /* namespace SAFplus */
