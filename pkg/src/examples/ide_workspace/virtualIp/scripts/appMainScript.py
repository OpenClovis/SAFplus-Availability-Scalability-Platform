import sys
import xml.dom.minidom
import os

resourcedataXmi = sys.argv[1]
componentdataXmi = sys.argv[2]
alarmdataXmi = sys.argv[3]
compileConfigsXmi = sys.argv[4]
staticdir = sys.argv[5]
templatesdir = sys.argv[6]
dataTypeXmi = sys.argv[7]
modelchangesXmi = sys.argv[8]
codetemplateDir = sys.argv[9]
idlXml = sys.argv[10]
associatedResXml = sys.argv[11]
associatedAlarmXml = sys.argv[12]
compTemplateGroupXml = sys.argv[13]
alarmRuleXml = sys.argv[14]

if os.path.exists(resourcedataXmi):
	resourcedataDoc = xml.dom.minidom.parse(resourcedataXmi)
else:
	print "Cannot find file "+resourcedataXmi 
if os.path.exists(componentdataXmi):
	componentdataDoc = xml.dom.minidom.parse(componentdataXmi)
else:
	print "Cannot find file "+componentdataXmi 
	
if os.path.exists(associatedResXml):
	associatedResDoc = xml.dom.minidom.parse(associatedResXml)
else:
	print "Cannot find file "+associatedResXml
	
if os.path.exists(associatedAlarmXml):
	associatedAlarmDoc = xml.dom.minidom.parse(associatedAlarmXml)
else:
	print "Cannot find file "+associatedAlarmXml	
	
if os.path.exists(alarmdataXmi):
	alarmdataDoc = xml.dom.minidom.parse(alarmdataXmi)
else :
	alarmdataDoc = None

if os.path.exists(modelchangesXmi):
	modelchangesDoc = xml.dom.minidom.parse(modelchangesXmi)
else :
	modelchangesDoc = None

if os.path.exists(alarmRuleXml):
	alarmRuleDoc = xml.dom.minidom.parse(alarmRuleXml)
else :
	alarmRuleDoc = None

#-------------------------------------------------------------------------------------
#Model changes mapping

import oldNewNameMapping
if modelchangesDoc != None:
	oldNewNameMap = oldNewNameMapping.getOldNewNameMap(resourcedataDoc, componentdataDoc, modelchangesDoc)
else :
	oldNewNameMap = oldNewNameMapping.getDefaultOldNewNameMap(resourcedataDoc, componentdataDoc)
	
#createthe new mapping file each time
oldNewNameMapFile = open("../../.nameMapFile", "w")

#-------------------------------------------------------------------------------------
sys.path[len(sys.path):] = [codetemplateDir]
import alarmprofiles
import clCompConfig
import clCompSNMPConfig
import om

defaultTemplateDir = os.path.join(codetemplateDir, "default")

#-------------------------------------------------------------------------------------
#Template group mapping

import templateGroup
templateGroupMap = dict()
templateGroupMap = templateGroup.getCompTemplateGroupMap(compTemplateGroupXml, componentdataDoc)
#-------------------------------------------------------------------------------------

clCompSNMPConfig.mainMethod(resourcedataDoc, componentdataDoc, alarmdataDoc, associatedAlarmDoc, staticdir, templatesdir, oldNewNameMap, oldNewNameMapFile)
if alarmdataDoc != None :
	alarmprofiles.mainMethod(resourcedataDoc, componentdataDoc, alarmdataDoc, associatedResDoc, associatedAlarmDoc, alarmRuleDoc)
clCompConfig.mainMethod(componentdataDoc, defaultTemplateDir, idlXml, templateGroupMap, codetemplateDir, oldNewNameMap, oldNewNameMapFile)
om.mainMethod(resourcedataDoc, componentdataDoc, associatedResDoc, templateGroupMap, codetemplateDir, oldNewNameMap, oldNewNameMapFile, defaultTemplateDir)

#save the map file
oldNewNameMapFile.close()
