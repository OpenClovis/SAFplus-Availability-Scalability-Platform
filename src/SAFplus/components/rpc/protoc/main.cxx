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

#include <google/protobuf/compiler/command_line_interface.h>
#include "RpcGenerator.hxx"

using namespace std;

int main(int argc, char *argv[])
  {
    google::protobuf::compiler::CommandLineInterface cli;
    string path;
    for (int i = 0; i < argc; i++)
      {
        if (!strncmp(argv[i], "--rpc_out", 9))
          {
            char* dir;
            dir = strrchr(argv[i], '=');
            if (dir)
              path.assign(dir + 1);
            break;
          }
      }
    SAFplus::Rpc::RpcGenerator rpc_generator(path);
    cli.RegisterGenerator("--rpc_out", &rpc_generator, "Generate C++ Service file.");
    return cli.Run(argc, argv);
  }
