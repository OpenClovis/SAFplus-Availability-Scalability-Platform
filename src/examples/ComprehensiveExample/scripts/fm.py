################################################################################
# ModuleName  : com
# $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/scripts/fm.py $
# $Author: bkpavan $
# $Date: 2007/03/26 $
################################################################################
# Description :
# usage: python fm.py <args>
# create clfmlCfg.c
################################################################################
import sys
import string
import xml.dom.minidom
from string import Template
import time
import getpass
import fmTemplate

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def getFunction(cat, sev, level, type):
	
	global tableEntries
	global methodSkeletons
	global methodExterns
	for entry in tableEntries:
		category = entry.attributes["Category"].value
		severity = entry.attributes["Severity"].value
		escLevel = entry.attributes["EscalationLevel"].value
		function = entry.attributes["Function"].value
		isExtern = entry.attributes["IsExtern"].value
		if cat == category and sev == severity and level == escLevel :
			if isExtern == "false":
				methodSkeletons += fmTemplate.methodTemplate.safe_substitute(methodName = function)
			methodExterns += fmTemplate.methodExternTemplate.safe_substitute(methodName = function)
			return function
	
	if cat == "proc" and (sev == "CR" or sev == "MJ"):
		if type == "local":
			if level == "Level5":
				return "clFaultEscalate"

			else:
				return "ClFaultCompRestartRequest"
		else:
			if level == "Level1":
				return "ClFaultNodeRestartRequest"
			
	return "NULL"


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def populateOneDTables(cat, sev, type):

	global oneDSeqTables
	symbolMap['cat'] = cat
	symbolMap['sev'] = sev
	if type == "local":
		symbolMap['scope'] = ""
	
	else:
		symbolMap['scope'] = "g"
	
	symbolMap['fn1'] = getFunction(cat, sev, "Level1", type)
	symbolMap['fn2'] = getFunction(cat, sev, "Level2", type)
	symbolMap['fn3'] = getFunction(cat, sev, "Level3", type)
	symbolMap['fn4'] = getFunction(cat, sev, "Level4", type)
	symbolMap['fn5'] = getFunction(cat, sev, "Level5", type)
	
	oneDSeqTables += fmTemplate.oneDSeqTableTemplate.safe_substitute(symbolMap)

#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def createFaultRepairHandles(resList):
	faultRepairEntries = ""
	faultRepairFunctionEntries = ""
	headerFileContents = fmTemplate.FaultHeaderTemplate.safe_substitute()
	headerFileContents += fmTemplate.FaultRepairTableTemplate.safe_substitute(faultRepairEntry = faultRepairEntries)
	out_file = open ("clFaultRepairHandlers.c", "w")
	out_file.write(headerFileContents)
	out_file.close()
	for res in resList:
		#creating repairHandlers only for those resourses whose alarm is enabled
		if isAlarmEnabled(res):
			resName = string.upper(res.attributes["name"].value)
			faultRepairEntries += fmTemplate.FaultRepairTableEntryTemplate.safe_substitute(classname=resName, enum_class="CLASS_"+resName+"_MO")
			faultRepairFunctionEntries += fmTemplate.FaultRepairFunctionEntryTemplate.safe_substitute(classname=resName)
			
			headerFileContents = fmTemplate.FaultHeaderTemplate.safe_substitute()
			headerFileContents += fmTemplate.FaultRepairFunctionTemplate.safe_substitute(faultRepairFunctionEntry = faultRepairFunctionEntries)
			headerFileContents += fmTemplate.FaultRepairTableTemplate.safe_substitute(faultRepairEntry = faultRepairEntries)
			
			out_file = open ("clFaultRepairHandlers.c", "w")
			out_file.write(headerFileContents)
			out_file.close()
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
def isAlarmEnabled(resource):
	alarmElements=resource.getElementsByTagName("alarmManagement")		
	if len(alarmElements) > 0:
		if alarmElements[0].attributes["isEnabled"].value == "true":				
			return True
	
	return False		
#------------------------------------------------------------------------------	
#------------------------------------------------------------------------------
#Script Execution Starts Here.


def mainMethod(buildConfigsDoc, resourceDoc):
	
	categoryList = ["com", "qos", "proc", "equip", "env"]
	severityList = ["CR", "MJ", "MN", "WR", "IN", "CL"]
	
	
	buildConfigs = buildConfigsDoc.getElementsByTagName("CompileConfigs:ComponentsInfo")
	fm = buildConfigsDoc.getElementsByTagName("FM")
	fmElement = None
	
	localProbPeriod = ""
	
	if len(fm) > 0:
		fmElement=fm[0]
	
	#----------------- Local FM data ----------------------------------------------
	global tableEntries
	tableEntries = ""
	if fmElement != None:
		tableEntries = fmElement.getElementsByTagName("localSeqTableEntry")
		localProbPeriod = fmElement.attributes["localProbationPeriod"].value
	
	fileContents = ""
	
	global methodSkeletons
	global methodExterns

	global oneDSeqTables
	oneDSeqTables = ""
	twoDSeqTables = ""
	methodSkeletons = fmTemplate.handlerHeaderTemplate.safe_substitute(scope = "Local")
	methodExterns = ""
	fourDSeqTable = fmTemplate.localFourDSeqTableTemplate.safe_substitute()
	global symbolMap
	symbolMap = dict()
	for cat in categoryList:
		symbolMap['category']=cat;
		symbolMap['scope']="";
		twoDSeqTables += fmTemplate.twoDSeqTableTemplate.safe_substitute(symbolMap)
		for sev in severityList:
			populateOneDTables(cat, sev, "local")
	
	
	out_file = open("clFaultCfg.c","w")
	fileContents += fmTemplate.headerTemplate.safe_substitute(externList = fmTemplate.localExternListTemplate.safe_substitute(), scope = "Local", probPeriodVar = "clFaultLocalProbationPeriod", period = localProbPeriod)
	fileContents += " /*************************/ "
	fileContents += methodExterns
	fileContents += " /*************************/ "
	fileContents += oneDSeqTables
	fileContents += " /*************************/ "
	fileContents += twoDSeqTables
	fileContents += " /*************************/ "
	fileContents += fourDSeqTable
	out_file.write(fileContents)
	out_file.close()
	
	if(methodSkeletons != ""):	
		out_file = open ("clFaultReportHandlers.c", "w")
		out_file.write(methodSkeletons)
		out_file.close()

	#########################
	#
	#clFaultRepairHandlers.c
	#
	#########################
	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	resourceList = resourceInfoObj[0].getElementsByTagName("dataStructure") + resourceInfoObj[0].getElementsByTagName("chassisResource") + resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("mibResource") + resourceInfoObj[0].getElementsByTagName("systemController") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource")
	createFaultRepairHandles(resourceList)

