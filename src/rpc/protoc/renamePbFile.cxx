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
#include <iostream>
#include <stdio.h>
#include "RenamePbFile.hxx"

namespace SAFplus
  {
    namespace Rpc
      {

        RenamePbFile::RenamePbFile(const std::string &dir, const std::string &basename)
          {
            // TODO Auto-generated constructor stub
            if (1)
              {
                //Header
                std::string headerFile = dir + "/" + basename + ".pb.h";
                std::string headerFileNew = dir + "/" + basename + ".pb.hxx";

                //Impl
                std::string implFile = dir + "/" + basename + ".pb.cc";
                std::string implFileNew = dir + "/" + basename + ".pb.cxx";

                /* Search and replace pb.h => pb.hxx without google/protobuf */
                Replace(headerFile, headerFileNew);
                Replace(implFile, implFileNew);
              }
          }

        RenamePbFile::~RenamePbFile()
          {
            // TODO Auto-generated destructor stub
          }

        void RenamePbFile::Replace(const std::string &infile, const std::string &outfile)
          {
            std::ifstream ifs(infile, std::ios::binary);
            if (ifs)
              {
                std::ofstream ofs(outfile, std::ios::binary);
                if (ofs)
                  {
                    std::string buf;
                    while (getline(ifs, buf))
                      {
                        size_t foundpos = buf.find(".pb.h\"");
                        if ((foundpos != std::string::npos) && (buf.find("#include") != std::string::npos)
                            && (buf.find("google/protobuf") == std::string::npos))
                          {
                            std::string tmpstring = buf.substr(0, foundpos);
                            tmpstring += ".pb.hxx\"";
                            buf = tmpstring;
                          }
                        ofs<<buf;
                        ofs<<std::endl;
                      }
                    ofs.close();
                  }
                ifs.close();
                //Remove old file
                remove(infile.c_str());
              }
          }
      } /* namespace Rpc */
  } /* namespace SAFplus */
