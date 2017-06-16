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
	m_checkpointProfile.init(SAFplus::ALARM_PROFILE_CKPT,Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, 1024*1024, SAFplusI::CkptDefaultRows);
	m_checkpointProfile.name = ALARM_PROFILE_CKPT_NAME;
	m_checkpointAlarm.init(SAFplus::ALARM_CKPT,Checkpoint::SHARED | Checkpoint::REPLICATED, SAFplusI::CkptRetentionDurationDefault, 1024*1024, SAFplusI::CkptDefaultRows);
	m_checkpointAlarm.name = ALARM_CKPT_NAME;
	isUpdate = false;
	//restore alarm
}
bool StorageAlarmData::findAlarmInfo(const AlarmKey& key,AlarmInfo& alarmInfo)
{
	boost::unordered_map<std::string,MAPALARMINFO>::iterator itmap = m_mapAlarmInfoData.find(key.tuple.get<0>());
	if(itmap == m_mapAlarmInfoData.end())
	{
		return false;
	}
	MAPALARMINFO mapAlarmInfo = itmap->second;
	boost::unordered_map<AlarmKey,AlarmInfo>::iterator itinfo = mapAlarmInfo.find(key);
    if(itinfo == mapAlarmInfo.end())
    {
    	return false;
    }
    alarmInfo = itinfo->second;
    return true;
}
void StorageAlarmData::updateAlarmInfo(const AlarmKey& key,const AlarmInfo& alarmInfo)
{
	if(isUpdate)
	{
		m_mapAlarmInfoData[key.tuple.get<0>()][key] = alarmInfo;
		size_t valLen = sizeof(AlarmInfo);
		char vdata[sizeof(Buffer)-1+valLen];
		Buffer* val = new(vdata) Buffer(valLen);
		memcpy(val->data, &alarmInfo, valLen);
		m_checkpointAlarm.write(toString(key).c_str(),*val);
		isUpdate = false;
	}
}
void StorageAlarmData::removeAlarmInfo(const AlarmKey& key)
{
	m_mapAlarmInfoData[key.tuple.get<0>()].erase(key);
	m_checkpointAlarm.remove(toString(key).c_str());
}
bool StorageAlarmData::findAlarmProfileInfo(const AlarmKey& key,AlarmProfileInfo& alarmProfileInfo)
{
	boost::unordered_map<AlarmKey,AlarmProfileInfo>::iterator it = m_ProfileInfoData.find(key);
    if(it != m_ProfileInfoData.end())
    {
    	alarmProfileInfo = it->second;
    	return true;
    }
    return false;
}
void StorageAlarmData::updateAlarmProfileInfo(const AlarmKey& key,const AlarmProfileInfo& alarmProfileInfo)
{
	if(isUpdate)
	{
		m_ProfileInfoData[key] = alarmProfileInfo;
		size_t valLen = sizeof(AlarmProfileInfo);
		char vdata[sizeof(Buffer)-1+valLen];
		Buffer* val = new(vdata) Buffer(valLen);
		memcpy(val->data, &alarmProfileInfo, valLen);
		m_checkpointProfile.write(toString(key).c_str(),*val);
		isUpdate = false;
	}
}

void StorageAlarmData::removeAlarmProfileInfo(const AlarmKey& key)
{
	m_ProfileInfoData.erase(key);
	m_checkpointProfile.remove(toString(key).c_str());
	removeAlarmInfo(key);
}
void StorageAlarmData::printAllData()
{
	for(Summary& summary:vectSummary)
	{
		logInfo(ALARM_SERVER,"DUMP","Summary [%s]", summary.toString().c_str());
	}
	for(const MAPALARMPROFILEINFO::value_type& it : m_ProfileInfoData)
	{
		logInfo(ALARM_SERVER,"DUMP","key name [%s]", toString(it.first).c_str());
		logInfo(ALARM_SERVER,"DUMP","profile [%s]",it.second.toString().c_str());
		AlarmInfo alarmInfo;
		if(findAlarmInfo(it.first,alarmInfo))
		{
			logInfo(ALARM_SERVER,"DUMP","alarm info [%s]",alarmInfo.toString().c_str());
		}
	}
}
std::vector<AlarmKey> StorageAlarmData::getKeysAlarmInfo(const std::string& resourceId)
{
	std::vector<AlarmKey> vectKeys;
	boost::unordered_map<std::string,MAPALARMINFO>::iterator itmap = m_mapAlarmInfoData.find(resourceId);
	if(itmap == m_mapAlarmInfoData.end())
	{
		return vectKeys;
	}
	MAPALARMINFO mapAlarmInfo = itmap->second;//access once
	for(const MAPALARMINFO::value_type& k : mapAlarmInfo)
	{
	  if(k.first.tuple.get<0>().compare(resourceId) == 0)
	  {
	    vectKeys.push_back(k.first);
	  }
	}
	return vectKeys;
}
void StorageAlarmData::updateSummary()
{
	if(lastUpdateSummary < lastUpdateAlarmData)
	{
		lastUpdateSummary = lastUpdateAlarmData;
		for(Summary& summary:vectSummary)
		{
			summary.intTotal = 0;
		    summary.intCleared = 0;
		    summary.intClearedNotClosed = 0;
			summary.intClearedClosed = 0;
			summary.intNotClearedClosed = 0;
			summary.intNotClearedNotClosed = 0;
		}//for

		for (const MAPMAPALARMINFO::value_type& it: m_mapAlarmInfoData)
		{
			for(const MAPALARMINFO::value_type& itAlarmInfo: it.second)
			{
				for(Summary& summary:vectSummary)
				{
					if(itAlarmInfo.second.severity == summary.severity)
					{
						summary.intTotal++;
						if(AlarmState::CLEAR == itAlarmInfo.second.state) summary.intCleared++;
					}
				}//for
			}//for
		}//for
	}//if
}
void StorageAlarmData::loadAlarmProfile()
{
	logInfo(ALARM,"DUMP","loadAlarmProfile---------------------------------");
	SAFplus::Checkpoint::Iterator ibegin = m_checkpointProfile.begin();
	SAFplus::Checkpoint::Iterator iend = m_checkpointProfile.end();
	for(SAFplus::Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
	{
		BufferPtr curkey = iter->first;
		if (curkey)
		{
			logInfo(ALARM,"DUMP","key name [%s]", curkey->data);
		}
		BufferPtr& curval = iter->second;
		if (curval)
		{
			AlarmProfileInfo *data = (AlarmProfileInfo *) curval->data;
			AlarmKey key(curkey->data);
			m_ProfileInfoData[key] = *data;
		}
	}
}
void StorageAlarmData::loadAlarmData()
{
	logInfo(ALARM,"DUMP","loadAlarmData---------------------------------");
	SAFplus::Checkpoint::Iterator ibegin = m_checkpointAlarm.begin();
	SAFplus::Checkpoint::Iterator iend = m_checkpointAlarm.end();
	for(SAFplus::Checkpoint::Iterator iter = ibegin; iter != iend; iter++)
	{
		BufferPtr curkey = iter->first;
		if (curkey)
		{
			logInfo(ALARM,"DUMP","key name [%s]", curkey->data);
		}
		BufferPtr& curval = iter->second;
		if (curval)
		{
			AlarmInfo *data = (AlarmInfo*) curval->data;
			AlarmKey key(curkey->data);
			m_mapAlarmInfoData[key.tuple.get<0>()][key] = *data;
		}
	}
}
