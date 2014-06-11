// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

// Modified by OpenClovis
#include <string>
#include <iostream>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/common.h>
#include "RpcFileGenerator.hxx"

using namespace std;
using namespace google::protobuf;

namespace SAFplus
  {
    namespace Rpc
      {
        // Convert a file name into a valid identifier.
        string FilenameIdentifier(const string& filename)
          {
            string result;
            for (unsigned int i = 0; i < filename.size(); i++)
              {
                if (google::protobuf::ascii_isalnum(filename[i]))
                  {
                    result.push_back(filename[i]);
                  }
                else
                  {
                    // Not alphanumeric.  To avoid any possibility of name conflicts we
                    // use the hex code for the character.
                    result.push_back('_');
                    char buffer[google::protobuf::kFastToBufferSize];
                    result.append(google::protobuf::FastHexToBuffer(static_cast<uint8_t>(filename[i]), buffer));
                  }
              }
            return result;
          }
        bool HasDescriptorMethods(const google::protobuf::FileDescriptor* file)
          {
            return file->options().optimize_for() != google::protobuf::FileOptions::LITE_RUNTIME;
          }

        // Return the name of the AddDescriptors() function for a given file.
        string GlobalAddDescriptorsName(const string& filename)
          {
            return "protobuf_AddDesc_" + FilenameIdentifier(filename);
          }

        // Return the name of the AssignDescriptors() function for a given file.
        string GlobalAssignDescriptorsName(const string& filename)
          {
            return "protobuf_AssignDesc_Rpc_" + FilenameIdentifier(filename);
          }

        RpcFileGenerator::RpcFileGenerator(const google::protobuf::FileDescriptor *fileDesc, const std::string &fileName) :
            fileDesc(fileDesc), fileName(fileName)
          {
            google::protobuf::SplitStringUsing(fileDesc->package(), ".", &package_parts_);
            for (int i = 0; i < fileDesc->service_count(); i++)
              {
                service_generators.push_back(new ServiceGenerator(fileDesc->service(i)));
              }
          }

        RpcFileGenerator::~RpcFileGenerator()
          {
            std::vector<ServiceGenerator*>::iterator iter = service_generators.begin();
            for (; iter != service_generators.end(); ++iter)
              {
                delete *iter;
              }
          }

        void RpcFileGenerator::GenerateHeader(google::protobuf::io::Printer* printer)
          {
            string filename_identifier = FilenameIdentifier(fileName);

            // Generate top of header.
            printer->Print("// Generated by the protocol buffer compiler.\n"
                "#pragma once\n"
                "#include <string>\n"
                "\n", "filename", fileDesc->name(), "filename_identifier", filename_identifier);

            printer->Print("#include <google/protobuf/service.h>\n"
                "#include <google/protobuf/stubs/common.h>\n"
                "#include <google/protobuf/stubs/once.h>\n"
                "#include <clCommon.hxx>\n"
                "#include <clHandleApi.hxx>\n"
                "#include <clRpcService.hxx>\n"
                "#include <clRpcChannel.hxx>\n"
                "#include \"$stubs$\"\n", "stubs", google::protobuf::StripSuffixString(fileDesc->name(), ".proto") + ".pb.h");

            GenerateNamespaceOpeners(printer);

            // Generate service definitions.
            for (int i = 0; i < fileDesc->service_count(); i++)
              {
                if (i > 0)
                  {
                    printer->Print("\n");
                  }
                service_generators[i]->GenerateDeclarations(printer);
              }

            // Close up namespace.
            GenerateNamespaceClosers(printer);
          }

        void RpcFileGenerator::GenerateImplementation(google::protobuf::io::Printer* printer)
          {
            printer->Print("#include \"$header$\"\n", "header", fileName + ".hxx");

            GenerateNamespaceOpeners(printer);

            printer->Print("\n"
                "namespace {\n"
                "\n");
            for (int i = 0; i < fileDesc->service_count(); i++)
              {
                //service_generators[i]->GenerateDescriptorInitializer(printer, i);
                printer->Print("const ::google::protobuf::ServiceDescriptor* $name$_descriptor_ = "
                    "NULL;\n", "name", fileDesc->service(i)->name());
              }

            printer->Print("\n"
                "}  // namespace\n"
                "\n");

            // Define our externally-visible BuildDescriptors() function.  (For the lite
            // library, all this does is initialize default instances.)
            GenerateBuildDescriptors(printer);

            // Generate service definitions.
            for (int i = 0; i < fileDesc->service_count(); i++)
              {
                if (i > 0)
                  {
                    printer->Print("\n");
                  }
                service_generators[i]->GenerateImplementation(printer);
              }

            // Close up namespace.
            GenerateNamespaceClosers(printer);
          }

        void RpcFileGenerator::GenerateServerImplementation(google::protobuf::io::Printer* printer)
          {
            printer->Print("#include \"$header$\"\n", "header", fileName + ".hxx");

            GenerateNamespaceOpeners(printer);

            // Generate service definitions.
            for (int i = 0; i < fileDesc->service_count(); i++)
              {
                if (i > 0)
                  {
                    printer->Print("\n");
                  }
                service_generators[i]->GenerateServerImplementation(printer);
              }

            // Close up namespace.
            GenerateNamespaceClosers(printer);
          }

        void RpcFileGenerator::GenerateNamespaceOpeners(google::protobuf::io::Printer* printer)
          {
            if (package_parts_.size() > 0)
              printer->Print("\n");

            for (int i = 0; i < package_parts_.size(); i++)
              {
                printer->Print("namespace $part$ {\n", "part", package_parts_[i]);
              }
          }

        void RpcFileGenerator::GenerateNamespaceClosers(google::protobuf::io::Printer* printer)
          {
            if (package_parts_.size() > 0)
              printer->Print("\n");

            for (int i = package_parts_.size() - 1; i >= 0; i--)
              {
                printer->Print("}  // namespace $part$\n", "part", package_parts_[i]);
              }
          }

        void RpcFileGenerator::GenerateBuildDescriptors(google::protobuf::io::Printer* printer)
          {
            // AddDescriptors() is a file-level procedure which adds the encoded
            // FileDescriptorProto for this .proto file to the global DescriptorPool for
            // generated files (DescriptorPool::generated_pool()). It either runs at
            // static initialization time (by default) or when default_instance() is
            // called for the first time (in LITE_RUNTIME mode with
            // GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER flag enabled). This procedure also
            // constructs default instances and registers extensions.
            //
            // Its sibling, AssignDescriptors(), actually pulls the compiled
            // FileDescriptor from the DescriptorPool and uses it to populate all of
            // the global variables which store pointers to the descriptor objects.
            // It also constructs the reflection objects.  It is called the first time
            // anyone calls descriptor() or GetReflection() on one of the types defined
            // in the file.

            // In optimize_for = LITE_RUNTIME mode, we don't generate AssignDescriptors()
            // and we only use AddDescriptors() to allocate default instances.
            if (HasDescriptorMethods(fileDesc))
              {
                printer->Print("\n"
                    "void $assigndescriptorsname$() {\n", "assigndescriptorsname", GlobalAssignDescriptorsName(fileDesc->name()));
                printer->Indent();

                // Make sure the file has found its way into the pool.  If a descriptor
                // is requested *during* static init then AddDescriptors() may not have
                // been called yet, so we call it manually.  Note that it's fine if
                // AddDescriptors() is called multiple times.
                printer->Print("$adddescriptorsname$();\n", "adddescriptorsname", GlobalAddDescriptorsName(fileDesc->name()));

                // Get the file's descriptor from the pool.
                printer->Print("const ::google::protobuf::FileDescriptor* file =\n"
                    "  ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName("
                    "\n"
                    "    \"$filename$\");\n"
                    // Note that this GOOGLE_CHECK is necessary to prevent a warning about
                    // "file" being unused when compiling an empty .proto file.
                    "GOOGLE_CHECK(file != NULL);\n", "filename", fileDesc->name());

                for (int i = 0; i < fileDesc->service_count(); i++)
                  {
                    service_generators[i]->GenerateDescriptorInitializer(printer, i);
                  }

                printer->Outdent();
                printer->Print("}\n"
                    "\n");
                // ---------------------------------------------------------------

                // protobuf_AssignDescriptorsOnce():  The first time it is called, calls
                // AssignDescriptors().  All later times, waits for the first call to
                // complete and then returns.
                printer->Print("namespace {\n"
                    "\n"
                    "GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);\n"
                    "inline void protobuf_AssignDescriptorsOnce() {\n"
                    "  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,"
                    "\n"
                    "                 &$assigndescriptorsname$);\n"
                    "}\n"
                    "\n", "assigndescriptorsname", GlobalAssignDescriptorsName(fileDesc->name()));

                printer->Print("}  // namespace\n");
              }
          }
      } /* namespace Rpc */
  } /* namespace SAFplus */
