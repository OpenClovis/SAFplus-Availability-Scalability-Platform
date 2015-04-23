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


#include <string>
#include <fstream>

#include "proc.hxx"

ProcFile::ProcFile(const char *fileName)
{
    name = fileName;
}

ProcFile::~ProcFile()
{
}

std::string ProcFile::getFileBuf()
{
    std::ifstream fStr(name.c_str());
    fStr.open(name.c_str());
    
    if(fStr.is_open())
    {
        std::string file;
        fStr.seekg(0, std::ios::end);
        file.resize(fStr.tellg());
        fStr.seekg(0, std::ios::beg);
        fStr.read(&file[0], file.size());
        fStr.close();
        return file;
    }
   
    //printf("Unable to open file\n"); 
    //TODO: print an error message and throw an exceptipon
}
