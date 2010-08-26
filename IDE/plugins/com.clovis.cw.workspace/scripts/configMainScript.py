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
alarmAssociationXml = sys.argv[11]

if os.path.exists(resourcedataXmi):
	resourcedataDoc = xml.dom.minidom.parse(resourcedataXmi)
else:
	print "Cannot find file "+resourcedataXmi 
if os.path.exists(componentdataXmi):
	componentdataDoc = xml.dom.minidom.parse(componentdataXmi)
else:
	print "Cannot find file "+componentdataXmi 

if os.path.exists(compileConfigsXmi):
	compileConfigsDoc = xml.dom.minidom.parse(compileConfigsXmi)
else:
	print "Cannot find file "+compileConfigsXmi 
if os.path.exists(dataTypeXmi):	
	dataTypeDoc = xml.dom.minidom.parse(dataTypeXmi)
else:
	print "Cannot find file "+dataTypeXmi 

if os.path.exists(alarmdataXmi):
	alarmDataDoc = xml.dom.minidom.parse(alarmdataXmi)
else :
	alarmDataDoc = None
if os.path.exists(alarmAssociationXml):	
	alarmAssociationDoc = xml.dom.minidom.parse(alarmAssociationXml)
else:
	alarmAssociationDoc = None

sys.path[len(sys.path):] = [os.path.join(codetemplateDir, "default")]

import cor
import fm
import omids

cor.mainMethod(resourcedataDoc, dataTypeDoc, compileConfigsDoc, alarmDataDoc, alarmAssociationDoc)
fm.mainMethod(compileConfigsDoc, resourcedataDoc)
omids.mainMethod(resourcedataDoc)
