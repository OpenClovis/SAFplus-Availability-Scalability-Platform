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
#include <AlarmMessageProtocol.hxx>
#include <clAlarmApi.hxx>

#include <clNameApi.hxx>

using namespace std;

namespace SAFplus
{
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
  eventClient.eventChannelOpen(ALARM_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
  eventClient.eventChannelPublish(ALARM_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
}

Alarm::Alarm(const SAFplus::Handle& handleClient, const SAFplus::Handle& handleServer)
{
  wakeable = NULL;
  handleClientAlarm = handleClient;
  handleAlarmServer = handleServer;
  if (!alarmMsgServer)
  {
    alarmMsgServer = &safplusMsgServer;
  }
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
      while (nullptr != appAlarms[indexApp].MoAlarms)
      {
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
          //profileInfo.intPollTime = appAlarms[index].MoAlarms[j].intPollTime;
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
          AlarmMessageProtocol sndMessage;
          sndMessage.messageType = AlarmMessageType::MSG_CREATE_PROFILE_ALARM;
          sndMessage.data.alarmProfileData = it.second;
          sendAlarmNotification((void *) &sndMessage, sizeof(AlarmMessageProtocol));
        }
        break;
      } //while
    } //if
  } //if
  return CL_OK;
}

ClRcT Alarm::raiseAlarm(const std::string& resourceId, const AlarmCategory& category, const AlarmProbableCause& probCause, const AlarmSeverity& severity, const AlarmSpecificProblem& specificProblem, const AlarmState& state)
{
  AlarmMessageProtocol sndMessage;
  sndMessage.messageType = AlarmMessageType::MSG_ENTITY_ALARM;
  memset(sndMessage.data.alarmData.resourceId, 0, SAFplusI::MAX_RESOURCE_NAME_SIZE);
  strcpy(sndMessage.data.alarmData.resourceId, resourceId.c_str());
  sndMessage.data.alarmData.category = category;
  sndMessage.data.alarmData.probCause = probCause;
  sndMessage.data.alarmData.severity = severity;
  sndMessage.data.alarmData.specificProblem = specificProblem;
  sndMessage.data.alarmData.state = state;
  sndMessage.data.alarmData.syncData[0] = 0;
  return sendAlarmNotification((void *) &sndMessage, sizeof(AlarmMessageProtocol));
}

//Sending a Alarm notification to Alarm server
ClRcT Alarm::sendAlarmNotification(const void* data, const int& dataLength)
{
  logDebug(ALARM, ALARM_ENTITY, "Sending Alarm Notification");
  if (handleAlarmServer == INVALID_HDL)
  {
    logError(ALARM, ALARM_ENTITY, "Alarm Server is not initialized");
    return CL_NO;
  }
  //will update node address active SC
  logDebug(ALARM, ALARM_ENTITY, "Send Alarm Notification to active Alarm Server  : node Id [%d]", handleAlarmServer.getNode());
  try
  {
    alarmMsgServer->SendMsg(handleAlarmServer, (void *) data, dataLength, SAFplusI::ALARM_MSG_TYPE);
  }
  catch (...)
  {
    logDebug(ALARM, ALARM_ENTITY, "Failed to send message to alarm server.");
    return CL_NO;
  }
  return CL_OK;
} //function

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
      while (nullptr != appAlarms[indexApp].MoAlarms)
      {
        AlarmMessageProtocol sndMessage;
        sndMessage.messageType = AlarmMessageType::MSG_DELETE_PROFILE_ALARM;
        memset(sndMessage.data.alarmProfileData.resourceId, 0, SAFplusI::MAX_RESOURCE_NAME_SIZE);
        strcpy(sndMessage.data.alarmProfileData.resourceId, appAlarms[indexApp].resourceId.c_str());
        sndMessage.data.alarmProfileData.category = appAlarms[indexApp].MoAlarms[indexProfile].category;
        sndMessage.data.alarmProfileData.probCause = appAlarms[indexApp].MoAlarms[indexProfile].probCause;
        sendAlarmNotification((void *) &sndMessage, sizeof(AlarmMessageProtocol));
        if (AlarmCategory::INVALID == appAlarms[indexApp].MoAlarms[indexProfile].category)
        {
          logDebug(ALARM, ALARM_ENTITY, "Profile size is %d", indexProfile);
          break;
        }
        indexProfile++;
      } //while
    } //if
  } //if
  return CL_OK;
}
Alarm::~Alarm()
{
  eventClient.eventChannelClose(ALARM_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
}
} //namespace
