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
#include <AlarmMessageProtocol.hxx>
#include <AlarmMessageType.hxx>
#include <clAlarmApi.hxx>

#include <clNameApi.hxx>


using namespace std;

#ifdef ALARM_CLIENT
extern AlarmComponentResAlarms appAlarms[];
extern std::string myresourceId;
#endif

namespace SAFplus
{
Alarm::Alarm()
{
	createAlarmProfile();
}

void Alarm::init(const SAFplus::Handle& clientHandle, SAFplus::Wakeable& wake)
{
	wakeable = &wake;
	handleClientAlarm = clientHandle;
}

Alarm::Alarm(const SAFplus::Handle& clientHandle, const SAFplus::Handle& serverHandle)
{
  wakeable = NULL;
  handleAlarmServer = serverHandle;
  handleClientAlarm = INVALID_HDL;
  if(!alarmMsgServer)
  {
	  alarmMsgServer = &safplusMsgServer;
  }
}

ClRcT Alarm::raiseAlarm(const AlarmData& alarmData)
{
	return raiseAlarm(alarmData.resourceId,alarmData.category, alarmData.probCause, alarmData.severity, alarmData.specificProblem, alarmData.state);
}

ClRcT Alarm::subscribe(EventCallback* pEvtCallbacks)
{
	//eventClient.OpenChannel(ALARM_CHANNEL,GLOBAL,pEvtCallbacks);
	return CL_OK;
}

ClRcT Alarm::unSubscriber()
{
	//eventClient.CloseChannel();
	return CL_OK;
}



ClRcT Alarm::createAlarmProfile()
{
#ifdef ALARM_CLIENT
	std::cout<<"createAlarmProfile"<<std::endl;
	int index = 0;
	if(nullptr != appAlarms)
	{
		while(appAlarms[index].resourceId.compare(myresourceId) != 0)
		{
			if(appAlarms[index].resourceId.size() == 0)
			{
				break;//end array
			}
			index++;
		}
		index--;

		if(nullptr != appAlarms[index].MoAlarms)
		{
			while(nullptr != appAlarms[index].MoAlarms)
			{

				AlarmMessageProtocol sndMessage;
				sndMessage.messageType = AlarmMessageType::MSG_CREATE_PROFILE_ALARM;
				sndMessage.alarmData.resourceId = appAlarms[index].resourceId;
				int j = 0;
			    std::vector<AlarmProfile> vectAlarm;
			    while(j < SAFplusI::MAX_ALARM_SIZE)
			    {
				  if(AlarmCategory::INVALID == appAlarms[index].MoAlarms[j].category)
				  {
					  logDebug(ALARM,ALARM_ENTITY,"Profile size is %d",j);
					  break;
				  }
			      AlarmKey key(appAlarms[index].resourceId,appAlarms[index].MoAlarms[j].category,appAlarms[index].MoAlarms[j].probCause);
				  AlarmProfileInfo profileInfo;
				  profileInfo.isSend = appAlarms[index].MoAlarms[j].isSend;
				  profileInfo.specificProblem = appAlarms[index].MoAlarms[j].specificProblem;
				  profileInfo.intAssertSoakingTime = appAlarms[index].MoAlarms[j].intAssertSoakingTime;
				  profileInfo.intClearSoakingTime = appAlarms[index].MoAlarms[j].intClearSoakingTime;
				  //profileInfo.intPollTime = appAlarms[index].MoAlarms[j].intPollTime;
				  profileInfo.genRuleRelation = appAlarms[index].MoAlarms[j].generationRule->relation;
				  profileInfo.suppRuleRelation = appAlarms[index].MoAlarms[j].suppressionRule->relation;
				  profileInfo.intIndex = j;
				  sndMessage.alarmProfileData[key] = profileInfo;
				  j++;
			    }//while
			    j = 0;
			    int currentSize = j;
			    //set generule and supprule
			    while(j < currentSize)
				{
				  BITMAP64 resultRuleBitmap = 0;
				  AlarmKey key(appAlarms[index].resourceId,appAlarms[index].MoAlarms[j].category,appAlarms[index].MoAlarms[j].probCause);
				  for(int k =0; k < SAFplusI::MAX_ALARM_RULE_DEPENDENCIES, k < appAlarms[index].MoAlarms[j].generationRule->vectAlarmRelation.size();k++)
				  {
					  AlarmKey keyRule(appAlarms[index].resourceId,appAlarms[index].MoAlarms[j].category,appAlarms[index].MoAlarms[j].generationRule->vectAlarmRelation[k].probCause);
					  if(sndMessage.alarmProfileData.find(keyRule) != sndMessage.alarmProfileData.end())
					  {
						  resultRuleBitmap |= (BITMAP64)1<<sndMessage.alarmProfileData[keyRule].intIndex;
						  sndMessage.alarmProfileData[keyRule].affectedBitmap |= (BITMAP64)1<<sndMessage.alarmProfileData[keyRule].intIndex;
					  }
				  }
				  sndMessage.alarmProfileData[key].generationRuleBitmap = resultRuleBitmap;
				  resultRuleBitmap = 0;
				  for(int k =0; k < SAFplusI::MAX_ALARM_RULE_DEPENDENCIES, k < appAlarms[index].MoAlarms[j].suppressionRule->vectAlarmRelation.size();k++)
				  {
					  AlarmKey keyRule(appAlarms[index].resourceId,appAlarms[index].MoAlarms[j].category,appAlarms[index].MoAlarms[j].suppressionRule->vectAlarmRelation[k].probCause);
					  if(sndMessage.alarmProfileData.find(keyRule) != sndMessage.alarmProfileData.end())
					  {
						  resultRuleBitmap |= (BITMAP64)1<<sndMessage.alarmProfileData[keyRule].intIndex;
						  sndMessage.alarmProfileData[keyRule].affectedBitmap |= (BITMAP64)1<<sndMessage.alarmProfileData[keyRule].intIndex;
					  }
				  }
				  sndMessage.alarmProfileData[key].suppressionRuleBitmap = resultRuleBitmap;
				  j++;
				}//while
                sendAlarmNotification((void *)&sndMessage,sizeof(AlarmMessageProtocol));
			}//while
		}//if
	}//if
#endif
	return CL_OK;
}

Alarm::~Alarm()
{
	deleteAlarmProfile();
}

ClRcT Alarm::raiseAlarm(const std::string& resourceId, const AlarmCategory& category, const AlarmProbableCause& probCause, const AlarmSeverity& severity, const AlarmSpecificProblem& specificProblem, const AlarmState& state)
{
	AlarmMessageProtocol sndMessage;
	sndMessage.messageType = AlarmMessageType::MSG_ENTITY_ALARM;
	sndMessage.alarmData.resourceId = resourceId;
	sndMessage.alarmData.category = category;
	sndMessage.alarmData.probCause = probCause;
	sndMessage.alarmData.severity = severity;
	sndMessage.alarmData.specificProblem = specificProblem;
	sndMessage.alarmData.state = state;
	sndMessage.alarmData.syncData[0] = 0;
	return sendAlarmNotification((void *)&sndMessage,sizeof(AlarmMessageProtocol));
}

//Sending a Alarm notification to Alarm server
ClRcT Alarm::sendAlarmNotification(const void* data, const int& dataLength)
{
	logDebug(ALARM,ALARM_ENTITY,"Sending Alarm Notification");
	if (handleAlarmServer==INVALID_HDL)
	{
		logError(ALARM,ALARM_ENTITY,"Alarm Server is not initialized");
		return CL_NO;
	}
	//will update node address active SC
	logDebug(ALARM,ALARM_ENTITY,"Send Fault Notification to active Fault Server  : node Id [%d]",handleAlarmServer.getNode());
	try
	{
		alarmMsgServer->SendMsg(handleAlarmServer, (void *)data, dataLength, SAFplusI::ALARM_MSG_TYPE);
	}
	catch (...)
	{
	   logDebug(ALARM,ALARM_ENTITY,"Failed to send message to alarm server.");
	   return CL_NO;
	}
	return CL_OK;
}//function

ClRcT Alarm::deleteAlarmProfile()
{
#ifdef ALARM_CLIENT
	int i = 0;
	if(nullptr != appAlarms)
	{
		if(nullptr != appAlarms[i].MoAlarms)
		{
			while(nullptr != appAlarms[i].MoAlarms)
			{
				AlarmMessageProtocol sndMessage;
				sndMessage.messageType = AlarmMessageType::MSG_DELETE_PROFILE_ALARM;
				sndMessage.alarmData.resourceId = appAlarms[i].resourceId;
				sendAlarmNotification((void *)&sndMessage,sizeof(AlarmMessageProtocol));
				i++;
			}//while
		}//if
	}//if
#endif
	return CL_OK;
}
}//namespace
