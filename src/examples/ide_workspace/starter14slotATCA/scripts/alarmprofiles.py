################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python alarmprofiles.py <args>
# Alarm Profiles Definition for components
################################################################################
import sys
import string
import os
import xml.dom.minidom
from string import Template
import time
import getpass
from default import alarmprofilesTemplate

#-----------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processComponentList(compList, resMap, alarmMap, associatedResListMap, associatedAlarmListMap, alarmRuleResMap):
	# This method needs to be cleaned
	pollingTime = 100
	for comp in compList:
		result = ""
		compName = comp.attributes["name"].value
		isSnmpAgent = comp.attributes["isSNMPSubAgent"].value
		if isSnmpAgent == "true":
			continue;
		compAlarms = ""
		alarmRulesTables =""
		alarmProfilesTables=""
		isBUILD_CPP = "false"
		isAlarmAssociated = "false"
				
		try:
			isBUILD_CPP = comp.attributes["isBuildCPP"].value
		except:
			pass
			
		associatedResList = associatedResListMap[compName] #----------------
		if len(associatedResList) > 0:
			tempAlarmMap = dict();
			for resName in associatedResList:
				res = resMap[resName]
				alarm = res.getElementsByTagName("alarmManagement")
				alarmProfiles = ""
				if len(alarm) > 0 and alarm[0].attributes["isEnabled"].value=="true":
					isAlarmAssociated = "true"
					alarmRuleResObj = []
					alarmIdMap = dict()
					pollingTime = alarm[0].attributes["pollingInterval"].value
					alarmids = associatedAlarmListMap[resName]
					if len(alarmids) > 0:
						try:
							alarmRuleResObj = alarmRuleResMap[resName] 
							alarmIdMap = getAlarmIdMapForRes(alarmRuleResObj)
						except:
							pass
						for alarmid in alarmids:
							alarmProfile = alarmMap[alarmid]
							d = dict();
							alarmProfileName = alarmProfile.attributes["ProbableCause"].value
							alarmObj = ""
							try:
								alarmObj = alarmIdMap[alarmid]
							except:
								pass
							genRule = ""
							try:
								genRule = alarmObj.getElementsByTagName("generationRule")
							except:
								pass
							ruleIDs = ""
							if len(genRule) > 0 and len(genRule[0].getElementsByTagName("alarmIDs")) > 0:
								alarmRulesTables += createGenRuleTable(genRule[0], resName + alarmProfileName, alarmMap)
	
							suppRule = ""
							try:
								suppRule = alarmObj.getElementsByTagName("suppressionRule")
							except:
								pass
							ruleIDs = ""				
							if len(suppRule) > 0 and len(suppRule[0].getElementsByTagName("alarmIDs")) > 0:
								alarmRulesTables += createSuppRuleTable(suppRule[0], resName +alarmProfileName, alarmMap)
							
							alarmProfiles += createAlarmProfileDef(alarmProfile, resName, genRule, suppRule)
				alarmProfilesTables += createAlarmProfileTable(alarmProfiles, resName)
				
				className = "CLASS_" + resName.upper() + "_MO"
				m = dict();
				m["className"] = className
				m["pollingInterval"] = pollingTime
				m["profileTableName"] = resName + "AlmProfile"
				compAlarms += alarmprofilesTemplate.compAlarmTemplate.safe_substitute(m)
		result += createComponentAlarmTable(compAlarms, alarmProfilesTables, alarmRulesTables, compName)
		if os.path.exists(compName) == False:
			os.mkdir(compName)
	
		file = compName + "/" + "cl" + compName + "alarmMetaStruct.c"
		
		file= ""
		cFile = compName + "/" + "cl" + compName + "alarmMetaStruct" + ".c"
		cppFile = compName + "/" + "cl" + compName + "alarmMetaStruct" + ".C"
			
		if isBUILD_CPP=="true":
			file = cppFile	
			if os.path.isfile(cFile):
				os.remove(cFile)
		else:
			file = cFile
			if os.path.isfile(cppFile):
				os.remove(cppFile)
		if isAlarmAssociated == "true":			
			out_file = open(file, "w")
			out_file.write(result)
			out_file.close()
		
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getAssociatedResListMap(linkObjList, componentList):
	associtedResObjList = []
	associatedResListMap = dict()
	
	for comp in componentList :
		compName = comp.attributes["name"].value
		associatedResListMap[compName] = []
			
	for link in linkObjList :
		if link.attributes["linkType"].value == "associatedResource" :
			associtedResObjList = link
			break	
	linkDetailList = associtedResObjList.getElementsByTagName("linkDetail")
	for linkDetail in linkDetailList :
		linkSourceName = linkDetail.attributes["linkSource"].value
		linkTargetsList = linkDetail.getElementsByTagName("linkTarget")
		associatedResList = []
		for linkTargets in linkTargetsList :
			associatedResList.append(linkTargets.firstChild.data)
		associatedResListMap[linkSourceName] = associatedResList
	return associatedResListMap	
			
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getAssociatedAlarmListMap(linkObjList, resourceList):
	associtedAlarmObjList = []
	associatedAlarmListMap = dict()
	
	for res in resourceList :
		resName = res.attributes["name"].value
		associatedAlarmListMap[resName] = []
	
	for link in linkObjList :
		if link.attributes["linkType"].value == "associatedAlarm" :
			associtedAlarmObjList = link
			break	
	linkDetailList = associtedAlarmObjList.getElementsByTagName("linkDetail")
	for linkDetail in linkDetailList :
		linkSourceName = linkDetail.attributes["linkSource"].value
		linkTargetsList = linkDetail.getElementsByTagName("linkTarget")
		associatedAlarmList = []
		for linkTargets in linkTargetsList :
			associatedAlarmList.append(linkTargets.firstChild.data)
		associatedAlarmListMap[linkSourceName] = associatedAlarmList
	return associatedAlarmListMap	
					
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createRuleIDsTable(ids, alarmMap):
	#This method needs to be cleaned
	result = ""
        if len(ids) > 0:
		for id in ids:
                	idMap = dict();
                	alarmProfile = alarmMap[id.firstChild.data]
                	probCause = alarmProfile.attributes["ProbableCause"].value
                	idMap["ruleID"] = probCause
                	result += alarmprofilesTemplate.ruleIDTemplate.safe_substitute(idMap)
	for n in range((4-len(ids))):
		idMap = dict();
		idMap["ruleID"] = "CL_ALARM_ID_INVALID"
		result += alarmprofilesTemplate.ruleIDTemplate.safe_substitute(idMap)
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createGenRuleTable(genRule, name, alarmMap):
	result = ""
	ids = genRule.getElementsByTagName("alarmIDs")
        ruleIDs = createRuleIDsTable(ids, alarmMap)
        ruleMap = dict();
        ruleMap["ruleIDsTableName"] = name + "GenRuleAlarmIds"
        ruleMap["genRuleRelationName"] = genRule.attributes["relationType"].value
        ruleMap["ruleIDs"] = ruleIDs
        result += alarmprofilesTemplate.ruleIDTableTemplate.safe_substitute(ruleMap)
        ruleMap["genRules"] = alarmprofilesTemplate.generationRuleTemplate.safe_substitute(ruleMap)
        ruleMap["genRuleTableName"] = name + "GenRule"
        result += alarmprofilesTemplate.generationRuleTableTemplate.safe_substitute(ruleMap)

	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createSuppRuleTable(suppRule, name, alarmMap):
	result = ""
        ids = suppRule.getElementsByTagName("alarmIDs")
        ruleIDs = createRuleIDsTable(ids, alarmMap)
        ruleMap = dict();
        ruleMap["ruleIDsTableName"] = name + "SuppressRuleAlarmIds"
        ruleMap["genRuleRelationName"] = suppRule.attributes["relationType"].value
        ruleMap["ruleIDs"] = ruleIDs
        result += alarmprofilesTemplate.ruleIDTableTemplate.safe_substitute(ruleMap)
        ruleMap["genRules"] = alarmprofilesTemplate.generationRuleTemplate.safe_substitute(ruleMap)
        ruleMap["genRuleTableName"] = name +"SuppressRule"
        result += alarmprofilesTemplate.generationRuleTableTemplate.safe_substitute(ruleMap)
	
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createComponentAlarmTable(compAlarms, alarmProfilesTables, alarmRules, name):
	map = dict();
	map["compAlarms"] = compAlarms
	map["alarmRulesTables"] = alarmRules
	map["alarmProfilesTables"] = alarmProfilesTables
	map["compName"] = name
	result = alarmprofilesTemplate.mainTemplate.safe_substitute(map)
	return result 
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createAlarmProfileTable(alarmProfiles, tableName):
	tableMap = dict();
	tableMap["alarmProfiles"] = alarmProfiles
	tableMap["profileTableName"] = tableName + "AlmProfile"
	result = alarmprofilesTemplate.alarmProfileTableTemplate.safe_substitute(tableMap)
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createAlarmProfileDef(alarmProfile, resName, genRule, suppRule):
	alarmMap = dict();
	alarmMap["category"] = alarmProfile.attributes["Category"].value
	alarmMap["severity"] = alarmProfile.attributes["Severity"].value
	alarmMap["assertSoakingTime"] = alarmProfile.attributes["AssertSoakingTime"].value
	alarmMap["clearSoakingTime"] = alarmProfile.attributes["ClearSoakingTime"].value
	probableCauseName = alarmProfile.attributes["ProbableCause"].value
	alarmMap["probableCauseName"] = probableCauseName.upper()
	if len(genRule) > 0 and len(genRule[0].getElementsByTagName("alarmIDs")) > 0:
		alarmMap["genRuleTableName"] = "Cl" + resName + alarmProfile.attributes["ProbableCause"].value + "GenRule"
	else:
		alarmMap["genRuleTableName"] = "NULL"
	if len(suppRule) > 0 and len(suppRule[0].getElementsByTagName("alarmIDs")) > 0:
		alarmMap["suppressRuleTableName"] = "Cl" + resName + alarmProfile.attributes["ProbableCause"].value +"SuppressRule" 
	else:
		alarmMap["suppressRuleTableName"] = "NULL"
	result = alarmprofilesTemplate.alarmProfileTemplate.safe_substitute(alarmMap)

	return result	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processResourceList(resList):
	resMap = dict();
	for res in resList:
		resKey = res.attributes["name"].value
		resMap[resKey] = res;
	return resMap
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processAlarmProfileList(alarmList):
	alarmMap = dict();
        for alarm in alarmList:
		uniqueName = alarm.attributes["alarmID"].value
		alarmMap[uniqueName] = alarm;
	return alarmMap
	
	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
# Creates hash map - resource name to resource object for alarm rule
def getAlarmRuleResMap(alarmRuleResObjList):
	alarmRuleResMap = dict()
	for res in alarmRuleResObjList:
		resKey = res.attributes["name"].value
		alarmRuleResMap[resKey] = res;
	return alarmRuleResMap

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
# Creates hash map - alarm Id to alarm object for alarm rule
def getAlarmIdMapForRes(alarmRuleResObj):
	alarmRuleResMap = dict()
	alarmList = alarmRuleResObj.getElementsByTagName("alarm");
	for alarm in alarmList:
		alarmID = alarm.attributes["alarmID"].value
		alarmRuleResMap[alarmID] = alarm;
	return alarmRuleResMap

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Script Execution Starts Here.
def mainMethod(resourceDoc, componentDoc, alarmProfileDoc, associatedResDoc, associatedAlarmDoc, alarmRuleDoc):
		
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	componentList = componentInfoObj[0].getElementsByTagName("component") + componentInfoObj[0].getElementsByTagName("safComponent")
	resourceList = resourceInfoObj[0].getElementsByTagName("softwareResource")+resourceInfoObj[0].getElementsByTagName("mibResource")+resourceInfoObj[0].getElementsByTagName("hardwareResource")+resourceInfoObj[0].getElementsByTagName("nodeHardwareResource")+resourceInfoObj[0].getElementsByTagName("systemController")+resourceInfoObj[0].getElementsByTagName("chassisResource")
	alarmInfoObj = alarmProfileDoc.getElementsByTagName("alarm:alarmInformation")
	alarmProfileList = alarmInfoObj[0].getElementsByTagName("AlarmProfile")
	
	associatedResObj = associatedResDoc.getElementsByTagName("modelMap:mapInformation")
	resLinkObjList = associatedResObj[0].getElementsByTagName("link")
	associatedResListMap = getAssociatedResListMap(resLinkObjList, componentList)
		
	associatedAlarmObj = associatedAlarmDoc.getElementsByTagName("modelMap:mapInformation")
	alarmLinkObjList = associatedAlarmObj[0].getElementsByTagName("link")
	associatedAlarmListMap = getAssociatedAlarmListMap(alarmLinkObjList, resourceList)
	
	alarmRuleResMap = dict()	
	if(None != alarmRuleDoc):
		alarmRuleObj = alarmRuleDoc.getElementsByTagName("alarmRule:alarmRuleInformation")
		alarmRuleResObjList = alarmRuleObj[0].getElementsByTagName("resource")
		alarmRuleResMap = getAlarmRuleResMap(alarmRuleResObjList)
	
	resourceMap = processResourceList(resourceList)
	alarmProfileMap = processAlarmProfileList(alarmProfileList)
	result  = processComponentList(componentList, resourceMap, alarmProfileMap, associatedResListMap, associatedAlarmListMap, alarmRuleResMap)	
	



