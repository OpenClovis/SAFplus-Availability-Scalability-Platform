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
#include "AlarmData.hxx"

namespace SAFplus
{
    AlarmData::AlarmData()
    {
    	resourceId = "";
		probCause = AlarmProbableCause::INVALID;
		specificProblem = 0;
		category = AlarmCategory::INVALID;
		severity = AlarmSeverity::INVALID;
		state = AlarmState::INVALID;
		syncData[0] = 0;
    }
    AlarmData::AlarmData(const AlarmData& other)
    {
    	*this = other;
    }
    AlarmData::AlarmData(const string& inresourceId,const AlarmProbableCause& inprobCause, const AlarmSpecificProblem& inspecificProblem,
			const AlarmCategory& incategory, const AlarmSeverity& inseverity, const AlarmState& instate)
    {
    	resourceId = inresourceId;
		probCause = inprobCause;
		specificProblem = inspecificProblem;
		category = incategory;
		severity = inseverity;
		state = instate;
		syncData[0] = 0;
    }
	AlarmData& AlarmData::operator=(const AlarmData& other)
	{
		resourceId = other.resourceId;
		probCause = other.probCause;
		specificProblem = other.specificProblem;
		category = other.category;
		severity = other.severity;
		state = other.state;
		syncData[0] = other.syncData[0];
	}
	bool AlarmData::operator==(const AlarmData& other) const
		{
		return ((resourceId.compare(other.resourceId) == 0) && (probCause == other.probCause)  && (specificProblem == other.specificProblem)
				&& (category == other.category) && (severity == other.severity)&& (state == other.state)
				&&(syncData[0] == other.syncData[0]));
		}
	std::string AlarmData::toString()
	{
		std::ostringstream oss;
		oss <<"resourceId:"<<resourceId<<" category:"<<category<<" probCause:"<<probCause<<" specificProblem:"<<specificProblem
			<<" severity:"<<severity <<" state:"<<state<<" syncData:"<<syncData[0];
		return oss.str();
	}
}
