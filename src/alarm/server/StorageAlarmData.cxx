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
#include <clCustomization.hxx>
#include "StorageAlarmData.hxx"

using namespace SAFplus;
using namespace SAFplusI;

StorageAlarmData::StorageAlarmData()
{
  isUpdate = false;
  Summary summaryClear(AlarmSeverity::CLEAR);
  Summary summaryWarn(AlarmSeverity::WARNING);
  Summary summaryMinor(AlarmSeverity::MINOR);
  Summary summaryMajor(AlarmSeverity::MAJOR);
  Summary summaryCritical(AlarmSeverity::CRITICAL);
  vectSummary.push_back(summaryClear);
  vectSummary.push_back(summaryWarn);
  vectSummary.push_back(summaryMinor);
  vectSummary.push_back(summaryMajor);
  vectSummary.push_back(summaryCritical);
}
void StorageAlarmData::initialize(void)
{
  m_checkpointProfile.init(SAFplus::ALARM_PROFILE_CKPT, Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, 1024 * 1024, SAFplusI::CkptDefaultRows);
  m_checkpointProfile.name = ALARM_PROFILE_CKPT_NAME;
  m_checkpointAlarm.init(SAFplus::ALARM_CKPT, Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, 1024 * 1024, SAFplusI::CkptDefaultRows);
  m_checkpointAlarm.name = ALARM_CKPT_NAME;
  isUpdate = false;
  //restore alarm
}
bool StorageAlarmData::findAlarmInfo(const AlarmKey& key, AlarmInfo& alarmInfo)
{
  std::size_t seedLevel0 = hash_value(key.tuple.get<0>());
  boost::unordered_map<std::size_t, MAPALARMINFO>::iterator itmap = m_mapAlarmInfoData.find(seedLevel0);
  if (itmap == m_mapAlarmInfoData.end())
  {
    return false;
  }
  std::size_t seedLevel1 = hash_value(key);
  MAPALARMINFO mapAlarmInfo = itmap->second;
  boost::unordered_map<std::size_t, AlarmInfo>::iterator itinfo = mapAlarmInfo.find(seedLevel1);
  if (itinfo == mapAlarmInfo.end())
  {
    return false;
  }
  alarmInfo = m_mapAlarmInfoData[seedLevel0][seedLevel1];
  return true;
}
void StorageAlarmData::updateAlarmInfo(AlarmInfo& alarmInfo)
{
  if (isUpdate)
  {
    std::lock_guard < std::mutex > lock(mtxAlarmData);
    std::size_t seedLevel0 = hash_value(alarmInfo.resourceId);
    AlarmKey key(alarmInfo.resourceId, alarmInfo.category, alarmInfo.probCause);
    std::size_t seedLevel1 = hash_value(key);
    alarmInfo.statusChangeTime = boost::posix_time::second_clock::local_time();
    m_mapAlarmInfoData[seedLevel0][seedLevel1] = alarmInfo;
    size_t valLen = sizeof(AlarmInfo);
    char vdata[sizeof(Buffer) - 1 + valLen];
    Buffer* val = new (vdata) Buffer(valLen);
    memcpy(val->data, &alarmInfo, valLen);
    m_checkpointAlarm.write(seedLevel1, *val);
    isUpdate = false;
  }
}

void StorageAlarmData::removeAlarmInfo(const AlarmKey& key)
{
  std::lock_guard < std::mutex > lock(mtxAlarmData);
  std::size_t seedLevel0 = hash_value(key.tuple.get<0>());
  std::size_t seedLevel1 = hash_value(key);
  if (nullptr != m_mapAlarmInfoData[seedLevel0][seedLevel1].sharedAssertTimer)
  {
    m_mapAlarmInfoData[seedLevel0][seedLevel1].sharedAssertTimer->timerStop();
    m_mapAlarmInfoData[seedLevel0][seedLevel1].sharedAssertTimer = nullptr;
  }
  if (nullptr != m_mapAlarmInfoData[seedLevel0][seedLevel1].sharedAssertTimer)
  {
    m_mapAlarmInfoData[seedLevel0][seedLevel1].sharedClearTimer->timerStop();
    m_mapAlarmInfoData[seedLevel0][seedLevel1].sharedClearTimer = nullptr;
  }
  m_mapAlarmInfoData[seedLevel0].erase(seedLevel1);
  m_checkpointAlarm.remove(seedLevel1);
}
bool StorageAlarmData::findAlarmProfileData(const AlarmKey& key, AlarmProfileData& alarmProfileData)
{
  std::size_t seedLevel1 = hash_value(key);
  boost::unordered_map<std::size_t, AlarmProfileData>::iterator it = m_ProfileInfoData.find(seedLevel1);
  if (it != m_ProfileInfoData.end())
  {
    alarmProfileData = it->second;
    return true;
  }
  return false;
}
void StorageAlarmData::updateAlarmProfileData(const AlarmProfileData& alarmProfileData)
{
  if (isUpdate)
  {
    std::lock_guard < std::mutex > lock(mtxProfileData);
    AlarmKey key(alarmProfileData.resourceId, alarmProfileData.category, alarmProfileData.probCause);
    std::size_t seedLevel1 = hash_value(key);
    m_ProfileInfoData[seedLevel1] = alarmProfileData;
    size_t valLen = sizeof(AlarmProfileData);
    char vdata[sizeof(Buffer) - 1 + valLen];
    Buffer* val = new (vdata) Buffer(valLen);
    memcpy(val->data, &alarmProfileData, valLen);
    m_checkpointProfile.write(seedLevel1, *val);
    isUpdate = false;
  }
}

void StorageAlarmData::removeAlarmProfileData(const AlarmKey& key)
{
  std::lock_guard < std::mutex > lock(mtxProfileData);
  std::size_t seedLevel1 = hash_value(key);
  m_ProfileInfoData.erase(seedLevel1);
  m_checkpointProfile.remove(seedLevel1);
}
void StorageAlarmData::printSummary()
{
  std::ostringstream os;
  os<<"*****************Summary****************\n";
  for (Summary& summary : vectSummary)
  {
    os << summary.toString().c_str()<<"\n";
  }
  logInfo(ALARM_SERVER, "DUMP", "\n%s", os.str().c_str());
}
void StorageAlarmData::printProfile()
{
  std::ostringstream os;
  os << "*****************Profile****************\n";
  for (const MAPALARMPROFILEINFO::value_type& it : m_ProfileInfoData)
  {
    os<<"key name ["<<it.first<<"]\n";
    os<<it.second.toString().c_str()<<"\n";
  }//for
  logInfo(ALARM_SERVER, "DUMP", "\n%s", os.str().c_str());
}
void StorageAlarmData::printAlarm()
{
  std::ostringstream os;
  int count = 0;
  os<<"*******************Alarm****************\n";
  for (const MAPALARMPROFILEINFO::value_type& it : m_ProfileInfoData)
  {
    count++;
    os<<"key name ["<<it.first<<"]\n";
    AlarmInfo alarmInfo;
    AlarmKey key(it.second.resourceId, it.second.category, it.second.probCause);
    if (findAlarmInfo(key, alarmInfo))
    {
      os << alarmInfo.toString().c_str()<<"\n";
    }
    if((count%3 == 0) && (count > 0))
    {
      logInfo(ALARM_SERVER, "DUMP", "%s", os.str().c_str());
      os.str("");
      os.clear();//clear
    }
  }
  logInfo(ALARM_SERVER, "DUMP", "%s", os.str().c_str());
}
std::vector<AlarmKey> StorageAlarmData::getKeysAlarmInfo(const std::string& resourceId)
{
  std::vector < AlarmKey > vectKeys;
  boost::unordered_map<std::size_t, MAPALARMINFO>::iterator itmap = m_mapAlarmInfoData.find(hash_value(resourceId));
  if (itmap == m_mapAlarmInfoData.end())
  {
    return vectKeys;
  }
  MAPALARMINFO mapAlarmInfo = itmap->second; //access once
  for (const MAPALARMINFO::value_type& k : mapAlarmInfo)
  {
    if (strcmp(k.second.resourceId, resourceId.c_str()) == 0)
    {
      AlarmKey key(k.second.resourceId, k.second.category, k.second.probCause);
      vectKeys.push_back(key);
    }
  }
  return vectKeys;
}

void StorageAlarmData::accumulateSummary(const AlarmSeverity& severity,const AlarmState& state,const bool& isIncrease)
{
  for (Summary& summary : vectSummary)
  {
    if (severity == summary.severity)
    {
      if(AlarmState::ASSERT == state)
      {
        if(isIncrease) summary.intTotal++;
        else
        {
          summary.intTotal--;
        }
      }else if(AlarmState::CLEAR == state)
      {
        if(isIncrease)
        {
          summary.intCleared++;
        }
        else summary.intCleared--;
      }
    }
  } //for
}
//must be state assert
void StorageAlarmData::changeSummary(const AlarmSeverity& fromSeverity, const AlarmSeverity& toSeverity)
{
  for (Summary& summary : vectSummary)
  {
    if (fromSeverity == summary.severity)
    {
      summary.intTotal--;
    }else if (toSeverity == summary.severity)
    {
      summary.intTotal++;
    }
    /*else if(AlarmState::CLEAR == state)
    {
      if (fromSeverity == summary.severity)
      {
        summary.intCleared--;
      }else if (toSeverity == summary.severity)
      {
        summary.intCleared++;
      }
    }*/
  } //for
}
void StorageAlarmData::loadAlarmProfile()
{
  logInfo(ALARM, "DUMP", "loadAlarmProfile---------------------------------");
  SAFplus::Checkpoint::Iterator ibegin = m_checkpointProfile.begin();
  SAFplus::Checkpoint::Iterator iend = m_checkpointProfile.end();
  for (SAFplus::Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
  {
    BufferPtr curkey = iter->first;
    if (curkey)
    {
      logInfo(ALARM, "DUMP", "key name [%s]", curkey->data);
    }
    BufferPtr& curval = iter->second;
    if (curval)
    {
      AlarmProfileData *data = (AlarmProfileData*) curval->data;
      AlarmKey key(data->resourceId, data->category, data->probCause);
      m_ProfileInfoData[hash_value(key)] = *data;
    }
  }
}
void StorageAlarmData::loadAlarmData()
{
  logInfo(ALARM, "DUMP", "loadAlarmData---------------------------------");
  SAFplus::Checkpoint::Iterator ibegin = m_checkpointAlarm.begin();
  SAFplus::Checkpoint::Iterator iend = m_checkpointAlarm.end();
  for (SAFplus::Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
  {
    BufferPtr curkey = iter->first;
    if (curkey)
    {
      logInfo(ALARM, "DUMP", "key name [%s]", curkey->data);
    }
    BufferPtr& curval = iter->second;
    if (curval)
    {
      AlarmInfo *data = (AlarmInfo*) curval->data;
      data->sharedAssertTimer = nullptr; //this can not restore
      data->sharedClearTimer = nullptr; //this can not restore
      data->sharedAlarmData = nullptr; //this can not restore
      std::string resourceId = data->resourceId;
      AlarmKey key(data->resourceId, data->category, data->probCause);
      m_mapAlarmInfoData[hash_value(key.tuple.get<0>())][hash_value(key)] = *data;
    }
  }
}
