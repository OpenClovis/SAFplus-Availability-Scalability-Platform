################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python cor.py <args>
# create corMetaStruct.c
################################################################################
import sys
import string
import xml.dom.minidom
from string import Template
import time
import getpass
import snmpTemplate

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
			moIdString = snmpTemplate.moTemplate.safe_substitute(classid = resNames[length])
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
		moMap[name] = snmpTemplate.moTemplate.safe_substitute(classid = string.upper(res.attributes["name"].value) )
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
		serviceString = snmpTemplate.serviceIdTemplate.safe_substitute(serviceMap)
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
	serviceString = snmpTemplate.serviceIdTemplate.safe_substitute(serviceMap)
	result += processAttributeList(filteredAttrList, moName, moString, serviceString)
	return result
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
		elif len(parentList) == 1:
			pathList = []
			depthList = []
			pathList = getMOPaths(res, "", pathList, chassisList)
			moStringList = processMOPaths(pathList)
			moString = moStringList[0]
			depth = getDepths(res, 0, depthList, chassisList) [0]
			result = processResourceAttributes(res, moString, result, depth, res.attributes["name"].value)
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
			result += snmpTemplate.oidTemplate.safe_substitute(oid = attrOid)
			result += moString + serviceString
			attr = string.upper(moName) + "_" + string.upper(attr.attributes["name"].value)
			result += snmpTemplate.AttributeIdTemplate.safe_substitute(attrId = attr)
			result += snmpTemplate.AttributeEndString
	return result
	
def isSNMPSubAgentPresent(cmpList):		
	result = "-1"
	for cmp in cmpList:
		try:
			if cmp.attributes["isSNMPSubAgent"].value == "true":
				cpmName = cmp.attributes["name"].value
				result = "1"
		except:
			result = "-1"
	return result
#------------------------------------------------------------------------------
#Script Execution Starts Here.
#Resource Tree information


def mainMethod(resourceDoc, componentDoc):
	headerMap = dict();
	mapTableEntries = ""
	symbolMap = dict()	
	
	global resourceList
	global componentList
	global chassisList
	
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	componentList = componentInfoObj[0].getElementsByTagName("safComponent");
	#identify SAF component with SNMP subagent set to true
	snmpSubAgent = isSNMPSubAgentPresent(componentList)
	

	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	resourceList = resourceInfoObj[0].getElementsByTagName("chassisResource") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource") + resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("mibResource") + resourceInfoObj[0].getElementsByTagName("systemController")
	chassisList = resourceInfoObj[0].getElementsByTagName("chassisResource")
	
	moMap = createMoMap(resourceList)
	srcFileContents = snmpTemplate.headerTemplate.safe_substitute(headerMap)
	
	mapTableEntries += "\n /********************* Resource entries START *****************/"
	for res in resourceList:
		parentList = getParents(res)
		if len(parentList) > 1:
			pathList = []
			depthList = []
			pathList = getMOPaths(res, "", pathList, chassisList)
			moStringList = processMOPaths(pathList)
			for i in range(len(moStringList)):
				path = moStringList[i]
				path = "{" + path + "}"
				symbolMap['moPath'] = path
				symbolMap['depth'] = getDepths(res, 0, depthList, chassisList)[i]
				mapTableEntries += snmpTemplate.imTableEntryTemplate.safe_substitute(symbolMap)
				
		elif len(parentList) == 1:
						
			pathList = []
			depthList = []
			pathList = getMOPaths(res, "", pathList, chassisList)

			
			moStringList = processMOPaths(pathList)
			path = moStringList[0]
			path = "{" + path + "}"
			symbolMap['moPath'] = path
			symbolMap['depth'] = getDepths(res, 0, depthList, chassisList)[0]
			mapTableEntries += snmpTemplate.imTableEntryTemplate.safe_substitute(symbolMap)
	#---------------------------		
	
	
	mapTableEntries += " /********************* Resource entries END *****************/ \n"
	
	#------------ Removing Component Table Entries. It will be a static code ---------------
	
	mapTableEntries += "\n /********************* Alarm entries START *****************/"		
	for res in resourceList:
		if isAlarmEnabled(res):
			parentList = getParents(res)
			if len(parentList) > 1:
				pathList = []
				depthList = []
				pathList = getMOPaths(res, "", pathList, chassisList)
				moStringList = processMOPaths(pathList)
				for i in range(len(moStringList)):
					path = moStringList[i]
					path = "{" + path + "}"
					symbolMap['moPath'] = path
					symbolMap['depth'] = getDepths(res, 0, depthList, chassisList)[i]
					mapTableEntries += snmpTemplate.alarmTableEntryTemplate.safe_substitute(symbolMap)
			else:
				pathList = []
				depthList = []
				pathList = getMOPaths(res, "", pathList, chassisList)
				moStringList = processMOPaths(pathList)
				try:
					path = moStringList[0]
					path = "{" + path + "}"
					symbolMap['moPath'] = path
					symbolMap['depth'] = getDepths(res, 0, depthList, chassisList)[0]
				except:
					print "Resource '" + res.attributes["name"].value + "' is not contained in any other resource"
				mapTableEntries += snmpTemplate.alarmTableEntryTemplate.safe_substitute(symbolMap)
	mapTableEntries += " /********************* Alarm entries END *****************/ \n"
	

	
	srcFileContents += snmpTemplate.oidMapTableTemplate.safe_substitute(oidMapTableEntries = mapTableEntries)
	
	#Now generated in clCompSNMPConfig.py if there is an snmp subagent present
	
	if snmpSubAgent != "1":
		srcFileContents += processResourceList(resourceList)
		srcFileContents += processInheritedResourceList(resourceList)
				
	#srcFileContents += snmpTemplate.staticComponentTableEntries.safe_substitute()
	srcFileContents += snmpTemplate.StructEndString
	srcFileContents += snmpTemplate.trapOidMapTableTemplate.safe_substitute()
	
	
	out_file = open("clSNMPConfig.c","w")
	out_file.write(srcFileContents)
	out_file.close()
