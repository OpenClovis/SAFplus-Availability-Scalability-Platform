################################################################################
# Description :
# usage: python oldNewNameMapping.py <args>
# Old new name Mapping for nodes
################################################################################


import os
import xml.dom.minidom


#-----------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------
def getOldNewNameMap(resourceDoc, componentDoc, modelchangesDoc):
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	
	componentList = componentInfoObj[0].getElementsByTagName("nonSAFComponent") + componentInfoObj[0].getElementsByTagName("safComponent")
	resourceList = resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("mibResource") + resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource") + resourceInfoObj[0].getElementsByTagName("systemController") + resourceInfoObj[0].getElementsByTagName("chassisResource")
	
	modelchangesList = modelchangesDoc.getElementsByTagName("ModelChanges:SAFComponent") + modelchangesDoc.getElementsByTagName("ModelChanges:NonSAFComponent") + modelchangesDoc.getElementsByTagName("ModelChanges:NodeHardwareResource") + modelchangesDoc.getElementsByTagName("ModelChanges:HardwareResource") + modelchangesDoc.getElementsByTagName("ModelChanges:SoftwareResource") + modelchangesDoc.getElementsByTagName("ModelChanges:MibResource")
	
	modelChangeMap = dict()
	
		
	for comp in componentList:
		compName = comp.attributes["name"].value
		modelChangeMap[compName] = compName

	for res in resourceList:
		resName = res.attributes["name"].value
		modelChangeMap[resName] = resName

	for node in modelchangesList:
		newName = node.attributes["NewName"].value
		oldName = node.attributes["OldName"].value	
		try:		
			modelChangeMap[newName] = modelChangeMap[oldName]
		except:
			modelChangeMap[newName] = oldName
		
	return modelChangeMap;

#-----------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------	
def getDefaultOldNewNameMap(resourceDoc, componentDoc):
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	
	componentList = componentInfoObj[0].getElementsByTagName("nonSAFComponent") + componentInfoObj[0].getElementsByTagName("safComponent")
	resourceList = resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("mibResource") + resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource") + resourceInfoObj[0].getElementsByTagName("systemController") + resourceInfoObj[0].getElementsByTagName("chassisResource")
	
	modelChangeMap = dict()
		
	for comp in componentList:
		compName = comp.attributes["name"].value
		modelChangeMap[compName] = compName

	for res in resourceList:
		resName = res.attributes["name"].value
		modelChangeMap[resName] = resName
	
	return modelChangeMap;
	
#-----------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------
def getNewOldEONameMap(modelchangesDoc, componentDoc):
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	
	componentList = componentInfoObj[0].getElementsByTagName("safComponent")

	modelchangesList = modelchangesDoc.getElementsByTagName("ModelChanges:EO")

	modelChangeMap = dict()
	
	for comp in componentList:
		EoPropertiestObj = comp.getElementsByTagName("eoProperties")
		EOName = EoPropertiestObj[0].attributes["eoName"].value
		modelChangeMap[EOName] = EOName
	
	for EOobj in modelchangesList:
		newName = EOobj.attributes["NewName"].value
		oldName = EOobj.attributes["OldName"].value			
		try:		
			modelChangeMap[newName] = modelChangeMap[oldName]
		except:
			modelChangeMap[newName] = oldName
		
	return modelChangeMap;

#-----------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------
def getDefaultNewOldEONameMap(componentDoc):
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	
	componentList = componentInfoObj[0].getElementsByTagName("safComponent")

	modelChangeMap = dict()
	
	for comp in componentList:
		EoPropertiestObj = comp.getElementsByTagName("eoProperties")
		EOName = EoPropertiestObj[0].attributes["eoName"].value
		modelChangeMap[EOName] = EOName
	
	return modelChangeMap;
			
#-----------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------
#Creates rdn to name map for PORT and GROUP of idl only when change type is modify
def getRdnToOldNameMap(modelchangesDoc):

	modelchangesList = modelchangesDoc.getElementsByTagName("ModelChanges:PORT") + modelchangesDoc.getElementsByTagName("ModelChanges:GROUP")

	modelChangeMap = dict()
		
	for obj in modelchangesList:
		rdn = obj.attributes["rdn"].value
		oldName = obj.attributes["OldName"].value
		changeType = obj.attributes["ChangeType"].value
		
		if not modelChangeMap.has_key(rdn) and changeType == "modify":
			modelChangeMap[rdn] = oldName
		
	return modelChangeMap;
		
#-----------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------
	