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

#codetemplateDir = sys.argv[8]
#sys.path[len(sys.path):] = [os.path.join(codetemplateDir, "default")]

from templates import clAppSubAgentConfigTemplate

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
	
def processComponentList(compList, moduleMap, pathMap, compiler, srcDir, deployconfigDir, buildToolsDir, baseMibPath):
	for	comp in compList :
		compName = comp.attributes["name"].value
		isSnmpAgent = comp.attributes["isSNMPSubAgent"].value
		if isSnmpAgent == "true":
			if compName in moduleMap and compName in pathMap:
				moduleName = moduleMap[compName]
				mibPath = pathMap[compName]
				if(baseMibPath != None and baseMibPath != ""):
					mibPath = mibPath + ":" + baseMibPath
				if(moduleName != None and mibPath != None and moduleName != "" and mibPath != ""):
                                	def getPerlVersion():
						l = os.popen('perl -V::PERL_REVISION: '
                                                             '-V::PERL_VERSION: '
                                                             '-V::PERL_SUBVERSION:').readlines()
                                                l = l[0].split()
						assert len(l) == 3
						l = [e[1:-1] for e in l]
						return '.'.join(l)

					perlVersion = getPerlVersion()
                                        perl5LibPaths = [
                                                         'local/lib/perl/site_perl/%s' % perlVersion,
                                                         'local/lib/perl5/site_perl/%s' % perlVersion,
                                                         'local/lib/perl/',
                                                         'local/lib/perl5/',
                                                         'local/lib64/perl/site_perl/%s' % perlVersion,
                                                         'local/lib64/perl5/site_perl/%s' % perlVersion,
                                                         'local/lib64/perl',
                                                         'local/lib64/perl5',
                                                        ]
                                        perl5LibPaths = [os.path.abspath(buildToolsDir + os.path.sep + e) for e in perl5LibPaths]
                                        perl5LibPaths = filter(os.path.exists, perl5LibPaths)
                                        perl5Lib = os.path.pathsep.join(perl5LibPaths) + os.path.pathsep + '$PERL5LIB'
					moduleNames = moduleName.split(',')
					moduleNames = [e.strip() for e in moduleNames]
					moduleNames = [e for e in moduleNames if len(e) != 0]
					for name in moduleNames:
						status,output = commands.getstatusoutput("cd " + srcDir + "/app/" + compName + ";" + "export PERL5LIB=" + perl5Lib + ";" + "export MIBDIRS=" + mibPath +";" + "export MIBS=+ALL;env MIBS=ALL;" + compiler + " -i -I " + srcDir + "/config/mib2c-config/mib2c-data -c " + srcDir + "/config/mib2c-config/mib2c.clovis.conf " + name)
						print output
					global headerIncludes
					headerIncludes = ""
					global sourceIncludes
					sourceIncludes = ""
					global scalarsMap
					scalarsMap = dict()
					global tablesMap
					tablesMap = dict()
					global oidsMap
					oidsMap = dict()
					global callbacksMap
					callbacksMap = dict()	
					for name in moduleNames:
						import string
						for i in string.punctuation:
							if i in name:
								name = name.replace(i,"_")
						doc = xml.dom.minidom.parse(srcDir + "/app/" + compName + "/mib2c/" + "clAppSubAgent" +  name +  "Config.xml")
						headerIncludes += clAppSubAgentConfigTemplate.includeTemplate.safe_substitute(includeFileName="clSnmpSubAgent" + name + "Config.h")
						sourceIncludes += clAppSubAgentConfigTemplate.includeTemplate.safe_substitute(includeFileName="clSnmp" + name + "InstXlation.h")
						sourceIncludes += clAppSubAgentConfigTemplate.includeTemplate.safe_substitute(includeFileName="clSnmp" + name + "Util.h")
						mibDataObj = doc.getElementsByTagName("mibDataT")[0]
						scalarsObj = mibDataObj.getElementsByTagName("scalarsT")[0]
						if len(scalarsObj.getElementsByTagName("scalar")) > 0:
							scalarsMap[name] = scalarsObj.getElementsByTagName("scalar")
						tablesObj = mibDataObj.getElementsByTagName("tablesT")[0]
						tablesMap[name] = tablesObj.getElementsByTagName("table")
						appOidsObj =  mibDataObj.getElementsByTagName("appOidInfoT")[0]
						oidsMap[name] = appOidsObj.getElementsByTagName("appOidInfo")
						callbacksObj = mibDataObj.getElementsByTagName("appNotifyCallbackT")[0]
						if len(callbacksObj.getElementsByTagName("appCallback")) > 0:
							sourceIncludes += clAppSubAgentConfigTemplate.includeTemplate.safe_substitute(includeFileName="clSnmp" + name + "Notifications.h")
						callbacksMap[name] = callbacksObj.getElementsByTagName("appCallback")
					createConfigFiles(srcDir + "/app/" + compName + "/mib2c/") 	
			out_file = open(deployconfigDir + "/" + "snmp_subagent.conf", "w")
			out_file.write("agentXSocket tcp:localhost:3456")
			out_file.close()
			
def createConfigFiles(mib2cDir):
	unions = ""
	enums = ""
	appOidInfos = ""
	trapInfos = ""
	moduleInits = ""
	if len(scalarsMap) > 0:
		unions += clAppSubAgentConfigTemplate.scalarUnionTemplate.safe_substitute()
	for module in tablesMap.keys():
		moduleInits += "    init_" + module + "MIB();\n"
		tables = tablesMap[module]
		enums += "\n    /* " + module + " */\n"
		if scalarsMap.has_key(module):
			enums += clAppSubAgentConfigTemplate.scalarEnumTemplate.safe_substitute(moduleName=module.upper())
			
		for table in tables:
			unions += clAppSubAgentConfigTemplate.tableUnionTemplate.safe_substitute(tableName=table.attributes["name"].value)
			enums += clAppSubAgentConfigTemplate.tableEnumTemplate.safe_substitute(tableName=table.attributes["name"].value.upper())	

		oids = oidsMap[module]
		for oid in oids:
			oidInfoMap = dict()			
			attrTypes = ""
			attrs = oid.getElementsByTagName("attrT")
			for attr in attrs:
				attrTypeMap = dict()
				attrTypeMap["type"] = attr.attributes["type"].value
				attrTypeMap["size"] = attr.attributes["size"].value
				attrTypes += clAppSubAgentConfigTemplate.attrTypeTemplate.safe_substitute(attrTypeMap)
			oidInfoMap["tableType"] = oid.attributes["tableType"].value
			oidInfoMap["oid"] = oid.attributes["oid"].value
			oidInfoMap["numAttr"] = oid.attributes["numAttr"].value
			oidInfoMap["fpIndexGet"] = oid.attributes["fpIndexGet"].value
			oidInfoMap["fpInstXlator"] = oid.attributes["fpInstXlator"].value
			oidInfoMap["fpInstCompare"] = oid.attributes["fpInstCompare"].value
			oidInfoMap["attrTypes"] = attrTypes
			appOidInfos += clAppSubAgentConfigTemplate.appOidInfoTemplate.safe_substitute(oidInfoMap)

		callbacks = callbacksMap[module]
		for callback in callbacks:
			trapInfoMap = dict()
			trapInfoMap["trapOid"] = callback.attributes["trapOid"].value
			trapInfoMap["fpIndexGet"] = callback.attributes["fpIndexGet"].value
			trapInfoMap["notifyCallback"] = callback.attributes["notifyCallback"].value
			trapInfos += clAppSubAgentConfigTemplate.trapTemplate.safe_substitute(trapInfoMap)
	
	configFilesMap = dict()
	configFilesMap["enums"] = enums
        if not unions: # Add at least one item in so union is not empty causing a compilation failure.
	     unions = clAppSubAgentConfigTemplate.scalarUnionTemplate.safe_substitute()	
	configFilesMap["unions"] = unions
	configFilesMap["appOidInfos"] = appOidInfos
	configFilesMap["trapInfos"] = trapInfos
	configFilesMap["sourceIncludes"] = sourceIncludes
	configFilesMap["headerIncludes"] = headerIncludes
	configFilesMap["moduleInits"] = moduleInits

	source_file = open(mib2cDir + "clAppSubAgentConfig.c", "w")
	source_file.write(clAppSubAgentConfigTemplate.clAppSubAgentConfigSourceFileTemplate.safe_substitute(configFilesMap))
	source_file.close()		

	header_file = open(mib2cDir + "clAppSubAgentConfig.h", "w")
	header_file.write(clAppSubAgentConfigTemplate.clAppSubAgentConfigHeaderFileTemplate.safe_substitute(configFilesMap))
	header_file.close()

def mainMethod(componentDoc, compmibmapXmi, mibCompiler, srcDir, buildtoolsLoc, mibsPath):
	configDir = os.path.join(srcDir, "config")
	componentInfoObj = componentDoc.getElementsByTagName("component:componentInformation")
	componentList = componentInfoObj[0].getElementsByTagName("component") + componentInfoObj[0].getElementsByTagName("safComponent")

	if os.path.exists(compmibmapXmi) == True:
		compModuleMapDoc = xml.dom.minidom.parse(compmibmapXmi)
		mapList = compModuleMapDoc.getElementsByTagName("mibmap:Map")

		compModuleMap = processCompModuleMap(mapList)
		compPathMap = processCompPathMap(mapList)

		processComponentList(componentList,compModuleMap,compPathMap, mibCompiler, srcDir, configDir, buildtoolsLoc, mibsPath)
