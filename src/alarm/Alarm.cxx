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

#include <clMsgPortsAndTypes.hxx>
#include <clCustomization.hxx>
#include <AlarmMessageType.hxx>
#include <clAlarmApi.hxx>


using namespace std;

namespace SAFplus
{
SAFplus::Handle Alarm::activeServerAddress;

void alarmCallback(const std::string& channelId,const EventChannelScope& scope,const std::string& data,const int& length)
{
  //if
  if(channelId.compare(ALARM_CHANNEL_ADDRESS_CHANGE) == 0)//notitfy address changed
  {
    std::stringstream is(data);
    is >> Alarm::activeServerAddress.id[0];
    is >> Alarm::activeServerAddress.id[1];
  }else
  {
    eventCallback(channelId,scope,data,length);
  }
}
//For alarm using rpc

void rpcDone(SAFplus::Rpc::rpcAlarm::alarmResponse* response)
{
  logDebug(ALARM, ALARM_ENTITY, "Alarm request Done");
}

Alarm::Alarm()
{
}

void Alarm::initialize(const SAFplus::Handle& handleClient, SAFplus::Wakeable& wake)
{
  handleClientAlarm = handleClient;
  handleAlarmServer = getProcessHandle(SAFplusI::ALARM_IOC_PORT, SAFplus::ASP_NODEADDR);
  wakeable = &wake;
  if (!alarmMsgServer)
  {
    alarmMsgServer = &safplusMsgServer;
  }
  eventClient.eventInitialize(handleClientAlarm, &eventCallback);
  //For Rpc
  logDebug(ALARM, ALARM_ENTITY, "Initialize alarm Rpc client");
  channel = new SAFplus::Rpc::RpcChannel(alarmMsgServer, handleAlarmServer);
  activeServerAddress = handleAlarmServer;
  channel->setMsgReplyType(SAFplusI::ALARM_REPLY_HANDLER_TYPE);
  channel->msgSendType = SAFplusI::ALARM_REQ_HANDLER_TYPE;
  service = new SAFplus::Rpc::rpcAlarm::rpcAlarm_Stub(channel);
}

ClRcT Alarm::raiseAlarm(const AlarmData& alarmData)
{
  return raiseAlarm(alarmData.resourceId, alarmData.category, alarmData.probCause, alarmData.severity, alarmData.specificProblem, alarmData.state);
}

ClRcT Alarm::subscriber()
{
  eventClient.eventChannelSubscriber(ALARM_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
  return CL_OK;
}

ClRcT Alarm::unSubscriber()
{
  eventClient.eventChannelUnSubscriber(ALARM_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
  return CL_OK;
}

ClRcT Alarm::createAlarmProfile()
{
  int indexApp = 0;
  int indexProfile = 0;
  if (nullptr != appAlarms)
  {
    while (appAlarms[indexApp].resourceId.compare(myresourceId) != 0)
    {
      if (appAlarms[indexApp].resourceId.size() == 0)
      {
        break; //end array
      }
      indexApp++;
    }
    if (nullptr != appAlarms[indexApp].MoAlarms)
    {
      //while (nullptr != appAlarms[indexApp].MoAlarms)
      //{
        MAPALARMPROFILEINFO alarmProfileData;
        while (indexProfile < SAFplusI::MAX_ALARM_SIZE)
        {
          if (AlarmCategory::INVALID == appAlarms[indexApp].MoAlarms[indexProfile].category)
          {
            logDebug(ALARM, ALARM_ENTITY, "Profile size is %d", indexProfile);
            break;
          }
          AlarmKey key(appAlarms[indexApp].resourceId, appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause);
          AlarmProfileData profileData;
          memset(profileData.resourceId, 0, SAFplusI::MAX_RESOURCE_NAME_SIZE);
          strcpy(profileData.resourceId, appAlarms[indexApp].resourceId.c_str());
          profileData.category = appAlarms[indexApp].MoAlarms[indexProfile].category;
          profileData.probCause = appAlarms[indexApp].MoAlarms[indexProfile].probCause;
          profileData.isSend = appAlarms[indexApp].MoAlarms[indexProfile].isSend;
          profileData.specificProblem = appAlarms[indexApp].MoAlarms[indexProfile].specificProblem;
          profileData.intAssertSoakingTime = appAlarms[indexApp].MoAlarms[indexProfile].intAssertSoakingTime;
          profileData.intClearSoakingTime = appAlarms[indexApp].MoAlarms[indexProfile].intClearSoakingTime;
          profileData.isSuppressChild = appAlarms[indexApp].MoAlarms[indexProfile].isSuppressChild;
          if (NULL != appAlarms[indexApp].MoAlarms[indexProfile].generationRule)
          {
            profileData.genRuleRelation = appAlarms[indexApp].MoAlarms[indexProfile].generationRule->relation;
          }
          if (NULL != appAlarms[indexApp].MoAlarms[indexProfile].suppressionRule)
          {
            profileData.suppRuleRelation = appAlarms[indexApp].MoAlarms[indexProfile].suppressionRule->relation;
          }
          profileData.intIndex = indexProfile;
          alarmProfileData[hash_value(key)] = profileData;
          indexProfile++;
        } //while
        int currentSize = indexProfile;
        indexProfile = 0;
        //set generule and supprule
        while (indexProfile < currentSize)
        {
          BITMAP64 resultRuleBitmap = 0;
          AlarmKey key(appAlarms[indexApp].resourceId, appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].probCause);
          if (NULL != appAlarms[indexApp].MoAlarms[indexProfile].generationRule)
          {
            for (int k = 0; k < SAFplusI::MAX_ALARM_RULE_DEPENDENCIES, k < appAlarms[indexApp].MoAlarms[indexProfile].generationRule->vectAlarmRelation.size(); k++)
            {
              AlarmKey keyRule(appAlarms[indexApp].resourceId, appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].generationRule->vectAlarmRelation[k].probCause);
              if (alarmProfileData.find(hash_value(keyRule)) != alarmProfileData.end())
              {
                resultRuleBitmap |= (BITMAP64) 1 << alarmProfileData[hash_value(keyRule)].intIndex;
                alarmProfileData[hash_value(keyRule)].affectedBitmap |= (BITMAP64) 1 << alarmProfileData[hash_value(key)].intIndex;
              }
            }
            alarmProfileData[hash_value(key)].generationRuleBitmap = resultRuleBitmap;
            resultRuleBitmap = 0;
          }
          if (NULL != appAlarms[indexApp].MoAlarms[indexProfile].suppressionRule)
          {
            for (int k = 0; k < SAFplusI::MAX_ALARM_RULE_DEPENDENCIES, k < appAlarms[indexApp].MoAlarms[indexProfile].suppressionRule->vectAlarmRelation.size(); k++)
            {
              AlarmKey keyRule(appAlarms[indexApp].resourceId, appAlarms[indexApp].MoAlarms[indexProfile].category, appAlarms[indexApp].MoAlarms[indexProfile].suppressionRule->vectAlarmRelation[k].probCause);
              if (alarmProfileData.find(hash_value(keyRule)) != alarmProfileData.end())
              {
                resultRuleBitmap |= (BITMAP64) 1 << alarmProfileData[hash_value(keyRule)].intIndex;
                alarmProfileData[hash_value(keyRule)].affectedBitmap |= (BITMAP64) 1 << alarmProfileData[hash_value(key)].intIndex;
              }
            }
            alarmProfileData[hash_value(key)].suppressionRuleBitmap = resultRuleBitmap;
          }
          indexProfile++;
        } //while
        //send a map profile to server
        for (const MAPALARMPROFILEINFO::value_type& it : alarmProfileData)
        {
          SAFplus::Rpc::rpcAlarm::alarmProfileCreateRequest openRequest;
          SAFplus::Rpc::rpcAlarm::alarmResponse openRequestRes;
          openRequest.set_resourceid(it.second.resourceId);
          openRequest.set_category((int)it.second.category);
          openRequest.set_probcause((int)it.second.probCause);
          openRequest.set_specificproblem((int)it.second.specificProblem);
          openRequest.set_issend(it.second.isSend);
          openRequest.set_intassertsoakingtime((int)it.second.intAssertSoakingTime);
          openRequest.set_intclearsoakingtime((int)it.second.intClearSoakingTime);
          openRequest.set_generationrulebitmap(it.second.generationRuleBitmap);
          openRequest.set_genrulerelation((int)it.second.genRuleRelation);
          openRequest.set_suppressionrulebitmap(it.second.suppressionRuleBitmap);
          openRequest.set_supprulerelation((int)it.second.suppRuleRelation);
          openRequest.set_intindex((int)it.second.intIndex);
          openRequest.set_affectedbitmap(it.second.affectedBitmap);
          openRequest.set_issuppresschild(it.second.isSuppressChild);
          service->alarmCreateRpcMethod(activeServerAddress,&openRequest,&openRequestRes,SAFplus::BLOCK);
          if(CL_OK != openRequestRes.saerror())
          {
            logDebug(ALARM, ALARM_ENTITY, "Error : %s ... ",openRequestRes.errstr().c_str());
            return openRequestRes.saerror();
          }
        }//for
      //} //while
    } //if
  } //if
  return CL_OK;
}

ClRcT Alarm::raiseAlarm(const std::string& resourceId, const AlarmCategory& category, const AlarmProbableCause& probCause, const AlarmSeverity& severity, const AlarmSpecificProblem& specificProblem, const AlarmState& state)
{
  SAFplus::Rpc::rpcAlarm::alarmDataRequest openRequest;
  SAFplus::Rpc::rpcAlarm::alarmResponse openRequestRes;
  openRequest.set_resourceid(resourceId);
  openRequest.set_category((int)category);
  openRequest.set_probcause((int)probCause);
  openRequest.set_specificproblem((int)specificProblem);
  openRequest.set_severity((int)severity);
  openRequest.set_state((int)state);
  openRequest.set_syncdata(" ");//temporary
  service->alarmRaiseRpcMethod(activeServerAddress,&openRequest,&openRequestRes,SAFplus::BLOCK);
  if(CL_OK != openRequestRes.saerror())
  {
    logDebug(ALARM, ALARM_ENTITY, "Error : %s ... ",openRequestRes.errstr().c_str());
    return openRequestRes.saerror();
  }
  return CL_OK;
}

ClRcT Alarm::deleteAlarmProfile()
{
  int indexApp = 0;
  int indexProfile = 0;
  if (nullptr != appAlarms)
  {
    while (appAlarms[indexApp].resourceId.compare(myresourceId) != 0)
    {
      if (appAlarms[indexApp].resourceId.size() == 0)
      {
        break; //end array
      }
      indexApp++;
    }
    if (nullptr != appAlarms[indexApp].MoAlarms)
    {
      while (true)
      {
        if (AlarmCategory::INVALID == appAlarms[indexApp].MoAlarms[indexProfile].category)
        {
          logDebug(ALARM, ALARM_ENTITY, "Profile size is %d", indexProfile);
          break;
        }
        SAFplus::Rpc::rpcAlarm::alarmProfileDeleteRequest openRequest;
        SAFplus::Rpc::rpcAlarm::alarmResponse openRequestRes;
        openRequest.set_resourceid(appAlarms[indexApp].resourceId);
        openRequest.set_category((int)appAlarms[indexApp].MoAlarms[indexProfile].category);
        openRequest.set_probcause((int)appAlarms[indexApp].MoAlarms[indexProfile].probCause);
        service->alarmDeleteRpcMethod(activeServerAddress,&openRequest,&openRequestRes,SAFplus::BLOCK);
        if(CL_OK != openRequestRes.saerror())
        {
          logDebug(ALARM, ALARM_ENTITY, "Error : %s ... ",openRequestRes.errstr().c_str());
        }
        indexProfile++;
      } //while
    } //if
  } //if
  return CL_OK;
}
Alarm::~Alarm()
{
  if(nullptr != channel)
  {
    delete channel;
    channel = nullptr;
  }
  if(nullptr != service)
  {
    delete service;
    service = nullptr;
  }
 }
} //namespace
