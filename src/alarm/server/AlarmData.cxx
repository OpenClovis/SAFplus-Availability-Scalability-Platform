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
#include <cstring>
#include "AlarmData.hxx"
using namespace std;

namespace SAFplus
{
AlarmData::AlarmData()
{
  resourceId[0] = 0;
  category = AlarmCategory::INVALID;
  probCause = AlarmProbableCause::INVALID;
  specificProblem = 0;
  severity = AlarmSeverity::INVALID;
  state = AlarmState::INVALID;
  syncData[0] = 0;
}
AlarmData::AlarmData(const AlarmData& other)
{

  memset(resourceId, 0, sizeof(resourceId));
  strcpy(resourceId, other.resourceId);
  category = other.category;
  probCause = other.probCause;
  specificProblem = other.specificProblem;
  severity = other.severity;
  state = other.state;
  syncData[0] = other.syncData[0];
}
AlarmData::AlarmData(const AlarmInfo& info)
{
  memset(resourceId, 0, sizeof(resourceId));
  strcpy(resourceId, info.resourceId);
  category = info.category;
  probCause = info.probCause;
  specificProblem = info.specificProblem;
  severity = info.severity;
  state = info.currentState;
  syncData[0] = ' ';
}
AlarmData::AlarmData(const char* inresourceId, const AlarmCategory& incategory, const AlarmProbableCause& inprobCause, const AlarmSpecificProblem& inspecificProblem, const AlarmSeverity& inseverity, const AlarmState& instate)
{
  memset(resourceId, 0, sizeof(resourceId));
  strcpy(resourceId, inresourceId);
  category = incategory;
  probCause = inprobCause;
  specificProblem = inspecificProblem;
  severity = inseverity;
  state = instate;
  syncData[0] = 0;
}
AlarmData& AlarmData::operator=(const AlarmData& other)
{
  memset(resourceId, 0, sizeof(resourceId));
  strcpy(resourceId, other.resourceId);
  category = other.category;
  probCause = other.probCause;
  specificProblem = other.specificProblem;
  severity = other.severity;
  state = other.state;
  syncData[0] = other.syncData[0];
}
bool AlarmData::operator==(const AlarmData& other) const
{
  return ((strcmp(resourceId, other.resourceId) == 0) && (category == other.category) && (probCause == other.probCause) && (specificProblem == other.specificProblem) && (severity == other.severity) && (state == other.state) && (syncData[0] == other.syncData[0]));
}
std::string AlarmData::toString() const
{
  std::ostringstream oss;
  oss << "resourceId:[" << resourceId << "] category:[" << category << "] probCause:[" << probCause << "] specificProblem:[" << specificProblem << "] severity:" << severity << "] state:[" << state << "] syncData:[" << syncData[0] << "]";
  return oss.str();
}
std::ostream& operator<<(std::ostream& os, const AlarmData& e)
{
  return os << e.resourceId << " " << e.category << " " << e.probCause << " " << e.specificProblem << " " << e.severity << " " << e.state << " " << e.syncData;
}
std::istream& operator>>(std::istream& is, AlarmData& e)
{
  return is >> e.resourceId >> e.category >> e.probCause >> e.specificProblem >> e.severity >> e.state >> e.syncData;
}
}