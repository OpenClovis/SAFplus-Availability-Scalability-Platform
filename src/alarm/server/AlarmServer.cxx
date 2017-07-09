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
#include <clObjectMessager.hxx>
#include <AlarmMessageType.hxx>
#include <clAlarmServerApi.hxx>


using namespace SAFplusAlarm;

namespace SAFplus
{
StorageAlarmData AlarmServer::data;
boost::mutex AlarmServer::g_data_mutex;
SAFplus::EventClient AlarmServer::eventClient;

AlarmServer::AlarmServer()
{
  channel = nullptr;
}
void AlarmServer::initialize()
{
  ClRcT rc;
  logDebug(ALARM_SERVER, ALARM_ENTITY, "Initialize Alarm Server");
  handleAlarmServer = Handle::create(); // This is the handle for this specific alarm server
  logDebug(ALARM_SERVER, ALARM_ENTITY, "Register Alarm Server to Alarm Group");
  group.init(ALARM_GROUP);
  if ((rc = timerInitialize(NULL)) != CL_OK)
  {
    logError(ALARM_SERVER, ALARM_ENTITY, "Initialize Timer failed [%d]", rc);
    return;
  }
  group.setNotification(*this);
  SAFplus::objectMessager.insert(handleAlarmServer, this);
  alarmMsgServer = &safplusMsgServer;
  logDebug(ALARM_SERVER, ALARM_ENTITY, "SAFplus::objectMessager.insert(handleAlarmServer,this)");
  group.registerEntity(handleAlarmServer, SAFplus::ASP_NODEADDR, NULL, 0, Group::ACCEPT_STANDBY | Group::ACCEPT_ACTIVE | Group::STICKY);
  activeServer = group.getActive();
  //logDebug(ALARM_SERVER, ALARM_ENTITY, "Register alarm message server");
  //alarmMsgServer->RegisterHandler(SAFplusI::ALARM_MSG_TYPE, this, NULL); //  Register the main message handler (no-op if already registered)
  logDebug(ALARM_SERVER, ALARM_ENTITY, "Register rpc stub for Alarm");
  channel = new SAFplus::Rpc::RpcChannel(alarmMsgServer, this);
  if(nullptr == channel)
  {
    logError(ALARM_SERVER, ALARM_ENTITY, "channel is nullptr");
    return;
  }
  channel->setMsgSendType(SAFplusI::ALARM_REQ_HANDLER_TYPE);
  channel->msgReplyType = SAFplusI::ALARM_REPLY_HANDLER_TYPE;
  assert(activeServer != INVALID_HDL); // It can't be invalid because I am available to be active.
  logDebug(ALARM_SERVER, ALARM_ENTITY, "Initialize alarm checkpoint");
  data.initialize();
  logDebug(ALARM_SERVER, ALARM_ENTITY, "Initialize event client on server");
  eventClient.eventInitialize(handleAlarmServer, nullptr);
  eventClient.eventChannelOpen(ALARM_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
  eventClient.eventChannelPublish(ALARM_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
  //active address
  eventClient.eventChannelOpen(ALARM_CHANNEL_ADDRESS_CHANGE, EventChannelScope::EVENT_GLOBAL_CHANNEL);
  eventClient.eventChannelPublish(ALARM_CHANNEL_ADDRESS_CHANGE, EventChannelScope::EVENT_GLOBAL_CHANNEL);
}


ClRcT AlarmServer::publishAlarm(const AlarmData& alarmData)
{
  logDebug(ALARM_SERVER, "MSG", "**************************************************");
  logDebug(ALARM_SERVER, "MSG", "YOU ARE SUCCESSFULL PUBLIC ALARM [%s]", alarmData.toString().c_str());
  logDebug(ALARM_SERVER, "MSG", "**************************************************");
  std::stringstream ss;
  ss << alarmData;
  eventClient.eventPublish(ss.str(), ss.str().length(), ALARM_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
  AlarmKey key(alarmData.resourceId, alarmData.category, alarmData.probCause);
  AlarmProfileData alarmProfile;
  if (!data.findAlarmProfileData(key, alarmProfile))
  {
    logError(ALARM_SERVER, ALARMINFO, "Profile not found [%s]", key.toString().c_str());
    return false;
  }
  if(alarmProfile.isSend)
  {
    logDebug(ALARM_SERVER, "MSG", "YOU IMPLEMENT SEND FAULT to MANAGEMENT [%s]", alarmData.toString().c_str());
  }
}

AlarmServer::~AlarmServer()
{
  eventClient.eventChannelClose(ALARM_CHANNEL, EventChannelScope::EVENT_GLOBAL_CHANNEL);
  eventClient.eventChannelClose(ALARM_CHANNEL_ADDRESS_CHANGE, EventChannelScope::EVENT_GLOBAL_CHANNEL);
  if(nullptr != channel)
  {
    delete channel;
    channel = nullptr;
  }
}

void AlarmServer::purgeRpcRequest(const AlarmFilter& inputFilter)
{
}

void AlarmServer::deleteRpcRequest(const AlarmFilter& inputFilter)
{
}
void AlarmServer::processAssertAlarmData(const AlarmData& alarmData)
{
  processPublishAlarmData(alarmData);
}
void AlarmServer::processClearAlarmData(const AlarmData& alarmData)
{
  processPublishAlarmData(alarmData);
}
bool AlarmServer::alarmConditionCheck(const AlarmInfo& alarmInfo)
{
  AlarmKey key(alarmInfo.resourceId, alarmInfo.category, alarmInfo.probCause);
  AlarmProfileData alarmProfile;
  bool isConditionMatch = false;
  //process rule
  if (!data.findAlarmProfileData(key, alarmProfile))
  {
    logError(ALARM_SERVER, ALARMINFO, "Profile not found [%s]", key.toString().c_str());
    return false;
  }
  //check generate rule
  BITMAP64 alarmResult = alarmProfile.generationRuleBitmap & alarmInfo.afterSoakingBitmap;
  if ((AlarmRuleRelation::LOGICAL_AND == alarmProfile.genRuleRelation) && (alarmResult == alarmInfo.afterSoakingBitmap))
  {
    isConditionMatch = true;
  }
  else if (((AlarmRuleRelation::LOGICAL_OR == alarmProfile.genRuleRelation) || (AlarmRuleRelation::NO_RELATION == alarmProfile.genRuleRelation)) && alarmResult)
  {
    isConditionMatch = true;
  }
  logDebug(ALARM_SERVER, ALARMINFO, "Generate Rule [%s] isConditionMatch[%d]", alarmInfo.toString().c_str(),isConditionMatch);
  if (!isConditionMatch) return isConditionMatch;
  //check suppression rule
  alarmResult = alarmProfile.suppressionRuleBitmap & alarmInfo.afterSoakingBitmap;
  if ((AlarmRuleRelation::LOGICAL_AND == alarmProfile.suppRuleRelation) && (alarmResult == alarmInfo.afterSoakingBitmap))
  {
    isConditionMatch = false;
  }
  else if (((AlarmRuleRelation::LOGICAL_OR == alarmProfile.suppRuleRelation) || (AlarmRuleRelation::NO_RELATION == alarmProfile.suppRuleRelation)) && alarmResult)
  {
    isConditionMatch = false;
  }
  logDebug(ALARM_SERVER, ALARMINFO, "Suppression Rule [%s] isConditionMatch[%d]", alarmInfo.toString().c_str(), isConditionMatch);

  return isConditionMatch;
}
//isSet = true: after soaking
//isSet = false: under soaking
void AlarmServer::processAffectedAlarm(const AlarmKey& key, const bool& isSet)
{
  AlarmInfo alarmInfo;
  AlarmProfileData alarmProfile;
  if (!data.findAlarmProfileData(key, alarmProfile))
  {
    logError(ALARM_SERVER, ALARMINFO, "Alarm Profile not found [%s]", key.toString().c_str());
    return;
  }
  BITMAP64 affectedBitmap = alarmProfile.affectedBitmap;
  int intIndex = alarmProfile.intIndex; //keep index
  std::vector < AlarmKey > vectKeys = data.getKeysAlarmInfo(key.tuple.get<0>());
  for (AlarmKey keyAffected : vectKeys)
  {
    if (!data.findAlarmProfileData(keyAffected, alarmProfile))
    {
      logError(ALARM_SERVER, ALARMINFO, "Alarm Profile not found [%s]", keyAffected.toString().c_str());
      continue;
    }
    if (affectedBitmap & ((BITMAP64) 1 << alarmProfile.intIndex))
    {
      if (!data.findAlarmInfo(keyAffected, alarmInfo))
      {
        logError(ALARM_SERVER, ALARMINFO, "Alarm Info not found [%s]", keyAffected.toString().c_str());
        continue;
      }
      if (isSet)
      {
        alarmInfo.afterSoakingBitmap |= ((BITMAP64) 1 << intIndex); //after soaking
      }
      else
      {
        alarmInfo.afterSoakingBitmap &= ~((BITMAP64) 1 << intIndex); //under soaking
      }
      data.isUpdate = true;
      data.updateAlarmInfo(alarmInfo);
    } //if
  } //for
}

void AlarmServer::processPublishAlarmData(const AlarmData& alarmData)
{
  AlarmKey key(alarmData.resourceId, alarmData.category, alarmData.probCause);
  AlarmInfo alarmInfo;
  AlarmProfileData alarmProfile;
  int i = 0;
  bool isGenerate = false;
  //update state
  if (!data.findAlarmInfo(key, alarmInfo))
  {
    logError(ALARM_SERVER, ALARMINFO, "Alarm not found [%s]", key.toString().c_str());
    return;
  }

  if (!data.findAlarmProfileData(key, alarmProfile))
  {
    logError(ALARM_SERVER, ALARMINFO, "Alarm Profile not found [%s]", key.toString().c_str());
    return;
  }
  if ((AlarmState::CLEAR == alarmData.state) || ((AlarmState::ASSERT == alarmData.state) && alarmConditionCheck(alarmInfo)))
  {
    //generate alarm
    alarmInfo.state = alarmData.state;
    if(AlarmState::ASSERT == alarmData.state)
    {
      if (!checkHandMasking(alarmInfo))
      {
        if (AlarmState::ASSERT == alarmData.state)
        {
          publishAlarm(alarmData);
        }
        else
        {
          logDebug(ALARM_SERVER, ALARMINFO, "Status clear for [%s]", alarmData.toString().c_str());
        }
      }
      else
      {
        logDebug(ALARM_SERVER, ALARMINFO, "checkHandMasking not passed for [%s]", alarmData.toString().c_str());
      }
    }
    data.isUpdate = true;
    data.updateAlarmInfo(alarmInfo);
  }
}
ClRcT AlarmServer::processAlarmDataCallBack(void* apAlarmData)
{
  logDebug(ALARM_SERVER, ALARMINFO, "processAlarmDataCallBack");
  AlarmData* pAlarmData = (AlarmData*) apAlarmData;
  AlarmData alarmData = *pAlarmData;
  AlarmKey key(alarmData.resourceId, alarmData.category, alarmData.probCause);
  processAffectedAlarm(key); //true after soaking
  if (AlarmState::ASSERT == pAlarmData->state) processAssertAlarmData(alarmData);
  else processClearAlarmData(alarmData);
  return CL_OK;
}
void AlarmServer::processAlarmData(const AlarmData& alarmData)
{
  AlarmKey key(alarmData.resourceId, alarmData.category, alarmData.probCause);
  AlarmInfo alarmInfo;
  AlarmProfileData alarmProfile;
  bool isNeedCall = false;
  if (!data.findAlarmInfo(key, alarmInfo))
  {
    logError(ALARM_SERVER, ALARMINFO, "Alarm not found [%s]", key.toString().c_str());
    return;
  }
  if (!data.findAlarmProfileData(key, alarmProfile))
  {
    logError(ALARM_SERVER, ALARMINFO, "Profile not found [%s]", key.toString().c_str());
    return;
  }
  if (alarmInfo.severity != alarmData.severity)
  {
    alarmInfo.severity = alarmData.severity;
    data.isUpdate = true;
  }
  if (AlarmState::ASSERT == alarmData.state)
  {
    if (nullptr == alarmInfo.sharedClearTimer)
    {
      if (alarmProfile.intAssertSoakingTime > 0)
      {
        TimerTimeOutT timeOut;
        timeOut.tsSec = 0;
        timeOut.tsMilliSec = alarmProfile.intAssertSoakingTime;
        processAffectedAlarm(key, false); //false under soaking
        alarmInfo.sharedAlarmData = std::make_shared < AlarmData > (alarmData);
        alarmInfo.sharedAssertTimer = std::make_shared < Timer > (timeOut, TIMER_ONE_SHOT, TimerContextT::TIMER_SEPARATE_CONTEXT, &AlarmServer::processAlarmDataCallBack, static_cast<void *>(alarmInfo.sharedAlarmData.get()));
        logDebug(ALARM_SERVER, ALARMINFO, "Start Assert Timer");
        alarmInfo.sharedAssertTimer->timerStart(); //leak pointer
        data.isUpdate = true;
      }
      else
      {
        isNeedCall = true;
      }
    } //if
  }
  else
  { //process clear
    if (nullptr != alarmInfo.sharedAssertTimer)
    {
      alarmInfo.sharedAssertTimer->timerStop();
      alarmInfo.sharedAssertTimer = nullptr;
      alarmInfo.sharedAlarmData = nullptr;
      data.isUpdate = true;
      processAffectedAlarm(key); //after soaking
    }
    if (nullptr != alarmInfo.sharedClearTimer)
    {
      alarmInfo.sharedClearTimer->timerStop();
      alarmInfo.sharedClearTimer = nullptr;
      alarmInfo.sharedAlarmData = nullptr;
      data.isUpdate = true;
      processAffectedAlarm(key); //after soaking
    }
    if (alarmProfile.intClearSoakingTime > 0)
    {
      TimerTimeOutT timeOut;
      timeOut.tsSec = 0;
      timeOut.tsMilliSec = alarmProfile.intClearSoakingTime;
      processAffectedAlarm(key, false); //false under soaking
      alarmInfo.sharedAlarmData = std::make_shared < AlarmData > (alarmData);
      alarmInfo.sharedClearTimer = std::make_shared < Timer > (timeOut, TIMER_ONE_SHOT, TimerContextT::TIMER_SEPARATE_CONTEXT, &AlarmServer::processAlarmDataCallBack, static_cast<void *>(alarmInfo.sharedAlarmData.get()));
      logDebug(ALARM_SERVER, ALARMINFO, "Start clear Timer");
      alarmInfo.sharedClearTimer->timerStart();
      data.isUpdate = true;
    }
    else
    {
      isNeedCall = true;
    }
  } //else
  data.updateAlarmInfo(alarmInfo);
  if (isNeedCall)
  {
    if (AlarmState::ASSERT == alarmData.state)
    {
      processAssertAlarmData(alarmData);
    }
    else
    {
      processClearAlarmData(alarmData);
    }
  }
}
bool AlarmServer::checkHandMasking(const AlarmInfo& aalarmInfo)
{
  std::vector < std::string > paths = convertResourceId2vector(aalarmInfo.resourceId);
  for (int i = 0; i < paths.size(); i++)
  {
    std::string rootPath = formResourceIdforPtree(paths, i + 1);
    try
    {
      AlarmInfo alarmInfo;
      AlarmKey key(rootPath, aalarmInfo.category, aalarmInfo.probCause);
      if (data.findAlarmInfo(key, alarmInfo))
      {
        if (rootPath.compare(aalarmInfo.resourceId) == 0)
        { //same resource
          if ((AlarmState::ASSERT == alarmInfo.state) && (AlarmState::ASSERT == aalarmInfo.state)) return true;
        }
        else
        {
          AlarmProfileData localProfileData;
          if (data.findAlarmProfileData(key, localProfileData))
          {
            std::cout<<key.toString()<<"issupress:"<<localProfileData.isSuppressChild<<std::endl;
            if ((AlarmState::ASSERT == alarmInfo.state)&& localProfileData.isSuppressChild) return true;
          }
        }
      }
    }
    catch (...)
    {
      //do nothing,continue check hand masking
    }
  } //for
  return false;
}
void AlarmServer::wake(int amt, void* cookie)
{
  Group* g = (Group*) cookie;
  logDebug(ALARM_SERVER, ALARMINFO, "Group [%" PRIx64 ":%" PRIx64 "] changed",g->handle.id[0], g->handle.id[1]);
  if (activeServer != g->getActive())
  {
    if (handleAlarmServer == g->getActive())
    {
      //TODO Load global data from checkpoint
      //data.loadAlarmProfile();
      //data.loadAlarmData();
    }
    activeServer = g->getActive();
    std::stringstream ss;
    ss<<activeServer.id[0]<<" "<<activeServer.id[1];
    //eventClient.eventPublish(ss.str(), ss.str().length(), ALARM_CHANNEL_ADDRESS_CHANGE, EventChannelScope::EVENT_GLOBAL_CHANNEL);
  }
}
int count = 0;
void AlarmServer::alarmCreateRpcMethod(const ::SAFplus::Rpc::rpcAlarm::alarmProfileCreateRequest* request,
                           ::SAFplus::Rpc::rpcAlarm::alarmResponse* response)
{
   AlarmProfileData localProfileData;
   AlarmKey key(request->resourceid(),(AlarmCategory)request->category(),(AlarmProbableCause)request->probcause());
   logDebug(ALARM_SERVER, PROFILE,"********** Received create profile message from client key [%s] **********",key.toString().c_str());
   response->set_saerror(CL_OK);
   std::cout<<"DANGLE:alarmCreateRpcMethod:"<<count++<<std::endl;
   if(AlarmCategory::INVALID == (AlarmCategory)request->category())
   {
     logDebug(ALARM_SERVER, PROFILE,"********** Received invalid profile message from client key [%s] **********",key.toString().c_str());
     response->set_saerror(CL_NO);
     return;
   }
   if (!data.findAlarmProfileData(key, localProfileData))
   {
     boost::lock_guard < boost::mutex > lock { g_data_mutex };
     std::cout<<"CreateProfile:isSuppress:"<<request->issuppresschild()<<std::endl;
     AlarmProfileData alarmProfileData(request->resourceid(), (AlarmCategory)request->category(), (AlarmProbableCause)request->probcause(), (AlarmSpecificProblem)request->specificproblem(),request->issend(),request->intassertsoakingtime(),request->intclearsoakingtime(),(AlarmRuleRelation)request->genrulerelation(),
         (BITMAP64)request->generationrulebitmap(),(AlarmRuleRelation)request->supprulerelation(),(BITMAP64)request->suppressionrulebitmap(),request->intindex(),(BITMAP64)request->affectedbitmap(),request->issuppresschild());
     data.isUpdate = true;
     data.updateAlarmProfileData(alarmProfileData);
     //convert get key only
     AlarmInfo alarmInfo;
     copyKey(alarmProfileData, alarmInfo);
     data.isUpdate = true;
     alarmInfo.afterSoakingBitmap = alarmProfileData.generationRuleBitmap;
     if (!data.findAlarmInfo(key, alarmInfo))
     {
       data.updateAlarmInfo(alarmInfo); //alarm info auto
     }
     else
     {
       logDebug(ALARM_SERVER, PROFILE, "Alarm Info found [%s]", key.toString().c_str());
       response->set_saerror(CL_ERR_ALREADY_EXIST);
       response->set_errstr("Alarm Info found!");
     }
     logDebug(ALARM_SERVER, PROFILE, "Create alarm profile [%s]", key.toString().c_str());
   }
   else
   {
     logDebug(ALARM_SERVER, PROFILE, "Alarm profile [%s] is existed.", key.toString().c_str());
     response->set_saerror(CL_ERR_ALREADY_EXIST);
     response->set_errstr("Alarm profile found!");
   }
}
void AlarmServer::alarmDeleteRpcMethod(const ::SAFplus::Rpc::rpcAlarm::alarmProfileDeleteRequest* request,
                   ::SAFplus::Rpc::rpcAlarm::alarmResponse* response)
{
  AlarmKey key(request->resourceid(),(AlarmCategory)request->category(), (AlarmProbableCause)request->probcause());
  boost::lock_guard < boost::mutex > lock { g_data_mutex };
  data.removeAlarmProfileData(key);
  data.removeAlarmInfo(key);
  response->set_saerror(CL_OK);
  logDebug(ALARM_SERVER, PROFILE, "Delete alarm profile [%s]", key.toString().c_str());
}
void AlarmServer::alarmRaiseRpcMethod(const ::SAFplus::Rpc::rpcAlarm::alarmDataRequest* request,
                   ::SAFplus::Rpc::rpcAlarm::alarmResponse* response)
{
  boost::lock_guard < boost::mutex > lock { g_data_mutex };
  AlarmData alarmData(request->resourceid().c_str(),(AlarmCategory)request->category(),
      (AlarmProbableCause)request->probcause(),(AlarmSpecificProblem)request->specificproblem(),
      (AlarmSeverity)request->severity(),(AlarmState)request->state());
  processAlarmData(alarmData);
  response->set_saerror(CL_OK);
  logDebug(ALARM_SERVER, ALARMINFO, "Raise alarm [%s]", alarmData.toString().c_str());
}
}
