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
#include <EventClient.hxx>
#include <clMsgApi.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clGlobals.hxx>
#include <MgtMsg.pb.hxx>
#include <AlarmData.hxx>
#include <AlarmProfile.hxx>
#include <AlarmUtils.hxx>
#include <clRpcChannel.hxx>
#include <rpcAlarm.hxx>
#include <EventSharedMem.hxx>

using namespace boost::asio;
using namespace SAFplusAlarm;
extern AlarmComponentResAlarms appAlarms[];
extern std::string myresourceId;
extern void eventCallback(const std::string& channelId,const EventChannelScope& scope,const std::string& data,const int& length);
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
    // Return: none
    void initialize(const SAFplus::Handle& handleClient,SAFplus::Wakeable& wake = SAFplus::BLOCK);
    // raise Alarm
    // Param:
    // AlarmData alarmData: data
    // return: ClRcT
    ClRcT raiseAlarm(const AlarmData& alarmData);
    // raise Alarm
    // Param:
    // resourceId: resource name
    // category: type
    // probCause: cause id
    // severity: severity id
    // specificProblem: specific code
    // state: ASSERT/CLEAR
    // return: ClRcT
    ClRcT raiseAlarm(const std::string& resourceId,const AlarmCategory& category, const AlarmProbableCause& probCause,
        const AlarmSeverity& severity, const AlarmSpecificProblem& specificProblem,const AlarmState& state);
    // This function is used to subscribe for the events published by the alarm server.
    // Param: none
    // return: ClRcT
    ClRcT subscriber();
    // This function is used to unsubscribe for the event published by the alarm server.
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

  private:
    // handle for identify a alarm entity
    SAFplus::Handle handleClientAlarm;
    // safplus message for send alarm notification to alarm server
    SAFplus::SafplusMsgServer* alarmMsgServer;
    // Wakeable object for change notification
    SAFplus::Wakeable* wakeable;
    // handle alarm server
    SAFplus::Handle handleAlarmServer;
    //event client
    SAFplus::EventClient eventClient;
    //channel
    SAFplus::Rpc::RpcChannel * channel;
    //service
    SAFplus::Rpc::rpcAlarm::rpcAlarm_Stub *service;
    //shared memory
    EventSharedMem alarmSharedMemory;
    //active server address
    SAFplus::Handle activeServer;
};
}

#endif /* ALARM_H_HEADER_INCLUDED_A6DE837D */