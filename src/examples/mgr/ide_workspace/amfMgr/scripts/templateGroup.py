################################################################################
# Description :
# usage: python templateGroup.py <args>
# Template Group Mapping for components
################################################################################


import os
import xml.dom.minidom
import sys


#-----------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------
def getCompTemplateGroupMap(compTemplateGroupXml, componentDoc):
	compTemplateGroupMap = dict()
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	componentList = componentInfoObj[0].getElementsByTagName("safComponent")
	
	if os.path.exists(compTemplateGroupXml):
		compTemplateGroupDoc = xml.dom.minidom.parse(compTemplateGroupXml)
		safComponentInfoObj = compTemplateGroupDoc.getElementsByTagName("templateGroupMap:componentMapInformation")
		safComponentList = safComponentInfoObj[0].getElementsByTagName("safComponent")
		for comp in safComponentList:
			compName = comp.attributes["name"].value
			templateGroup = comp.attributes["templateGroupName"].value
			compTemplateGroupMap[compName] = templateGroup
			
	for comp in componentList:
		compName = comp.attributes["name"].value
		if not compTemplateGroupMap.has_key(compName):
			compTemplateGroupMap[compName] = "default"
	
	return compTemplateGroupMap
	
#-----------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------	