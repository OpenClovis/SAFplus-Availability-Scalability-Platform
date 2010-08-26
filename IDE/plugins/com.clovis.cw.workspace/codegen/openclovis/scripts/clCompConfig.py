################################################################################
# ModuleName  : plugins
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python clConfig.py <args>
# create clCompConfig.h
#
# Copyright (c) 2002-2008 OpenClovis 
#
###############################################################################
import sys
import string
import os
import xml.dom.minidom
from string import Template
from string import upper, lower
import shutil

#------------------------------------------------------------------------------
def remove3rdpartyComponents(compList):
	newList = []
	for comp in compList:
		if comp.attributes.has_key("is3rdpartyComponent"):
			is3rdparty = comp.attributes["is3rdpartyComponent"].value
			if is3rdparty == "false":
				newList.append(comp)
		else:
			newList.append(comp)		
	return newList
				
#------------------------------------------------------------------------------
def processComponent(cmpList):

	global idlFile
	global globalEOMap
	idlExists = 0
	eoList = []
	if os.path.exists(idlFile):
		idlDoc  = xml.dom.minidom.parse(idlFile)
		eoList = idlDoc.getElementsByTagName("Service")
		idlExists = 1
	symbolMap = dict()
	portIds = ""
	for cmp in cmpList:
	
		#----------Changes to support templateGroup association--------
		templateGroup = templateGroupMap[cmp.attributes["name"].value]
		import_string = "from "+ templateGroup + " import clCompConfigTemplate"
		exec import_string
		reload(clCompConfigTemplate)
		#--------------------------------------------------------------
		
		
		cpmName = cmp.attributes["name"].value
		cpmName = string.replace(cpmName, " ", "_")
		cpmName = string.replace(cpmName, "-", "_")
		symbolMap["cpmName"] = cpmName
		eo = cmp.getElementsByTagName("eoProperties")[0]
		symbolMap["eoName"]        = eo.attributes["eoName"].value
		symbolMap["eoThreadPri"]   = eo.attributes["threadPriority"].value
		eoThreadCount = int(eo.attributes["threadCount"].value)
		symbolMap["eoNumThread"]   = eoThreadCount
		iocPort = eo.attributes["iocPortNumber"].value
		symbolMap["iocPort"]       = iocPort
		symbolMap["threadPolicy"]  = eo.attributes["mainThreadUsagePolicy"].value
		symbolMap["eoName"]  = eo.attributes["eoName"].value
		symbolMap["maxNoClients"]  = eo.attributes["maxNoClients"].value
		eoThreadPriority = eo.attributes["threadPriority"].value
		if eoThreadPriority == "PRIORITY_LOW":
			eoThreadPriority = "CL_OSAL_THREAD_PRI_LOW"
		if eoThreadPriority == "PRIORITY_MEDIUM":
			eoThreadPriority = "CL_OSAL_THREAD_PRI_MEDIUM"
		if eoThreadPriority == "PRIORITY_HIGH":
			eoThreadPriority = "CL_OSAL_THREAD_PRI_HIGH"		
		symbolMap["eoThreadPriority"] = eoThreadPriority
			
		aspLibObj = eo.getElementsByTagName("aspLib")[0]

		symbolMap["omLibEnable"]   = aspLibObj.attributes["OM"].value
		symbolMap["halLibEnable"]  = aspLibObj.attributes["HAL"].value
		symbolMap["dbalLibEnable"] = aspLibObj.attributes["DBAL"].value

		symbolMap["corClLib"]      = aspLibObj.attributes["COR"].value
		symbolMap["cmClLib"]       = aspLibObj.attributes["CM"].value
		symbolMap["nameClLib"]     = aspLibObj.attributes["NameService"].value
		symbolMap["logClLib"]      = aspLibObj.attributes["LOG"].value
		symbolMap["traceClLib"]    = aspLibObj.attributes["Trace"].value
		symbolMap["diagClLib"]     = aspLibObj.attributes["Diagnostics"].value
		symbolMap["txnClLib"]      = aspLibObj.attributes["Transaction"].value        
		symbolMap["provLib"]       = aspLibObj.attributes["Prov"].value
		symbolMap["msoLib"]        = aspLibObj.attributes["Prov"].value
		symbolMap["alarmClLib"]    = aspLibObj.attributes["Alarm"].value
		symbolMap["debugClLib"]    = aspLibObj.attributes["Debug"].value        
		symbolMap["pmLib"]         = "CL_FALSE"

		portIds += clCompConfigTemplate.portIdDefTemplate.safe_substitute(compName=cpmName.upper(), iocPort=iocPort)
		
		pmConfigured = False
		associatedResList = associatedResourceListMap[cpmName]
		if len(associatedResList) > 0:			
			for associatedResName in associatedResList:
				associatedRes = resourceMap[associatedResName]
				pmObj = associatedRes.getElementsByTagName("pm")
				if len(pmObj) > 0:
					if len(pmObj[0].getElementsByTagName("attribute")) > 0:
						pmConfigured = True
						break
	
		if pmConfigured:
			symbolMap["pmLib"] = "CL_TRUE"
			pmConfigured = False

		symbolMap["hasEOServices"] = 0
		globalEOMap[cpmName] = 0
		if idlExists == 1:			
			for eo in eoList:
				eoName = symbolMap["eoName"]
				if eo.attributes["name"].value == symbolMap["eoName"]:
					serviceList = eo.getElementsByTagName("Port")
					if len(serviceList) > 0:
						symbolMap["hasEOServices"] = 1
						globalEOMap[cpmName] = eoName
				
		result = clCompConfigTemplate.clConfigHeaderFileTemplate.safe_substitute(symbolMap)
		if os.path.exists(cpmName) == False:       	    
			os.mkdir(cpmName)   
		    
		file = cpmName + "/" + "clCompCfg.h"     
		fd = open(file , "w")
		fd.write(result)
		fd.close()
		
		result = clCompConfigTemplate.clConfigCFileTemplate.safe_substitute(symbolMap)
		file = cpmName + "/" + "clCompCfg.c"     
		fd = open(file , "w")
		fd.write(result)
		fd.close()
		
		isSNMPAgent = "false"
		try:
			isSNMPAgent = cmp.attributes["isSNMPSubAgent"].value
		except:
			pass
		
		isBUILD_CPP = "false"
				
		try:
			isBUILD_CPP = cmp.attributes["isBuildCPP"].value
		except:
			pass
		########################################################################
		
		templateGroupDir = os.path.join(templatesDir, templateGroup)
		
		
		global templateApphFilePath
		templateApphFilePath=templateGroupDir + "/" + templateApphFile
		global templateAppCFilePath
		templateAppCFilePath=templateGroupDir + "/" + templateAppCFile
		global proxyTemplateApphFilePath
		proxyTemplateApphFilePath=templateGroupDir + "/" + proxyTemplateApphFile
		global proxyTemplateAppCFilePath
		proxyTemplateAppCFilePath=templateGroupDir + "/" + proxyTemplateAppCFile
		global snmpTemplateApphFilePath
		snmpTemplateApphFilePath=templateGroupDir + "/" + snmpTemplateApphFile
		global snmpTemplateAppCFilePath
		snmpTemplateAppCFilePath=templateGroupDir + "/" + snmpTemplateAppCFile
			
		
		########################################################################			
			
		##########################files under app/cmpName/
			
		snmpdestApphFile = cpmName + "/" + "clSnmpAppMain.h"
		snmpdestAppCFile = cpmName + "/" + "clSnmpAppMain.c"
		snmpdestAppCppFile = cpmName + "/" + "clSnmpAppMain.C"
		destApphFile = cpmName + "/" + "clCompAppMain.h"
		destAppCFile = cpmName + "/" + "clCompAppMain.c"
		destAppCppFile = cpmName + "/" + "clCompAppMain.C"
		proxydestApphFile = cpmName + "/" + "clProxyCompAppMain.h"
		proxydestAppCFile = cpmName + "/" + "clProxyCompAppMain.c"
		proxydestAppCppFile = cpmName + "/" + "clProxyCompAppMain.C"
		
		############################################################
		
		if isSNMPAgent == "true":
				
			shutil.copyfile(snmpTemplateApphFilePath, snmpdestApphFile)
			#-----------------------------------------------------------
			oldFilePath = "app/" + oldNewNameMap[cpmName] + "/" + "clSnmpAppMain.h"
			newFilePath = "app/" + snmpdestApphFile
			writeToMapFile(oldFilePath, newFilePath)
			#-----------------------------------------------------------		

			if isBUILD_CPP == "true" :
				snmpdestAppFile = snmpdestAppCppFile
			else:
				snmpdestAppFile = snmpdestAppCFile
				if os.path.isfile(snmpdestAppCppFile):
					print "Warning : Renaming ",snmpdestAppCppFile," to", snmpdestAppFile
					print "You might face some compilation problems!"
					
			#-----------------------------------------------------------
			oldCFile = oldNewNameMap[cpmName] + "/" + "clSnmpAppMain.c"
			oldCppFile = oldNewNameMap[cpmName] + "/" + "clSnmpAppMain.C"
			oldFilePath = "app/" + snmpdestAppFile	
			if os.path.isfile("../../src/app/" + oldCppFile):
				oldFilePath = "app/" + oldCppFile
			if os.path.isfile("../../src/app/" + oldCFile):
				oldFilePath = "app/" + oldCFile
			newFilePath = "app/" + snmpdestAppFile
			writeToMapFile(oldFilePath, newFilePath)
			#-----------------------------------------------------------
			
			shutil.copyfile(snmpTemplateAppCFilePath, snmpdestAppFile)
					
		else:
			isProxy = isProxyComponent(cmp)
			if isProxy == "false":
				shutil.copyfile(templateApphFilePath, destApphFile)
				#-----------------------------------------------------------
				oldFilePath = "app/" + oldNewNameMap[cpmName] + "/" + "clCompAppMain.h"
				newFilePath = "app/" + destApphFile
				writeToMapFile(oldFilePath, newFilePath)
				#-----------------------------------------------------------
				
				if isBUILD_CPP == "true" :
					destAppFile = destAppCppFile
				else:
					destAppFile = destAppCFile
					if os.path.isfile(destAppCppFile):
						print "Warning : Renaming ",destAppCppFile," to", destAppFile
						print "You might face some compilation problems!"
									
				#-----------------------------------------------------------
				oldCFile = oldNewNameMap[cpmName] + "/" + "clCompAppMain.c"
				oldCppFile = oldNewNameMap[cpmName] + "/" + "clCompAppMain.C"
				oldFilePath = "app/" + destAppFile	
				if os.path.isfile("../../src/app/" + oldCppFile):
					oldFilePath = "app/" + oldCppFile
				if os.path.isfile("../../src/app/" + oldCFile):
					oldFilePath = "app/" + oldCFile
				newFilePath = "app/" + destAppFile
				writeToMapFile(oldFilePath, newFilePath)
				#-----------------------------------------------------------
				templateFile = open(templateAppCFilePath, "r")
				template = templateFile.read(1000000)
				templateFile.close()
				appMainTemplate = Template(template)
				destAppCFileDesc = open(destAppFile, "w")
				destAppCFileDesc.write(appMainTemplate.safe_substitute())
				destAppCFileDesc.close()
					
			else:
								
				shutil.copyfile(proxyTemplateApphFilePath, proxydestApphFile)
				#-----------------------------------------------------------
				oldFilePath = "app/" + oldNewNameMap[cpmName] + "/" + "clProxyCompAppMain.h"
				newFilePath = "app/" + proxydestApphFile
				writeToMapFile(oldFilePath, newFilePath)
				#-----------------------------------------------------------
					
				if isBUILD_CPP == "true" :
					proxydestAppFile = proxydestAppCppFile
					if 	os.path.isfile(proxydestAppCFile):
						print "Warning : Renaming ",proxydestAppCFile," to", proxydestAppFile
						print "You might face some compilation problems!"
				else:
					proxydestAppFile = proxydestAppCFile
					if 	os.path.isfile(proxydestAppCppFile):
						print "Warning : Renaming ",proxydestAppCppFile," to", proxydestAppFile
						print "You might face some compilation problems!"
						
				#-----------------------------------------------------------
				oldCFile = oldNewNameMap[cpmName] + "/" + "clProxyCompAppMain.c"
				oldCppFile = oldNewNameMap[cpmName] + "/" + "clProxyCompAppMain.C"
				oldFilePath = "app/" + proxydestAppFile	
				if os.path.isfile("../../src/app/" + oldCppFile):
					oldFilePath = "app/" + oldCppFile
				if os.path.isfile("../../src/app/" + oldCFile):
					oldFilePath = "app/" + oldCFile
				newFilePath = "app/" + proxydestAppFile
				writeToMapFile(oldFilePath, newFilePath)
				#-----------------------------------------------------------						
				templateFile = open(proxyTemplateAppCFilePath, "r")
				template = templateFile.read(1000000)
				templateFile.close()
				appMainTemplate = Template(template)
				destAppCFileDesc = open(proxydestAppFile, "w")
				destAppCFileDesc.write(appMainTemplate.safe_substitute())
				destAppCFileDesc.close()
	portIdHeaderFile = open("clCompPorts.h", "w")
	portIdHeaderFile.write(clCompConfigTemplate.clCompPortHeaderFileTemplate.safe_substitute(portIdDefs=portIds))
	portIdHeaderFile.close()				
#------------------------------------------------------------------------------
def isProxyComponent(comp):
	proxyList = comp.getElementsByTagName("proxies")
	if len(proxyList) > 0 :
		return "true"
	return "false"
#------------------------------------------------------------------------------
def writeToMapFile(oldFilePath, newFilePath):
	oldNewNameMapFile.write(oldFilePath);
	oldNewNameMapFile.write(",");
	oldNewNameMapFile.write(newFilePath);
	oldNewNameMapFile.write("\n");
			
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

def mainMethod(componentDoc, defaultTemplateDir, idlXmlFile, templateGroupMapName, codetemplateDir, nameMap, nameMapFile, codeGenMode, resourceDoc, associatedResDoc):
		
	global templatesDir  
	templatesDir = codetemplateDir
	global templateGroupMap
	templateGroupMap = templateGroupMapName
	
	global oldNewNameMap
	oldNewNameMap = nameMap
	global oldNewNameMapFile
	oldNewNameMapFile = nameMapFile
		
	global globalEOMap
	globalEOMap = dict()
	
	global snmpTemplateApphFile
	snmpTemplateApphFile = "clSnmpAppMain.h"
	global snmpTemplateAppCFile
	snmpTemplateAppCFile = "clSnmpAppMain.c"
	global templateApphFile
	templateApphFile = "clCompAppMain.h"
	global templateAppCFile
	templateAppCFile = "clCompAppMain.c"
	global proxyTemplateApphFile
	proxyTemplateApphFile = "clProxyCompAppMain.h"
	global proxyTemplateAppCFile
	proxyTemplateAppCFile = "clProxyCompAppMain.c"

	global templateGroupDir
	templateGroupDir = defaultTemplateDir
	
	global idlFile
	idlFile = idlXmlFile

	global sourceCodeGenMode
	sourceCodeGenMode = codeGenMode	
	
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation");
	global componentList
	componentList = componentInfoObj[0].getElementsByTagName("safComponent");
	componentList = remove3rdpartyComponents(componentList)
	
	global idlExists
	idlExists=0

	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	resourceList = resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("mibResource") + resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource") + resourceInfoObj[0].getElementsByTagName("systemController") + resourceInfoObj[0].getElementsByTagName("chassisResource")
	global resourceMap
	resourceMap = processResourceList(resourceList)
	mapInfoObj = associatedResDoc.getElementsByTagName("modelMap:mapInformation")
	linkObjList	= mapInfoObj[0].getElementsByTagName("link")
	global associatedResourceListMap
	associatedResourceListMap = getAssociatedResListMap(linkObjList, componentList)
	
	processComponent(componentList)
	

	#--------------------------------------------------------------------------
	#Generate Makefile for App
	compList = ""
	snmpComp = ""
	appMakefile = open("Makefile", "w")
	for component in componentList:
		cpmName = component.attributes["name"].value
		isSNMPAgent = "false"
		try:
			isSNMPAgent = component.attributes["isSNMPSubAgent"].value
		except:
			pass
			
		if isSNMPAgent == "true":
			snmpComp = cpmName
		else:
			compList += cpmName + " "

	#-----------------------------------------------------------
	writeToMapFile("app/Makefile", "app/Makefile")
	#-----------------------------------------------------------		

	#------------------------------------------------------------------------------

	
	#Generate Makefile for each Component 
	for component in componentList:
	
		#----------Changes to support templateGroup association--------
		templateGroup = templateGroupMap[component.attributes["name"].value]
		import_string = "from "+ templateGroup + " import clCompConfigTemplate"
		exec import_string
		reload(clCompConfigTemplate)
		#--------------------------------------------------------------
		
		cpmName = component.attributes["name"].value
		map = dict()
		map["compName"] = cpmName
		imageName = component.attributes["instantiateCommand"].value
		if imageName == None or imageName == "":
			imageName = "asp_" + cpmName
		map["exeName"] = imageName
		map["cmClientLib"] = ""
		map["alarmClientLib"] = ""
		map["omClientLib"] = ""
		map["provClientLib"] = ""
		map["msoClientLib"] = ""
		map["pmClientLib"] = ""
		map["alarmUtilsLib"] = ""
		eo = component.getElementsByTagName("eoProperties")[0]
		aspLibObj = eo.getElementsByTagName("aspLib")[0]
		map["idlPtrLib"] = ""
		map["alarmXdrLib"] = ""
		map["faultClientLib"] = ""
			
		class anonClass:
			def __init__(self, **entries): self.__dict__.update(entries)

		badattr = anonClass(value="CL_FALSE")
	
		if aspLibObj.attributes.get("CM",badattr).value == "CL_TRUE":
			map["cmClientLib"] = "libClCm.a"
			
		if aspLibObj.attributes.get("Alarm",badattr).value == "CL_TRUE":
			map["alarmClientLib"] = "libClAlarmClient.a"
			map["idlPtrLib"] = "libClIdlPtr.a"
			map["alarmXdrLib"] = "libClAlarmXdr.a"
	
		if aspLibObj.attributes.get("Prov",badattr).value == "CL_TRUE":
			map["provClientLib"] = "libClProv.a"
			map["msoClientLib"] = "libClMsoClient.a"
	
		if aspLibObj.attributes.get("OM",badattr).value == "CL_TRUE":
			map["omClientLib"] = "libClOmClient.a"
	
		if aspLibObj.attributes.get("Fault",badattr).value == "CL_TRUE":
			map["faultClientLib"] = "libClFaultClient.a"

		pmConfigured = False
		associatedResList = associatedResourceListMap[cpmName]
		if len(associatedResList) > 0:			
			for associatedResName in associatedResList:
				associatedRes = resourceMap[associatedResName]
				pmObj = associatedRes.getElementsByTagName("pm")
				if len(pmObj) > 0:
					if len(pmObj[0].getElementsByTagName("attribute")) > 0:
						pmConfigured = True
						break
	
		if pmConfigured:
			map["pmClientLib"] = "libClPMClient.a"
			map["alarmUtilsLib"] = "libClAlarmUtils.a"
			map["omClientLib"] = "libClOmClient.a"
			map["msoClientLib"] = "libClMsoClient.a"
			map["alarmClientLib"] = "libClAlarmClient.a"
			map["alarmXdrLib"] = "libClAlarmXdr.a"
			pmConfigured = False

		map["idlLib"] = ""
		map["idlOpenLib"] = ""
		map["idlXdrInclude"] = ""
		
		if globalEOMap[cpmName] != 0:
			idlExists = 1
			eoName = globalEOMap[cpmName]
			EoName = upper(eoName[0]) + eoName[1:]
			map["idlLib"] = "libCl" + EoName + "Server.a"
			map["idlPtrLib"] = "libClIdlPtr.a"
			map["idlOpenLib"] = "libCl" + EoName + "IdlOpen.a"
			map["idlXdrInclude"] = "-I../idl/" + EoName + "/xdr"
		
		file = cpmName + "/Makefile" 	
		compMakefile = open(file, "w")
		
		#-----------------------------------------------------------
		oldFilePath = "app/" + oldNewNameMap[cpmName] + "/Makefile"
		newFilePath = "app/" + file
		writeToMapFile(oldFilePath, newFilePath)
		#-----------------------------------------------------------		
		
		
		isBUILD_CPP = "false"
		
		try:
			isBUILD_CPP = component.attributes["isBuildCPP"].value
		except:
			pass
					
		if isBUILD_CPP == "true" :
			map['Build_Cpp'] = 1
		else :
			map['Build_Cpp'] = 0
			
		isSNMPAgent = "false"
		try:
			isSNMPAgent = component.attributes["isSNMPSubAgent"].value
		except:
			pass
		
		extraLibs = ""
		libPaths = ""
		libNames = ""
		if len(eo.getElementsByTagName("libs")) > 0:
			libsObj = eo.getElementsByTagName("libs")[0]
			libNameList = libsObj.getElementsByTagName("libName")
			libPathList = libsObj.getElementsByTagName("libPath")
			for libName in libNameList:
				libNames += " -l" + libName.firstChild.data
			for libPath in libPathList:
				libPaths += " -L" + libPath.firstChild.data
			map['libNames'] = libNames
			map['libPaths'] = libPaths	
		if isSNMPAgent == "true":
			compMakefile.write(clCompConfigTemplate.snmpAgentMakefileTemplate.safe_substitute(map))
		else:
			if libNames != "" or libPaths != "":
				extraLibs = clCompConfigTemplate.extraLibTemplate.safe_substitute(map)
			map['extraLibs'] = extraLibs
			compMakefile.write(clCompConfigTemplate.compMakefileTemplate.safe_substitute(map))
		compMakefile.close()
	
	idlSubDir = ""
	if idlExists == 1:
		idlSubDir = "idl"
	from templates import clCompConfigTemplate
	reload (clCompConfigTemplate)
	appMakefile.write(clCompConfigTemplate.appMakeFileTemplate.safe_substitute(compList = compList, snmpAgent = snmpComp, idlSubDir = idlSubDir))
	appMakefile.close()
	
