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
#define ${macroName}         ${macroID}
""")         