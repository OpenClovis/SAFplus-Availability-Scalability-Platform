# ModuleName  : plugins
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python cor.py <args>
# create corMetaStruct.c
#
# Copyright (c) 2002-2008 OpenClovis 
#
import os
import sys
import string
import xml.dom.minidom
from string import Template
import time
import getpass
from templates import corTemplate


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
omClassDataEntries = ""
classDefEntries = ""
moMsoClassDefEntries = ""
simpleAttrDefEntries = ""
arrAttrDefEntries = ""
containedAttrDefEntries = ""
associationAttrDefEntries = ""
msoClassDefEntries = ""


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createOmClassData():
	omEntries=""	
	counter = 9
	for res in resourceList:
		resName = string.upper(res.attributes["name"].value)	
		if isProvEnabled(res):				
			counter = counter + 1
			omClassName="CL_OM_PROV_" + resName + "_CLASS_TYPE"
			omEntries += corTemplate.omDataTemplate.safe_substitute(omClassName=omClassName, omClassId=counter);
		if isAlarmEnabled(res):
			counter = counter + 1
			omClassName="CL_OM_ALARM_" + resName + "_CLASS_TYPE"
			omEntries += corTemplate.omDataTemplate.safe_substitute(omClassName=omClassName, omClassId=counter);
		if isPMEnabled(res):				
			counter = counter + 1
			omClassName="CL_OM_PM_" + resName + "_CLASS_TYPE"
			omEntries += corTemplate.omDataTemplate.safe_substitute(omClassName=omClassName, omClassId=counter);
	return corTemplate.omTemplate.safe_substitute(omClassDataEntries=omEntries);
	

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

def createResourcePaths(res, parentPath):
	if res != None:
		resName = res.attributes["name"].value
		pathList = resourcePathMap.get(resName)
		if pathList == None:
			pathList = []
			resourcePathMap[resName] = pathList
		parentPath += "\\" + resName
		pathList.append(parentPath)
		children = []
		children = getChildren(res)
		for child in children:
			createResourcePaths(child, parentPath);		
			
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

def getChildren(resource):	
	children=[]
	for res in resourceList:
		parents = getParents(res)
		for parent in parents:
			if parent == resource:
				children.append(res);	
		
	return children
	

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def isAlarmEnabled(resource):
	try:
		alarmElements=resource.getElementsByTagName("alarmManagement")		
		if len(alarmElements) > 0:
			if alarmElements[0].attributes["isEnabled"].value == "true":				
				return True
	except:
		return False
	
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
		return False
	
	return False
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def isPMEnabled(resource):
	try:
		pmElements=resource.getElementsByTagName("pm")	
		if len(pmElements) >0:
			if pmElements[0].attributes["isEnabled"].value == "true":				
				return True
	except:
		return False
	
	return False
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def writeMoDefinition(res, definedResList, classDefinitionEntries):
	if res not in definedResList:
		resName = res.attributes["name"].value
		superClassName = getSuperClass(resName)
		if superClassName != "":
			superObj = getResourceFromName(superClassName)
			if superObj not in definedResList:
				classDefinitionEntries = writeMoDefinition(superObj, definedResList, classDefinitionEntries)
		symbolMap['className']=resName
		symbolMap['classRdn']=classIdMap["CLASS_" + resName.upper() + "_MO"]
		superClassName = getSuperClass(res.attributes["name"].value)
		symbolMap['superClassName']=superClassName
		allattributeList = res.getElementsByTagName("attribute")
		provAttributeList = []
		pmAttributeList = []
		moAttributeList = []
		prov = res.getElementsByTagName("provisioning")
		if len(prov) > 0:
			provAttributeList = prov[0].getElementsByTagName("attribute")
		pm = res.getElementsByTagName("pm")
		if len(pm) > 0:
			pmAttributeList = pm[0].getElementsByTagName("attribute")
		for attrib in allattributeList:
			if attrib not in provAttributeList and attrib not in pmAttributeList:
				moAttributeList.append(attrib)
		# create the MO Class definition
		symbolMap['simpleAttrDefEntries'] = createAttributeClassDefinition(resName, moAttributeList)
		symbolMap['arrAttrDefEntries'] = createArrayAttributeClassDefinition(resName, moAttributeList)
		associationMap = createAssocAttributeClassDefinition(res)
		moAttrEntries = associationMap.get("MO_ASSOC_ATTRIBUTES")
		if moAttrEntries == None:
			moAttrEntries = ""
		symbolMap['associationAttrDefEntries'] = moAttrEntries
		symbolMap['containedAttrDefEntries'] = createContAttributeClassDefinition(res)
		classDefinitionEntries += corTemplate.corClassTemplate.safe_substitute(symbolMap)
		
		#create the PROV MSO class definition
		if isProvEnabled(res):
			symbolMap['className']="CLASS_" + resName.upper() + "_PROV_MSO"
			symbolMap['classRdn']=classIdMap[symbolMap['className']]
			symbolMap['superClassName']= ""
			
			symbolMap['simpleAttrDefEntries'] = createAttributeClassDefinition(resName, provAttributeList)
			symbolMap['arrAttrDefEntries'] = createArrayAttributeClassDefinition(resName, provAttributeList)
			msoAttrEntries = associationMap.get("PROV_MSO_ASSOC_ATTRIBUTES")
				
			if msoAttrEntries == None:
				msoAttrEntries = ""
			symbolMap['associationAttrDefEntries'] = msoAttrEntries
			symbolMap['containedAttrDefEntries'] = ""
			classDefinitionEntries += corTemplate.corClassTemplate.safe_substitute(symbolMap)
	
		#create the Alarm MSO class definiton
		if isAlarmEnabled(res):
			symbolMap['msoClassName']="CLASS_" + resName.upper() + "_ALARM_MSO"
			symbolMap['msoClassId']=classIdMap[symbolMap['msoClassName']]
			classDefinitionEntries += corTemplate.alarmMsoClassTemplate.safe_substitute(symbolMap)
			
		if isPMEnabled(res):
			symbolMap['className']="CLASS_" + resName.upper() + "_PM_MSO"
			symbolMap['classRdn']=classIdMap[symbolMap['className']]
			symbolMap['superClassName']= ""
			
			symbolMap['simpleAttrDefEntries'] = createAttributeClassDefinition(resName, pmAttributeList)
			symbolMap['arrAttrDefEntries'] = createArrayAttributeClassDefinition(resName, pmAttributeList)
			msoAttrEntries += associationMap.get("PM_MSO_ASSOC_ATTRIBUTES")
				
			if msoAttrEntries == None:
				msoAttrEntries = ""
			symbolMap['associationAttrDefEntries'] = msoAttrEntries
			symbolMap['containedAttrDefEntries'] = ""
			classDefinitionEntries += corTemplate.corClassTemplate.safe_substitute(symbolMap)
	
		definedResList.append(res)
	return classDefinitionEntries
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createMOClassDefinitions():
	classDefinitionEntries=""
	definedResList = []
	for res in resourceList:
		# This code takes care of writing the MO Class definition in inheritence order
		if res not in definedResList:
			classDefinitionEntries = writeMoDefinition(res, definedResList, classDefinitionEntries)
	return classDefinitionEntries
	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getCummulativeMaxInstances(res, resPath, maxInstances):
	try:
		parents = getParents(res)
		for parent in parents:
			if parent in chassisResourceList:
				return int(maxInstances)
			else:
				pathList = []
				pathList = resourcePathMap.get(parent.attributes["name"].value)
				for path in pathList:
					if resPath.find(path) != -1:
						maxInstances = int(maxInstances) * int(parent.attributes["maxInstances"].value)
						maxInstances = getCummulativeMaxInstances(parent, resPath, maxInstances)
						return int(maxInstances)
	except:
		return -1
	return int(maxInstances)
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createMoMsoTreeDefinitions():
    moMsoClassDefinitionEntries = ""
    resPathList = []
    pathList = []
    # code written to write the MO definition in the order sorted on moPath
    iterator = resourcePathMap.itervalues()
    for pathList in iterator:
    	resPathList.extend(pathList)
    resPathList.sort()
    for resPath in resPathList:
    	list = []
    	list = resPath.split("\\")
    	resName = list[len(list) - 1]
    	res = getResourceFromName(resName)
    	if res != None:
	    	if res in chassisResourceList:
	    		maxInstances = 1
	    	elif res in datastructureList:
	    		maxInstances = 0
	    	else:
	    		maxInstances = res.attributes["maxInstances"].value
	    	maxinst = maxInstances
	    	symbolMap['moPath'] = resPath
	    	symbolMap['maxInst'] = maxinst
	    	symbolMap['msoClassDefEntries'] = createMsoDefinition(res)
	    	moMsoClassDefinitionEntries += corTemplate.corMoClassTemplate.safe_substitute(symbolMap)
    return moMsoClassDefinitionEntries
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createMsoDefinition(res):
	msoClassDefinitionEntries=""
	resName = res.attributes["name"].value
	if isProvEnabled(res):
		symbolMap['msoClassName']= "CLASS_" + resName.upper() + "_PROV_MSO"
		symbolMap['msoType']="CL_COR_SVC_ID_PROVISIONING_MANAGEMENT"
		symbolMap['omClassType']="CL_OM_PROV_" + string.upper(resName) + "_CLASS_TYPE"
		msoClassDefinitionEntries += corTemplate.corMsoClassTemplate.safe_substitute(symbolMap)
	if isAlarmEnabled(res):
		symbolMap['msoClassName']= "CLASS_" + resName.upper() + "_ALARM_MSO"
		symbolMap['msoType']="CL_COR_SVC_ID_ALARM_MANAGEMENT"
		symbolMap['omClassType']="CL_OM_ALARM_" + string.upper(resName) + "_CLASS_TYPE"
		msoClassDefinitionEntries += corTemplate.corMsoClassTemplate.safe_substitute(symbolMap)
	if isPMEnabled(res):
		symbolMap['msoClassName']= "CLASS_" + resName.upper() + "_PM_MSO"
		symbolMap['msoType']="CL_COR_SVC_ID_PM_MANAGEMENT"
		symbolMap['omClassType']="CL_OM_PM_" + string.upper(resName) + "_CLASS_TYPE"
		msoClassDefinitionEntries += corTemplate.corMsoClassTemplate.safe_substitute(symbolMap)
	return msoClassDefinitionEntries
		
		
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getSuperClass(resource):
	global parentMap
	parent = parentMap.get(resource) 
	if parent != None:
		return parent
	else: 
		return ""
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Creates SNMP Type - Clovis Type Map
def createDataTypeMap(doc):
	result = dict();	
	dataTypeList = doc.getElementsByTagName("datatypesmap:Map")
	for dataType in dataTypeList:
		snmpType = dataType.attributes["SnmpType"].value
		clvsTypeObj = dataType.getElementsByTagName("ClovisType")
		if len(clvsTypeObj) >0:
			result[snmpType] = clvsTypeObj[0]
	return result	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createAttributeClassDefinition(resName, attrList):
	attributeEntries=""	
	for attrib in attrList:
		attrName = attrib.attributes["name"].value
		multiplicity = attrib.attributes["multiplicity"].value
		dataType = attrib.attributes["dataType"].value
		symbolMap['attrName']=string.upper(resName) + "_" + string.upper(attrName)
		symbolMap['attrId']=attrIdMap[symbolMap['attrName']]
		typeObj = typeMap.get(dataType)
		if typeObj != None:
			symbolMap['attrDataType']="CL_COR_" + string.upper(typeObj.attributes["AttributeType"].value)
			if dataType != 'DisplayString' and dataType != 'OCTET STRING':
				multiplicity = typeObj.attributes["ArraySize"].value
		else:
			symbolMap['attrDataType']="CL_COR_" + string.upper(attrib.attributes["dataType"].value)
			
		symbolMap['defaultVal']=attrib.attributes["defaultValue"].value
		symbolMap['minVal']=attrib.attributes["minValue"].value
		symbolMap['maxVal']=attrib.attributes["maxValue"].value
		symbolMap['attrType']=attrib.attributes["type"].value
		try:
			symbolMap['isPersistent']=string.upper(attrib.attributes["persistency"].value)
		except:			
			symbolMap['isPersistent'] = "TRUE"
		try:
			symbolMap['isCached']=string.upper(attrib.attributes["caching"].value)
		except:
			symbolMap['isCached'] = "TRUE"
		try:
			symbolMap['isInitialized']=string.upper(attrib.attributes["initialized"].value)
		except:			
			symbolMap['isInitialized'] = "TRUE"
		try:
			symbolMap['isWritable']=string.upper(attrib.attributes["writable"].value)
		except:
			symbolMap['isWritable'] = "TRUE"
		try:
			symbolMap['access']=attrib.attributes["access"].value
		except:
			symbolMap['access'] = "READWRITE"
		if multiplicity == "0" or multiplicity == "1":
			if attrib.attributes["type"].value == "CONFIG":
				attributeEntries += corTemplate.corSimpleAttrConfigTemplate.safe_substitute(symbolMap)
			else:
				attributeEntries += corTemplate.corSimpleAttrTemplate.safe_substitute(symbolMap)
	return attributeEntries
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createArrayAttributeClassDefinition(resName, attrList):
	arrAttributeEntries=""	
	for attrib in attrList:
		attrName = attrib.attributes["name"].value
		multiplicity=attrib.attributes["multiplicity"].value
		dataType = attrib.attributes["dataType"].value
		symbolMap['attrName']=string.upper(resName) + "_" + string.upper(attrName)
		symbolMap['attrId']=attrIdMap[symbolMap['attrName']]
		typeObj = typeMap.get(dataType)
		if typeObj != None:
			symbolMap['attrDataType']="CL_COR_" + string.upper(typeObj.attributes["AttributeType"].value)
			if dataType != 'DisplayString' and dataType != 'OCTET STRING':
				multiplicity = typeObj.attributes["ArraySize"].value
		else:
			symbolMap['attrDataType']="CL_COR_" + string.upper(attrib.attributes["dataType"].value)
		symbolMap['attrType']=attrib.attributes["type"].value
		try:
			symbolMap['isPersistent']=string.upper(attrib.attributes["persistency"].value)
		except: 	
			symbolMap['isPersistent']= "TRUE"
		try:	
			symbolMap['isCached']=string.upper(attrib.attributes["caching"].value)
		except:
			symbolMap['isCached']= "TRUE"
		try:
			symbolMap['isInitialized']=string.upper(attrib.attributes["initialized"].value)
		except:			
			symbolMap['isInitialized'] = "TRUE"
		try:
			symbolMap['isWritable']=string.upper(attrib.attributes["writable"].value)
		except:
			symbolMap['isWritable'] = "TRUE"
		try:	
			symbolMap['access']=attrib.attributes["access"].value
		except:
			symbolMap['access']= "READWRITE"
			
		if multiplicity != "0" and multiplicity != "1":
			symbolMap['arrSize']=multiplicity
			if attrib.attributes["type"].value == "CONFIG":
				arrAttributeEntries += corTemplate.corArrayAttrConfigTemplate.safe_substitute(symbolMap)
			else:
				arrAttributeEntries += corTemplate.corArrayAttrTemplate.safe_substitute(symbolMap)
	return arrAttributeEntries
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createAssocAttributeClassDefinition(resource):
	assocMap = dict()
	assocAttributeEntries=""
	provAssocAttributeEntries=""
	pmAssocAttributeEntries=""
	resName = resource.attributes["name"].value	
	associationList = resource.getElementsByTagName("associatedTo")
	for assoc in associationList:
		target = getResourceFromName(assoc.attributes["target"].value)
		allattributeList = target.getElementsByTagName("attribute")
		provAttributeList = []
		prov = resource.getElementsByTagName("provisioning")
		if len(prov) >0:
			provAttributeList = prov[0].getElementsByTagName("attribute")
		pmAttributeList = []
		pm = resource.getElementsByTagName("pm")
		if len(pm) >0:
			pmAttributeList = pm[0].getElementsByTagName("attribute")
		for attrib in allattributeList:
			attrName = attrib.attributes["name"].value
			symbolMap['attrName']=string.upper(resName) + "_" + string.upper(attrName)
			symbolMap['attrId']=attrIdMap[symbolMap['attrName']]
			symbolMap['assocClassName']=target.attributes["name"].value
			symbolMap['maxAssoc']=assoc.attributes["targetMultiplicity"].value
			symbolMap['attrType']=attrib.attributes["type"].value
			symbolMap['isPersistent']=string.upper(attrib.attributes["persistency"].value)
			symbolMap['isCached']=string.upper(attrib.attributes["caching"].value)
			symbolMap['access']=attrib.attributes["access"].value
			if attrib in provAttributeList:
				if attrib.attributes["type"].value == "CONFIG":
					provAssocAttributeEntries += corTemplate.corAssociatedAttrConfigTemplate.safe_substitute(symbolMap)
				else:
					provAssocAttributeEntries += corTemplate.corAssociatedAttrTemplate.safe_substitute(symbolMap)
			elif attrib in pmAttributeList:
				if attrib.attributes["type"].value == "CONFIG":
					pmAssocAttributeEntries += corTemplate.corAssociatedAttrConfigTemplate.safe_substitute(symbolMap)
				else:
					pmAssocAttributeEntries += corTemplate.corAssociatedAttrTemplate.safe_substitute(symbolMap)
			else:
				if attrib.attributes["type"].value == "CONFIG":
					assocAttributeEntries += corTemplate.corAssociatedAttrConfigTemplate.safe_substitute(symbolMap)
				else:
					assocAttributeEntries += corTemplate.corAssociatedAttrTemplate.safe_substitute(symbolMap)
	assocMap['MO_ASSOC_ATTRIBUTES']=assocAttributeEntries
	assocMap['PROV_MSO_ASSOC_ATTRIBUTES']=provAssocAttributeEntries
	assocMap['PM_MSO_ASSOC_ATTRIBUTES']=pmAssocAttributeEntries
	return assocMap
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createContAttributeClassDefinition(resource):
	contAttributeEntries=""	
	resName = resource.attributes["name"].value	
	compositionList = resource.getElementsByTagName("contains")
	for contains in compositionList:
		target = getResourceFromName(contains.attributes["target"].value)
		if isDS(target.attributes["name"].value) == True:
			attributeList = target.getElementsByTagName("attribute")
			for attrib in attributeList:
				attrName = attrib.attributes["name"].value
				symbolMap['attrName']=string.upper(resName) + "_" + string.upper(attrName)
				symbolMap['attrId']=attrIdMap[symbolMap['attrName']]
				symbolMap['contClassName']=target.attributes["name"].value
				symbolMap['minInstance']=contains.attributes["sourceMultiplicity"].value
				symbolMap['maxInstance']=contains.attributes["targetMultiplicity"].value
				symbolMap['attrType']=attrib.attributes["type"].value
				symbolMap['isPersistent']=string.upper(attrib.attributes["persistency"].value)
				symbolMap['isCached']=string.upper(attrib.attributes["caching"].value)
				symbolMap['access']=attrib.attributes["access"].value
				if attrib.attributes["type"].value == "CONFIG":
					contAttributeEntries += corTemplate.corContainedAttrConfigTemplate.safe_substitute(symbolMap)
				else:
					contAttributeEntries += corTemplate.corContainedAttrTemplate.safe_substitute(symbolMap)
	return contAttributeEntries
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def isDS(name):
	for ds in datastructureList:
		dsName = ds.attributes["name"].value
		if dsName == name:
			return True
	return False		 
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createAttrEnums(resList):
	global attrIdMap
	id = 1	
	enumResult = ""
	for res in resList:
		resName = string.upper(res.attributes["name"].value)
		attrList = res.getElementsByTagName("attribute")
		for attr in attrList:
			attrName = string.upper(resName) + "_" + string.upper(attr.attributes["name"].value)
			id += 1
			attrIdMap[attrName] = id
			enumResult  += corTemplate.attributeEnumEntry.safe_substitute(attributeEnumName = attrName)
		
		#getContainedAttributes(res)
		
	return corTemplate.attributesEnumsTemplate.safe_substitute(attributesEnums = enumResult)
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processinheritenceList():
	global parentMap
	global parentObjectMap
	for res in resourceList:
		inheritenceList = res.getElementsByTagName("inherits")
		source = res
		for inherits in inheritenceList:
			target = getResourceFromName(inherits.attributes["target"].value)
			parentObjectMap[source.attributes["name"].value]= target
			parentMap[source.attributes["name"].value] = target.attributes["name"].value

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createCorMetaStructFile():

	#-------------------- Removing Component List for class enum generation--------
	headerFileContents = corTemplate.corMetaStructHeaderTemplate.safe_substitute(FileName = CL_COR_METASTRUCT_FILE)
	headerFileContents += createClassEnums(resourceList)
	headerFileContents += createAttrEnums(resourceList)
	headerFileContents += "\n#ifdef __cplusplus\n}\n#endif\n\n#endif\n"
	out_file = open(CL_COR_METASTRUCT_FILE,"w")
	out_file.write(headerFileContents)
	out_file.close()

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createClassEnums(resList):
	global classIdMap
	id = 2
	enumEntries=""	
	for res in resList:
		resName = string.upper(res.attributes["name"].value)
		className = "CLASS_" + resName + "_MO"
		id += 1
		classIdMap[className] = id
		enumEntries += corTemplate.classEnumEntryTemplate.safe_substitute(classname=resName, suffix="MO")
		
		if isProvEnabled(res):				
			className = "CLASS_" + resName + "_PROV_MSO"
			id += 1
			classIdMap[className] = id
			enumEntries += corTemplate.classEnumEntryTemplate.safe_substitute(classname=resName, suffix="PROV_MSO");
		
		if isAlarmEnabled(res):
			className = "CLASS_" + resName + "_ALARM_MSO"
			id += 1
			classIdMap[className] = id
			enumEntries += corTemplate.classEnumEntryTemplate.safe_substitute(classname=resName, suffix="ALARM_MSO");
		
		if isPMEnabled(res):				
			className = "CLASS_" + resName + "_PM_MSO"
			id += 1
			classIdMap[className] = id
			enumEntries += corTemplate.classEnumEntryTemplate.safe_substitute(classname=resName, suffix="PM_MSO");
	
	return corTemplate.classEnumsTemplate.safe_substitute(classenums = enumEntries)

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createCorConfigFile(compileConfigsDoc):

	srcFileContents = ""
	srcFileContents += corTemplate.headerTemplate.safe_substitute(FileName = CL_COR_CONFIG_FILE)
	
	compileConfigsObj = compileConfigsDoc.getElementsByTagName("CompileConfigs:ComponentsInfo")
	corObj = compileConfigsObj[0].getElementsByTagName("COR")
	saveType = corObj[0].attributes["savetype"].value

	srcFileContents += corTemplate.corConfigTemplate.safe_substitute(corSaveOption = saveType)

	#------------------------------------------------------------------------------
	#-------------------- writing file------------------
	
	out_file = open(CL_COR_CONFIG_FILE,"w")
	out_file.write(srcFileContents)
	out_file.close()

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createPmConfigFile(alarmDataDoc, alarmAssociationDoc, compileConfigsDoc):

	srcFileContents = ""
	srcFileContents += corTemplate.headerTemplate.safe_substitute(FileName = CL_PM_CONFIG_FILE)

	if alarmAssociationDoc:
		alarmAssociationList = alarmAssociationDoc.getElementsByTagName("associationInfo")[0].getElementsByTagName("resource")
	else:
		alarmAssociationList = []	

	global alarmProfileMap
	alarmProfileMap = createAlarmProfileMap(alarmDataDoc)

	pmConfigMap = dict()

	pmConfigMap['pmClassInfoEntries'] = ""
	for resource in alarmAssociationList:
		resName = string.upper(resource.attributes["name"].value)
		pmAttrs = resource.getElementsByTagName("attribute")
		pmConfigMap['pmAttrInfoEntries'] = ""
		if len(pmAttrs) > 0:
			for pmAttr in pmAttrs:
				attrName = string.upper(pmAttr.attributes["name"].value)
				pmAttrAlarms = pmAttr.getElementsByTagName("alarm")
				pmConfigMap['pmAlarmInfoEntries'] = ""
				if len(pmAttrAlarms) > 0:
					pmAttrAlarms.sort(compareAlarmSeverity)
					for pmAttrAlarm in pmAttrAlarms:
						alarmProfile = alarmProfileMap[pmAttrAlarm.attributes["alarmID"].value]
						pmConfigMap['probableCause'] = alarmProfile.attributes["ProbableCause"].value
						pmConfigMap['specificProblem'] = alarmProfile.attributes["SpecificProblem"].value
						pmConfigMap['severity'] = string.upper(pmAttrAlarm.attributes["severity"].value)
						pmConfigMap['thresholdValue'] = pmAttrAlarm.attributes["thresholdValue"].value
						pmConfigMap['pmAlarmInfoEntries'] += corTemplate.pmAlarmInfoTemplate.safe_substitute(pmConfigMap)
				pmConfigMap['attrId'] = attrIdMap[resName + "_" + attrName]
				pmConfigMap['pmAttrInfoEntries'] += corTemplate.pmAttrInfoTemplate.safe_substitute(pmConfigMap)
		pmConfigMap['classId'] = classIdMap["CLASS_" + resName + "_PM_MSO"]
		resetAttrs = resource.getElementsByTagName("operationalAttribute")
		pmConfigMap['pmResetAttrId'] = "0"
		if len(resetAttrs) > 0:
			pmConfigMap['pmResetAttrId'] = attrIdMap[resName + "_" + string.upper(resetAttrs[0].attributes["name"].value)]
		pmConfigMap['pmClassInfoEntries'] += corTemplate.pmClassInfoTemplate.safe_substitute(pmConfigMap)
	
	compileConfigsObj = compileConfigsDoc.getElementsByTagName("CompileConfigs:ComponentsInfo")
	pmObj = compileConfigsObj[0].getElementsByTagName("PM")
	appConfigIntervals = pmObj[0].getElementsByTagName("appConfigInterval")
	appIntervalMap = dict()
	for appConfigInterval in appConfigIntervals:
		appIntervalMap[appConfigInterval.attributes["applicationName"].value] = appConfigInterval.attributes["value"].value
	compConfigIntervals = pmObj[0].getElementsByTagName("compConfigInterval")
	pmConfigMap['pmConfigIntervals'] = ""
	for configInterval in compConfigIntervals:
		compName = configInterval.attributes["componentName"].value
		interval = configInterval.attributes["value"].value
		pmConfigMap['compName'] = compName
		compTypeName = configInterval.attributes["type"].value
		if appIntervalMap.has_key(compTypeName):
			if interval == "0":
				pmConfigMap['interval'] = appIntervalMap[compTypeName]
			else:
				pmConfigMap['interval']	= interval	
			pmConfigMap['pmConfigIntervals'] += corTemplate.pmConfigIntervalTemplate.safe_substitute(pmConfigMap)
	pmConfigMap['version'] = "v0=\"4.0.0\""

	srcFileContents += corTemplate.pmConfigTemplate.safe_substitute(pmConfigMap)

	#------------------------------------------------------------------------------
	#-------------------- writing file------------------
	
	out_file = open(CL_PM_CONFIG_FILE,"w")
	out_file.write(srcFileContents)
	out_file.close()


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Compares alarms for their severity
def compareAlarmSeverity(alarm1, alarm2):
	sev1 = getSeverityNumber(alarm1.attributes["severity"].value)
	sev2 = getSeverityNumber(alarm2.attributes["severity"].value)
	if sev1 > sev2:
		return -1;
	elif sev1 == sev2:
		return 0;
	else:
		return 1;
		
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#Returns number for severity
def getSeverityNumber(severity):
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
#Creates AlarmID - Alarm object map
def createAlarmProfileMap(doc):
    result = dict()
    if doc is None:
        return result
    alarmProfileList = doc.getElementsByTagName("alarm:alarmInformation")[0].getElementsByTagName("AlarmProfile")
    for alarmProfile in alarmProfileList:
        result[alarmProfile.attributes["alarmID"].value] = alarmProfile
    return result	


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

#Script Execution Starts Here.
#Resource Tree information
#------ variables -----------
#------ variables -----------

def mainMethod(srcDir, resourceDoc, dataTypeDoc, compileConfigsDoc, alarmDataDoc, alarmAssociationDoc):
	
	#------ variables -----------
	global symbolMap
	symbolMap = dict()
	global typeMap 
	typeMap = dict()
	global attributesEnums
	attributesEnums = ""
	global parentMap
	parentMap = dict()
	global parentObjectMap
	parentObjectMap = dict()
	srcFileContents = ""
	global classIdMap
	classIdMap = dict()
	global attrIdMap
	attrIdMap = dict()

	global CL_COR_METADATA_FILE
	CL_COR_METADATA_FILE = os.path.join(srcDir, "config/clCorMOClass.xml")
	global CL_COR_CONFIG_FILE
	CL_COR_CONFIG_FILE = os.path.join(srcDir, "config/clCorConfig.xml")
	global CL_COR_METASTRUCT_FILE
	CL_COR_METASTRUCT_FILE = os.path.join(srcDir, "config/clCorMetaStruct.h")
	global CL_PM_CONFIG_FILE
	CL_PM_CONFIG_FILE = os.path.join(srcDir, "config/clPMConfig.xml")
   	#------ variables -----------	

	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	global resourceList
	resourceList = resourceInfoObj[0].getElementsByTagName("dataStructure") + resourceInfoObj[0].getElementsByTagName("chassisResource") + resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("mibResource") + resourceInfoObj[0].getElementsByTagName("systemController") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource")
	
	createCorMetaStructFile()
	
	global datastructureList
	datastructureList = resourceInfoObj[0].getElementsByTagName("dataStructure")
	
	processinheritenceList()
	
	global chassisResourceList
	chassisResourceList = resourceInfoObj[0].getElementsByTagName("chassisResource")
	
	global resourcePathMap
	resourcePathMap = dict()
	
	for chassis in chassisResourceList:
		createResourcePaths(chassis, "")
	
	srcFileContents += corTemplate.headerTemplate.safe_substitute(FileName = CL_COR_METADATA_FILE)
	typeMap  = createDataTypeMap(dataTypeDoc)
	omEntries = createOmClassData()
	classDefEntries = createMOClassDefinitions()

	moMsoClassDefEntries = createMoMsoTreeDefinitions()
	
	srcFileContents += corTemplate.corDataTemplate.safe_substitute(omEntries=omEntries, classDefEntries=classDefEntries, moMsoClassDefEntries=moMsoClassDefEntries)
	#------------------------------------------------------------------------------
	#-------------------- writing file------------------
	
	out_file = open(CL_COR_METADATA_FILE,"w")
	out_file.write(srcFileContents)
	out_file.close()
	
	createCorConfigFile(compileConfigsDoc)
	createPmConfigFile(alarmDataDoc, alarmAssociationDoc, compileConfigsDoc)
