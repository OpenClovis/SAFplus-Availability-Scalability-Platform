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

#include "AlarmProfile.hxx"
using namespace SAFplusAlarm;

namespace SAFplus
{
AlarmProfile::AlarmProfile()
	{
    	    category = AlarmCategory::INVALID;
    	    probCause = AlarmProbableCause::INVALID;
    	    severity = AlarmSeverity::INVALID;
    	    isSend = false;
    	    intAssertSoakingTime = 0;
    	    intClearSoakingTime = 0;
    	    generationRule = nullptr;
    	    suppressionRule = nullptr;
    	    specificProblem = 0;
	}
AlarmProfile::AlarmProfile(const AlarmCategory& acategory,const AlarmProbableCause& aprobCause,const AlarmSeverity& aseverity,const bool& aisSend,const int& aassertSoakingTime,
			const int& aclearSoakingTime,AlarmRuleInfo* agenerationRule,AlarmRuleInfo* asuppressionRule,const AlarmSpecificProblem aspecificProblem)
	{
		category = acategory;
		probCause = aprobCause;
		severity = aseverity;
		isSend = aisSend;
		intAssertSoakingTime = aassertSoakingTime;
		intClearSoakingTime = aclearSoakingTime;
		generationRule = agenerationRule;
		suppressionRule = asuppressionRule;
		specificProblem = aspecificProblem;
	}
	AlarmProfile AlarmProfile::operator=(const AlarmProfile& other)
	{
		category = other.category;
		probCause = other.probCause;
		severity = other.severity;
		isSend = other.isSend;
		intAssertSoakingTime = other.intAssertSoakingTime;
		intClearSoakingTime = other.intClearSoakingTime;
		generationRule = other.generationRule;
		suppressionRule = other.suppressionRule;
		specificProblem = other.specificProblem;
		return *this;
	}

}
