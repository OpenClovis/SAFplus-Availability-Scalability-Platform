################################################################################
# ModuleName  : Demo
# $File$
# $Author$
# $Date$
################################################################################
# Description :
# usage: python alarm.py <args>
# Creates enum for om
#############################################################################
import sys
import string
import xml.dom.minidom
from string import Template

# Enum Template
omClassEnumTemplate = Template("""\
#ifndef _CL_OM_CLASS_ID_H_
#define _CL_OM_CLASS_ID_H_
                                                                                                                                                                                    
#ifdef __cplusplus
extern "C"
{
#endif

#include <clOmCommonClassTypes.h>

#define OM_APP_MIN_CLASS_TYPE (CL_OM_CMN_CLASS_TYPE_END + 1)
#define OM_APP_MAX_CLASS_TYPE (${lastOmClassID})

enum ClOmClassIdsT
{
${IDsList}	
};

#ifdef __cplusplus
}
#endif
#endif /* _CL_OM_CLASS_ID_H_ */

""")

# ID Template
omClassIDTemplate = Template("""    ${resourceID}""")

#------------------------------------------------------------------------------
def processResourceList(resList, dic):
	idList = []
	idList.append("    CL_OM_CLASS_TYPE_START = CL_OM_CMN_CLASS_TYPE_END + 1")
	result = ""
	lastOmClassID = "CL_OM_CMN_CLASS_TYPE_END + 1"
	for res in resList:
		rsName = res.attributes["Name"].value
                rsName = rsName.upper()
		alarm = res.getElementsByTagName("AlarmManagement")
		alarmEnabled = False
		if len(alarm) > 0:
			alarmEnabled = alarm[0].attributes["isEnabled"].value
			if alarmEnabled == "true":
				lastOmClassID = "CL_OM_ALARM_" + rsName + "_CLASS_TYPE"
				alarmOmClassID = "CL_OM_ALARM_" + rsName + "_CLASS_TYPE"
                        	d=dict();
				d["resourceID"] = alarmOmClassID;
                
                        	idList.append(omClassIDTemplate.safe_substitute(d))
				booleanFlag = "true"
		prov = res.getElementsByTagName("Provisioning")
		provEnabled = True
		if len(prov) > 0:
			provEnabled = prov[0].attributes["isEnabled"].value
			if provEnabled == "true":
				lastOmClassID = "CL_OM_PROV_" + rsName + "_CLASS_TYPE"
				provOmClassID = "CL_OM_PROV_" + rsName + "_CLASS_TYPE"
                       		d=dict();
				d["resourceID"] = provOmClassID;
				
				idList.append(omClassIDTemplate.safe_substitute(d))
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
resourceXmi = sys.argv[1];

resourceDoc = xml.dom.minidom.parse(resourceXmi)
resourceList = resourceDoc.getElementsByTagName("resource:SoftwareResource")+resourceDoc.getElementsByTagName("resource:HardwareResource")+resourceDoc.getElementsByTagName("resource:NodeHardwareResource")+resourceDoc.getElementsByTagName("resource:SystemController")+resourceDoc.getElementsByTagName("resource:ChassisResource")
d=dict();
IDsList  = processResourceList(resourceList, d)
d["IDsList"] = IDsList;
result = omClassEnumTemplate.safe_substitute(d)
out_file = open("clOmClassId.h", "w")
out_file.write(result)
out_file.close()


