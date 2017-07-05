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

#ifndef ALARMDATA_HXX_HEADER_INCLUDED_A6DE8F01
#define ALARMDATA_HXX_HEADER_INCLUDED_A6DE8F01
#include <string>
#include <clCustomization.hxx>
#include <AlarmCategory.hxx>
#include <AlarmProbableCause.hxx>
#include <AlarmSeverity.hxx>
#include <AlarmMessageType.hxx>
#include <AlarmState.hxx>

using namespace std;
using namespace SAFplusAlarm;

namespace SAFplus
{

// this class to contain data send between AlarmClient and Alarm Server, AlarmServer and Application Management
class AlarmData
{
public:
  AlarmData();
  AlarmData(const AlarmData& other);
  AlarmData(const char* inresourceId, const AlarmProbableCause& inprobCause, const AlarmSpecificProblem& inspecificProblem, const AlarmCategory& incategory, const AlarmSeverity& inseverity, const AlarmState& instate);
  AlarmData& operator=(const AlarmData& other);
  bool operator==(const AlarmData& other) const;

  std::string toString() const;
  // resource name id
  char resourceId[SAFplusI::MAX_RESOURCE_NAME_SIZE];
  // alarm cagegory type
  AlarmCategory category;
  // alarm probalbe  cause
  AlarmProbableCause probCause;
  // specific problem
  AlarmSpecificProblem specificProblem;
  // severity
  AlarmSeverity severity;
  // alarm status
  AlarmState state;
  //data payload
  char syncData[1];
};
std::ostream& operator<<(std::ostream& os, const AlarmData& e);
std::istream& operator>>(std::istream& is, AlarmData& e);
}

#endif /* ALARMDATA_HXX_HEADER_INCLUDED_A6DE8F01 */
