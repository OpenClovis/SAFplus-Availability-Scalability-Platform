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

#ifndef CLRPCSERVICE_HXX_
#define CLRPCSERVICE_HXX_

#include <map>
#include <string>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include "RpcService.hxx"

namespace SAFplus
  {
    namespace Rpc
      {
        class ServiceGenerator
          {
          public:
            // See generator.cc for the meaning of dllexport_decl.
            explicit ServiceGenerator(const google::protobuf::ServiceDescriptor* descriptor);
            ~ServiceGenerator();

            // Header stuff.
            // Generate the class definitions for the service's interface
            void GenerateDeclarations(google::protobuf::io::Printer* printer);

            // Generate implementations of everything declared by GenerateDeclarations().
            void GenerateImplementation(google::protobuf::io::Printer* printer);

          private:
            enum RequestOrResponse
              {
              REQUEST, RESPONSE
              };
            enum VirtualOrNon
              {
              VIRTUAL, NON_VIRTUAL
              };

            // Header stuff.

            // Prints signatures for all methods in the
            void GenerateMethodSignatures(VirtualOrNon virtual_or_non, google::protobuf::io::Printer* printer);

            const google::protobuf::ServiceDescriptor* descriptor_;
            std::map<std::string,std::string> vars_;

            GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ServiceGenerator);
          };
      } // namespace SAFplus

  } // namespace SAFplus

#endif  // CLRPCSERVICE_HXX_
