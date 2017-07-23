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

#ifndef ALARMTIMERINFO_H_HEADER_INCLUDED_A6DEC96B
#define ALARMTIMERINFO_H_HEADER_INCLUDED_A6DEC96B
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <AlarmData.hxx>
#include <Timer.hxx>

using namespace std;
using namespace boost::posix_time;
using namespace SAFplusAlarm;
using namespace SAFplus;
// The AlarmTimerInfo data structure is used to store the entire list of alarm attributes that include probable cause, resource id, category, probable cause,specific problem, severity, and event time along with additional information of the alarm. It also  ascertains whether the alarm is asserted or cleared.
namespace SAFplus
{
class AlarmTimerInfo
{
public:
  AlarmTimerInfo();
  AlarmTimerInfo& operator=(const AlarmTimerInfo& other);
  AlarmTimerInfo(const AlarmTimerInfo& other);
  bool operator==(const AlarmTimerInfo& other) const;
  std::string toString() const;
  ~AlarmTimerInfo();
  //std::shared_ptr<Timer> sharedTimer;
  Timer*sharedTimer;
  //std::shared_ptr<AlarmData> sharedAlarmData;
  AlarmData* sharedAlarmData;
};
}
#endif /* ALARMTIMERINFO_H_HEADER_INCLUDED_A6DEC96B */
