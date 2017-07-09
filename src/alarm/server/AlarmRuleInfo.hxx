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

#ifndef ALARMRULEINFO_HXX_HEADER_INCLUDED_A6DE837C
#define ALARMRULEINFO_HXX_HEADER_INCLUDED_A6DE837C
#include <vector>
#include <SAFplusAlarmCommon.hxx>
#include <AlarmRuleRelation.hxx>
#include <AlarmProbableCause.hxx>

using namespace SAFplusAlarm;
using namespace SAFplus;

namespace SAFplus
{
struct AlarmRuleEntry
{
  AlarmProbableCause probCause;
  AlarmSpecificProblem specificProblem;
  AlarmRuleEntry(const AlarmProbableCause aprobCause, const AlarmSpecificProblem& aspecificProblem)
  {
    probCause = aprobCause;
    specificProblem = aspecificProblem;
  }
};
// contain relation between rule and list of rule
class AlarmRuleInfo
{
public:
  AlarmRuleInfo();
  AlarmRuleInfo(const AlarmRuleRelation& arelation, const AlarmRuleEntry* arrAlarmRuleEntry);
  AlarmRuleInfo(const AlarmRuleRelation& arelation, std::vector<AlarmRuleEntry> vectAlarmRelation);
  AlarmRuleInfo operator=(const AlarmRuleInfo& other);
  // rule relation
  AlarmRuleRelation relation;
  // list of alarm key
  std::vector<AlarmRuleEntry> vectAlarmRelation;
};
}

#endif /* ALARMRULEINFO_HXX_HEADER_INCLUDED_A6DE837C */
