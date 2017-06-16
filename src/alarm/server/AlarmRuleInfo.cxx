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
#include <clCustomization.hxx>
#include "AlarmRuleInfo.hxx"

using namespace SAFplus;

AlarmRuleInfo::AlarmRuleInfo()
{
	relation = AlarmRuleRelation::INVALID;
}
AlarmRuleInfo::AlarmRuleInfo(const AlarmRuleRelation& arelation,const AlarmRuleEntry* arrAlarmRuleEntry)
{
	relation = arelation;
	vectAlarmRelation.clear();
	if(NULL != arrAlarmRuleEntry)
	{
		for(int i = 0;i < SAFplusI::MAX_ALARM_RULE_DEPENDENCIES;i++)
		{
			if(AlarmProbableCause::INVALID != arrAlarmRuleEntry[i].probCause)
			{
				vectAlarmRelation.push_back(arrAlarmRuleEntry[i]);
			}
		}
	}
}
AlarmRuleInfo::AlarmRuleInfo(const AlarmRuleRelation& arelation,std::vector<AlarmRuleEntry> avectAlarmRelation)
{
	relation = arelation;
	vectAlarmRelation = avectAlarmRelation;
}
AlarmRuleInfo AlarmRuleInfo::operator=(const AlarmRuleInfo& other)
{
	relation = other.relation;
	vectAlarmRelation = other.vectAlarmRelation;
}
