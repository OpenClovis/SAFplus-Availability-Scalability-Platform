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
#include <EventClient.hxx>
#include <StorageAlarmData.hxx>
#include <AlarmData.hxx>
#include <AlarmProfile.hxx>
#include <AlarmMessageProtocol.hxx>
#include <clFaultApi.hxx>
#include <AlarmUtils.hxx>

using namespace SAFplusAlarm;
namespace SAFplus
{
// Since the AMF (safplus_amf process) includes a alarm server instance, this alarm server class is only used by applications in a no AMF deployment
// Alarm Server control 2 roles: Alarm Server(operator,administrator,manage alarm application,fault manager) and Alarm Client
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
    // Proccess alarm
     static void processAssertAlarmData(const AlarmData& alarmData);
     static void processClearAlarmData(const AlarmData& alarmData);
     static bool alarmConditionCheck(const AlarmInfo& alarmInfo);
     static void processAffectedAlarm(const AlarmKey& key,const bool& isSet = true);
     static void processPublishAlarmData(const AlarmData& alarmData);
     static ClRcT processAlarmDataCallBack(void* apAlarmData);
     static void processAlarmData(const AlarmData& alarmData);
     static void processData();
     static bool checkHandMasking(const AlarmInfo& alarmInfo);
     static void createAlarmProfileData(const AlarmProfileData& alarmProfileData);
     void wake(int amt,void* cookie);
     static void deleteAlarmProfileData(const AlarmProfileData& alarmProfileData);
     static StorageAlarmData data;
     static boost::atomic<bool> isDataAvailable;
     static boost::lockfree::spsc_queue<AlarmMessageProtocol, boost::lockfree::capacity<1024> > spsc_queue;
     static boost::thread threadProcess;
     static SAFplus::EventClient eventClient;
     static boost::mutex g_queue_mutex;
  private:
     static bool checkHandMasking(const std::vector<std::string>& paths);
     void loadAlarmProfile();
  protected:
    SAFplus::Handle handleAlarmServer;
    // alarm message server
    SAFplus::SafplusMsgServer* alarmMsgServer;
    // Alarm client
    //object fault client
    SAFplus::Fault faultClient;
    // object event client
    SAFplus::Group group;
    SAFplus::Handle activeServer;
};
}


#endif /* ALARMSERVER_H_HEADER_INCLUDED_A6DEFBE5 */
