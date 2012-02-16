################################################################################
# Description : Script for SNMP auto code generation
# usage: python snmpautocodegen.py
# create source files for MIB
################################################################################
from string import Template
import commands
import sys
import os
import xml.dom.minidom

def processCompModuleMap(mapList):
	cmMap = dict()
	for map in mapList :
		compName = map.attributes["compName"].value
		moduleName = map.attributes["moduleName"].value
		cmMap[compName] = moduleName
	return cmMap
	
def processCompPathMap(mapList):
	cpMap = dict()
	for map in mapList :
		compName = map.attributes["compName"].value
		mibPath = map.attributes["mibPath"].value
		cpMap[compName] = mibPath
	return cpMap
	
def processComponentList(compList, moduleMap, pathMap, compiler, srcDir, deployconfigDir, buildToolsDir):
	for	comp in compList :
		compName = comp.attributes["name"].value
		isSnmpAgent = comp.attributes["isSNMPSubAgent"].value
		if isSnmpAgent == "true":
			if compName in moduleMap and compName in pathMap:
				moduleName = moduleMap[compName]
				mibPath = pathMap[compName]
				if(moduleName != None and mibPath != None and moduleName != "" and mibPath != ""):
					perl5Lib=buildToolsDir + "/local/lib/perl:" + buildToolsDir + "/local/lib/perl5:$PERL5LIB"
					status = commands.getstatusoutput("cd " + srcDir + "/app/" + compName + ";" + "export PERL5LIB=" + perl5Lib + ";" + "export MIBDIRS=" + mibPath +";" + "export MIBS=+ALL;env MIBS=ALL;" + compiler + " -i -I " + srcDir + "/config/mib2c-config/mib2c-data -c " + srcDir + "/config/mib2c-config/mib2c.clovis.conf " + moduleName)
					print status
			out_file = open(deployconfigDir + "/" + moduleName + ".conf", "w")
			out_file.write("agentXSocket tcp:localhost:3456")
			out_file.close()
			
	
componentDoc = xml.dom.minidom.parse(sys.argv[1])
componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
componentList = componentInfoObj[0].getElementsByTagName("component") + componentInfoObj[0].getElementsByTagName("safComponent")

if os.path.exists(sys.argv[2]) == True:
	compModuleMapDoc = xml.dom.minidom.parse(sys.argv[2])
	mapList = compModuleMapDoc.getElementsByTagName("mibmap:Map")

	compModuleMap = processCompModuleMap(mapList)
	compPathMap = processCompPathMap(mapList)


	processComponentList(componentList,compModuleMap,compPathMap,sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6])
