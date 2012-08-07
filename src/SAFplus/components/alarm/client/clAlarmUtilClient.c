/*
 * Copyright (C) 2002-2012 OpenClovis Solutions Inc.  All Rights Reserved.
 *
 * This file is available  under  a  commercial  license  from  the
 * copyright  holder or the GNU General Public License Version 2.0.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * For more  information, see  the file  COPYING provided with this
 * material.
 */
/*******************************************************************************
 * ModuleName  : alarm
 * File        : clAlarmUtil.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This file provide implemantation of the Eonized application main function
 *          This C file SHOULD NOT BE MODIFIED BY THE USER
 *
 *****************************************************************************/


/* standard includes */
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

/* ASP includes */
#include <clCommon.h>
#include <clEoApi.h>

/* alarm includes */
#include <clAlarmErrors.h>
#include <clAlarmUtil.h>
#include <clAlarmOMIpi.h>
#include "clAlarmOmClass.h"
#include "clAlarmCommons.h"
#include "xdrClAlarmIdIDLT.h"

ClRcT 
GetAlarmUniqueId(ClUint32T *alarmId)
{
    ClRcT rc=CL_OK;
    static ClUint32T uniqueid=1;
    if(uniqueid<65535)
        *alarmId=uniqueid++;
    else
        rc=CL_ERR_INVALID_HANDLE;
    return rc;
}

ClRcT 
GetAlmAttrValue(ClCorMOIdPtrT pMoId, ClCorObjectHandleT hMSOObj, ClUint32T attrId, 
                                 ClUint32T idx, void* value, ClUint32T* size, ClUint32T lockApplied)
{
    ClRcT        rc = CL_OK;
    ClCorAttrPathT * tempContAttrPath = NULL;
    ClUint32T       index=0;
    
                                                                                        
    /* check if the attribute is with alarm OM object */


    if ( (attrId > CL_ALARM_OM_ATTR_START) && (attrId < CL_ALARM_OM_ATTR_END) )
    {

        rc = clAlarmOmObjectOperation(pMoId, 
                       (ClAlarmOMAttrIdsT) attrId, 
                       idx, 
                       value, 
                       CL_ALARM_OM_OPERATION_GET);
        if (rc != CL_OK)
        {
            clLogError("ALM", "OME", 
                    "Failed to fetch the alarm attribute [%d] from OM.  rc[0x%x]", attrId, rc);
        } 
        
        return rc;
    }

	if((attrId == CL_ALARM_PROBABLE_CAUSE ||
	   attrId == CL_ALARM_POLLING_INTERVAL ||
	   attrId == CL_ALARM_MSO_TO_BE_POLLED  ||
	   attrId == CL_ALARM_ID ||
	   attrId == CL_ALARM_CATEGORY ||
	   attrId == CL_ALARM_AFFECTED_ALARMS ||
	   attrId == CL_ALARM_GENERATION_RULE ||
	   attrId == CL_ALARM_SUPPRESSION_RULE ||
	   attrId == CL_ALARM_GENERATE_RULE_RELATION ||
	   attrId == CL_ALARM_SUPPRESSION_RULE_RELATION))
	{
        rc = clAlarmStaticInfoCntDataGet(pMoId, attrId,idx,value, lockApplied);
        if (rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        {
            clLogError("ALM", "STATICINFO", "Failed to get attrId [%u] from static info container. rc [0x%x]",
                    attrId, rc);
            return rc;
        }

        if (rc == CL_OK)
            return rc;

        /* Otherwise retrieve it from COR as below */
	}

    /* it is an MSO attribute */

    if ( (attrId > CL_ALARM_SIMPLE_ATTR_START) &&
         (attrId < CL_ALARM_SIMPLE_ATTR_END) )
    {
        index = CL_COR_INVALID_ATTR_IDX;
    }
    if ( (attrId == CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY) || ( (attrId > CL_ALARM_CONT_ATTR_START) &&
         (attrId < CL_ALARM_CONT_ATTR_END)) )
    {

        rc = clCorAttrPathAlloc(&tempContAttrPath);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n clCorAttrPathAlloc, not able to get the\
                                   alm COR attr value for attrId:%d, rc:%0x \n", attrId, rc));
            return rc;
        }
        
        rc = clCorAttrPathAppend( tempContAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n clCorAttrPathAppend, not able to get the\
                                   alm COR attr value for attrId:%d, rc:%0x \n", attrId, rc));
            clCorAttrPathFree(tempContAttrPath);
            return rc;
        }        

        index = CL_COR_INVALID_ATTR_IDX; 
    }
    else if ( (attrId > CL_ALARM_ARRAY_ATTR_START) && 
            (attrId < CL_ALARM_ARRAY_ATTR_END) )
    {
        index = idx;
    }


    rc = clCorObjectAttributeGet(hMSOObj, tempContAttrPath, attrId, index,  value, size);
    clCorAttrPathFree(tempContAttrPath);

    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n clCorObjectAttributeGet, not able to get the\
                               alm COR attr value for attrId:%d, rc:%0x \n", attrId, rc));
        return rc;
    }
     
    return rc;
}

ClRcT SetAlmAttrValue(ClCorMOIdPtrT pMoId, 
                        ClCorTxnSessionIdT *ptxnSessionId,ClCorObjectHandleT hMSOObj, ClUint32T attrId,
                        ClUint32T idx, void* value, ClUint32T size)
{

    ClRcT rc = CL_OK;
    ClCorAttrPathT * tempContAttrPath = NULL;
    ClUint32T       index=0;    

    if ( (attrId > CL_ALARM_OM_ATTR_START) && (attrId < CL_ALARM_OM_ATTR_END) )
    {
        rc = clAlarmOmObjectOperation(pMoId,
                       (ClAlarmOMAttrIdsT) attrId, 
                       idx, 
                       value, 
                       CL_ALARM_OM_OPERATION_SET);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n clAlarmOmObjectOperation failed with rc: %x \n", rc));
        } 
        
        return rc;
    }

    if ( (attrId > CL_ALARM_SIMPLE_ATTR_START) &&
         (attrId < CL_ALARM_SIMPLE_ATTR_END) )
    {
        index = CL_COR_INVALID_ATTR_IDX; 
    }
    if ( (attrId == CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY) || ( (attrId > CL_ALARM_CONT_ATTR_START) &&
         (attrId < CL_ALARM_CONT_ATTR_END)) )
    {

        rc = clCorAttrPathAlloc(&tempContAttrPath);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n clCorAttrPathAlloc failed for attrId:%d, rc:%0x \n", attrId, rc));
            return rc;
        }        

        clCorAttrPathAppend( tempContAttrPath, CL_ALARM_INFO_CONT_CLASS, idx);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n clCorAttrPathAppend failed for attrId:%d, rc:%0x \n", attrId, rc));
            clCorAttrPathFree(tempContAttrPath);
            return rc;
        }
        
        index = CL_COR_INVALID_ATTR_IDX;
    }
    else if ( (attrId > CL_ALARM_ARRAY_ATTR_START) && 
            (attrId < CL_ALARM_ARRAY_ATTR_END) )
    {
        index = idx;
    }


    rc = clCorObjectAttributeSet(ptxnSessionId, hMSOObj, tempContAttrPath ,  attrId, index, value, size);
    clCorAttrPathFree(tempContAttrPath);

    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n clCorObjectAttributeSet failed for attrId:%d, rc:%0x \n", attrId, rc));
        return rc;
    }

     
    return rc;

}


ClRcT 
alarmUtilAlmIdxGet(ClCorMOIdPtrT pMoId, ClCorObjectHandleT objH,  ClAlarmRuleEntryT alarmKey, 
                        ClUint32T* index, ClUint32T lockApplied){

    ClRcT                   rc = CL_ERR_NOT_EXIST;
    ClUint32T               i = 0;
    ClUint32T               size=0;
    ClAlarmProbableCauseT   cause = 0;
    ClAlarmSpecificProblemT specificProblem = 0;

    CL_FUNC_ENTER();
    clLogDebug("ACU", "IDG", "Querying the almInfo structure for "
                          "probable cause [%d], specific Alarm [%d]",
                          alarmKey.probableCause, alarmKey.specificProblem);
                                                                                         
    for(i=0; i<CL_ALARM_MAX_ALARMS; i++)
    {
        size = sizeof(ClUint32T);
        rc = GetAlmAttrValue(pMoId, objH, CL_ALARM_PROBABLE_CAUSE, i, &cause, &size, lockApplied);
        if(rc != CL_OK) 
        {
            clLogError("ACU", "IDG", "Could not retrieve probable cause value "
                                  "for index [%d]. rc [0x%x]",i, rc);
            return(rc);
        }
                                                                                             
        if (cause == CL_ALARM_ID_INVALID)
        {
            clLogTrace("ACU", "IDG", "The probable cause found invalid at index [%d], so breaking from loop.", i); 
            break;
        }

        size = sizeof(ClUint32T);
        rc = GetAlmAttrValue(pMoId, objH, CL_ALARM_SPECIFIC_PROBLEM, i, &specificProblem, &size, lockApplied);
        if(rc != CL_OK) 
        {
            clLogError("ACU", "IDG", "Could not retrieve specific problem value "
                                  "for index [%d]. rc [0x%x]",i, rc);
            return(rc);
        }

        if((alarmKey.probableCause == cause) && (alarmKey.specificProblem == specificProblem))
        {
            clLogTrace("ACU", "IDG", 
                    "The alarm index found [%d] for probable "
                    "cause [%d]: specific problem [%d]", 
                    i, cause, specificProblem);
            *index = i;
            return(CL_OK);
        }
                                                                                             
    }

    rc = CL_ERR_NOT_EXIST;

    clLogError("ACU", "IDG", 
            "Failed to get the alarm index for probable cause [%d] and "
            "specific problem [%d]", alarmKey.probableCause, 
            alarmKey.specificProblem);
    CL_FUNC_EXIT();
    return rc;

}

/*
 * This function will get the MOClasstype from the moid and will find out
 * if polling is enabled for this classtype.
 */

ClRcT clAlarmPollingStatusGet(ClUint32T *pValue)
{
    
    ClRcT rc = CL_OK;
    ClUint32T index = 0,i;
    
    while( (appAlarms[index].moClassID != 0))
    { 
        for (i=0; i< CL_ALARM_MAX_ALARMS; i++)
        {
            /* set the prob cause */
            if (appAlarms[index].MoAlarms[i].probCause == CL_ALARM_ID_INVALID)
                break;

            /* check if any MSO has alarms to be polled */
            if (appAlarms[index].MoAlarms[i].fetchMode == CL_ALARM_POLL)
            {
                *pValue = 1;
                return rc;
            }
        }
        index++;
    }
    return rc;
}
        

/*
 * This function will get the MOClasstype from the moid and will find out
 * if there is soaking time specified for this moclasstype.
 */

ClRcT clAlarmSoakTimeStatusGet(ClUint32T *pValue)
{
    
    ClRcT rc = CL_OK;
    ClUint32T index = 0,i;
    
    while( (appAlarms[index].moClassID != 0))
    { 
        for (i=0; i< CL_ALARM_MAX_ALARMS; i++)
        {
            /* set the prob cause */
            if (appAlarms[index].MoAlarms[i].probCause == CL_ALARM_ID_INVALID)
                break;

            /* check if any MSO has alarms to be polled */
            if (appAlarms[index].MoAlarms[i].assertSoakingTime != 0 ||
                appAlarms[index].MoAlarms[i].clearSoakingTime != 0)
            {
                *pValue = 1;
                return rc;
            }
        }
        index++;
    }
    return rc;
}

/*
 * This function will return the first entry in the route list
 * maintained by COR. This is required so that the notification via
 * event is handled only by the primary OI.
 */
ClRcT 
clAlarmCheckIfPrimaryOI(ClCorMOIdPtrT pMoId, ClUint32T* primaryOI)
{
    ClRcT rc = CL_OK;
    ClCorAddrT     alarmClientAddr;
    ClCorAddrT         addr;    
    ClEoExecutionObjT*   eoObj;

    rc = clEoMyEoObjectGet(&eoObj);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clEoMyEoObjectGet failed [%x]",rc));
        CL_FUNC_EXIT();
        return rc;
    }

    alarmClientAddr.nodeAddress = clIocLocalAddressGet();
    alarmClientAddr.portId = eoObj->eoPort;

	rc = clCorMoIdToComponentAddressGet(pMoId, &addr);
	if (CL_OK != rc)
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorMoIdToComponentAddressGet failed with rc 0x:%x\n", rc));
		return rc;
	}
    if(addr.portId == alarmClientAddr.portId &&
        addr.nodeAddress == alarmClientAddr.nodeAddress)
        *primaryOI = 1;        
    return rc;
}


/**
 * This function is used to un-register for all the MOs for which the registration was happened
 * during initialization phase. It will un-register for all the resource entires of the resource
 * table (rt) xml file.
 */ 
ClRcT _clAlarmOIUnregister()
{
    ClRcT                   rc = CL_OK;
    ClCorAddrT              almAddr = {0};
    ClCorMOIdT              moId ;
    ClUint32T               index = 0;
    ClOampRtResourceArrayT  resourcesArray = {0};    

    rc = clAlarmClientResourceTableInfoGet( &almAddr, &resourcesArray );
    if (CL_OK != rc)
    {
        clLogError("ALM","URG", "Failed while getting the resource information. rc[0x%x] ", rc);
        return rc;
    }

    for (index = 0; index < resourcesArray.noOfResources ; index++)
    {
        clCorMoIdInitialize(&moId);

        rc = clCorMoIdNameToMoIdGet(&(resourcesArray.pResources[index].resourceName), &moId);
        if (CL_OK != rc)
        {
            clLogError("ALM", "URG", "Failed to get the MOID from the give moid name[%s]. rc[0x%x]", 
                    resourcesArray.pResources[index].resourceName.value, rc);
            continue;
        }

        clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_ALARM_MANAGEMENT);

        rc = clCorOIUnregister(&moId, &almAddr);
        if (CL_OK != rc)
        {
            clLogError("ALM", "URG", 
                    "Failed while unregistering the route entry for resource [%s], rc[0x%x]", 
                    resourcesArray.pResources[index].resourceName.value, rc);
            continue;
        }
    }


    if (resourcesArray.noOfResources != 0)
        clHeapFree(resourcesArray.pResources);

    return rc;
}

ClRcT clAlarmConfigDataGet(ClCorMOIdPtrT pMoId, ClAlarmInfoT** ppAlarmInfo, ClUint32T* pCount)
{
    ClRcT rc = CL_OK;
    ClAlarmInfoT* pAlarmInfo = NULL;
    ClAlarmInfoT* pTempAlarmInfo = NULL;
    ClCorBundleHandleT bundleHandle = 0;
    ClCorBundleConfigT bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrValueDescriptorT attrDesc[4] = {{0}};
    ClCorAttrValueDescriptorListT attrDescList = {0};
    ClCorAttrPathT attrPath = {{{0}}};
    ClCorJobStatusT jobStatus = 0;
    ClUint32T i = 0;
    ClCorObjectHandleT objH = 0;

    if (!pMoId || !ppAlarmInfo)
    {
        clLogError("ALM", "UTIL", "NULL pointer passed in pMoId or ppAlarmInfo.");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    clCorMoIdServiceSet(pMoId, CL_COR_SVC_ID_ALARM_MANAGEMENT);

    rc = clCorObjectHandleGet(pMoId, &objH);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to get object handle from MoId. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to initialize the bundle. rc [0x%x]", rc);
        goto out_free;
    }

    pAlarmInfo = (ClAlarmInfoT *) clHeapAllocate(CL_ALARM_MAX_ALARMS * sizeof(ClAlarmInfoT));
    if (!pAlarmInfo)
    {
        clLogError("ALM", "UTIL", "Failed to allocate memory.");
        clCorBundleFinalize(bundleHandle);
        rc = CL_ALARM_RC(CL_ERR_NO_MEMORY); 
        goto out_free;
    }

    clCorAttrPathInitialize(&attrPath);
    clCorAttrPathAppend(&attrPath, 0x3, 0); 

    for (i=0; i<CL_ALARM_MAX_ALARMS; i++)
    {
        pTempAlarmInfo = (pAlarmInfo + i);

        clCorAttrPathIndexSet(&attrPath, 1, i);

        memcpy(&(pTempAlarmInfo->moId), pMoId, sizeof(ClCorMOIdT));

        attrDesc[0].pAttrPath = &attrPath;
        attrDesc[0].attrId = CL_ALARM_PROBABLE_CAUSE;
        attrDesc[0].index = -1;
        attrDesc[0].bufferPtr = &(pTempAlarmInfo->probCause);
        attrDesc[0].bufferSize = sizeof(ClUint32T);
        attrDesc[0].pJobStatus = &jobStatus;

        attrDesc[1].pAttrPath = &attrPath;
        attrDesc[1].attrId = CL_ALARM_SPECIFIC_PROBLEM;
        attrDesc[1].index = -1;
        attrDesc[1].bufferPtr = &(pTempAlarmInfo->specificProblem);
        attrDesc[1].bufferSize = sizeof(ClUint32T);
        attrDesc[1].pJobStatus = &jobStatus;

        attrDesc[2].pAttrPath = &attrPath;
        attrDesc[2].attrId = CL_ALARM_CATEGORY; 
        attrDesc[2].index = -1;
        attrDesc[2].bufferPtr = &(pTempAlarmInfo->category);
        attrDesc[2].bufferSize = sizeof(ClUint8T);
        attrDesc[2].pJobStatus = &jobStatus;

        attrDesc[3].pAttrPath = &attrPath;
        attrDesc[3].attrId = CL_ALARM_SEVERITY; 
        attrDesc[3].index = -1;
        attrDesc[3].bufferPtr = &(pTempAlarmInfo->severity);
        attrDesc[3].bufferSize = sizeof(ClUint8T);
        attrDesc[3].pJobStatus = &jobStatus;

        attrDescList.numOfDescriptor = 4;
        attrDescList.pAttrDescriptor = attrDesc;

        rc = clCorBundleObjectGet(bundleHandle, &objH, &attrDescList);
        if (rc != CL_OK)
        {
            clLogError("ALM", "UTIL", "Failed to add the bundle object. rc [0x%x]", rc);
            clCorBundleFinalize(bundleHandle);
            goto out_free;
        }
    }

    rc = clCorBundleApply(bundleHandle);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to retrieve alarm attribute values from COR. rc [0x%x]", rc);
        clCorBundleFinalize(bundleHandle);
        goto out_free;
    }

    rc = clCorBundleFinalize(bundleHandle);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to finalize the bundle. rc [0x%x]", rc);
        goto out_free;
    }

    /* Set the no. of alarm profiles */
    for (i=0; i<CL_ALARM_MAX_ALARMS; i++)
    {
        if ( (pAlarmInfo + i)->probCause == CL_ALARM_ID_INVALID)
            break;

        (*pCount)++;
    }

    *ppAlarmInfo = pAlarmInfo;
    pAlarmInfo = NULL;

    out_free:
    clCorObjectHandleFree(&objH);
    if(pAlarmInfo)
        clHeapFree(pAlarmInfo);
    return rc;
}

ClRcT clAlarmReset(ClCorMOIdPtrT pMoId, VDECL_VER(ClAlarmIdT, 4, 1, 0)* pAlarmId, ClUint32T count)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT inMsgHandle = 0;
    VDECL_VER(ClAlarmIdIDLT, 4, 1, 0) alarmIdIDL = {0};
    ClIocAddressT destAddr = {{0}};
    ClUint32T rmdFlags = CL_RMD_CALL_ATMOST_ONCE;
    ClRmdOptionsT      rmdOptions = CL_RMD_DEFAULT_OPTIONS; 
    ClVersionT version = {0};
    ClCorAddrT alarmOwner = {0};

    if (!pAlarmId)
    {
        clLogError("ALM", "UTIL", "NULL parameter passed in pAlarmId.");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    if (count <= 0)
    {
        clLogError("ALM", "UTIL", "Invalid no. of alarms passed.");
        return CL_ALARM_RC(CL_ERR_INVALID_PARAMETER);
    }

    rc = clBufferCreate(&inMsgHandle);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to create buffer. rc [0x%x]", rc);
        return rc;
    }

    CL_ALARM_VERSION_SET(version);

    alarmIdIDL.numEntries = count;
    alarmIdIDL.pAlarmId = pAlarmId;

    rc = clXdrMarshallClVersionT(&version, inMsgHandle, 0);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to marshall ClVersionT. rc [0x%x]", rc);
        clBufferDelete(&inMsgHandle);
        return rc;
    }

    rc = clCorMoIdServiceSet(pMoId, CL_COR_SVC_ID_ALARM_MANAGEMENT);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to set service id in the moid. rc [0x%x]", rc);
        clBufferDelete(&inMsgHandle);
        return rc;
    }

    rc = VDECL_VER(clXdrMarshallClCorMOIdT, 4, 0, 0)(pMoId, inMsgHandle, 0);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to marshall ClCorMOIdT. rc [0x%x]", rc);
        clBufferDelete(&inMsgHandle);
        return rc;
    }

    rc = VDECL_VER(clXdrMarshallClAlarmIdIDLT, 4, 1, 0)(&alarmIdIDL, inMsgHandle, 0);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to marshall ClAlarmIdIDLT. rc [0x%x]", rc);
        clBufferDelete(&inMsgHandle);
        return rc;
    }

    /* Send RMD to the alarm server running on the node same that of alarm owner. */
    rc = clCorMoIdToComponentAddressGet(pMoId, &alarmOwner);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to alarm owner address. rc [0x%x]", rc);
        clBufferDelete(&inMsgHandle);
        return rc;
    }
    
    destAddr.iocPhyAddress.nodeAddress = alarmOwner.nodeAddress;
    destAddr.iocPhyAddress.portId = CL_IOC_ALARM_PORT;

    rc = clRmdWithMsg(destAddr, CL_ALARM_RESET, inMsgHandle, 0, (rmdFlags & ~CL_RMD_CALL_ASYNC), &rmdOptions, NULL);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to send rmd to the alarm server. rc [0x%x]", rc);
        clBufferDelete(&inMsgHandle);
        return rc;
    }

    clBufferDelete(&inMsgHandle);

    return rc;
}
