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
def remove3rdpartyComponents(compList):
	newList = list()
	for comp in compList:
		if comp.attributes.has_key("is3rdpartyComponent"):
			is3rdparty = comp.attributes["is3rdpartyComponent"].value
			if is3rdparty == "false":
				newList.append(comp)
		else:
			newList.append(comp)
	return newList
				
#-----------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processComponentList(compList, resMap, alarmMap, associatedResListMap, associatedAlarmListMap, alarmRuleResMap, alarmAssociationMap):
	# This method needs to be cleaned
	pollingTime = 100
	for comp in compList:
		result = ""
		compName = comp.attributes["name"].value
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
				alarmAttributeList = list()
				alarmSeverityMap = dict()
				if alarmAssociationMap.has_key(resName):
					alarmRes = alarmAssociationMap[resName]
					alarmAttributeList = alarmRes.getElementsByTagName("attribute")
					if len(alarmAttributeList) > 0:
						alarmSeverityMap = getAlarmSeverityMap(alarmAttributeList)
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
							specificProblem = alarmProfile.attributes["SpecificProblem"].value
							alarmObj = ""
							try:
								alarmObj = alarmIdMap[alarmid]
							except:
								pass
							
							genRule = ""	
							genRuleIDs = list()
							try:
								genRule = alarmObj.getElementsByTagName("generationRule")
								if len(genRule) > 0:
									genRuleIDs = genRule[0].getElementsByTagName("alarmIDs")	
							except:
								pass
							ruleIDs = ""
							if len(genRuleIDs) > 0:
								alarmRulesTables += createGenRuleTable(genRule[0].attributes["relationType"].value, genRuleIDs, resName + alarmProfileName + specificProblem, alarmMap)

							suppRule = ""
							suppRuleIDs = list()
							try:
								suppRule = alarmObj.getElementsByTagName("suppressionRule")
								if len(suppRule) > 0:
									suppRuleIDs = suppRule[0].getElementsByTagName("alarmIDs")
							except:
								pass
							ruleIDs = ""				
							if len(suppRuleIDs) > 0:
								alarmRulesTables += createSuppRuleTable(suppRule[0].attributes["relationType"].value, suppRuleIDs, resName + alarmProfileName + specificProblem, alarmMap)
							elif len(suppRuleIDs) < 1:
								if len(alarmAttributeList) > 0:
									if alarmSeverityMap.has_key(alarmid):
										suppRuleIDs = getAlarmsForSuppRule(alarmAttributeList, alarmid, alarmSeverityMap[alarmid])
									if len(suppRuleIDs) > 0:
										alarmRulesTables += createDefaultSuppRuleTable(resName + alarmProfileName + specificProblem, alarmMap, suppRuleIDs)		 
								
							alarmProfiles += createAlarmProfileDef(alarmProfile, resName, genRuleIDs, suppRuleIDs)
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
		if len(associatedAlarmList) > 0:
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
                	specificProblem = alarmProfile.attributes["SpecificProblem"].value
                	idMap["ruleID"] = probCause
                	idMap["specificProblem"] = specificProblem
                	result += alarmprofilesTemplate.ruleIDTemplate.safe_substitute(idMap)
	for n in range((4-len(ids))):
		idMap = dict();
		idMap["ruleID"] = "CL_ALARM_ID_INVALID"
		idMap["specificProblem"] = "0"
		result += alarmprofilesTemplate.ruleIDTemplate.safe_substitute(idMap)
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createGenRuleTable(relationName, ids, name, alarmMap):
	result = ""
	ruleIDs = createRuleIDsTable(ids, alarmMap)
        ruleMap = dict();
        ruleMap["ruleIDsTableName"] = name + "GenRuleAlarmIds"
        ruleMap["genRuleRelationName"] = relationName
        ruleMap["ruleIDs"] = ruleIDs
        result += alarmprofilesTemplate.ruleIDTableTemplate.safe_substitute(ruleMap)
        ruleMap["genRules"] = alarmprofilesTemplate.generationRuleTemplate.safe_substitute(ruleMap)
        ruleMap["genRuleTableName"] = name + "GenRule"
        result += alarmprofilesTemplate.generationRuleTableTemplate.safe_substitute(ruleMap)

	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createSuppRuleTable(relationName, ids, name, alarmMap):
	result = ""
        ruleIDs = createRuleIDsTable(ids, alarmMap)
        ruleMap = dict();
        ruleMap["ruleIDsTableName"] = name + "SuppressRuleAlarmIds"
        ruleMap["genRuleRelationName"] = relationName
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
def createAlarmProfileDef(alarmProfile, resName, genRuleIDs, suppRuleIDs):
	alarmMap = dict();
	alarmMap["category"] = alarmProfile.attributes["Category"].value
	alarmMap["severity"] = alarmProfile.attributes["Severity"].value
	alarmMap["assertSoakingTime"] = alarmProfile.attributes["AssertSoakingTime"].value
	alarmMap["clearSoakingTime"] = alarmProfile.attributes["ClearSoakingTime"].value
	probableCauseName = alarmProfile.attributes["ProbableCause"].value
	specificProblem = alarmProfile.attributes["SpecificProblem"].value
	alarmMap["probableCauseName"] = probableCauseName.upper()
	if len(genRuleIDs) > 0:
		alarmMap["genRuleTableName"] = "Cl" + resName + probableCauseName + specificProblem + "GenRule"
	else:
		alarmMap["genRuleTableName"] = "NULL"
	if len(suppRuleIDs) > 0:
		alarmMap["suppressRuleTableName"] = "Cl" + resName + probableCauseName + specificProblem + "SuppressRule" 
	else:
		alarmMap["suppressRuleTableName"] = "NULL"
	alarmMap["specificProblem"] = specificProblem
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
# Creates hash map - resoure name to resource object
def getResMap(alarmAssociationList):
	alarmAssociationMap = dict()
	for resource in alarmAssociationList:
		resName = resource.attributes["name"].value
		alarmAssociationMap[resName] = resource;
	return alarmAssociationMap

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
# Gets alarms list for default supp rule
def getAlarmsForSuppRule(attributeList, alarm, severity):
	suppAlarms = list()
	for attribute in attributeList:
		alarmList = attribute.getElementsByTagName("alarm")
		for alarm in alarmList:
			alarmID = alarm.attributes["alarmID"].value
			if alarm != alarmID:
				if severity < getNumericValueForSeverity(alarm.attributes["severity"].value):
					suppAlarms.append(alarmID) 
	return suppAlarms

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Creates alarm severity map
def getAlarmSeverityMap(attributeList):
	alarmSeverityMap = dict()
	for attribute in attributeList:
		alarmList = attribute.getElementsByTagName("alarm")
		for alarm in alarmList:
			alarmSeverityMap[alarm.attributes["alarmID"].value] = getNumericValueForSeverity(alarm.attributes["severity"].value)
	return alarmSeverityMap
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Gets the numeric value for severity
def getNumericValueForSeverity(severity):
	if severity == "Critical":
		return 4;
	elif severity == "Major":
		return 3;
	elif severity == "Minor":
		return 2;
	elif severity == "Warning":
		return 1;
	return 0;
	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createDefaultSuppRuleTable(name, alarmMap, ids):
	result = ""
        ruleIDs = createDefaultSuppRuleIDsTable(ids, alarmMap)
        ruleMap = dict();
        ruleMap["ruleIDsTableName"] = name + "SuppressRuleAlarmIds"
        ruleMap["genRuleRelationName"] = "CL_ALARM_RULE_LOGICAL_OR"
        ruleMap["ruleIDs"] = ruleIDs
        result += alarmprofilesTemplate.ruleIDTableTemplate.safe_substitute(ruleMap)
        ruleMap["genRules"] = alarmprofilesTemplate.generationRuleTemplate.safe_substitute(ruleMap)
        ruleMap["genRuleTableName"] = name +"SuppressRule"
        result += alarmprofilesTemplate.generationRuleTableTemplate.safe_substitute(ruleMap)
	
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createDefaultSuppRuleIDsTable(ids, alarmMap):
	#This method needs to be cleaned
	result = ""
        if len(ids) > 0:
		for id in ids:
                	idMap = dict();
                	alarmProfile = alarmMap[id]
                	probCause = alarmProfile.attributes["ProbableCause"].value
                	specificProblem = alarmProfile.attributes["SpecificProblem"].value
                	idMap["ruleID"] = probCause
                	idMap["specificProblem"] = specificProblem
                	result += alarmprofilesTemplate.ruleIDTemplate.safe_substitute(idMap)
	for n in range((4-len(ids))):
		idMap = dict();
		idMap["ruleID"] = "CL_ALARM_ID_INVALID"
		idMap["specificProblem"] = "0"
		result += alarmprofilesTemplate.ruleIDTemplate.safe_substitute(idMap)
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Script Execution Starts Here.
def mainMethod(resourceDoc, componentDoc, alarmProfileDoc, associatedResDoc, associatedAlarmDoc, alarmRuleDoc, alarmAssociationDoc):
		
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	componentList = componentInfoObj[0].getElementsByTagName("component") + componentInfoObj[0].getElementsByTagName("safComponent")
	componentList = remove3rdpartyComponents(componentList)
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
	alarmAssociationMap = dict()
	if(None != alarmAssociationDoc):
		alarmAssociationList = alarmAssociationDoc.getElementsByTagName("resource")
		alarmAssociationMap = getResMap(alarmAssociationList)
	resourceMap = processResourceList(resourceList)
	alarmProfileMap = processAlarmProfileList(alarmProfileList)
	result  = processComponentList(componentList, resourceMap, alarmProfileMap, associatedResListMap, associatedAlarmListMap, alarmRuleResMap, alarmAssociationMap)	
	


