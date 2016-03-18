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

#include <google/protobuf/io/printer.h>
#include "RpcService.hxx"
#include "protobufUtils.hxx"
using namespace std;
using namespace google::protobuf;


namespace SAFplus
  {
    namespace Rpc
      {

        const std::string NO_RESPONSE = "NO_RESPONSE";
        bool isNoResponse(const google::protobuf::MethodDescriptor* method)
          {
            if (!method->output_type()->name().compare(NO_RESPONSE))
              {
                return true;
              }
            return false;
          }

        string DotsToUnderscores(const string& name)
          {
            return StringReplace(name, ".", "_", true);
          }

        string DotsToColons(const string& name)
          {
            return StringReplace(name, ".", "::", true);
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
            vars_["classname"] = descriptor_->name();
            vars_["full_name"] = descriptor_->full_name();
          }

        ServiceGenerator::~ServiceGenerator()
          {
          }

        void ServiceGenerator::GenerateDescriptorInitializer(google::protobuf::io::Printer* printer, int index)
          {
            map<string,string> vars;
            vars["classname"] = descriptor_->name();
            vars["index"] = SimpleItoa(index);

            printer->Print(vars, "$classname$_descriptor_ = file->service($index$);\n");
          }

        void ServiceGenerator::GenerateDeclarations(google::protobuf::io::Printer* printer)
          {
            // Forward-declare the stub type.
            printer->Print(vars_, "class $classname$_Stub;\n"
                "\n");

            GenerateInterface(printer);
            GenerateStubDefinition(printer);

            //Generator server implementation class
            printer->Print(vars_, "class $classname$Impl : public $classname$ {\n"
                " public:\n");

            printer->Indent();

            printer->Print(vars_, "$classname$Impl();\n"
                "~$classname$Impl();\n"
                "\n");

            GenerateMethodSignatures(NON_VIRTUAL, printer, false, true);

            printer->Outdent();
            printer->Print("};\n");
          }

        void ServiceGenerator::GenerateMethodSignatures(VirtualOrNon virtual_or_non, google::protobuf::io::Printer* printer, bool client,
            bool impl)
          {
            if (impl == true)
              {
                printer->Print(vars_, "\n"
                    "// implements $classname$Impl ----------------------------------------------\n");

                for (int i = 0; i < descriptor_->method_count(); i++)
                  {
                    const google::protobuf::MethodDescriptor* method = descriptor_->method(i);
                    map<string,string> sub_vars;
                    sub_vars["name"] = method->name();
                    sub_vars["input_type"] = ClassName(method->input_type(), true);
                    sub_vars["output_type"] = ClassName(method->output_type(), true);
                    sub_vars["virtual"] = virtual_or_non == VIRTUAL ? "virtual " : "";

                    if (isNoResponse(method))
                      {
                        printer->Print(sub_vars, "$virtual$void $name$(const $input_type$* request);\n");
                      }
                    else
                      {
                        printer->Print(sub_vars, "$virtual$void $name$(const $input_type$* request,\n"
                            "                     $output_type$* response);\n");
                      }
                  }
              }

            if (client == true)
              {
                printer->Print(vars_, "\n"
                    "// implements $classname$ ------------------------------------------\n");

                for (int i = 0; i < descriptor_->method_count(); i++)
                  {
                    const google::protobuf::MethodDescriptor* method = descriptor_->method(i);
                    map<string,string> sub_vars;
                    sub_vars["name"] = method->name();
                    sub_vars["input_type"] = ClassName(method->input_type(), true);
                    sub_vars["output_type"] = ClassName(method->output_type(), true);
                    sub_vars["virtual"] = virtual_or_non == VIRTUAL ? "virtual " : "";

                    if (isNoResponse(method))
                      {
                        printer->Print(sub_vars, "$virtual$void $name$(SAFplus::Handle destination,\n"
                            "                     const $input_type$* request);\n");
                      }
                    else
                      {
                        printer->Print(sub_vars, "$virtual$void $name$(SAFplus::Handle destination,\n"
                            "                     const $input_type$* request,\n"
                            "                     $output_type$* response,\n"
                            "                     SAFplus::Wakeable& wakeable = *((SAFplus::Wakeable*)nullptr));\n");
                      }
                  }
              }
          }

        void ServiceGenerator::GenerateInterface(google::protobuf::io::Printer* printer)
          {
            printer->Print(vars_, "class $classname$ : public SAFplus::Rpc::RpcService {\n"
                " protected:\n"
                "  // This class should be treated as an abstract interface.\n"
                "  inline $classname$() {};\n"
                " public:\n"
                "  virtual ~$classname$();\n");
            printer->Indent();

            printer->Print(vars_, "\n"
                "typedef $classname$_Stub Stub;\n"
                "\n"
                "static const ::google::protobuf::ServiceDescriptor* descriptor();\n"
                "\n");

            GenerateMethodSignatures(VIRTUAL, printer);

            printer->Print("\n"
                "\n"
                "const ::google::protobuf::ServiceDescriptor* GetDescriptor();\n"
                "void CallMethod(const ::google::protobuf::MethodDescriptor* method,\n"
                "                SAFplus::Handle destination,\n"
                "                const ::google::protobuf::Message* request,\n"
                "                ::google::protobuf::Message* response,\n"
                "                SAFplus::Wakeable& wakeable = *((SAFplus::Wakeable*)nullptr));\n"
                "const ::google::protobuf::Message& GetRequestPrototype(\n"
                "  const ::google::protobuf::MethodDescriptor* method) const;\n"
                "const ::google::protobuf::Message& GetResponsePrototype(\n"
                "  const ::google::protobuf::MethodDescriptor* method) const;\n");

            printer->Outdent();
            printer->Print(vars_, "\n"
                " private:\n"
                "  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS($classname$);\n"
                "};\n"
                "\n");
          }

        void ServiceGenerator::GenerateImplementation(google::protobuf::io::Printer* printer)
          {
            printer->Print(vars_, "$classname$::~$classname$() {}\n"
                "\n"
                "const ::google::protobuf::ServiceDescriptor* $classname$::descriptor() {\n"
                "  protobuf_AssignDescriptorsOnce();\n"
                "  return $classname$_descriptor_;\n"
                "}\n"
                "\n"
                "const ::google::protobuf::ServiceDescriptor* $classname$::GetDescriptor() {\n"
                "  protobuf_AssignDescriptorsOnce();\n"
                "  return $classname$_descriptor_;\n"
                "}\n"
                "\n");

            // Generate methods of the interface.
            GenerateNotImplementedMethods(printer);
            GenerateCallMethod(printer);
            GenerateGetPrototype(REQUEST, printer);
            GenerateGetPrototype(RESPONSE, printer);

            // Generate stub implementation.
            printer->Print(vars_, "$classname$_Stub::$classname$_Stub(SAFplus::Rpc::RpcChannel* channel)\n"
                "  : channel_(channel), owns_channel_(false) {}\n"
                "$classname$_Stub::$classname$_Stub(\n"
                "    SAFplus::Rpc::RpcChannel* channel,\n"
                "    ::google::protobuf::Service::ChannelOwnership ownership)\n"
                "  : channel_(channel),\n"
                "    owns_channel_(ownership == ::google::protobuf::Service::STUB_OWNS_CHANNEL) {}\n"
                "$classname$_Stub::~$classname$_Stub() {\n"
                "  if (owns_channel_) delete channel_;\n"
                "}\n"
                "\n");

            GenerateStubMethods(printer);
          }

        void ServiceGenerator::GenerateStubDefinition(google::protobuf::io::Printer* printer)
          {
            printer->Print(vars_, "class $classname$_Stub : public $classname$ {\n"
                " public:\n");

            printer->Indent();

            printer->Print(vars_, "$classname$_Stub(SAFplus::Rpc::RpcChannel* channel);\n"
                "$classname$_Stub(SAFplus::Rpc::RpcChannel* channel,\n"
                "                 ::google::protobuf::Service::ChannelOwnership ownership);\n"
                "~$classname$_Stub();\n"
                "\n"
                "inline SAFplus::Rpc::RpcChannel* channel() { return channel_; }\n"
                "\n");

            GenerateMethodSignatures(NON_VIRTUAL, printer, true, false);

            printer->Outdent();
            printer->Print(vars_, " private:\n"
                "  SAFplus::Rpc::RpcChannel* channel_;\n"
                "  bool owns_channel_;\n"
                "  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS($classname$_Stub);\n"
                "};\n"
                "\n");
          }

        void ServiceGenerator::GenerateServerImplementation(google::protobuf::io::Printer* printer)
          {
            printer->Indent();
            // Constructor, Destructor
            printer->Print(vars_, "\n"
                "$classname$Impl::$classname$Impl()\n"
                "{\n"
                "  //TODO: Auto-generated constructor stub\n"
                "}\n"
                "\n"
                "$classname$Impl::~$classname$Impl()\n"
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
                if (isNoResponse(method))
                  {
                    printer->Print(sub_vars, "\nvoid $classname$Impl::$name$(const $input_type$* request)\n");
                  }
                else
                  {
                    printer->Print(sub_vars, "\nvoid $classname$Impl::$name$(const $input_type$* request,\n"
                        "                              $output_type$* response)\n");
                  }

                printer->Print("{\n"
                    "  //TODO: put your code here\n"
                    "}\n");
                printer->Outdent();
              }
          }

        void ServiceGenerator::GenerateNotImplementedMethods(google::protobuf::io::Printer* printer)
          {
            for (int i = 0; i < descriptor_->method_count(); i++)
              {
                const MethodDescriptor* method = descriptor_->method(i);
                map<string,string> sub_vars;
                sub_vars["classname"] = descriptor_->name();
                sub_vars["name"] = method->name();
                sub_vars["index"] = SimpleItoa(i);
                sub_vars["input_type"] = ClassName(method->input_type(), true);
                sub_vars["output_type"] = ClassName(method->output_type(), true);

                if (isNoResponse(method))
                  {
                    printer->Print(sub_vars, "void $classname$::$name$(const $input_type$*)\n");
                  }
                else
                  {
                    printer->Print(sub_vars, "void $classname$::$name$(const $input_type$*,\n"
                                            "                         $output_type$*)\n");
                  }
                    printer->Print(sub_vars,
                        "{\n"
                        "  logError(\"RPC\",\"SVR\",\"Method $name$() not implemented.\");\n"
                        "}\n"
                        "\n");
              }

              for (int i = 0; i < descriptor_->method_count(); i++)
                {
                  const MethodDescriptor* method = descriptor_->method(i);
                  map<string,string> sub_vars;
                  sub_vars["classname"] = descriptor_->name();
                  sub_vars["name"] = method->name();
                  sub_vars["index"] = SimpleItoa(i);
                  sub_vars["input_type"] = ClassName(method->input_type(), true);
                  sub_vars["output_type"] = ClassName(method->output_type(), true);

                  if (isNoResponse(method))
                    {
                      printer->Print(sub_vars, "void $classname$::$name$(SAFplus::Handle destination,\n"
                              "                     const $input_type$* request)\n");
                    }
                  else
                    {
                      printer->Print(sub_vars, "void $classname$::$name$(SAFplus::Handle destination,\n"
                              "                     const $input_type$* request,\n"
                              "                     $output_type$* response,\n"
                              "                     SAFplus::Wakeable& wakeable)\n");
                    }
                  printer->Print(sub_vars,
                      "{\n"
                      "  logError(\"RPC\",\"SVR\",\"Method $name$() not implemented.\");\n"
                      "}\n"
                      "\n");

                }
          }

        void ServiceGenerator::GenerateCallMethod(google::protobuf::io::Printer* printer)
          {
            printer->Print(vars_, "void $classname$::CallMethod(const ::google::protobuf::MethodDescriptor* method,\n"
                "                             SAFplus::Handle destination,\n"
                "                             const ::google::protobuf::Message* request,\n"
                "                             ::google::protobuf::Message* response,\n"
                "                             SAFplus::Wakeable& wakeable) {\n"
                "  GOOGLE_DCHECK_EQ(method->service(), $classname$_descriptor_);\n"
                "  switch(method->index()) {\n");

            for (int i = 0; i < descriptor_->method_count(); i++)
              {
                const google::protobuf::MethodDescriptor* method = descriptor_->method(i);
                map<string,string> sub_vars;
                sub_vars["name"] = method->name();
                sub_vars["index"] = SimpleItoa(i);
                sub_vars["input_type"] = ClassName(method->input_type(), true);
                sub_vars["output_type"] = ClassName(method->output_type(), true);

                // Note:  down_cast does not work here because it only works on pointers,
                //   not references.
                if (isNoResponse(method))
                  {
                    printer->Print(sub_vars, "    case $index$:\n"
                        "      $name$(::google::protobuf::down_cast<const $input_type$*>(request));\n"
                        "      break;\n");
                  }
                else
                  {
                    printer->Print(sub_vars, "    case $index$:\n"
                        "      $name$(::google::protobuf::down_cast<const $input_type$*>(request),\n"
                        "             ::google::protobuf::down_cast< $output_type$*>(response));\n"
                        "      break;\n");
                  }
              }

            printer->Print(vars_, "    default:\n"
                "      GOOGLE_LOG(FATAL) << \"Bad method index; this should never happen.\";\n"
                "      break;\n"
                "  }\n"
                "}\n"
                "\n");
          }

        void ServiceGenerator::GenerateGetPrototype(RequestOrResponse which, google::protobuf::io::Printer* printer)
          {
            if (which == REQUEST)
              {
                printer->Print(vars_, "const ::google::protobuf::Message& $classname$::GetRequestPrototype(\n");
              }
            else
              {
                printer->Print(vars_, "const ::google::protobuf::Message& $classname$::GetResponsePrototype(\n");
              }

            printer->Print(vars_, "    const ::google::protobuf::MethodDescriptor* method) const {\n"
                "  GOOGLE_DCHECK_EQ(method->service(), descriptor());\n"
                "  switch(method->index()) {\n");

            for (int i = 0; i < descriptor_->method_count(); i++)
              {
                const google::protobuf::MethodDescriptor* method = descriptor_->method(i);
                const google::protobuf::Descriptor* type = (which == REQUEST) ? method->input_type() : method->output_type();

                map<string,string> sub_vars;
                sub_vars["index"] = SimpleItoa(i);
                sub_vars["type"] = ClassName(type, true);

                printer->Print(sub_vars, "    case $index$:\n"
                    "      return $type$::default_instance();\n");
              }

            printer->Print(vars_, "    default:\n"
                "      GOOGLE_LOG(FATAL) << \"Bad method index; this should never happen.\";\n"
                "      return *reinterpret_cast< ::google::protobuf::Message*>(NULL);\n"
                "  }\n"
                "}\n"
                "\n");
          }

        void ServiceGenerator::GenerateStubMethods(google::protobuf::io::Printer* printer)
          {
            for (int i = 0; i < descriptor_->method_count(); i++)
              {
                const google::protobuf::MethodDescriptor* method = descriptor_->method(i);
                map<string,string> sub_vars;
                sub_vars["classname"] = descriptor_->name();
                sub_vars["name"] = method->name();
                sub_vars["index"] = SimpleItoa(i);
                sub_vars["input_type"] = ClassName(method->input_type(), true);
                sub_vars["output_type"] = ClassName(method->output_type(), true);

                if (isNoResponse(method))
                  {
                    printer->Print(sub_vars, "void $classname$_Stub::$name$(SAFplus::Handle dest,\n"
                        "                              const $input_type$* request) {\n"
                        "  channel_->CallMethod(descriptor()->method($index$), dest, request, NULL, *((SAFplus::Wakeable*)nullptr));\n"
                        "}\n");
                  }
                else
                  {
                    printer->Print(sub_vars, "void $classname$_Stub::$name$(SAFplus::Handle dest,\n"
                        "                              const $input_type$* request,\n"
                        "                              $output_type$* response,\n"
                        "                              SAFplus::Wakeable& wakeable) {\n"
                        "  channel_->CallMethod(descriptor()->method($index$), dest, request, response, wakeable);\n"
                        "}\n");
                  }
              }
          }
      } // namespace Rpc
  } // namespace SAFplus
