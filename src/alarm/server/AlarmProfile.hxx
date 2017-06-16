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

#ifndef ALARMPROFILE_H_HEADER_INCLUDED_A6DE96AF
#define ALARMPROFILE_H_HEADER_INCLUDED_A6DE96AF
//#include <SAFplusAlarmCommon.hxx>
#include <AlarmRuleInfo.hxx>
#include <AlarmCategory.hxx>
#include <AlarmSeverity.hxx>

using namespace SAFplusAlarm;
using namespace SAFplus;

namespace SAFplus
{
// This structure is used to store the alarm related attributes including the category, sub-category, severity,soaking time for assert and clear, probable cause of alarm,and the generation and suppression rule for alarm.

class AlarmProfile
{
  public:
	AlarmProfile();
	AlarmProfile(const AlarmCategory& acategory,const AlarmProbableCause& aprobCause,const AlarmSeverity& aseverity,const bool& aisSend,const int& aassertSoakingTime,
			const int& aclearSoakingTime,AlarmRuleInfo* agenerationRule,AlarmRuleInfo* asuppressionRule,const AlarmSpecificProblem aspecificProblem);
    AlarmProfile operator=(const AlarmProfile& other);
	// alarm category
    AlarmCategory category;
    //severity
    AlarmSeverity severity;
    // alarm probable cause
    AlarmProbableCause probCause;
    //send to fault manager or not
    bool isSend;
    // period to wait for assert soaking time
    int intAssertSoakingTime;
    // period to wait for clear soaking time
    int intClearSoakingTime;
    // rule for generate
    AlarmRuleInfo *generationRule;
    // rule for suppression
    AlarmRuleInfo *suppressionRule;
    // specific problem
    AlarmSpecificProblem specificProblem;
};

}

#endif /* ALARMPROFILE_H_HEADER_INCLUDED_A6DE96AF */
