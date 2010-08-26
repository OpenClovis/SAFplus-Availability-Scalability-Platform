################################################################################
# ModuleName  : Demo
# $File$
# $Author$
# $Date$
################################################################################
# Description :
################################################################################
# cor.py
# usage: python cor.py <args>
# Author Pushparaj
# DO
#
# Copyright (c) 2005 Clovis Solutions
#

import sys
import string
import xml.dom.minidom
from string import Template

headerTemplate = Template("""\
#ifndef _CL_${COMPNAME}HAL_CONF_H_
#define _CL_${COMPNAME}HAL_CONF_H_

#include <clCommon.h>
#include <clHalApi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * List of user specified operations for HAL.
 * The user can add his operations here depending on the requirement.   
 * In this example user has not added any operation to the default List defined 
 * in clHalApi.h
 */
#define USER_DEFINED_OP_ADDED       CL_FALSE

#if USER_DEFINED_OP_ADDED == CL_TRUE
    typedef enum
        {
            HAL_DEV_USER_DEFINED_OPERATION = CL_HAL_DEV_NUM_STD_OPERATIONS + 1
        }ClHalDevUserOperationT;
    #define HAL_DEV_NUM_OPERATIONS  (HAL_DEV_USER_DEFINED_OPERATION+1)
#else
    #define HAL_DEV_NUM_OPERATIONS (HAL_DEV_NUM_STD_OPERATIONS+1)
#endif
/**
 * The Device Id of the Ethernet Device Object , the name space of device Id is the
 * process.This implies that device Id needs to be unique in a process context but
 * same device ID could be used to address two different devices provided they are 
 * managed by different components/processes. 
 */

${macros}
    
#ifdef __cplusplus
}
#endif

#endif /* _CL_${COMPNAME}HAL_CONF_H_ */


""")
sourceTemplate = Template("""\

/*Clovis Includes */
#include <clHal.h>
#include <cl${compName}HalConf.h>
#include <clOmClassId.h>

${functions}

${devOperations}

/* This table will be used by the initialization function of the process
 * to create all the Device  Objects , corresponding to the resources 
 * managed by this process.  
 */
${DevObjectTable}
/*
The section below conatins the Configuration code for different MSO to attach
appropriate HAL Objects to their OM objects. 
*/	
${HalDevObjectInfoTable}
	
	/*List of HAL Objects for the component */
${HalObjectsTable}
	
	ClHalConfT appHalConfig= 
    {
        HAL_DEV_NUM_OPERATIONS, /*From halConf.h*/
        (sizeof(${compName}devObjectTable))/(sizeof(ClHalDevObjectT)),/*halNumDevObject*/
        ${compName}devObjectTable,/*DevObject Table */
        sizeof(${compName}halObjConf)/sizeof(ClHalObjConfT),/*halNumHalObject*/
        ${compName}halObjConf/* Hal Object Table */
    };
""")

halObjectsTableTemplate = Template("""\
	ClHalObjConfT ${compName}halObjConf[]= 
    {
${halObjects}        
    };

""")

halObjectDefTemplate = Template("""\
		{
            ${OMClassID},/*om ClassId*/
            "${MOID}",/*MOID in string  for port 0*/
            ${compName}HalDevObjInfo, /* Information about the Devices */
            /*Num of DevObjects */
            (sizeof(${compName}HalDevObjInfo))/(sizeof(ClHalDevObjectInfoT))   
        },	
""")

halDevObjectInfoTableTemplate = Template("""\
	ClHalDevObjectInfoT ${compName}HalDevObjInfo[]=
    {
${halDevObjectInfos}
        
    };
""")

halDevObjectInfoDefTemplate = Template("""\
		{
          ${deviceID} /* Device ID */,
          ${accessPriority} /*Device Access Priority*/
        },
""")
devObjectTableTemplate = Template("""\
	ClHalDevObjectT devObjectTable[]=
	{
${devObjects}    	
    };
""")

devObjectTemplate = Template("""\
		{
        	${deviceID},/*deviceId*/
            NULL,/*pdevCapability*/
            0,/*devCapLen*/
            ${maxResponseTime},/*Max Response Time */
            ${bootPriority},/*Boot Up Priority*/
            ${compName}${deviceID}DevOperations
        },
""")

devOperationTemplate = Template("""\
	ClfpDevOperationT ${compName}${deviceID}DevOperations[HAL_DEV_NUM_OPERATIONS]=
    {
    	${HAL_DEV_INIT},/*HAL_DEV_INIT */
        ${HAL_DEV_OPEN}, /*HAL_DEV_OPEN */
        ${HAL_DEV_CLOSE}, /*HAL_DEV_CLOSE */
        ${HAL_DEV_READ}, /*HAL_DEV_READ*/
        ${HAL_DEV_WRITE}, /*HAL_DEV_WRITE */
        ${HAL_DEV_COLD_BOOT}, /*HAL_DEV_COLD_BOOT*/
        ${HAL_DEV_WARM_BOOT}, /*HAL_DEV_WARM_BOOT*/
        ${HAL_DEV_PWR_OFF}, /*HAL_DEV_PWR_OFF*/
        ${HAL_DEV_IMAGE_DN_LOAD}, /*HAL_DEV_IMAGE_DN_LOAD*/
        ${HAL_DEV_DIRECT_ACCESS} /*HAL_DEV_DIRECT_ACCESS*/
     };
""")

functionTemplate = Template("""\
ClRcT ${funcName}
    (ClUint32T omId, 
     ClCorMOIdPtrT moId,
     ClUint32T subOperation,
     void *pUserData,
     ClUint32T usrDataLen) 
{
    ClRcT   rc = CL_OK;
    
    /* */
    
    return rc;
}
""")

externTemplate = Template("""\
extern ClRcT ${funcName}
    (ClUint32T omId, 
     ClCorMOIdPtrT moId,
     ClUint32T subOperation,
     void *pUserData,
     ClUint32T usrDataLen); 
""")

macroTemplate = Template("""\
#define ${macroName}		 ${macroID}
""")	     
#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------
def parseComponents(componentsList):
	for comp in componentsList:
		realizes = comp.getElementsByTagName("Realizes")
		compName = comp.attributes["Name"].value
 		halObjects = ""
		devOperations = ""
		functions = ""
		for realize in realizes:
			resource = resourceKeyMap[realize.firstChild.data]
			resName = resource.attributes["Name"].value
			alarms = resource.getElementsByTagName("AlarmManagement")
			if len(alarms) > 0 and alarms[0].attributes["isEnabled"].value=="true":
				omClassMap = dict();
				omClassMap["OMClassID"]	= "CL_OM_ALARM_" + resName.upper() + "_CLASS_TYPE"
				omClassMap["MOID"] = getMOID(resource,"\\\\" + resName + "*")
				omClassMap["compName"] = compName
				halObjects += halObjectDefTemplate.safe_substitute(omClassMap)
			provs = resource.getElementsByTagName("Provisioning")
			if len(provs) > 0 and provs[0].attributes["isEnabled"].value=="true":
				omClassMap = dict();
				omClassMap["OMClassID"]	= "CL_OM_PROV_" + resName.upper() + "_CLASS_TYPE,"
				omClassMap["MOID"] = getMOID(resource,"\\\\" + resName + "*")
				omClassMap["compName"] = compName
				halObjects += halObjectDefTemplate.safe_substitute(omClassMap)
		halDevObjectInfos = ""
		devObjects = ""
		
		macroID = 0;
		macros = ""		
		dos = comp.getElementsByTagName("DeviceObjects")
		for do in dos:
			doMap = dict();
			doMap["compName"] = compName
			deviceName = do.attributes["DeviceID"].value
			doMap["deviceID"] = deviceName
			doMap["accessPriority"] = do.attributes["AccessPriority"].value
			doMap["bootPriority"] = do.attributes["BootPriority"].value
			doMap["maxResponseTime"] = do.attributes["MaxResponseTime"].value
			macroID = macroID + 1
			macroMap = dict();
			macroMap["macroName"] = deviceName
			macroMap["macroID"] = macroID
			macros += macroTemplate.safe_substitute(macroMap)
			halDevObjectInfos  += halDevObjectInfoDefTemplate.safe_substitute(doMap)
			devObjects += devObjectTemplate.safe_substitute(doMap)
			doops = do.getElementsByTagName("DeviceOperations")
			if len(doops) > 0:
				devOpMap = createDevOperation(doops[0], compName, do.attributes["DeviceID"].value)
				devOperations += devOpMap["devOperations"]
				functions += devOpMap["functions"]				
		map = dict();
		map["compName"] = compName;
		map["halObjects"] = halObjects
		map["halDevObjectInfos"] = halDevObjectInfos
		map["devObjects"] = devObjects
		map["HalObjectsTable"] = halObjectsTableTemplate.safe_substitute(map)
		map["HalDevObjectInfoTable"] = halDevObjectInfoTableTemplate.safe_substitute(map)
		map["DevObjectTable"] = devObjectTableTemplate.safe_substitute(map)
		map["devOperations"] = devOperations
		map["functions"] = functions
		map["COMPNAME"] = compName.upper()
		map["macros"] = macros
		
		source = sourceTemplate.safe_substitute(map)
		out_file = open("cl" + compName + "HalConf.c", "w")
		out_file.write(source)
		out_file.close()
		
		header = headerTemplate.safe_substitute(map)
		out_file = open("cl" + compName + "HalConf.h", "w")
		out_file.write(header)
		out_file.close()		
#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------
def parseConnections(compositionsList):
	for comp in compositionsList:
		source = comp.attributes["source"].value
		target = comp.attributes["target"].value
		resourceParentMap[target] = source;
#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------
def parseResources(resourcesList):
	for res in resourcesList:
		key = res.attributes["CWKEY"].value
		resourceKeyMap[key] = res;
#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------
def getMOID(res, moid):
	key = res.attributes["CWKEY"].value
	if resourceParentMap.get(key) == None:
		return moid
	else:
		resource = resourceKeyMap[resourceParentMap.get(key)]
		resName = resource.attributes["Name"].value
		moid = "\\\\" + resName + "*" + moid
		return getMOID(resource, moid)
#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------
def createDevOperation(oper, name, id):
	map = dict()
	map["compName"] = name
	map["deviceID"] = id 
	functions = ""
	HAL_DEV_INIT = oper.attributes["HAL_DEV_INIT"].value
	if HAL_DEV_INIT == None or HAL_DEV_INIT == "":
		HAL_DEV_INIT = "NULL"
	else:
		opMap = dict();
		opMap["funcName"] = HAL_DEV_INIT
		functions += functionTemplate.safe_substitute(opMap)	
	map["HAL_DEV_INIT"] = HAL_DEV_INIT
	HAL_DEV_OPEN = oper.attributes["HAL_DEV_OPEN"].value
	if HAL_DEV_OPEN == None or HAL_DEV_OPEN == "":
		HAL_DEV_OPEN = "NULL"
	else:
		opMap = dict();
		opMap["funcName"] = HAL_DEV_OPEN
		functions += functionTemplate.safe_substitute(opMap)	
	map["HAL_DEV_OPEN"] = HAL_DEV_OPEN
	HAL_DEV_CLOSE = oper.attributes["HAL_DEV_CLOSE"].value
	if HAL_DEV_CLOSE == None or HAL_DEV_CLOSE == "":
		HAL_DEV_CLOSE = "NULL"
	else:
		opMap = dict();
		opMap["funcName"] = HAL_DEV_CLOSE
		functions += functionTemplate.safe_substitute(opMap)	
	map["HAL_DEV_CLOSE"] = HAL_DEV_CLOSE
	HAL_DEV_READ = oper.attributes["HAL_DEV_READ"].value
	if HAL_DEV_READ == None or HAL_DEV_READ == "":
		HAL_DEV_READ = "NULL"
	else:
		opMap = dict();
		opMap["funcName"] = HAL_DEV_READ
		functions += functionTemplate.safe_substitute(opMap)	
	map["HAL_DEV_READ"] = HAL_DEV_READ
	HAL_DEV_WRITE = oper.attributes["HAL_DEV_WRITE"].value
	if HAL_DEV_WRITE == None or HAL_DEV_WRITE == "":
		HAL_DEV_WRITE = "NULL"
	else:
		opMap = dict();
		opMap["funcName"] = HAL_DEV_WRITE
		functions += functionTemplate.safe_substitute(opMap)	
	map["HAL_DEV_WRITE"] = HAL_DEV_WRITE
	HAL_DEV_COLD_BOOT = oper.attributes["HAL_DEV_COLD_BOOT"].value
	if HAL_DEV_COLD_BOOT == None or HAL_DEV_COLD_BOOT == "":
		HAL_DEV_COLD_BOOT = "NULL"
	else:
		opMap = dict();
		opMap["funcName"] = HAL_DEV_COLD_BOOT
		functions += functionTemplate.safe_substitute(opMap)	
	map["HAL_DEV_COLD_BOOT"] = HAL_DEV_COLD_BOOT
	HAL_DEV_WARM_BOOT = oper.attributes["HAL_DEV_WARM_BOOT"].value
	if HAL_DEV_WARM_BOOT == None or HAL_DEV_WARM_BOOT == "":
		HAL_DEV_WARM_BOOT = "NULL"
	else:
		opMap = dict();
		opMap["funcName"] = HAL_DEV_WARM_BOOT
		functions += functionTemplate.safe_substitute(opMap)	
	map["HAL_DEV_WARM_BOOT"] = HAL_DEV_WARM_BOOT
	HAL_DEV_PWR_OFF = oper.attributes["HAL_DEV_PWR_OFF"].value
	if HAL_DEV_PWR_OFF == None or HAL_DEV_PWR_OFF == "":
		HAL_DEV_PWR_OFF = "NULL"
	else:
		opMap = dict();
		opMap["funcName"] = HAL_DEV_PWR_OFF
		functions += functionTemplate.safe_substitute(opMap)	
	map["HAL_DEV_PWR_OFF"] = HAL_DEV_PWR_OFF
	HAL_DEV_IMAGE_DN_LOAD = oper.attributes["HAL_DEV_IMAGE_DN_LOAD"].value
	if HAL_DEV_IMAGE_DN_LOAD == None or HAL_DEV_IMAGE_DN_LOAD == "":
		HAL_DEV_IMAGE_DN_LOAD = "NULL"
	else:
		opMap = dict();
		opMap["funcName"] = HAL_DEV_IMAGE_DN_LOAD
		functions += functionTemplate.safe_substitute(opMap)		
	map["HAL_DEV_IMAGE_DN_LOAD"] = HAL_DEV_IMAGE_DN_LOAD
	HAL_DEV_DIRECT_ACCESS = oper.attributes["HAL_DEV_DIRECT_ACCESS"].value
	if HAL_DEV_DIRECT_ACCESS == None or HAL_DEV_DIRECT_ACCESS == "":
		HAL_DEV_DIRECT_ACCESS = "NULL"
	else:
		opMap = dict();
		opMap["funcName"] = HAL_DEV_DIRECT_ACCESS
		functions += functionTemplate.safe_substitute(opMap)
	map["HAL_DEV_DIRECT_ACCESS"] = HAL_DEV_DIRECT_ACCESS
	
	devMap = dict();
	devMap["functions"] = functions
	devMap["devOperations"] = devOperationTemplate.safe_substitute(map)
	return devMap
#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------	
resourceKeyMap = dict();
resourceParentMap = dict();

resourceXml = sys.argv[1]
componentXml = sys.argv[2]

resourceDoc = xml.dom.minidom.parse(resourceXml)
componentDoc = xml.dom.minidom.parse(componentXml)

resourceList = resourceDoc.getElementsByTagName("resource:ChassisResource") + resourceDoc.getElementsByTagName("resource:HardwareResource") + resourceDoc.getElementsByTagName("resource:SoftwareResource") + resourceDoc.getElementsByTagName("resource:SystemController") + resourceDoc.getElementsByTagName("resource:NodeHardwareResource")
compositionList = resourceDoc.getElementsByTagName("resource:Composition")

componentList = componentDoc.getElementsByTagName("component:Component") + componentDoc.getElementsByTagName("component:SAFComponent")

parseResources(resourceList)
parseConnections(compositionList)

parseComponents(componentList)
