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
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <clMsgPortsAndTypes.hxx>
#include <EventClient.hxx>
#include <StorageAlarmData.hxx>
#include <AlarmData.hxx>
#include <AlarmProfile.hxx>
#include <clFaultApi.hxx>
#include <AlarmUtils.hxx>
#include <rpcAlarm.hxx>

using namespace SAFplusAlarm;
namespace SAFplus
{
// Since the AMF (safplus_amf process) includes a alarm server instance, this alarm server class is only used by applications in a no AMF deployment
// Alarm Server control 2 roles: Alarm Server(operator,administrator,manage alarm application,fault manager) and Alarm Client
// Alarm server check handmasking,send to alarm application,fault management
// Alarm client such as clearing,soaking,check generate rule,suppression rule,assert state

class AlarmServer:public SAFplus::MsgHandler,public SAFplus::Wakeable,public SAFplus::Rpc::rpcAlarm::rpcAlarm
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
    //virtual void msgHandler(SAFplus::Handle from, SAFplus::MsgServer* psrv, ClPtrT pmsg, ClWordT msglen, ClPtrT pcookie);

    // send alarm to application management
    // Return: ClRcT
    static ClRcT publishAlarm(const AlarmData& alarmData);
    // destructor
    virtual ~AlarmServer();
    // purge rpc for alarm
    void purgeRpcRequest(const AlarmFilter& inputFilter);
    // delete alarms on data storage
    void deleteRpcRequest(const AlarmFilter& inputFilter);
    // Proccess alarm
    static bool alarmConditionCheck(const AlarmInfo& alarmInfo);
    static void processAffectedAlarm(const AlarmKey& key,const bool& isSet = true);
    static void processPublishAlarmData(const AlarmData& alarmData);
    static void processSummaryAlarmData(const AlarmData& alarmData,const bool& isSeverityChanged = true);
    static ClRcT processAlarmDataCallBack(void* apAlarmData);
    void processAlarmData(const AlarmData& alarmData);
     //void processData();
    static bool checkHandMasking(const AlarmInfo& alarmInfo);
     //static void createAlarmProfileData(const AlarmProfileData& alarmProfileData);
     void wake(int amt,void* cookie);
     //static void deleteAlarmProfileData(const AlarmProfileData& alarmProfileData);
     static StorageAlarmData data;
     static SAFplus::EventClient eventClient;
     //object fault client
     static SAFplus::Fault faultClient;
     static boost::mutex g_data_mutex;
     SAFplus::Rpc::RpcChannel *channel;
     void alarmCreateRpcMethod(const ::SAFplus::Rpc::rpcAlarm::alarmProfileCreateRequest* request,
                           ::SAFplus::Rpc::rpcAlarm::alarmResponse* response);
     void alarmDeleteRpcMethod(const ::SAFplus::Rpc::rpcAlarm::alarmProfileDeleteRequest* request,
                         ::SAFplus::Rpc::rpcAlarm::alarmResponse* response);
     void alarmRaiseRpcMethod(const ::SAFplus::Rpc::rpcAlarm::alarmDataRequest* request,
                         ::SAFplus::Rpc::rpcAlarm::alarmResponse* response);
  private:
     bool checkHandMasking(const std::vector<std::string>& paths);
     void loadAlarmProfile();
  protected:
    SAFplus::Handle handleAlarmServer;
    // alarm message server
    SAFplus::SafplusMsgServer* alarmMsgServer;
    // Alarm client
    // object event client
    SAFplus::Group group;
    SAFplus::Handle activeServer;
};
}


#endif /* ALARMSERVER_H_HEADER_INCLUDED_A6DEFBE5 */
