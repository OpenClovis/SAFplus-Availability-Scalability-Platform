################################################################################
# ModuleName  : com
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python alarm.py <args>
# Creates enum for om
################################################################################
import sys
import string
import xml.dom.minidom
from string import Template
import time
import getpass
import omidsTemplate


#------------------------------------------------------------------------------
def processResourceList(resList, dic):
	idList = []
	idList.append("    CL_OM_CLASS_TYPE_START = CL_OM_CMN_CLASS_TYPE_END + 1")
	result = ""
	lastOmClassID = "CL_OM_CMN_CLASS_TYPE_END + 1"
	for res in resList:
		rsName = res.attributes["name"].value
                rsName = rsName.upper()
		prov = res.getElementsByTagName("provisioning")
		provEnabled = True
		if len(prov) > 0:
			provEnabled = prov[0].attributes["isEnabled"].value
			if provEnabled == "true":
				lastOmClassID = "CL_OM_PROV_" + rsName + "_CLASS_TYPE"
				provOmClassID = "CL_OM_PROV_" + rsName + "_CLASS_TYPE"
                       		d=dict();
				d["resourceID"] = provOmClassID;
				
				idList.append(omidsTemplate.omClassIDTemplate.safe_substitute(d))
				booleanFlag = "true"
		alarm = res.getElementsByTagName("alarmManagement")
		alarmEnabled = False
		if len(alarm) > 0:
			alarmEnabled = alarm[0].attributes["isEnabled"].value
			if alarmEnabled == "true":
				lastOmClassID = "CL_OM_ALARM_" + rsName + "_CLASS_TYPE"
				alarmOmClassID = "CL_OM_ALARM_" + rsName + "_CLASS_TYPE"
                        	d=dict();
				d["resourceID"] = alarmOmClassID;
                
                        	idList.append(omidsTemplate.omClassIDTemplate.safe_substitute(d))
				booleanFlag = "true"
		pm = res.getElementsByTagName("pm")
		pmEnabled = False
		if len(pm) > 0:
			pmEnabled = pm[0].attributes["isEnabled"].value
			if pmEnabled == "true":
				lastOmClassID = "CL_OM_PM_" + rsName + "_CLASS_TYPE"
				pmOmClassID = "CL_OM_PM_" + rsName + "_CLASS_TYPE"
                        	d=dict();
				d["resourceID"] = pmOmClassID;
                
                        	idList.append(omidsTemplate.omClassIDTemplate.safe_substitute(d))
				booleanFlag = "true"
	dic["lastOmClassID"] = lastOmClassID;
	idListLen = len(idList)
	i = 0
	for id in idList:
		if( i < idListLen-1 ):
			id += ",\n"
		result += id
		i += 1

	return result
#------------------------------------------------------------------------------

#Script Execution Starts Here.
def mainMethod(resourceDoc):
	
	resourceInfoObj = resourceDoc.getElementsByTagName("resource:resourceInformation")
	resourceList = resourceInfoObj[0].getElementsByTagName("chassisResource") + resourceInfoObj[0].getElementsByTagName("hardwareResource") + resourceInfoObj[0].getElementsByTagName("softwareResource") + resourceInfoObj[0].getElementsByTagName("mibResource") + resourceInfoObj[0].getElementsByTagName("systemController") + resourceInfoObj[0].getElementsByTagName("nodeHardwareResource") 
	d=dict();
	IDsList  = processResourceList(resourceList, d)
	d["IDsList"] = IDsList;
	
	result = omidsTemplate.omClassEnumTemplate.safe_substitute(d)
	out_file = open("clOmClassId.h", "w")
	out_file.write(result)
	out_file.close()


