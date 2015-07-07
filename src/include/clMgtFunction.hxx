/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
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
 *
 */

#ifndef CLMGTFUNCTION_HXX_
#define CLMGTFUNCTION_HXX_

#include <string>
#include <clHandleApi.hxx>
#include <clCkptApi.hxx> // to use checkpoint
namespace SAFplus
{
     extern Checkpoint* mgtCheckpoint;
     Checkpoint* getMgtCheckpoint();
     SAFplus::Handle& getMgtHandle(const std::string& pathSpec, ClRcT &errCode);

     std::string mgtGet(const std::string& pathSpec);
     std::string mgtGet(SAFplus::Handle src, const std::string& pathSpec);

     ClRcT mgtSet(SAFplus::Handle src, const std::string& pathSpec, const std::string& value);
     ClRcT mgtSet(const std::string& pathSpec, const std::string& value);

     ClRcT mgtCreate(SAFplus::Handle src, const std::string& pathSpec);
     ClRcT mgtCreate(const std::string& pathSpec);

     ClRcT mgtDelete(SAFplus::Handle src, const std::string& pathSpec);
     ClRcT mgtDelete(const std::string& pathSpec);

}
#endif /* CLMGT_HXX_ */
