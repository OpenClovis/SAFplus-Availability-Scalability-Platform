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
#include <google/protobuf/descriptor.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include "RpcGenerator.hxx"
#include "RpcFileGenerator.hxx"
#include "RenamePbFile.hxx"

using namespace std;
using namespace google::protobuf::compiler;

namespace SAFplus
  {

    namespace Rpc
      {
        RpcGenerator::RpcGenerator(const std::string &dir, bool renameFile)
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
            this->renameFile = renameFile;
          }

        RpcGenerator::~RpcGenerator()
          {
          }

        bool RpcGenerator::Generate(const google::protobuf::FileDescriptor* file, const std::string& parameter,
            google::protobuf::compiler::GeneratorContext* generator_context, std::string* error) const
          {
            std::string basename = google::protobuf::StripSuffixString(file->name(), ".proto");

            /* Rename pb.cc => pb.cxx, pb.h => pb.hxx */
            RenamePbFile renamePbFile(dir, basename);

            if (renameFile)
              {
                //Only rename file
                return true;
              }

            SAFplus::Rpc::RpcFileGenerator file_generator(file, basename);

            std::string headerFileName = basename + ".hxx";
            std::string headerFileNameFull = dir + "/" + basename + ".hxx";

            std::string codeFileName =  basename + ".cxx";
            std::string codeFileNameFull = dir + "/" + basename + ".cxx";

            std::string codeServerFileName = "server/" + basename + "Impl.cxx";
            std::string codeServerFileNameFull = dir + "/server/" + basename + "Impl.cxx";
            std::string backUpCodeServerFileName = dir + "/server/" + basename + "Impl.cxx.obs";

            bool mergeImpl = false;

            //Check to backup implementation file
            std::ifstream ifs(codeServerFileNameFull.c_str(), std::ios::binary);
            if (ifs)
              {
                mergeImpl = true;

                std::ofstream ofs(backUpCodeServerFileName.c_str(), std::ios::binary);

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

            google::protobuf::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> code_server_output(generator_context->Open(codeServerFileName));
            google::protobuf::io::Printer code_server_printer(code_server_output.get(), '$');
            file_generator.GenerateServerImplementation(&code_server_printer);

            if (mergeImpl)
              {
                //TODO: GAS: diff,patch,clean
                //Do merge implementation
              }

            return true;
          }
      } /* namespace Rpc */
  } /* namespace SAFplus */
