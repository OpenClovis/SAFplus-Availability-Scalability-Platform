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

#ifndef ALARMSERVER_H_HEADER_INCLUDED_A6DEFBE5
#define ALARMSERVER_H_HEADER_INCLUDED_A6DEFBE5

#include <ctime>
#include <clMsgApi.hxx>
#include <clGlobals.hxx>
#include <clLogApi.hxx>
#include <clMsgApi.hxx>
#include <StorageAlarmData.hxx>
#include <AlarmData.hxx>
#include <AlarmProfile.hxx>
#include <clAlarmApi.hxx>
#include <clFaultApi.hxx>
#include <AlarmUtils.hxx>
//#include <Event.hxx>
using namespace SAFplusAlarm;
namespace SAFplus
{
// Since the AMF (safplus_amf process) includes a alarm server instance, this alarm server class is only used by applications in a no AMF deployment
//Alarm Server control 2 roles: Alarm Server(operator,administrator,manage alarm application,fault manager) and Alarm Client
// Alarm server check handmasking,send to alarm application,fault management
// Alarm client such as clearing,soaking,check generate rule,suppression rule,assert state

class AlarmServer:public SAFplus::MsgHandler,public SAFplus::Wakeable
{
  public:
    // default constructor
    AlarmServer();

    // init server
    void initialize();

    // handle all message send to/out server
    // Param:
    // from: input from address
    // psrv: input message service
    // pmsg: input message body
    // msglen: input length of message
    // pcookie: input cookie
    // Return: ClRcT
    virtual void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* psrv, ClPtrT pmsg, ClWordT msglen, ClPtrT pcookie);

    // send alarm to application management
    // Return: ClRcT
    static ClRcT publishAlarm(const AlarmData& alarmData);
    // destructor
    ~AlarmServer();
    // purge rpc for alarm
    static void purgeRpcRequest(const AlarmFilter& inputFilter);


    // delete alarms on data storage
    static void deleteRpcRequest(const AlarmFilter& inputFilter);
    // Query alarm data
    // Param:
    // resourceid: input alarm object id
    // listAlarmInfo: output list of AlarmInfo
    // Return: ClRcT
    //ClRcT queryAlarmInfo(const std::string& resourceid,std::list<AlarmInfo>& listAlarmInfo);
    // Proccess alarm
     static void processAssertAlarmData(const AlarmData& alarmData);
     static void processClearAlarmData(const AlarmData& alarmData);
     static bool alarmConditionCheck(const AlarmData& alarmData);
     static void processAffectedAlarmData(const AlarmData& alarmData);
     static ClRcT processAlarmDataCallBack(void* apAlarmData);
     static void processAlarmData(const AlarmData& alarmData);
     static bool checkHandMasking(const AlarmData& alarmData);
     void createAlarmProfile(const std::string& resourceId, const MAPALARMPROFILEINFO& mapAlarmProfileInfo);
     void wake(int amt,void* cookie);
     void deleteAlarmProfile(const std::string& resourceId, const MAPALARMPROFILEINFO& mapAlarmProfileInfo);
     static StorageAlarmData data;
  private:
     static bool checkHandMasking(const std::vector<std::string>& paths);
     void loadAlarmProfile();
  protected:
    //alarm server data
    // alarm server reporter
    SAFplus::Handle handleAlarmServer;
    // alarm message server
    SAFplus::SafplusMsgServer* alarmMsgServer;
    // Alarm client
    SAFplus::Alarm alarmClient;
    //object fault client
    SAFplus::Fault faultClient;
    // object event client
    SAFplus::Group group;
    //SAFplus::EventClient eventClient;
    SAFplus::Handle activeServer;
    //alarm client data
    //map to store soaking clearing time

    //map to store soaking clearing time timer
};
}


#endif /* ALARMSERVER_H_HEADER_INCLUDED_A6DEFBE5 */
