################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python clCompSNMPConfig.py <args>
# create clSNMPConfig.c
################################################################################
import sys
import string
import xml.dom.minidom
from string import Template
import os
import time
import getpass
import shutil
from default import clCompSNMPConfigTemplate


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getBaseClasses(resource):	
	baseClasses = []
	inheritenceList = resource.getElementsByTagName("inherits")
	if len(inheritenceList)>0 :
		for inherits in inheritenceList:
			baseClassName = inherits.attributes["target"].value
			baseClass = getResourceFromName(baseClassName)
			if baseClass != None:
				baseClasses.append(baseClass)			
	return baseClasses
	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getParents(resource):	
	resKey = resource.attributes["rdn"].value
	parents = []
	
	for res in resourceList:
		compositionList = res.getElementsByTagName("contains")
		if len(compositionList)>0 and res!=resource :
			for contains in compositionList :
				resName = contains.attributes["target"].value
				if resName == resource.attributes["name"].value :
					parents.append(res)							
					break
	return parents

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getResourceFromName(resName) :
	for res in resourceList :
		if res.attributes["name"].value == resName :
			return res
	return None		
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getResourceFromKey(key):	
	for res in resourceList:				
		resKey = res.attributes["rdn"].value;		
		if resKey == key:
			return res
			
	return None

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getDepths(resource, depth, depthList, chassisList):
	depth += 1
	if resource in chassisList:
		depthList.append(depth)
	parents = getParents(resource)
	for i in range(len(parents)):
		parent = parents[i]
		getDepths(parent, depth, depthList, chassisList)	   
	return depthList		

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getMOPaths(resource, parentPath, moPathList, chassisList):
	resName = resource.attributes["name"].value
	parentPath +=  string.upper(resName) + "#"
	if resource in chassisList:
		moPathList.append(parentPath)
	parents = getParents(resource)
	for i in range(len(parents)):
		parent = parents[i]
		getMOPaths(parent, parentPath, moPathList, chassisList)	   
	return moPathList

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processMOPaths(moPathList):
	moStringList = []
	for moPath in moPathList:
		resNames = moPath.split("#")
		moString = ""
		length = len(resNames)
		length = length - 2 # to avoid the last blank value returned from split
		moString = ""
		while length >= 0:
			moIdString = clCompSNMPConfigTemplate.moTemplate.safe_substitute(classid = resNames[length])
			moString += moIdString
			if length != 0:
				moString += ", "
			length = length - 1
		moStringList.append(moString)
	return moStringList
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def isAlarmEnabled(resource):
	try:
		alarmElements=resource.getElementsByTagName("alarmManagement")		
		if len(alarmElements) > 0:
			if alarmElements[0].attributes["isEnabled"].value == "true":				
				return True
	except:
		print ""
	return False

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def isProvEnabled(resource):
	try:
		provElements=resource.getElementsByTagName("provisioning")	
		if len(provElements) >0:
			if provElements[0].attributes["isEnabled"].value == "true":				
				return True
	except:
		print ""
	return False
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createMoMap(resList):
	moMap = dict();
	for res  in resList:
		name = res.attributes["name"].value
		moMap[name] = clCompSNMPConfigTemplate.moTemplate.safe_substitute(classid = string.upper(res.attributes["name"].value) )
	return moMap
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processResourceAttributes(res, moString, result, depth, moName): # moName will not be same as name of res in case of Inheritence RelationShip
	provAttrList = []
	depthList = []
	serviceMap = dict();
	if isProvEnabled(res):
		provList = res.getElementsByTagName("provisioning")
		serviceMap["serviceId"] = "CL_COR_SVC_ID_PROVISIONING_MANAGEMENT"
		serviceMap["depth"] = depth
		serviceString = clCompSNMPConfigTemplate.serviceIdTemplate.safe_substitute(serviceMap)
		for prov in provList:
			provAttrList = prov.getElementsByTagName("attribute")
			result += processAttributeList(provAttrList, moName, moString, serviceString)
	moAttributeList = res.getElementsByTagName("attribute")
	filteredAttrList = []
	provList = res.getElementsByTagName("provisioning")
	if len(provList) > 0:
		provAttrList = provList[0].getElementsByTagName("attribute")
	for moAttribute in moAttributeList:
		if moAttribute not in provAttrList:
			filteredAttrList.append(moAttribute)
	serviceMap["serviceId"] = "CL_COR_INVALID_SVC_ID"
	serviceMap["depth"] = depth
	serviceString = clCompSNMPConfigTemplate.serviceIdTemplate.safe_substitute(serviceMap)
	result += processAttributeList(filteredAttrList, moName, moString, serviceString)
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processResourceAlarms(res, moString, depth, moName): # moName will not be same as name of res in case of Inheritence RelationShip
	global associatedAlarmListMap
	serviceMap = dict();
	if isAlarmEnabled(res):
		serviceMap["serviceId"] = "CL_COR_SVC_ID_ALARM_MANAGEMENT"
		serviceMap["depth"] = depth
		serviceString = clCompSNMPConfigTemplate.serviceIdTemplate.safe_substitute(serviceMap)
		serviceString = serviceString.replace("{","")
		associatedAlarmList = associatedAlarmListMap.get(moName)
		if associatedAlarmList != None:
			processAlarmList(associatedAlarmList, moName, moString, serviceString)

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processResourceList(resList):
	result = ""
	depthList = []
	for res  in resList:
		parentList = getParents(res)
		if len(parentList) > 1:
			pathList = []
			depthList = []
			pathList = getMOPaths(res, "", pathList, chassisList)
			moStringList = processMOPaths(pathList)
			for i in range(len(moStringList)):
				moString = moStringList[i]
				depth = getDepths(res, 0, depthList, chassisList) [i]
				result = processResourceAttributes(res, moString, result, depth, res.attributes["name"].value)
				processResourceAlarms(res, moString, depth, res.attributes["name"].value)
		elif len(parentList) == 1:
			pathList = []
			depthList = []
			pathList = getMOPaths(res, "", pathList, chassisList)
			moStringList = processMOPaths(pathList)
			moString = moStringList[0]
			depth = getDepths(res, 0, depthList, chassisList) [0]
			result = processResourceAttributes(res, moString, result, depth, res.attributes["name"].value)
			processResourceAlarms(res, moString, depth, res.attributes["name"].value)
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processInheritedResourceList(resList):
	result = ""
	depthList = []
	for res  in resList:
		baseClasses = getBaseClasses(res)
		if len(baseClasses) > 0:
			for baseResource in baseClasses:
				parentList = getParents(res)
				if len(parentList) > 1:
					pathList = []
					depthList = []
					pathList = getMOPaths(res, "", pathList, chassisList)
					moStringList = processMOPaths(pathList)
					for i in range(len(moStringList)):
						moString = moStringList[i]
						depth = getDepths(res, 0, depthList, chassisList) [i]
						result = processResourceAttributes(baseResource, moString, result, depth, res.attributes["name"].value)
				elif len(parentList) == 1:
					pathList = []
					depthList = []
					pathList = getMOPaths(res, "", pathList, chassisList)
					moStringList = processMOPaths(pathList)
					moString = moStringList[0]
					depth = getDepths(res, 0, depthList, chassisList) [0]
					result = processResourceAttributes(baseResource, moString, result, depth, res.attributes["name"].value)
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processAttributeList(attrList, moName, moString, serviceString):
	result = ""
	for attr  in attrList:
		attrOid   = attr.attributes["OID"].value
		if attrOid != "":
			result += clCompSNMPConfigTemplate.oidTemplate.safe_substitute(oid = attrOid)
			result += moString + serviceString
			attr = string.upper(moName) + "_" + string.upper(attr.attributes["name"].value)
			result += clCompSNMPConfigTemplate.AttributeIdTemplate.safe_substitute(attrId = attr)
			result += clCompSNMPConfigTemplate.AttributeEndString
	return result
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processAlarmList(alarmList, moName, moString, serviceString):
	global alarmEntryString
	global alarmProfileMap
	emptyAttributeTag = "{CL_MED_ATTR_VALUE_END}, "
	for alarm  in alarmList:
		try:
			alarmProfile = alarmProfileMap[alarm]
		except:
			alarmProfile = None
		if alarmProfile != None:
			alarmPC = alarmProfile.attributes["ProbableCause"].value
			try:
				alarmOid = alarmProfile.attributes["OID"].value
			except:
				alarmOid = ""
			if alarmOid != "":
				alarmEntryString += clCompSNMPConfigTemplate.oidTemplate.safe_substitute(oid = alarmOid)
				alarmEntryString += moString + serviceString + emptyAttributeTag
				alarmEntryString += clCompSNMPConfigTemplate.probableCauseTemplate.safe_substitute(probCause = alarmPC)
#------------------------------------------------------------------------------
#-----------------------------------------------------------------------
def processComponent(cmpList):		
	result = "-1"
	try:
		for cmp in cmpList:
			if cmp.attributes["isSNMPSubAgent"].value == "true":
				cpmName = cmp.attributes["name"].value
				cpmName = string.replace(cpmName, " ", "_")
				cpmName = string.replace(cpmName, "-", "_")
				result = cpmName
	except:
		result = "-1"				
	return result  
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processAlarmProfileList(alarmList):
	alarmMap = dict();
        for alarm in alarmList:
		alarmId = alarm.attributes["alarmID"].value
		alarmMap[alarmId] = alarm;
	return alarmMap
		    
#------------------------------------------------------------------------------
def writeToMapFile(oldFilePath, newFilePath):
	oldNewNameMapFile.write(oldFilePath);
	oldNewNameMapFile.write(",");
	oldNewNameMapFile.write(newFilePath);
	oldNewNameMapFile.write("\n");
		
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

#Script Execution Starts Here.

def mainMethod(resourceDoc, componentDoc, alarmDoc, associatedAlarmDoc, staticDir, templateDir, nameMap, nameMapFile):
	headerMap = dict();
	
	global resourceList
	global componentList
	global chassisList
	global alarmEntryString
	global associatedAlarmListMap
	global alarmProfileMap
	
	global oldNewNameMap
	oldNewNameMap = nameMap
	global oldNewNameMapFile
	oldNewNameMapFile = nameMapFile
		
	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	resourceList = resourceInfoObj[0].getElementsByTagName("chassisResource") + resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("mibResource") + resourceInfoObj[0].getElementsByTagName("systemController") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource")
	chassisList = resourceInfoObj[0].getElementsByTagName("chassisResource")
		
	moMap = createMoMap(resourceList)
	associatedAlarmObj = associatedAlarmDoc.getElementsByTagName("modelMap:mapInformation")
	alarmLinkObjList = associatedAlarmObj[0].getElementsByTagName("link")
	import alarmprofiles
	associatedAlarmListMap = alarmprofiles.getAssociatedAlarmListMap(alarmLinkObjList, resourceList)
	alarmProfileList = []
	alarmEntryString = ""
	if alarmDoc != None:
		alarmInfoObj = alarmDoc.getElementsByTagName("alarm:alarmInformation")
		alarmProfileList = alarmInfoObj[0].getElementsByTagName("AlarmProfile")
	alarmProfileMap = processAlarmProfileList(alarmProfileList)
		
	srcFileContents = clCompSNMPConfigTemplate.headerTemplate.safe_substitute(headerMap)
	
	srcFileContents += processResourceList(resourceList)
	srcFileContents += processInheritedResourceList(resourceList)
	srcFileContents += clCompSNMPConfigTemplate.StructEndString
	srcFileContents += clCompSNMPConfigTemplate.trapOidMapTableTemplate.safe_substitute()
	srcFileContents += alarmEntryString
	srcFileContents += clCompSNMPConfigTemplate.TrapEndString
	

	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	componentList = componentInfoObj[0].getElementsByTagName("safComponent");
	#identify SAF component with SNMP subagent set to true
	ComponentName = processComponent(componentList)
	#create cpmName directory, ComponentName/static, ComponentName/mdl_config, ComponentName/mib2c
	if( ComponentName != "-1" ):	
		if os.path.exists(ComponentName) == False:       	    
			os.mkdir(ComponentName)
		if os.path.exists(ComponentName+"/static") == False:
			os.mkdir(ComponentName+"/static")
		try:
			shutil.copyfile(staticDir+"/clSnmpDefs.h", ComponentName+"/static/clSnmpDefs.h")
			shutil.copyfile(staticDir+"/clSnmpInit.c", ComponentName+"/static/clSnmpInit.c")
			shutil.copyfile(staticDir+"/clSnmpLog.c", ComponentName+"/static/clSnmpLog.c")
			shutil.copyfile(staticDir+"/clSnmpLog.h", ComponentName+"/static/clSnmpLog.h")
			shutil.copyfile(staticDir+"/clSnmpOp.c", ComponentName+"/static/clSnmpOp.c")
			shutil.copyfile(staticDir+"/clSnmpOp.h", ComponentName+"/static/clSnmpOp.h")
		except:
			print "Supporting files for subagent were not found in static folder"			
		if os.path.exists(ComponentName+"/mdl_config") == False:
			os.mkdir(ComponentName+"/mdl_config")

		out_file = open(ComponentName + "/mdl_config/" + "clSNMPConfig.c","w")
		out_file.write(srcFileContents)
		out_file.close()		
		if os.path.exists(ComponentName+"/mib2c") == False:
			os.mkdir(ComponentName+"/mib2c")
