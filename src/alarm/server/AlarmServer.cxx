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
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <AlarmMessageType.hxx>
#include <clAlarmServerApi.hxx>
#include <AlarmMessageProtocol.hxx>
#include <Timer.hxx>

using namespace SAFplusAlarm;

namespace SAFplus
{
StorageAlarmData AlarmServer::data;

AlarmServer::AlarmServer()
{
initialize();
}
void AlarmServer::initialize()
{
	handleAlarmServer = Handle::create();  // This is the handle for this specific alarm server
    logDebug(ALARM_SERVER,ALARM_ENTITY,"SAFplus::objectMessager.insert(handleAlarmServer,this)");
	alarmMsgServer = &safplusMsgServer;
	group.init(ALARM_GROUP);
	group.setNotification(*this);
	logDebug(ALARM_SERVER,ALARM_ENTITY, "Register alarm server [] to event group");
	group.registerEntity(handleAlarmServer, SAFplus::ASP_NODEADDR,NULL,0,Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);
	activeServer=group.getActive();
	logDebug(ALARM_SERVER,ALARM_ENTITY, "Initialize alarm checkpoint");
	data.initialize();
	//TODO:Dump data from  checkpoint in to data storage object
	if(activeServer != handleAlarmServer)
	{
		//read data from checkpoint to global channel
		logDebug(ALARM_SERVER,ALARM_ENTITY, "Load data from checkpoint to data");
		data.loadAlarmProfile();
		data.loadAlarmData();
	}
    // the handleAlarmServer.handle is going to be a process handle.
	logDebug(ALARM_SERVER,ALARM_ENTITY,"Register alarm message server");
    alarmMsgServer->RegisterHandler(SAFplusI::ALARM_MSG_TYPE, this, NULL);  //  Register the main message handler (no-op if already registered)
  }

void AlarmServer::msgHandler(SAFplus::Handle from, SAFplus::MsgServer* srv, ClPtrT msg, ClWordT msglen, ClPtrT cookie)
{
	if(msg == NULL)
	{
		logError(ALARM,"MSG","Received NULL message. Ignored alarm message");
		return;
	}
	const AlarmMessageProtocol*rxMsg = (SAFplus::AlarmMessageProtocol*)msg;
	MAPALARMPROFILEINFO alarmProfileData;
	AlarmMessageType msgType=  rxMsg->messageType;
	AlarmData alarmData = rxMsg->alarmData;
	switch(msgType)
	{
		case SAFplusAlarm::AlarmMessageType::MSG_ENTITY_ALARM:
			processAlarmData(alarmData);
			break;
		case SAFplusAlarm::AlarmMessageType::MSG_CREATE_PROFILE_ALARM:
			alarmProfileData = rxMsg->alarmProfileData;
			createAlarmProfile(alarmData.resourceId,alarmProfileData);
			break;
		case SAFplusAlarm::AlarmMessageType::MSG_DELETE_PROFILE_ALARM:
			alarmProfileData = rxMsg->alarmProfileData;
			deleteAlarmProfile(alarmData.resourceId,alarmProfileData);
			break;
	    default:
		    logDebug(ALARM_SERVER,"MSG","Unknown message type [%d] from node [%d]",rxMsg->messageType,from.getNode());
		    break;
	}
}

ClRcT AlarmServer::publishAlarm(const AlarmData& alarmData)
{
}

AlarmServer::~AlarmServer()
{
}

void AlarmServer::purgeRpcRequest(const AlarmFilter& inputFilter)
{
}

void AlarmServer::deleteRpcRequest(const AlarmFilter& inputFilter)
{
}
/*  for assert
	* condition does not exist, no soaking, update curr, soaked,process for affected
	* condition does not exist, has soaking, not under soaking,	start soaking
	* condition does not exist, has soaking, under soakig, update curr
	* condition exists, no soaking, ignore
	* condition exists,  has soaking, not under soaking, ignore
	* condition exists,  has soaking, under soaking, update curr
*/
void AlarmServer::processAssertAlarmData(const AlarmData& alarmData)
{
	processAffectedAlarmData(alarmData);
}
/*  for clear:
	* condition does not exist, no soaking,  ignore
	* condition does not exist, has soaking,  ignore
	* condition exists, no soaking, process for affected alams
	* condition exists,  has soaking, not under soaking, start soaking,	update curr
	* condition exists,  has soaking, under soaking, update curr
*/
void AlarmServer::processClearAlarmData(const AlarmData& alarmData)
{
	processAffectedAlarmData(alarmData);
}
bool AlarmServer::alarmConditionCheck(const AlarmData& alarmData)
{
	AlarmKey key(alarmData.resourceId,alarmData.category,alarmData.probCause);
	AlarmProfileInfo alarmProfile;
	bool isGenerate = false;
	//process rule
	if(!data.findAlarmProfileInfo(key,alarmProfile))
	{
		logError(ALARM_SERVER,ALARMINFO,"Profile not found resourceId[%s] category[%d] probCause[%d]",alarmData.resourceId.c_str(),alarmData.category,alarmData.probCause);
		return false;
	}
	//check generate rule
	BITMAP64 bitmapAlarmResult = alarmProfile.generationRuleBitmap&((BITMAP64)1<<alarmProfile.intIndex);
	if((AlarmRuleRelation::LOGICAL_AND == alarmProfile.genRuleRelation) && bitmapAlarmResult)
	{
		isGenerate = true;
	}else if(((AlarmRuleRelation::LOGICAL_OR == alarmProfile.genRuleRelation)||(AlarmRuleRelation::NO_RELATION == alarmProfile.genRuleRelation))&& bitmapAlarmResult)
	{
		isGenerate = true;
	}
	logDebug(ALARM_SERVER,ALARMINFO,"Generate Rule resourceId[%s] category[%d] probCause[%d] isGenerate[%d]",alarmData.resourceId.c_str(),alarmData.category,alarmData.probCause,isGenerate);
	if(!isGenerate) return isGenerate;
	//check suppression rule
	bitmapAlarmResult = alarmProfile.suppressionRuleBitmap&((BITMAP64)1<<alarmProfile.intIndex);
	if((AlarmRuleRelation::LOGICAL_AND == alarmProfile.suppRuleRelation)&& bitmapAlarmResult)
	{
		isGenerate = false;
	}else if(((AlarmRuleRelation::LOGICAL_OR == alarmProfile.suppRuleRelation)||(AlarmRuleRelation::NO_RELATION == alarmProfile.suppRuleRelation))&& bitmapAlarmResult)
	{
		isGenerate = false;
	}
	logDebug(ALARM_SERVER,ALARMINFO,"Suppression Rule resourceId[%s] category[%d] probCause[%d] isGenerate[%d]",alarmData.resourceId.c_str(),alarmData.category,alarmData.probCause,isGenerate);
	return isGenerate;
}
void AlarmServer::processAffectedAlarmData(const AlarmData& alarmData)
{
	AlarmKey key(alarmData.resourceId,alarmData.category,alarmData.probCause);
	AlarmInfo alarmInfo;
	AlarmProfileInfo alarmProfile;
	int i = 0;
	bool isGenerate = false;
	//update state
	if(!data.findAlarmInfo(key,alarmInfo))
	{
		logError(ALARM_SERVER,ALARMINFO,"Alarm not found resourceId[%s] category[%d] probCause[%d]",alarmData.resourceId.c_str(),alarmData.category,alarmData.probCause);
		return;
	}
	alarmInfo.state = alarmData.state;
	data.isUpdate = true;
	data.updateAlarmInfo(key,alarmInfo);

	if(!data.findAlarmProfileInfo(key,alarmProfile))
	{
		logError(ALARM_SERVER,ALARMINFO,"Alarm not found resourceId[%s] category[%d] probCause[%d]",alarmData.resourceId.c_str(),alarmData.category,alarmData.probCause);
		return;
	}
	BITMAP64 affectedBitmap = alarmProfile.affectedBitmap;
	std::vector<AlarmKey> vectKeys = data.getKeysAlarmInfo(key.tuple.get<0>());
	for (AlarmKey key: vectKeys)
		{
			data.findAlarmProfileInfo(key,alarmProfile);
			if(((BITMAP64)1<<alarmProfile.intIndex)&affectedBitmap > 0)
			{
				if((AlarmState::CLEAR == alarmData.state)||((AlarmState::ASSERT == alarmData.state) && alarmConditionCheck(alarmData)))
				{
				   //generate alarm
					if(!checkHandMasking(alarmData))
					{
						publishAlarm(alarmData);
					}
				}
			}//if
		}//for
}
ClRcT AlarmServer::processAlarmDataCallBack(void* apAlarmData)
{
	AlarmData* pAlarmData = (AlarmData*)apAlarmData;
	AlarmData alarmData = *pAlarmData;
	if(AlarmState::ASSERT == pAlarmData->state)
		processAssertAlarmData(alarmData);
	else processClearAlarmData(alarmData);
	return CL_OK;
}
void AlarmServer::processAlarmData(const AlarmData& alarmData)
{
		AlarmKey key(alarmData.resourceId,alarmData.category,alarmData.probCause);
		AlarmInfo alarmInfo;
		AlarmProfileInfo alarmProfile;
		if(!data.findAlarmInfo(key,alarmInfo))
		{
			logError(ALARM_SERVER,ALARMINFO,"Alarm not found resourceId[%s] category[%d] probCause[%d]",alarmData.resourceId.c_str(),alarmData.category,alarmData.probCause);
			return;
		}
		if(!data.findAlarmProfileInfo(key,alarmProfile))
		{
			logError(ALARM_SERVER,ALARMINFO,"Profile not found resourceId[%s] category[%d] probCause[%d]",alarmData.resourceId.c_str(),alarmData.category,alarmData.probCause);
			return;
		}
		if(AlarmState::ASSERT == alarmData.state)
		{
			if(nullptr == alarmInfo.sharedClearTimer)
			{
			  if(alarmProfile.intAssertSoakingTime > 0)
			  {
				  TimerTimeOutT timeOut;
				  timeOut.tsSec = 0;
				  timeOut.tsMilliSec = alarmProfile.intAssertSoakingTime;
				  if(nullptr != alarmInfo.sharedAlarmData) alarmInfo.sharedAlarmData = nullptr;
				  alarmInfo.sharedAlarmData = std::make_shared<AlarmData>(alarmData);
				  alarmInfo.sharedAssertTimer = std::make_shared<Timer>(timeOut,TIMER_ONE_SHOT,TimerContextT::TIMER_SEPARATE_CONTEXT,&AlarmServer::processAlarmDataCallBack,static_cast<void *> (alarmInfo.sharedAlarmData.get()));
				  alarmInfo.sharedAssertTimer->timerStart();//leak pointer
				  data.isUpdate = true;
			  } else processAssertAlarmData(alarmData);
			}//if
		}else {//process clear
			if(nullptr != alarmInfo.sharedAssertTimer)
			{
				alarmInfo.sharedAssertTimer->timerStop();
				alarmInfo.sharedAssertTimer = nullptr;
				alarmInfo.sharedAlarmData = nullptr;
				data.isUpdate = true;
				//update checkpoint
			}
			if(nullptr != alarmInfo.sharedClearTimer)
			{
				alarmInfo.sharedClearTimer->timerStop();
				alarmInfo.sharedClearTimer = nullptr;
				alarmInfo.sharedAlarmData = nullptr;
				data.isUpdate = true;
			}
			if(alarmProfile.intClearSoakingTime > 0)
			{
				TimerTimeOutT timeOut;
				timeOut.tsSec = 0;
				timeOut.tsMilliSec = alarmProfile.intClearSoakingTime;
				if(nullptr != alarmInfo.sharedAlarmData) alarmInfo.sharedAlarmData = nullptr;
				alarmInfo.sharedAlarmData = std::make_shared<AlarmData>(alarmData);
				alarmInfo.sharedClearTimer = std::make_shared<Timer>(timeOut,TIMER_ONE_SHOT,TimerContextT::TIMER_SEPARATE_CONTEXT,&AlarmServer::processAlarmDataCallBack,static_cast<void *> (alarmInfo.sharedAlarmData.get()));
				alarmInfo.sharedClearTimer->timerStart();
				data.isUpdate = true;
			}else processClearAlarmData(alarmData);
		}//else
		//update severity
		if(alarmInfo.severity != alarmData.severity)
		{
			alarmInfo.severity = alarmData.severity;
			data.isUpdate = true;
		}
		data.updateAlarmInfo(key,alarmInfo);
}
bool AlarmServer::checkHandMasking(const AlarmData& alarmData)
{
	std::vector<std::string> paths = convertResourceId2vector(alarmData.resourceId);
	for(int i = 0; i < paths.size();i++)
	{
		std::string rootPath = formResourceIdforPtree(paths,i+1);
		try
		{
			AlarmInfo alarmInfo;
			AlarmKey key(rootPath,alarmData.category,alarmData.probCause);
			if(data.findAlarmInfo(key,alarmInfo))
			{
				if(AlarmState::ASSERT == alarmInfo.state) return true;
			}

		}catch(...)
		{
			//do nothing,continue check hand masking
		}
	}//for
	return false;
}
void AlarmServer::wake(int amt,void* cookie)
{
	Group* g = (Group*) cookie;
	logDebug(ALARM_SERVER,ALARMINFO, "Group [%" PRIx64 ":%" PRIx64 "] changed", g->handle.id[0],g->handle.id[1]);
	activeServer = g->getActive();
}
void AlarmServer::createAlarmProfile(const std::string& resourceId,const MAPALARMPROFILEINFO& mapAlarmProfileInfo)
{
	for (const MAPALARMPROFILEINFO::value_type& it : mapAlarmProfileInfo)
	{
	  data.updateAlarmProfileInfo(it.first,it.second);
	  AlarmInfo alarmInfo;
	  alarmInfo.state = AlarmState::CLEAR;
	  data.updateAlarmInfo(it.first,alarmInfo);//alarm info auto
	}
	logDebug(ALARM_SERVER,PROFILE,"Create alarm profile [%s]",resourceId.c_str());
}
void AlarmServer::deleteAlarmProfile(const std::string& resourceId,const MAPALARMPROFILEINFO& mapAlarmProfileInfo)
{
	for (const MAPALARMPROFILEINFO::value_type& it : mapAlarmProfileInfo)
	{
	  data.removeAlarmProfileInfo(it.first);
	  data.removeAlarmInfo(it.first);
	}
	logDebug(ALARM_SERVER,PROFILE,"Delete alarm profile [%s]",resourceId.c_str());
}
}
