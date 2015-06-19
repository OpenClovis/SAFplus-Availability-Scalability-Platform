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

#include "clLogApi.hxx"
#include "clProcFileSystem.hxx"

namespace SAFplusI
{

ProcFile::ProcFile(const char *fileName)
{
    name = fileName;
}

ProcFile::~ProcFile()
{
}

std::string ProcFile::loadFileContents()
{

    std::ifstream fStr(name.c_str());

    // A string with the contents of the file pointed by
    // fStr stream will be created here
	std::string content( (std::istreambuf_iterator<char>(fStr)),
                       (std::istreambuf_iterator<char>()    ) );

    if (!content.empty())
    {
	    return content;
    }
    else 
    {
        // Returning an empty string for error case. 
        return std::string();
    }

}

} /* namespace SAFplus */
