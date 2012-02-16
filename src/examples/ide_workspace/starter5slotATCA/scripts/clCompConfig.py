# Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
# This file is available  under  a  commercial  license  from  the
# copyright  holder or the GNU General Public License Version 2.0.
#
# The source code for  this program is not published  or otherwise 
# divested of  its trade secrets, irrespective  of  what  has been 
# deposited with the U.S. Copyright office.
# 
# This program is distributed in the  hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
# General Public License for more details.
# 
# For more  information,  see the  file COPYING provided with this
# material.
import sys
import string
import os
import xml.dom.minidom
from string import Template
from string import upper, lower
import shutil

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
	for cmp in cmpList:
	
		#----------Changes to support templateGroup association--------
		templateGroup = templateGroupMap[cmp.attributes["name"].value]
		sys.path[len(sys.path)-1] = os.path.join(templatesDir, templateGroup)
		global clCompConfigTemplate
		import clCompConfigTemplate
		reload(clCompConfigTemplate)
		#--------------------------------------------------------------
		
		
		idlIncludeFile = ""
		cpmName = cmp.attributes["name"].value
		cpmName = string.replace(cpmName, " ", "_")
		cpmName = string.replace(cpmName, "-", "_")
		symbolMap["cpmName"] = cpmName
		eo = cmp.getElementsByTagName("eoProperties")[0]
		cmpEoName = eo.attributes["eoName"].value
		symbolMap["eoName"]        = eo.attributes["eoName"].value
		symbolMap["eoThreadPri"]   = eo.attributes["threadPriority"].value
		eoThreadCount = int(eo.attributes["threadCount"].value)
		symbolMap["eoNumThread"]   = eoThreadCount
		symbolMap["iocPort"]       = eo.attributes["iocPortNumber"].value
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
		symbolMap["alarmClLib"]    = aspLibObj.attributes["Alarm"].value
		symbolMap["debugClLib"]    = aspLibObj.attributes["Debug"].value        

		symbolMap["hasEOServices"] = 0
		globalEOMap[cpmName] = 0
		if idlExists == 1:			
			for eo in eoList:
				eoName = symbolMap["eoName"]
				if eo.attributes["name"].value == symbolMap["eoName"]:
					idlIncludeFile = clCompConfigTemplate.idlIncludeFileTemplate.safe_substitute(eoName=eoName)
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
					
			if isBUILD_CPP == "true" :
				snmpdestAppFile = snmpdestAppCppFile
			else:
				snmpdestAppFile = snmpdestAppCFile
				if os.path.isfile(snmpdestAppCppFile):
					print "Warning : Renaming ",snmpdestAppCppFile," to", snmpdestAppFile
					print "You might face some compilation problems!"
					
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
				destAppCFileDesc.write(appMainTemplate.safe_substitute(idlIncludeFile=idlIncludeFile, eo=cmpEoName))
				destAppCFileDesc.close()
					
			else:
								
				shutil.copyfile(proxyTemplateApphFilePath, proxydestApphFile)
					
				if isBUILD_CPP == "true" :
					proxydestAppFile = proxydestAppCppFile
				else:
					proxydestAppFile = proxydestAppCFile
					if 	os.path.isfile(proxydestAppCppFile):
						print "Warning : Renaming ",proxydestAppCppFile," to", proxydestAppFile
						print "You might face some compilation problems!"
										
					templateFile = open(proxyTemplateAppCFilePath, "r")
					template = templateFile.read(1000000)
					templateFile.close()
					appMainTemplate = Template(template)
					destAppCFileDesc = open(proxydestAppFile, "w")
					destAppCFileDesc.write(appMainTemplate.safe_substitute(idlIncludeFile=idlIncludeFile, eo=cmpEoName))
					destAppCFileDesc.close()
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

def mainMethod(componentDoc, defaultTemplateDir, idlXmlFile, templateGroupMapName, codetemplateDir, nameMap, nameMapFile):
		
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
	
	
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation");
	global componentList
	componentList = componentInfoObj[0].getElementsByTagName("safComponent");

	global idlExists
	idlExists=0
	
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

	#------------------------------------------------------------------------------

	
	#Generate Makefile for each Component 
	for component in componentList:
	
		#----------Changes to support templateGroup association--------
		templateGroup = templateGroupMap[component.attributes["name"].value]
		sys.path[len(sys.path)-1] = os.path.join(templatesDir, templateGroup)
		import clCompConfigTemplate
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
		eo = component.getElementsByTagName("eoProperties")[0]
		aspLibObj = eo.getElementsByTagName("aspLib")[0]
		map["idlPtrLib"] = ""
		map["alarmXdrLib"] = ""
		map["faultClientLib"] = ""
			
		if aspLibObj.attributes["CM"].value == "CL_TRUE":
			map["cmClientLib"] = "libClCm.a"
			
		if aspLibObj.attributes["Alarm"].value == "CL_TRUE":
			map["alarmClientLib"] = "libClAlarmClient.a"
			map["idlPtrLib"] = "libClIdlPtr.a"
			map["alarmXdrLib"] = "libClAlarmXdr.a"
	
		if aspLibObj.attributes["Prov"].value == "CL_TRUE":
			map["provClientLib"] = "libClProv.a"
	
		if aspLibObj.attributes["OM"].value == "CL_TRUE":
			map["omClientLib"] = "libClOmClient.a"
	
		if aspLibObj.attributes["Fault"].value == "CL_TRUE":
			map["faultClientLib"] = "libClFaultClient.a"
	
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
		
		if isSNMPAgent == "true":
			compMakefile.write(clCompConfigTemplate.snmpAgentMakefileTemplate.safe_substitute(map))
		else:
			compMakefile.write(clCompConfigTemplate.compMakefileTemplate.safe_substitute(map))
		compMakefile.close()
	
	idlSubDir = ""
	if idlExists == 1:
		idlSubDir = "idl"
	from default import clCompConfigTemplate
	reload (clCompConfigTemplate)
	appMakefile.write(clCompConfigTemplate.appMakeFileTemplate.safe_substitute(compList = compList, snmpAgent = snmpComp, idlSubDir = idlSubDir))
	appMakefile.close()
	
