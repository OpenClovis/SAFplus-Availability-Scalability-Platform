import sys
import xml.dom.minidom
import os

def mainMethod(resourcedataDoc, componentdataDoc, alarmdataDoc, associatedResDoc, associatedAlarmDoc, alarmRuleDoc, alarmAssociationDoc, staticdir, templatesdir, defaultTemplateDir, idlXml, templateGroupMap, codetemplateDir, oldNewNameMap, codeGenMode):
	import alarmprofiles
	import clCompConfig
	import clCompSNMPConfig
	import om
	#createthe new mapping file each time
	oldNewNameMapFile = open("../../.nameMapFile", "w")
	
	clCompSNMPConfig.mainMethod(resourcedataDoc, componentdataDoc, alarmdataDoc, associatedAlarmDoc, staticdir, templatesdir, oldNewNameMap, oldNewNameMapFile)
	if alarmdataDoc != None :
		alarmprofiles.mainMethod(resourcedataDoc, componentdataDoc, alarmdataDoc, associatedResDoc, associatedAlarmDoc, alarmRuleDoc, alarmAssociationDoc)
	clCompConfig.mainMethod(componentdataDoc, defaultTemplateDir, idlXml, templateGroupMap, codetemplateDir, oldNewNameMap, oldNewNameMapFile, codeGenMode,resourcedataDoc, associatedResDoc)
	om.mainMethod(resourcedataDoc, componentdataDoc, associatedResDoc, templateGroupMap, codetemplateDir, oldNewNameMap, oldNewNameMapFile, defaultTemplateDir, staticdir)

	#save the map file
	oldNewNameMapFile.close()

