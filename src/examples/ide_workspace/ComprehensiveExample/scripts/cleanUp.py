#Clean up redundant files

import sys
import os
import xml.dom.minidom
import re

#---------------------------------------------------------------------------------------
#########Clean up *_rt.xml files from config directory
def clean_rtXml(nodeInstanceList):
	configDir = "config"
	fileList = os.listdir(configDir)
	for fileName in fileList:
		if re.match(".*_rt\.xml", fileName) :
			count = 0
			for nodeInstance in nodeInstanceList:
				nodeName = nodeInstance.attributes["name"].value
				xmlFile = nodeName+"_rt.xml"
				if( xmlFile == fileName):
					break
				count = count + 1	
			if(count == len(nodeInstanceList)):
				filePath = os.path.join(configDir, fileName)
				os.remove(filePath)
#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------	
########Deleting directory contents
def deleteDirContents(dirName):
	fileList = []
	if(os.path.isdir(dirName)):
		fileList = os.listdir(dirName)
	for fileName in fileList:
		filePath = os.path.join(dirName, fileName)
		os.remove(filePath)
#---------------------------------------------------------------------------------------
#---------------------------------------------------------------------------------------	

##Script execution starts here...

amfConfigXml = sys.argv[1]
componentXml = sys.argv[2]

#---------------------------------------------------------------------------------------
if os.path.exists(amfConfigXml):
	amfDoc = xml.dom.minidom.parse(amfConfigXml) 
	nodeInstanceList = amfDoc.getElementsByTagName("nodeInstance")
	
	########Cleaning *_rt.xml files from config directory
	clean_rtXml(nodeInstanceList)##call even if the list doesn't have any element

#---------------------------------------------------------------------------------------	

######Deleting "src/<compName>/dep/" directory
componentDoc = xml.dom.minidom.parse(componentXml)
componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation") 
componentList = componentInfoObj[0].getElementsByTagName("safComponent")

for comp in componentList :
	compName = comp.attributes["name"].value
	compDir = os.path.join("app", compName)
	depDir = os.path.join(compDir, "dep")
	objDir = os.path.join(compDir, "obj")
	deleteDirContents(depDir)
	deleteDirContents(objDir)
	
#---------------------------------------------------------------------------------------
