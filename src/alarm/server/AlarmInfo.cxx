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

#include "AlarmInfo.hxx"

using namespace SAFplusAlarm;

namespace SAFplus
{
AlarmInfo::AlarmInfo()
{
	severity = AlarmSeverity::INVALID;
	strText = "";
	strOperator = "";
	strOperatorText = "";
}

AlarmInfo& AlarmInfo::operator=(const AlarmInfo& other)
{
	statusChangeTime = other.statusChangeTime;
	severity = other.severity;
	vectAltResource = other.vectAltResource;
	strText = other.strText;
	strOperator = other.strOperator;
	strOperatorText = other.strOperatorText;
	operatorActionTime = other.operatorActionTime;
}
bool AlarmInfo::operator==(const AlarmInfo& other) const
{
	return (
	(statusChangeTime == other.statusChangeTime) &&
	(severity == other.severity) &&
	(vectAltResource == other.vectAltResource) &&
	(strText == other.strText) &&
	(strOperator == other.strOperator) &&
	(strOperatorText == other.strOperatorText) &&
	(operatorActionTime == other.operatorActionTime));
}
std::string AlarmInfo::toString() const
{
	std::ostringstream oss;
	oss <<" statusChangeTime:"<<statusChangeTime<<" severity:"<<severity;
	if(vectAltResource.size() > 0)
	{	oss << " altResource:[";
		int pos = 0;
		for(const std::string& altName:vectAltResource)
		{
			if(pos > 0 ) oss <<",";
			oss<<altName;
			pos++;
		}
		oss<<"]";
	}
	oss <<" Text:"<<strText<<" lastOperator:"<<strOperator<<" operatorText:"<<strOperatorText
		<<" operatorActionTime:"<<operatorActionTime;
	return oss.str();
}
}
