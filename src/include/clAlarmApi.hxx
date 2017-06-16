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

#ifndef ALARMCLIENT_H_HEADER_INCLUDED_A6DE837D
#define ALARMCLIENT_H_HEADER_INCLUDED_A6DE837D
#include <functional>                  //std::equal_to
#include <ctime>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/unordered_map.hpp>

#include <clMsgApi.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <MgtMsg.pb.hxx>
#include <AlarmData.hxx>
#include <AlarmProfile.hxx>
#include <AlarmProfileInfo.hxx>
#include <AlarmUtils.hxx>
//#include <EventClient.hxx>
using namespace boost::asio;
using namespace SAFplusAlarm;

namespace SAFplus
{
// This class contains Alarm Client related APIs



class Alarm
{
  public:
    // constructor
    Alarm();

    // Initialize a alarm entity with handle and comport information
    // Param:
    // clientHandle: input handle of client Alarm
    // Return: ClRcT
    void init(const SAFplus::Handle& clientHandle, SAFplus::Wakeable& wake = SAFplus::BLOCK);

    // constructor
    // Param:
    // clientHandle: input client alarm handle  
    // serverHandle: input: server alarm handle
    Alarm(const SAFplus::Handle& clientHandle, const SAFplus::Handle& serverHandle);

    // raise Alarm
    // Param:
    // std:string resourceId: resource id
    // AlarmCategoryId category: category
    // AlarmSeverity severity: severity alarm
    // std::string alarmText: text of alarm
    // return: ClRcT
    ClRcT raiseAlarm(const AlarmData& alarmData);

    // This function is used to subscribe for the events published by the alarm server. This function takes the function pointer as parameter which would be called whenever the event is published by the alarm server on the event channel CL_ALARM_EVENT_CHANNEL. After receiving the event the callback function would be called after filling the alarm information in the AlarmHandleInfo structure.The
    // event subscription done via this function can be unsubscribed using the UnSubscribe().
    // param:
    //   input: pointer to callback function
    //   return: ClRcT
    ClRcT subscribe(EventCallback* pEvtCallback);

    //  This function is used to unsubscribe for the event published by the alarm server on the channel CL_ALARM_EVENT_CHANNEL. After this function is called, the callback registered by the function Subscribe will not be called any further on the event publish. This function will finalize the event handle and will close the event channel opened by calling Subscribe().
    // return: ClRcT
    ClRcT unSubscriber();

    // createAlarmProfile
    // Return: ClRcT
    ClRcT createAlarmProfile();
    // deleteAlarmProfile
    // Return: ClRcT
    ClRcT deleteAlarmProfile();
    // destructor
    ~Alarm();
    // return ClRcT result
    ClRcT raiseAlarm(const std::string& resourceId,const AlarmCategory& category, const AlarmProbableCause& probCause,
        const AlarmSeverity& severity, const AlarmSpecificProblem& specificProblem,const AlarmState& state);
  private:
    ClRcT sendAlarmNotification(const void* data, const int& dataLength);
    // handle for identify a alarm entity
    SAFplus::Handle handleClientAlarm;
    // safplus message for send alarm notification to alarm server
    SAFplus::SafplusMsgServer* alarmMsgServer;
    // Wakeable object for change notification
    SAFplus::Wakeable* wakeable;
    // handle alarm server
    SAFplus::Handle handleAlarmServer;
    //event client
    //SAFplus::Event eventClient;
};
}

#endif /* ALARM_H_HEADER_INCLUDED_A6DE837D */
