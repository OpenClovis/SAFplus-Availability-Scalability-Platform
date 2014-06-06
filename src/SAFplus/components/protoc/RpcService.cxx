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

#include "RpcService.hxx"
#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/strutil.h>

using namespace std;

namespace SAFplus
  {
    namespace Rpc
      {
        string DotsToUnderscores(const string& name)
          {
            return google::protobuf::StringReplace(name, ".", "_", true);
          }

        string DotsToColons(const string& name)
          {
            return google::protobuf::StringReplace(name, ".", "::", true);
          }

        string ClassName(const google::protobuf::Descriptor* descriptor, bool qualified)
          {

            // Find "outer", the descriptor of the top-level message in which
            // "descriptor" is embedded.
            const google::protobuf::Descriptor* outer = descriptor;
            while (outer->containing_type() != NULL)
              outer = outer->containing_type();

            const string& outer_name = outer->full_name();
            string inner_name = descriptor->full_name().substr(outer_name.size());

            if (qualified)
              {
                return "::" + DotsToColons(outer_name) + DotsToUnderscores(inner_name);
              }
            else
              {
                return outer->name() + DotsToUnderscores(inner_name);
              }
          }

        ServiceGenerator::ServiceGenerator(const google::protobuf::ServiceDescriptor* descriptor) :
            descriptor_(descriptor)
          {
            vars_["classname"] = descriptor_->name() + "Impl";
            vars_["full_name"] = descriptor_->full_name() + "Impl";
            vars_["superclass"] = descriptor_->name();
          }

        ServiceGenerator::~ServiceGenerator()
          {
          }

        void ServiceGenerator::GenerateDeclarations(google::protobuf::io::Printer* printer)
          {
            printer->Print(vars_, "class $classname$ : public $superclass$ {\n"
                " public:\n");

            printer->Indent();

            printer->Print(vars_, "$classname$();\n"
                "~$classname$();\n"
                "\n"
                "// implements $classname$ \n");

            GenerateMethodSignatures(NON_VIRTUAL, printer);

            printer->Outdent();
            printer->Print("};\n");
          }

        void ServiceGenerator::GenerateMethodSignatures(VirtualOrNon virtual_or_non, google::protobuf::io::Printer* printer)
          {
            for (int i = 0; i < descriptor_->method_count(); i++)
              {
                const google::protobuf::MethodDescriptor* method = descriptor_->method(i);
                map<string,string> sub_vars;
                sub_vars["name"] = method->name();
                sub_vars["input_type"] = ClassName(method->input_type(), true);
                sub_vars["output_type"] = ClassName(method->output_type(), true);
                sub_vars["virtual"] = virtual_or_non == VIRTUAL ? "virtual " : "";

                printer->Print(sub_vars, "$virtual$void $name$(::google::protobuf::RpcController* controller,\n"
                    "                     const $input_type$* request,\n"
                    "                     $output_type$* response,\n"
                    "                     ::google::protobuf::Closure* done);\n");
              }
          }

        void ServiceGenerator::GenerateImplementation(google::protobuf::io::Printer* printer)
          {
            printer->Indent();
            // Constructor, Destructor
            printer->Print(vars_, "\n"
                "$classname$::$classname$()\n"
                "{\n"
                "  //TODO: Auto-generated constructor stub\n"
                "}\n"
                "\n"
                "$classname$::~$classname$()\n"
                "{\n"
                "  //TODO: Auto-generated destructor stub\n"
                "}\n");

            printer->Outdent();

            // Generate methods of the interface.
            for (int i = 0; i < descriptor_->method_count(); i++)
              {
                const google::protobuf::MethodDescriptor* method = descriptor_->method(i);
                map<string,string> sub_vars;
                sub_vars["classname"] = vars_["classname"];
                sub_vars["name"] = method->name();
                sub_vars["input_type"] = ClassName(method->input_type(), true);
                sub_vars["output_type"] = ClassName(method->output_type(), true);

                printer->Indent();
                printer->Print(sub_vars, "\nvoid $classname$::$name$(::google::protobuf::RpcController* controller,\n"
                    "                              const $input_type$* request,\n"
                    "                              $output_type$* response,\n"
                    "                              ::google::protobuf::Closure* done)\n");

                printer->Print("{\n"
                    "  //TODO: put your code here \n"
                    "\n"
                    "  done->Run(); // DO NOT removed this line!!! \n"
                    "}\n");
                printer->Outdent();
              }
          }

      } // namespace Rpc
  } // namespace SAFplus
