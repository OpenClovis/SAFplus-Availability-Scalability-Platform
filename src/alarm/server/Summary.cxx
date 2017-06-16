// /*
//  * Copyright (C) 2002-2014 OpenClovis Solutions Inc.  All Rights Reserved.
//  *
//  * This file is available  under  a  commercial  license  from  the
//  * copyright  holder or the GNU General Public License Version 2.0.
//  *
//  * The source code for  this program is not published  or otherwise
//  * divested of  its trade secrets, irrespective  of  what  has been
//  * deposited with the U.S. Copyright office.
//  *
//  * This program is distributed in the  hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied  warranty  of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//  * General Public License for more details.
//  *
//  * For more  information, see  the file  COPYING provided with this
//  * material.
//  */
#include <sstream>
#include "Summary.hxx"

namespace SAFplus
{
Summary::Summary()
{
	severity = AlarmSeverity::INVALID;
	intTotal = 0;
	intCleared = 0;
	intClearedNotClosed = 0;
	intClearedClosed = 0;
	intNotClearedClosed = 0;
	intNotClearedNotClosed = 0;
}
Summary::Summary(const AlarmSeverity aseverity)
{
	severity = aseverity;
	intTotal = 0;
	intCleared = 0;
	intClearedNotClosed = 0;
	intClearedClosed = 0;
	intNotClearedClosed = 0;
	intNotClearedNotClosed = 0;
}
std::string Summary::toString() const
{
	std::ostringstream oss;
	oss<<"severity:["<<severity<<"] intTotal:["<<intTotal<<"] intCleared:["<<intCleared<<"] intClearedNotClosed:["<<intClearedNotClosed
	   <<"] intClearedClosed:["<<intClearedClosed<<"] intNotClearedClosed:["<<intNotClearedClosed<<"] intNotClearedNotClosed:["<<intNotClearedNotClosed<<"]";
    return oss.str();
}
}
