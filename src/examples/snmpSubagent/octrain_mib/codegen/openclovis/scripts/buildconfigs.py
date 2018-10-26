################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python alarm.py <args>
# Author Pushparaj
################################################################################

import os
import sys
import xml.dom.minidom
import time
import getpass

from templates import buildConfigsTemplate

#------------------------------------------------------------------------------	
#------------------------------------------------------------------------------
def createNSHeader(ns):
	map = dict();
	map["maxentries"] = ns.attributes["maxentries"].value
	map["maxglobalcontexts"] = ns.attributes["maxglobalcontexts"].value
	map["maxlocalcontexts"] = ns.attributes["maxlocalcontexts"].value
	
	result = buildConfigsTemplate.nsHeaderTemplate.safe_substitute(map)
	return result
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
def createRMDHeader(rmd):
	map = dict();
	map["maxretries"] = rmd.attributes["maxretries"].value
		
	result = buildConfigsTemplate.rmdHeaderTemplate.safe_substitute(map)
	return result
	
#------------------------------------------------------------------------------

def mainMethod(srcDir, buildConfigsXmi):
	buildConfigsDoc = xml.dom.minidom.parse(buildConfigsXmi)
	buildConfigs = buildConfigsDoc.getElementsByTagName("CompileConfigs:ComponentsInfo")
	cor = buildConfigs[0].getElementsByTagName("COR")
	ns = buildConfigsDoc.getElementsByTagName("NS")
	result = createNSHeader(ns[0])
	rmd = buildConfigsDoc.getElementsByTagName("RMD")
	result+=createRMDHeader(rmd[0])
	
	cfgFile = os.path.join(srcDir, "config/clASPCfg.c")
	out_file = open(cfgFile, "w")
	out_file.write(buildConfigsTemplate.headerTemplate.safe_substitute())
	out_file.write(result)
	out_file.close()

