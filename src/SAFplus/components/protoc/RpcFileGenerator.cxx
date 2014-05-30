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

#include "clRpcFileGenerator.hxx"
#include <iostream>

namespace SAFplus
{

  RpcFileGenerator::RpcFileGenerator(const google::protobuf::FileDescriptor* file, const std::string& output_name)
  {
    //TODO: handle gen RPC client/server stub
    for (int i = 0; i < file->service_count(); i++) {
      std::cout<<file->service(i)->name()<<std::endl;
    }

  }

  RpcFileGenerator::~RpcFileGenerator()
  {
    // TODO Auto-generated destructor stub
  }

} /* namespace SAFplus */
