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
#include <EventSharedMem.hxx>

using namespace SAFplusAlarm;
namespace SAFplus
{
// Since the AMF (safplus_amf process) includes a alarm server instance, this alarm server class is only used by applications in a no AMF deployment
// Alarm Server control 2 roles: Alarm Server(operator,administrator,manage alarm application,fault manager) and Alarm Client
// Alarm server check clearing,soaking,check generate rule,suppression rule,handmasking,send to alarm application,fault management

class AlarmServer:public SAFplus::MsgHandler,public SAFplus::Wakeable,public SAFplus::Rpc::rpcAlarm::rpcAlarm
{
  public:
    // default constructor
    AlarmServer();
    // init server
    void initialize();
    // send alarm to application management
    // Param:
    // alarmData: input alarm data send to application
    // Return: ClRcT
    static ClRcT publishAlarm(const AlarmData& alarmData);
    // destructor
    virtual ~AlarmServer();
    // check condition of alarm
    // Param:
    // alarmInfo: input alarm information
    // Return: true/false
    static bool alarmConditionCheck(const AlarmInfo& alarmInfo);
    // process affected of this alarm
    // Param:
    // key: check for this key
    // isSet: true: after soaking,false: under soaking
    // Return: none
    static void processAffectedAlarm(const AlarmKey& key,const bool& isSet = true);
    // process publish for this alarm
    // Param:
    // alarmData: input alarm data to process
    // Return: none
    static void processPublishAlarmData(const AlarmData& alarmData);
    // process alarm data after assert/clear soaking time
    // Param:
    // apAlarmData: input alarm data to process
    // Return: ClRcT
    static ClRcT processAlarmDataCallBack(void* apAlarmData);
    // process alarm data from client
    // Param:
    // apAlarmData: input alarm data to process
    // Return: none
    void processAlarmData(const AlarmData& alarmData);
    // process handmasking to check does the parent resource was already assert or not
    // Param:
    // alarmInfo: input alarm information to process
    // Return: true/false
    static bool checkHandMasking(const AlarmInfo& alarmInfo);
    // this function is call when the active address changed
    // Param:
    // pgroup: input group
    // Return: none
    void wake(int amt,void* pgroup);
    // create profile for alarm
    // Param:
    // alarmProfileCreateRequest: input request profile
    // alarmResponse: input response result
    // Return: none
    void alarmCreateRpcMethod(const ::SAFplus::Rpc::rpcAlarm::alarmProfileCreateRequest* request,
                         ::SAFplus::Rpc::rpcAlarm::alarmResponse* response);
    // delete profile for alarm
    // Param:
    // alarmProfileDeleteRequest: input request profile
    // alarmResponse: input response result
    // Return: none
    void alarmDeleteRpcMethod(const ::SAFplus::Rpc::rpcAlarm::alarmProfileDeleteRequest* request,
                       ::SAFplus::Rpc::rpcAlarm::alarmResponse* response);
    // raise for alarm
    // Param:
    // alarmDataRequest: input request alarm data
    // alarmResponse: input response result
    // Return: none
    void alarmRaiseRpcMethod(const ::SAFplus::Rpc::rpcAlarm::alarmDataRequest* request,
                       ::SAFplus::Rpc::rpcAlarm::alarmResponse* response);
    // store all alarm data
    static StorageAlarmData data;
    // event client to publish to other client
    static SAFplus::EventClient eventClient;
    // fault connection to send to fault manager
    static SAFplus::Fault faultClient;
    // current active address server
    static SAFplus::Handle activeServer;
    // mutex for data to make consistent
    static boost::mutex g_data_mutex;
  private:
    // check handMasking
    // Param:
    // paths: input vector of paths
    // Return: true: is masked, false: not masked
    bool checkHandMasking(const std::vector<std::string>& paths);
  protected:
    //address server
    SAFplus::Handle handleAlarmServer;
    // alarm message server
    SAFplus::SafplusMsgServer* alarmMsgServer;
    // group alarm
    SAFplus::Group group;
    //rpc channel
    SAFplus::Rpc::RpcChannel * channel;
    //alarm shared memory
    EventSharedMem alarmSharedMemory;
};
}


#endif /* ALARMSERVER_H_HEADER_INCLUDED_A6DEFBE5 */
