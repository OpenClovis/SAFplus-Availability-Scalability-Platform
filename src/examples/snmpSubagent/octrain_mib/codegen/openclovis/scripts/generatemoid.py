################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python generatemoid.py <args>
# create moId.h
################################################################################

import sys
import os
import xml.dom.minidom
import time
import getpass
from templates import moIdTemplate

def generateMoIds(srcDir, nodeInstanceList):
 	moIdHeaderFile = os.path.join(srcDir, "config/clMOID.h")
	out_file = open(moIdHeaderFile,"w")
	moIDs = ""
	map = dict()
	for nodeInst in nodeInstanceList:
		nodeName = nodeInst.attributes["name"].value
		node_rtfile = os.path.join(srcDir, "config/" + nodeName + "_rt.xml")
		if os.path.exists(node_rtfile):
			rtDoc = xml.dom.minidom.parse(node_rtfile) 
			resMoIdList = rtDoc.getElementsByTagName("resource")
			uniqueMoIdList = []
			
			for resMoId in resMoIdList:
				moId = resMoId.attributes["moID"].value
				if  uniqueMoIdList.count(moId) == 0:
					uniqueMoIdList.append(moId)
					map1 = dict();
					moIdString_ = moId.replace(":","_")
					moIdString = moIdString_.replace("\\","_")
					moIdString = moIdString.lstrip("_")
					length = len(moId)
					if moId[length-1] == "\\":
						moId = moId[:length-1]
					map1['moId'] = moId
					moIdString = moIdString.replace("*", "All")
					map1['moIdString'] = moIdString
					moIDs += moIdTemplate.idTemplate.safe_substitute(map1)
						
	map['moIDs'] = moIDs			
	out_file.write(moIdTemplate.mainTemplate.safe_substitute(map))
	out_file.close()

def mainMethod(srcDir):
	amfConfigXml = os.path.join(srcDir, "/config/clAmfConfig.xml")
	if os.path.exists(amfConfigXml):
		amfDoc = xml.dom.minidom.parse(amfConfigXml) 
		nodeInstanceList = amfDoc.getElementsByTagName("nodeInstance")
		generateMoIds(srcDir, nodeInstanceList)
