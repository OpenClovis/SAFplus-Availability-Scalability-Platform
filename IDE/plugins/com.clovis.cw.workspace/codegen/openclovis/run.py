import sys
import xml.dom.minidom
import os
import shutil

def copyconfigfiles(src, dst):
	names = os.listdir(src)
	if (os.path.exists(dst)==False):
		os.mkdir(dst)
	for name in names:
		if name != 'compileconfigs.xml':
			srcfile = os.path.join(src, name)
			dstfile = os.path.join(dst, name)
			if (os.path.exists(dstfile)==True):
				os.remove(dstfile)
			try:
				shutil.copy2(srcfile, dst)
			except (IOError, os.error), why:
				print "Can't copy %s to %s: %s" % (`srcfile`, `dst`, str(why))
		
def copystaticfiles(src, dst):
	names = os.listdir(src)
	if (os.path.exists(dst)==False):
		os.mkdir(dst)
	for name in names:
		if (os.path.isdir(os.path.join(src, name)) == True):
			if name != '.svn':
				dirpath = os.path.join(dst, name)
				if (os.path.exists(dirpath) == False):
					os.mkdir(dirpath)
					copystaticfiles(os.path.join(src,name), dirpath)
		elif not name.startswith('clSnmp') and not name.startswith('clAppAlarmProcess') and not name.startswith('clCompAppPM'):
			srcfile = os.path.join(src, name)
			dstfile = os.path.join(dst, name)
			if (os.path.exists(dstfile)==True):
				os.remove(dstfile)
			try:
				shutil.copy2(srcfile, dst)
			except (IOError, os.error), why:
				print "Can't copy %s to %s: %s" % (`srcfile`, `dst`, str(why))
 
projectLoc = sys.argv[1]
pkgLoc = sys.argv[2]
dataTypeXmi = sys.argv[3]
codeGenMode = sys.argv[4]
srcDir = sys.argv[5]
mibCompiler = sys.argv[6]
mibsPath = sys.argv[7]

modelsDir = os.path.join(projectLoc, "models")
configsDir = os.path.join(projectLoc, "configs")
codeGenDir = os.path.join(projectLoc, "codegen")
codeGenModeDir = os.path.join(codeGenDir, codeGenMode)

buildtoolsLoc = os.path.join(pkgLoc, "../buildtools")

resourcedataXmi = os.path.join(modelsDir, "resourcedata.xml")
componentdataXmi = os.path.join(modelsDir, "componentdata.xml")
alarmdataXmi = os.path.join(modelsDir, "alarmdata.xml")
compileConfigsXmi = os.path.join(projectLoc, ".temp_dir/configs/compileconfigs.xml")
amfConfigXmi = os.path.join(projectLoc, ".temp_dir/configs/clAmfConfig.xml")
buildConfigsXmi = os.path.join(projectLoc, ".temp_dir/configs/compileconfigs.xml")
staticdir = os.path.join(pkgLoc, "IDE/ASP/static/src")
templatesdir = os.path.join(pkgLoc, "IDE/ASP/templates")
modelchangesXmi = os.path.join(modelsDir, "modelchanges.xmi")
idlXml = os.path.join(projectLoc, "idl/server_idl_interface.xml")
associatedResXml = os.path.join(modelsDir, "component_resource_map.xml")
associatedAlarmXml = os.path.join(modelsDir, "resource_alarm_map.xml")
compTemplateGroupXml = os.path.join(modelsDir, "component_templateGroup_map.xml")
alarmRuleXml = os.path.join(modelsDir, "alarmrule.xml")
alarmAssociationXml = os.path.join(modelsDir, "alarmAssociation.xml")
compmibmapXmi = os.path.join(modelsDir, "compmibmap.xmi")

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

if os.path.exists(compileConfigsXmi):
	compileConfigsDoc = xml.dom.minidom.parse(compileConfigsXmi)
else:
	print "Cannot find file "+compileConfigsXmi 
if os.path.exists(dataTypeXmi):	
	dataTypeDoc = xml.dom.minidom.parse(dataTypeXmi)
else:
	print "Cannot find file "+dataTypeXmi 
		
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

if os.path.exists(alarmAssociationXml):
	alarmAssociationDoc = xml.dom.minidom.parse(alarmAssociationXml)
else :
	alarmAssociationDoc = None
#-------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------
sys.path[len(sys.path):] = [codeGenModeDir]

defaultTemplateDir = os.path.join(codeGenModeDir, "templates")

#-------------------------------------------------------------------------------------

#Model changes mapping

from scripts import oldNewNameMapping
if modelchangesDoc != None:
	oldNewNameMap = oldNewNameMapping.getOldNewNameMap(resourcedataDoc, componentdataDoc, modelchangesDoc)
else :
	oldNewNameMap = oldNewNameMapping.getDefaultOldNewNameMap(resourcedataDoc, componentdataDoc)
	
#Template group mapping

from scripts import templateGroup
templateGroupMap = dict()
templateGroupMap = templateGroup.getCompTemplateGroupMap(compTemplateGroupXml, componentdataDoc)
#-------------------------------------------------------------------------------------

from scripts import buildconfigs
from scripts import appMain
from scripts import configMain
from scripts import cleanUp
from scripts import makefile
from scripts import generatemoid
from scripts import snmpautocodegen

#----Copy static files
copystaticfiles(staticdir, os.path.join(srcDir, 'config'))

#----Copy config XML files----
copyconfigfiles(configsDir, os.path.join(srcDir, 'config'))

buildconfigs.mainMethod(srcDir, buildConfigsXmi)
appMain.mainMethod(resourcedataDoc, componentdataDoc, alarmdataDoc, associatedResDoc, associatedAlarmDoc, alarmRuleDoc, alarmAssociationDoc, staticdir, templatesdir, defaultTemplateDir, idlXml, templateGroupMap, codeGenModeDir, oldNewNameMap, codeGenMode)    
configMain.mainMethod(srcDir, resourcedataDoc, dataTypeDoc, compileConfigsDoc, alarmdataDoc, alarmAssociationDoc)
cleanUp.mainMethod(srcDir, amfConfigXmi, componentdataXmi)
makefile.mainMethod(srcDir)
generatemoid.mainMethod(srcDir)
snmpautocodegen.mainMethod(componentdataDoc, compmibmapXmi, mibCompiler, srcDir, buildtoolsLoc, mibsPath)
