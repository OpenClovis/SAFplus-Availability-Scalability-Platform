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

#ifndef ALARMINFO_H_HEADER_INCLUDED_A6DEC96B
#define ALARMINFO_H_HEADER_INCLUDED_A6DEC96B
#include <vector>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <SAFplusAlarmCommon.hxx>
#include <clCustomization.hxx>
#include <AlarmCategory.hxx>
#include <AlarmProbableCause.hxx>
#include <AlarmSeverity.hxx>
#include <AlarmMessageType.hxx>
#include <AlarmState.hxx>
#include <TimerType.hxx>


using namespace std;
using namespace boost::posix_time;
using namespace SAFplusAlarm;
using namespace SAFplus;
// The AlarmInfo data structure is used to store the entire list of alarm attributes that include probable cause, resource id, category, probable cause,specific problem, severity, and event time along with additional information of the alarm. It also  ascertains whether the alarm is asserted or cleared.
namespace SAFplus
{
class AlarmInfo
{
public:
  AlarmInfo();
  AlarmInfo& operator=(const AlarmInfo& other);
  bool operator==(const AlarmInfo& other) const;
  std::string toString() const;
  char resourceId[SAFplusI::MAX_RESOURCE_NAME_SIZE];
  //alarm cagegory type
  AlarmCategory category;
  //alarm probalbe  cause
  AlarmProbableCause probCause;
  //vector of other name of this resource
  std::vector<std::string> vectAltResource;
  //last time changed
  boost::posix_time::ptime statusChangeTime;
  //severity of alarm
  AlarmSeverity severity;
  //alarm data
  char syncData[1];
  //Specific-Problem of the alarm. This field adds further refinement
  //to the probable cause specified while raising the alarm.
  AlarmSpecificProblem specificProblem;
  //current state, include soaking time,clear soaking time
  AlarmState currentState;
  AlarmState state;
  TimerType timerType;
  boost::posix_time::ptime startTimer;
  //after soaking bitmap
  BITMAP64 afterSoakingBitmap;
};
}
#endif /* ALARMINFO_H_HEADER_INCLUDED_A6DEC96B */
