################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python alarmprofiles.py <args>
# Creates source and header files for components 
################################################################################
import sys
import string
import os
import xml.dom.minidom
from string import Template
import time
import getpass

#-----------------------------------------------------------------------------
#------------------------------------------------------------------------------
def processComponentList(compList, resMap, associatedResListMap):
	for comp in compList:
	
		#----------Changes to support templateGroup association--------
		templateGroup = templateGroupMap[comp.attributes["name"].value]
		sys.path[len(sys.path)-1] = os.path.join(templatesDir, templateGroup)
		global omTemplate
		import omTemplate
		reload(omTemplate)
		#--------------------------------------------------------------
		
		map = dict();
		compName = comp.attributes["name"].value		
		associatedResList = associatedResListMap[compName]
		map['compName'] = compName
		map['capCompName'] = string.upper(compName)
		if os.path.exists(compName) == False:			
			os.mkdir(compName)
		
		omClasses = ""
		oampConfigSourceHashDefines = ""
		omClassDefinitions = ""
		consDestPrototypes = ""
		provFnPrototypes = ""
		alarmFnPrototypes = ""
		deviceObjectEnums = ""
		deviceOperationMethods = ""
		deviceOperationMethodProtoTypes = ""
		deviceOperationsTables = ""				
		halDeviceObjectInfoTables = ""
		deviceObjectTableToBegenerated = "false"
		halObjConfTableEntries = ""
		deviceObjectTableEntries = ""
		
		isBUILD_CPP = "false"
				
		try:
			isBUILD_CPP = comp.attributes["isBuildCPP"].value
		except:
			pass
			
		doMethodsFileContents = omTemplate.deviceOperationsHeadertemplate.safe_substitute(map)		
		
		if len(associatedResList) > 0:			
			for resName in associatedResList:									
				alarmHalDevInfoTableToBegenerated = "false"
				provHalDevInfoTableToBegenerated = "false"
				comp_res_File_To_Be_Generated = "false"
				provFnDefinitions = ""
				alarmFnDefinitions = ""
				res = resMap[resName]
				map['resName'] = resName
				map['mopath'] = getMOPath(resName)
				map['halObj'] = ""
				map['provConstructorHalBlob'] = ""
				map['provDestructorHalBlob'] = ""
				map['omHandle'] = ""				
				
				dos = res.getElementsByTagName("deviceObject")				
				if len(dos) > 0:
					deviceObjectTableToBegenerated = "true"					
					for do in dos:
						deviceOps = do.getElementsByTagName("deviceOperations")		
						deviceID = do.attributes["deviceId"].value
						deviceIDEnumName = string.upper(resName) + "_" + string.upper(deviceID)
						deviceObjectEnums += omTemplate.doEnumEntryTemplate.safe_substitute(doEnumName = deviceIDEnumName )
						deviceObjectTableEntries += getDeviceObjectTableEntry(resName, do)						
						if len(deviceOps) > 0:
							deviceOp = deviceOps[0]									
							deviceOperationMethods = getDeviceOperationMethodsAndProtoTypes(deviceOp, resName, deviceID)[0]
							deviceOperationMethodProtoTypes += getDeviceOperationMethodsAndProtoTypes(deviceOp, resName, deviceID)[1]
							deviceOperationsTables += getDeviceOperationsTable(deviceOp, resName, deviceID)							
							doMethodsFileContents += deviceOperationMethods
				
				alarm = res.getElementsByTagName("alarmManagement")
				if len(alarm) > 0 and alarm[0].attributes["isEnabled"].value == "true":
					comp_res_File_To_Be_Generated = "true"
					map['svc'] = "Alarm"
					map['svcid'] = "ALARM"
					map['res'] = resName.upper()	
					map['alarmConstructorHalBlob'] = ""
					map['alarmDestructorHalBlob'] = ""				
					alarmAssociatedDOTableEntries = ""
					associatedDOs = alarm[0].getElementsByTagName("associatedDO")
					if len(associatedDOs) > 0:
						map['halObj'] = "ClHalObjectHandleT halObj;"
						map['omHandle'] = "ClHandleT omHandle;"
						map['alarmConstructorHalBlob'] = omTemplate.alarmConstructorHalBlobTemplate.safe_substitute(map)						
						map['alarmDestructorHalBlob'] = omTemplate.alarmDestructorHalBlobTemplate.safe_substitute(map)						
						alarmHalDevInfoTableToBegenerated = "true"
						for associatedDO in associatedDOs:
							map['deviceID'] = resName.upper() + "_" + string.upper(associatedDO.attributes["deviceId"].value)
							map['accessPriority'] = associatedDO.attributes["accessPriority"].value
							alarmAssociatedDOTableEntries += omTemplate.associatedDOTableEntryTemplate.safe_substitute(map)
					
						map['associatedDOTableEntries']	= alarmAssociatedDOTableEntries
						halDeviceObjectInfoTables += omTemplate.halDevObjInfoTableTemplate.safe_substitute(map)											
						map['omClassId'] = "CL_OM_ALARM_"+ resName.upper() +"_CLASS_TYPE"
						halObjConfTableEntries += omTemplate.halObjConfTableEntryTemplate.safe_substitute(map)				 				
					
					omClasses += omTemplate.omClassTemplate.safe_substitute(map)
					oampConfigSourceHashDefines += omTemplate.oampConfigSourceHashDefineTemplate.safe_substitute(map)
					omClassDefinitions += omTemplate.omClassTableEntryTemplate.safe_substitute(map)
					consDestPrototypes += omTemplate.constDestPrototypeTemplate.safe_substitute(map)
					alarmFnPrototypes += omTemplate.alarmFunctionPrototypesTemplate.safe_substitute(map)
					alarmFnDefinitions += omTemplate.alarmFnDefsTemplate.safe_substitute(map)
					map['halObj'] = ""
										
					
				prov = res.getElementsByTagName("provisioning")
				if len(prov) > 0 and prov[0].attributes["isEnabled"].value == "true":
					comp_res_File_To_Be_Generated = "true"
					map['svc'] = "Prov"
					map['svcid'] = "PROV"
					map['res'] = resName.upper()
					map['provConstructorHalBlob'] = ""
					map['provDestructorHalBlob'] = ""									
					map['provUpdateHalBlob'] = ""
					provAssociatedDOTableEntries = ""
					associatedDOs = prov[0].getElementsByTagName("associatedDO")
					if len(associatedDOs) > 0:
						map['halObj'] = "ClHalObjectHandleT halObj;"						
						map['omHandle'] = "ClHandleT omHandle;"
						map['provConstructorHalBlob'] = omTemplate.provConstructorHalBlobTemplate.safe_substitute(map)						
						map['provDestructorHalBlob'] = omTemplate.provDestructorHalBlobTemplate.safe_substitute(map)
						map['provUpdateHalBlob'] = omTemplate.provUpdateHalBlobTemplate.safe_substitute(map)
						provHalDevInfoTableToBegenerated = "true"					
						for associatedDO in associatedDOs:
							map['deviceID'] = resName.upper() + "_" + string.upper(associatedDO.attributes["deviceId"].value)
							map['accessPriority'] = associatedDO.attributes["accessPriority"].value
							provAssociatedDOTableEntries += omTemplate.associatedDOTableEntryTemplate.safe_substitute(map)
							
						map['associatedDOTableEntries']	= provAssociatedDOTableEntries
						halDeviceObjectInfoTables += omTemplate.halDevObjInfoTableTemplate.safe_substitute(map)					
						map['omClassId'] = "CL_OM_PROV_"+ resName.upper() +"_CLASS_TYPE"
						halObjConfTableEntries += omTemplate.halObjConfTableEntryTemplate.safe_substitute(map)				
				
					omClasses += omTemplate.omClassTemplate.safe_substitute(map)
					oampConfigSourceHashDefines += omTemplate.oampConfigSourceHashDefineTemplate.safe_substitute(map)
					omClassDefinitions += omTemplate.omClassTableEntryTemplate.safe_substitute(map)
					consDestPrototypes += omTemplate.constDestPrototypeTemplate.safe_substitute(map)
					provFnPrototypes += omTemplate.provFunctionPrototypesTemplate.safe_substitute(map)
					provFnDefinitions += omTemplate.provFnDefsTemplate.safe_substitute(map)
					map['halObj'] = ""							
					
				omFnDefsFileContents = ""
				omFnDefsFileContents += omTemplate.provFnDefsHeaderTemplate.safe_substitute(map)
				omFnDefsFileContents += provFnDefinitions 
				omFnDefsFileContents += alarmFnDefinitions 	
					
				file = ""
				cFile = compName + "/" + "cl" + compName + resName + ".c"
				cppFile = compName + "/" + "cl" + compName + resName + ".C"
					
				if isBUILD_CPP=="true":
					file = cppFile
				else:
					file = cFile					

				if comp_res_File_To_Be_Generated == "true":
					#-----------------------------------------------------------
					oldCFile = oldNewNameMap[compName] + "/" + "cl" + oldNewNameMap[compName] + oldNewNameMap[resName] + ".c"
					oldCppFile = oldNewNameMap[compName] + "/" + "cl" + oldNewNameMap[compName] + oldNewNameMap[resName] + ".C"
					oldFilePath = "app/" + file	
					if os.path.isfile("../../src/app/" + oldCppFile):
						oldFilePath = "app/" + oldCppFile
					if os.path.isfile("../../src/app/" + oldCFile):
						oldFilePath = "app/" + oldCFile
					newFilePath = "app/" + file
					writeToMapFile(oldFilePath, newFilePath)
					#-----------------------------------------------------------
				
					omFnDefsFile = open(file, "w")				
					omFnDefsFile.write(omFnDefsFileContents)
					omFnDefsFile.close()
					
# fix for bug 4317 - remove old cl<Comp><Res>.c if it exists and comp_res_File is not to be generated this time
				else:
					if( os.path.isfile(file) ):
						try:
							os.remove(file)
						except os.error:
							pass
					

			if deviceObjectTableEntries != "":
				
				file = ""				
				cFile = compName + "/" + "cl" + compName + "DO" + ".c"
				cppFile = compName + "/" + "cl" + compName + "DO" + ".C"
				
				if isBUILD_CPP=="true":
					file = cppFile
				
				else:
					file = cFile
									
				if not (os.path.isfile(file)):
					doMethodsFile = open(file, "w")
					doMethodsFile.write(doMethodsFileContents)
					doMethodsFile.close()
				
				map['deviceIDEnums'] = deviceObjectEnums
				halConfHeaderFileContents = omTemplate.halConfHeaderFileTemplate.safe_substitute(map)		
				halConfHeaderFileContents += deviceOperationMethodProtoTypes
				halConfHeaderFileContents += omTemplate.halConfHeaderFileFooterTemplate.safe_substitute(map)
				file = compName + "/" + "cl" + compName + "HalConf.h"
				halConfHeaderFile = open(file, "w")
				halConfHeaderFile.write(halConfHeaderFileContents)
				halConfHeaderFile.close()
				#-----------------------------------------------------------
				oldFile = oldNewNameMap[compName] + "/" + "cl" + oldNewNameMap[compName] + "HalConf.h"
				oldFilePath = "app/" + oldFile
				newFilePath = "app/" + file
				writeToMapFile(oldFilePath, newFilePath)
				#-----------------------------------------------------------
								
			file= ""
			cFile = compName + "/" + "cl" + compName + "HalConf" + ".c"
			cppFile = compName + "/" + "cl" + compName + "HalConf" + ".C"
			
			if isBUILD_CPP=="true":
				file = cppFile	
			else:
				file = cFile
						
			if halObjConfTableEntries != "" and  deviceObjectTableEntries != "":
								
				halConfSrcFileContents = omTemplate.halConfSrcFileHeaderTemplate.safe_substitute(map)
				halConfSrcFileContents += deviceOperationsTables			
				map['deviceObjectTableEntries'] = deviceObjectTableEntries
				halConfSrcFileContents += omTemplate.deviceObjectTableTemplate.safe_substitute(map)				
				halConfSrcFileContents += halDeviceObjectInfoTables			
				map['halObjConfTableEntries'] = halObjConfTableEntries
				halConfSrcFileContents += omTemplate.halObjConfTableTemplate.safe_substitute(map)								
				halConfSrcFileContents += omTemplate.halConfigTemplate.safe_substitute(map)			
				#-----------------------------------------------------------
				oldCFile = oldNewNameMap[compName] + "/" + "cl" + oldNewNameMap[compName] + "HalConf" + ".c"
				oldCppFile = oldNewNameMap[compName] + "/" + "cl" + oldNewNameMap[compName] + "HalConf" + ".C"
				oldFilePath = "app/" + file	
				if os.path.isfile("../../src/app/" + oldCppFile):
					oldFilePath = "app/" + oldCppFile
				if os.path.isfile("../../src/app/" + oldCFile):
					oldFilePath = "app/" + oldCFile
				newFilePath = "app/" + file
				writeToMapFile(oldFilePath, newFilePath)
				#-----------------------------------------------------------
				
				halConfSrcFile = open(file, "w")
				halConfSrcFile.write(halConfSrcFileContents)
				halConfSrcFile.close()

								
# fix for bug 4317 - remove old cl<Comp>HalConf.c if it exists and there are no hal/do entries			
			else:
				if( os.path.isfile(file) ):
					try:
						os.remove(file)
					except os.error:
						pass

			oampConfigHeaderFile = ""
			oampConfigHeaderFile += omTemplate.oampConfigHeaderTemplate.safe_substitute(map)    # replace compName = compName with map by Abhay
			oampConfigHeaderFile += omClasses
			oampConfigHeaderFile +=	consDestPrototypes
			oampConfigHeaderFile += provFnPrototypes
			oampConfigHeaderFile += alarmFnPrototypes
			oampConfigHeaderFile += omTemplate.oampConfigFooterTemplate.safe_substitute(compName = compName)
			file = compName + "/" + "cl" + compName + "OAMPConfig.h"
			out_file = open(file, "w")
			out_file.write(oampConfigHeaderFile)		
			out_file.close()
			
			oampConfigSourceFile = ""
			oampConfigSourceFile += omTemplate.oampConfigSourceHeaderTemplate.safe_substitute(map)
			oampConfigSourceFile += oampConfigSourceHashDefines
			if omClassDefinitions != "":
				oampConfigSourceFile += omTemplate.omClassTableTemplate.safe_substitute(omClassTableEntries = omClassDefinitions )
			
			file = ""
			cFile = compName + "/" + "cl" + compName + "OAMPConfig" + ".c"
			cppFile = compName + "/" + "cl" + compName + "OAMPConfig" + ".C"
			
			if isBUILD_CPP=="true":
				file = cppFile	
			
			else:
				file = cFile
					
			out_file = open(file, "w")
			out_file.write(oampConfigSourceFile)		
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
def processResourceList(resList):
	resMap = dict();
	for res in resList:
		resName = res.attributes["name"].value
		resMap[resName] = res;	
	return resMap


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getDeviceObjectTableEntry(resName, do):
	map = dict();
	map['resName'] = resName
	map['deviceID'] = string.upper(resName) + "_" + string.upper(do.attributes["deviceId"].value)
	map['deviceCapability'] = "0"
	map['deviceCapabilityLen'] = "0"
	map['maxResponseTime'] = do.attributes["maxResponseTime"].value
	map['bootPriority'] = do.attributes["bootPriority"].value
	map['doOperationTable'] = "g" + resName + do.attributes["deviceId"].value + "DevOperations"
	
	return omTemplate.deviceObjectTableEntryTemplate.safe_substitute(map) 	


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getDeviceOperationsTable(deviceOp, resName, deviceID):
	map = dict();	
	map ['resName'] = resName	
	map ['doName'] = deviceID
	map['init'] = "NULL"
	map['close'] = "NULL"
	map['read'] = "NULL"
	map['write'] = "NULL"
	map['open'] ="NULL"
	map['warm_boot'] ="NULL"
	map['cold_boot'] = "NULL"
	map['pwr_off'] = "NULL"
	map['image_download'] = "NULL"
	map['direct_access'] = "NULL"
	
	if deviceOp.attributes["HAL_DEV_INIT"].value == "true":
		map['init'] = "cl" + resName + deviceID + "Init"

	if deviceOp.attributes["HAL_DEV_OPEN"].value == "true":
		map['open'] = "cl" + resName + deviceID + "Open"

	if deviceOp.attributes["HAL_DEV_CLOSE"].value == "true":
		map['close'] = "cl" + resName + deviceID  + "Close"
								
	if deviceOp.attributes["HAL_DEV_READ"].value == "true":
		map['read'] = "cl" + resName + deviceID + "Read"
						
	if deviceOp.attributes["HAL_DEV_WRITE"].value == "true":
		map['write'] = "cl" + resName + deviceID + "Write"
	
	if deviceOp.attributes["HAL_DEV_COLD_BOOT"].value == "true":
		map['cold_boot'] = "cl" + resName + deviceID + "ColdBoot"
						
	if deviceOp.attributes["HAL_DEV_WARM_BOOT"].value == "true":
		map['warm_boot'] = "cl" + resName + deviceID + "WarmBoot"
							
	if deviceOp.attributes["HAL_DEV_PWR_OFF"].value == "true":
		map['pwr_off'] = "cl" + resName + deviceID + "PwrOff"
								
	if deviceOp.attributes["HAL_DEV_IMAGE_DN_LOAD"].value == "true":
		map['image_download'] = "cl" + resName + deviceID + "ImageDownload"
								
	if deviceOp.attributes["HAL_DEV_DIRECT_ACCESS"].value == "true":
		map['direct_access'] = "cl" + resName + deviceID + "DirectAccess"

	return omTemplate.deviceOperationTableTemplate.safe_substitute(map)



#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getDeviceOperationMethodsAndProtoTypes(deviceOp, resName, deviceID):
	map = dict();
	map ['resName'] = resName	
	map ['deviceID'] = deviceID
	methods = ""
	protoTypes = ""
	
	if deviceOp.attributes["HAL_DEV_INIT"].value == "true":
		map['methodName'] = "Init"								
		methods += omTemplate.deviceOperationMethodTemplate.safe_substitute(map)
		protoTypes += omTemplate.deviceOperationMethodProtoTypeTemplate.safe_substitute(map)
		
	if deviceOp.attributes["HAL_DEV_OPEN"].value == "true":
		map['methodName'] = "Open"								
		methods += omTemplate.deviceOperationMethodTemplate.safe_substitute(map)								
		protoTypes += omTemplate.deviceOperationMethodProtoTypeTemplate.safe_substitute(map)
		
	if deviceOp.attributes["HAL_DEV_CLOSE"].value == "true":
		map['methodName'] = "Close"								
		methods += omTemplate.deviceOperationMethodTemplate.safe_substitute(map)
		protoTypes += omTemplate.deviceOperationMethodProtoTypeTemplate.safe_substitute(map)
								
	if deviceOp.attributes["HAL_DEV_READ"].value == "true":
		map['methodName'] = "Read"								
		methods += omTemplate.deviceOperationMethodTemplate.safe_substitute(map)	
		protoTypes += omTemplate.deviceOperationMethodProtoTypeTemplate.safe_substitute(map)
						
	if deviceOp.attributes["HAL_DEV_WRITE"].value == "true":
		map['methodName'] = "Write"								
		methods += omTemplate.deviceOperationMethodTemplate.safe_substitute(map)
		protoTypes += omTemplate.deviceOperationMethodProtoTypeTemplate.safe_substitute(map)
	
	if deviceOp.attributes["HAL_DEV_COLD_BOOT"].value == "true":
		map['methodName'] = "ColdBoot"								
		methods += omTemplate.deviceOperationMethodTemplate.safe_substitute(map)
		protoTypes += omTemplate.deviceOperationMethodProtoTypeTemplate.safe_substitute(map)				
		
	if deviceOp.attributes["HAL_DEV_WARM_BOOT"].value == "true":
		map['methodName'] = "WarmBoot"								
		methods += omTemplate.deviceOperationMethodTemplate.safe_substitute(map)
		protoTypes += omTemplate.deviceOperationMethodProtoTypeTemplate.safe_substitute(map)
							
	if deviceOp.attributes["HAL_DEV_PWR_OFF"].value == "true":
		map['methodName'] = "PwrOff"								
		methods += omTemplate.deviceOperationMethodTemplate.safe_substitute(map)
		protoTypes += omTemplate.deviceOperationMethodProtoTypeTemplate.safe_substitute(map)						
		
	if deviceOp.attributes["HAL_DEV_IMAGE_DN_LOAD"].value == "true":
		map['methodName'] = "ImageDownload"								
		methods += omTemplate.deviceOperationMethodTemplate.safe_substitute(map)
		protoTypes += omTemplate.deviceOperationMethodProtoTypeTemplate.safe_substitute(map)
								
	if deviceOp.attributes["HAL_DEV_DIRECT_ACCESS"].value == "true":
		map['methodName'] = "DirectAccess"								
		methods += omTemplate.deviceOperationMethodTemplate.safe_substitute(map)
		protoTypes += omTemplate.deviceOperationMethodProtoTypeTemplate.safe_substitute(map)
		
	methodsAndProtoTypes = []
	methodsAndProtoTypes.append(methods)
	methodsAndProtoTypes.append(protoTypes)
	return methodsAndProtoTypes
	
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------	
def getMOPath(resName):
	moPath = "\\\\"
	resource = getResourceFromName(resName)
	bottomToTopPath=[resource.attributes["name"].value + ":*"]   
	parents = getParents(resource)
	while len(parents) > 0:
		resource = parents[0]
		bottomToTopPath.append(resource.attributes["name"].value + ":*")        		 
		parents = getParents(resource)        		
          
	index = len(bottomToTopPath)-1    
	while index >= 0:
		moPath += bottomToTopPath[index]
		if index > 0:
			moPath += "\\\\"
		index -= 1
	
	return omTemplate.mopathTemplate.safe_substitute(path = moPath)
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getResourceFromName(resName):
	for resource in resourceList:
		if resource.attributes["name"].value == resName:
			return resource
	
	return None
	
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
def getResourceFromKey(key):
	for res in resourceList:				
		resKey = res.attributes["rdn"].value;		
		if resKey == key:
			return res
			
	return None	

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def writeToMapFile(oldFilePath, newFilePath):
	oldNewNameMapFile.write(oldFilePath);
	oldNewNameMapFile.write(",");
	oldNewNameMapFile.write(newFilePath);
	oldNewNameMapFile.write("\n");
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------

#Script Execution Starts Here.


def mainMethod(resourceDoc, componentDoc, associatedResDoc, templateGroupMapName, codetemplateDir, nameMap, nameMapFile):
		
	global templatesDir  
	templatesDir = codetemplateDir
	global templateGroupMap
	templateGroupMap = templateGroupMapName
	
	global oldNewNameMap
	oldNewNameMap = nameMap
	global oldNewNameMapFile
	oldNewNameMapFile = nameMapFile
	
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	
	componentList = componentInfoObj[0].getElementsByTagName("component") + componentInfoObj[0].getElementsByTagName("safComponent")
	
	global resourceList
	resourceList = resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("mibResource") + resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource") + resourceInfoObj[0].getElementsByTagName("systemController") + resourceInfoObj[0].getElementsByTagName("chassisResource")
	
	mapInfoObj = associatedResDoc.getElementsByTagName("modelMap:mapInformation")
	linkObjList	= mapInfoObj[0].getElementsByTagName("link")
	associatedResListMap = getAssociatedResListMap(linkObjList, componentList)
	
	resourceMap = processResourceList(resourceList)
	result  = processComponentList(componentList, resourceMap, associatedResListMap)



