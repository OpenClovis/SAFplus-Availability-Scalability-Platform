/*
 * Copyright (C) 2002-2009 by OpenClovis Inc. All  Rights Reserved.
 * 
 * The source code for  this program is not published  or otherwise 
 * divested of  its trade secrets, irrespective  of  what  has been 
 * deposited with the U.S. Copyright office.
 * 
 * This program is  free software; you can redistribute it and / or
 * modify  it under  the  terms  of  the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the  hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied  warranty  of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You  should  have  received  a  copy of  the  GNU General Public
 * License along  with  this program. If  not,  write  to  the 
 * Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Build: 4.2.0
 */
/*******************************************************************************
 * ModuleName  : PM 
 * File        : clPMClientUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This module contains the PM library utility functions implementation. 
 *****************************************************************************/

/* Standard Inculdes */
#include <string.h>

/* ASP Includes */
#include <clCommon.h>
#include <clVersionApi.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clOmApi.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCorMetaData.h>
#include <clCorTxnApi.h>
#include <clDebugApi.h>
#include <clOampRtApi.h>
#include <clCorErrors.h>
#include <clProvApi.h>
#include <clMsoConfig.h>

#include <clPMClientUtils.h>
#include <clPMErrors.h>

extern ClClistT gClPMMoIdList; 
extern ClPMCallbacksT gClPMCallbacks;
extern ClCorMOIdListT* pPMConfigMoIdList;

ClCorAttrDefT* _clPMAttrCachedGet(ClCorAttrDefT* pCorAttrDef, ClUint32T attrCount, ClCorAttrIdT attrId);

void clPMMoIdListDeleteCB(ClClistDataT userData)
{
    /* Delete the MoId pointer. */
    clHeapFree((void *) userData);
}

ClRcT clPMMoIdListNodeDel(ClClistT moIdList, ClCorMOIdPtrT pMoId)
{
    ClRcT rc = CL_OK;
    ClClistNodeT pFirstNode = NULL;
    ClClistNodeT pNextNode = NULL;
    ClClistDataT userData = NULL;

    rc = clClistFirstNodeGet(moIdList, &pFirstNode);
    if (rc != CL_OK)
    {
        clLogError("PM", "LISTDEL", "Failed to remove first node from PM MoId list. rc [0x%x]", rc);
        return rc;
    }

    pNextNode = pFirstNode;

    do
    {
        rc = clClistDataGet(moIdList, pNextNode, &userData);
        if (rc != CL_OK)
        {
            clLogError("PM", "LISTDEL", "Failed to get data from PM list node. rc [0x%x]", rc);
            return rc;
        }

        if (!memcmp((void *) pMoId, (void *) userData, sizeof(ClCorMOIdT)))
        {
            rc = clClistNodeDelete(moIdList, pNextNode);
            if (rc != CL_OK)
            {
                clLogError("PM", "LISTDEL", "Failed to delete a node from PM list. rc [0x%x]", rc);
                return rc;
            }

            break;
        }

        rc = clClistNextNodeGet(moIdList, pNextNode, &pNextNode);
        if (rc != CL_OK)
        {
            clLogError("PM", "LISTDEL", "Failed to get next node from PM list. rc [0x%x]", rc);
            return rc;
        }
    } while (pFirstNode != pNextNode);
    
    return CL_OK; 
}

ClUint32T _clPMAttrSizeGet(ClCorTypeT attrDataType)
{
    ClUint32T size = 0;

    switch (attrDataType)
    {
        case CL_COR_INT8:
        case CL_COR_UINT8:
            size = 1;
            break;

        case CL_COR_INT16:
        case CL_COR_UINT16:
            size = 2;
            break;

        case CL_COR_INT32:
        case CL_COR_UINT32:
            size = 4;
            break;

        case CL_COR_INT64:
        case CL_COR_UINT64:
            size = 8;
            break;

        default:
            size = 4;
            break;
    }

    return size;
}

ClRcT _clPMMoIdListWalkCB(ClClistDataT userData, void* userArg)
{
    ClRcT rc = CL_OK;
    ClCorMOIdPtrT pMoId = NULL;
    ClCorAttrDefT* pAttrDef = NULL;
    ClCorAttrDefT* pAttrCachedDef = NULL;
    ClCorAttrFlagT attrFlags = 0;
    ClCorClassTypeT classId = 0;
    ClPMObjectDataT pmObjData = {0};
    ClUint32T i = 0;
    ClUint32T attrCount = 0;
    ClUint32T attrCachedCount = 0;
    ClCharT className[CL_MAX_NAME_LENGTH] = {0};
    ClUint32T classNameSize = 0;
    ClCorObjectHandleT objH = 0;
    ClCorTxnSessionIdT* pCorTxnId = NULL;
    ClCorMOIdT moid;

    if (!userData || !userArg)
    {
        clLogError("PM", "PMLISTWALK", "NULL pointer passed.");
        return CL_OK; 
    }

    pMoId = (ClCorMOIdPtrT) userData;
    pCorTxnId = userArg;
    
    memcpy(&moid, pMoId, sizeof(moid));
    pMoId = &moid;

    rc = clCorMoIdServiceSet(pMoId, CL_COR_SVC_ID_PM_MANAGEMENT);
    CL_ASSERT(rc == CL_OK);

    rc = clCorMoIdToClassGet(pMoId, CL_COR_MSO_CLASS_GET, &classId);
    if (rc != CL_OK)
    {
        clLogError("PM", "PMLISTWALK", "Failed to get mso classId from MoId. rc [0x%x]", rc);
        return CL_OK;
    }

    rc = clCorClassNameGet(classId, className, &classNameSize);
    if (rc != CL_OK)
    {
        clLogError("PM", "PMLISTWALK", "Failed to get class name from class id. rc [0x%x]", rc);
        return CL_OK;
    }

    /* Get all the PM attributes */
    attrFlags = CL_COR_ATTR_RUNTIME; 

    rc = clCorClassAttrListGet(className, attrFlags, &attrCount, &pAttrDef);
    if (rc != CL_OK)
    {
        clLogError("PM", "PMLISTWALK", "Failed to get attribute list from COR. rc [0x%x]", rc);
        return CL_OK;
    }

    /* Get the cached PM attributes to update the value back into cor. 
     * This is a reduntant rmd call, needs to be optimized.
     */
    attrFlags = CL_COR_ATTR_RUNTIME | CL_COR_ATTR_CACHED; 

    rc = clCorClassAttrListGet(className, attrFlags, &attrCachedCount, &pAttrCachedDef);
    if (rc != CL_OK)
    {
        clLogError("PM", "PMLISTWALK", "Failed to get attribute list from COR. rc [0x%x]", rc);
        clHeapFree(pAttrDef);
        return CL_OK;
    }

    pmObjData.pMoId = pMoId;
    pmObjData.attrCount = attrCount;
    pmObjData.pAttrData = clHeapAllocate(attrCount * sizeof(ClPMAttrDataT));
    if (!pmObjData.pAttrData)
    {
        clLogError("PM", "PMLISTWALK", "Failed to allocate memory.");
        clHeapFree(pAttrDef);
        clHeapFree(pAttrCachedDef);
        return CL_OK;
    }

    for (i=0; i<attrCount; i++)
    {
        ClPMAttrDataPtrT pmAttr = NULL;
        ClUint32T attrSize = 0;
        ClPMAlmTableAttrDefT* pPMAlmTableAttr = NULL;
        ClUint32T j = 0;

        pmAttr = (pmObjData.pAttrData + i);

        pmAttr->attrType = (pAttrDef + i)->attrType;
        pmAttr->attrDataType = (pAttrDef + i)->u.attrInfo.arrDataType;
        pmAttr->attrId = (pAttrDef + i)->attrId;
        pmAttr->index = -1;
        pmAttr->pAttrPath = NULL;

        attrSize = _clPMAttrSizeGet(pmAttr->attrDataType);

        if (pmAttr->attrType == CL_COR_ARRAY_ATTR)
        {
            pmAttr->size = (pAttrDef+i)->u.attrInfo.maxElement * attrSize;
        }
        else if (pmAttr->attrType == CL_COR_SIMPLE_ATTR)
        {
            pmAttr->size = attrSize;
        }
        else
        {
            clLogNotice("PM", "PMLISTWALK", "Unsupported attribute type: [%d]",
                        pmAttr->attrType);
            continue;
        }

        pmAttr->pPMData = clHeapAllocate(pmAttr->size);
        if (!pmAttr->pPMData)
        {
            clLogError("PM", "PMLISTWALK", "Failed to allocate memory.");
            clHeapFree(pAttrDef);
            clHeapFree(pAttrCachedDef);
            return CL_OK; 
        }

        pmAttr->pAlarmData = NULL;
        pmAttr->alarmCount = 0;

        /* Check the value of the attribute and raise/clear alarm */
        rc = clPMAlarmInfoGet(classId, pmAttr->attrId, &pPMAlmTableAttr);
        if (rc != CL_OK && CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
        {
            /* No threshold configuration done. */
            clLogNotice("PM", "ALARM", "Alarm info doesnt exist for attr id [%d]", pmAttr->attrId);
            rc = CL_OK;
            continue;
        }
        if(!pPMAlmTableAttr || !pPMAlmTableAttr->alarmCount)
        {
            continue;
        }
        rc = CL_OK;
        pmAttr->alarmCount = pPMAlmTableAttr->alarmCount;
        pmAttr->pAlarmData = clHeapCalloc(pmAttr->alarmCount, sizeof(*pmAttr->pAlarmData));
        CL_ASSERT(pmAttr->pAlarmData != NULL);
        for(j = 0; j < pmAttr->alarmCount; ++j)
        {
            pmAttr->pAlarmData[j].probableCause = pPMAlmTableAttr->pAlarmAttrDef[j].probableCause;
            pmAttr->pAlarmData[j].specificProblem = pPMAlmTableAttr->pAlarmAttrDef[j].specificProblem;
            pmAttr->pAlarmData[j].pAlarmPayload = NULL;
        }
    }

    /* Call the user callback function */
    if (gClPMCallbacks.fpPMObjectRead)
    {
        rc = gClPMCallbacks.fpPMObjectRead(0, &pmObjData);
        if (rc != CL_OK)
        {
            clLogError("PM", "PMLISTWALK", "PM Object Read OI callback failed. rc [0x%x]", rc);
            goto error_exit;
        }
    }
    else
    {
        clLogError("PM", "PMLISTWALK", "PM Object Read callback is not registered by the OI.");
        rc = CL_PM_RC(CL_ERR_NULL_POINTER);
        goto error_exit;
    }

    /* Process the data.
     * 1. Update the values of cached attributes in COR.
     * 2. Validate the threshold values and raise/clear alarms. 
     */
    rc = clCorMoIdToObjectHandleGet(pmObjData.pMoId, &objH);
    if (rc != CL_OK)
    {
        clLogError("PM", "PMLISTWALK", "Failed to get Object Handle from MoId. rc [0x%x]", rc);
        goto error_exit;
    }

    for (i=0; i<attrCount; i++)
    {
        ClPMAttrDataPtrT pmAttr = NULL;
        ClCorAttrDefT* pCorAttrDef = NULL;
        
        pmAttr = (pmObjData.pAttrData + i);
        CL_ASSERT(pmAttr);

        /* Check if it is in the cached attribute list and update the value in COR. */
        if(pAttrCachedDef)
            pCorAttrDef = _clPMAttrCachedGet(pAttrCachedDef, attrCachedCount, pmAttr->attrId);
        if (pCorAttrDef)
        {
            rc = clCorObjectAttributeSet(pCorTxnId, objH, pmAttr->pAttrPath, pmAttr->attrId, 
                                         pmAttr->index, pmAttr->pPMData, pmAttr->size);
            if (rc != CL_OK)
            {
                clLogError("PM", "PMLISTWALK", "Failed to add SET job into corTxnList. rc [0x%x]", rc);
                clCorObjectHandleFree(&objH);
                goto error_exit;
            }
        }

        if(pmAttr->pAlarmData)
        {
            rc = clPMAttrAlarmProcess(pMoId, classId, pmAttr);
            if (rc != CL_OK)
            {
                clLogError("PM", "PMLISTWALK", "Failed to process the alarms associated with classId [%d], attrId [%d]. rc [0x%x]",
                           classId, pmAttr->attrId, rc);
                clCorObjectHandleFree(&objH);
                goto error_exit;
            }
        }
    }

    clCorObjectHandleFree(&objH);

    error_exit:
    clHeapFree(pAttrDef);
    clHeapFree(pAttrCachedDef);
    clPMObjectDataFree(&pmObjData);

    return CL_OK; 
}

ClRcT clPMAttrAlarmProcess(ClCorMOIdPtrT pMoId, ClCorClassTypeT classId, ClPMAttrDataPtrT pmAttr)
{
    ClRcT rc = CL_OK;
    ClPMAlmTableAttrDefT* pPMAlmTableAttr = NULL;
    ClUint32T j = 0;

    /* Check the value of the attribute and raise/clear alarm */
    rc = clPMAlarmInfoGet(classId, pmAttr->attrId, &pPMAlmTableAttr);
    if (rc != CL_OK && CL_GET_ERROR_CODE(rc) == CL_ERR_NOT_EXIST)
    {
        /* No threshold configuration done. */
        clLogNotice("PM", "ALARM", "Alarm info doesnt exist for attr id [%d]", pmAttr->attrId);
        return CL_OK;
    }

    if (rc != CL_OK)
    {
        clLogError("PM", "PMLISTWALK", "Failed to get attribute alarm info. rc [0x%x]", rc);
        return rc;
    }

    if(!pmAttr->pAlarmData)
        return CL_PM_RC(CL_ERR_NOT_EXIST);

    clLogDebug("PM", "PMLISTWALK", 
            "Processing alarms for classId [%d], attrId [%d] : no. of alarms configured : [%u].",
            classId, pmAttr->attrId, pPMAlmTableAttr->alarmCount);

    /*
     * Process each alarm threshold values and raise/clear the alarm.
     */
    for (j=0; j<pPMAlmTableAttr->alarmCount; j++)
    {
        ClPMAlmAttrDefT* pAlarmAttrDef = NULL;
        ClAlarmInfoPtrT pAlarmInfo = NULL; 
        ClAlarmHandleT alarmHandle = 0;
        ClUint8T* pBuf = NULL;
        ClUint32T dataSize = 0;

        pAlarmAttrDef = (pPMAlmTableAttr->pAlarmAttrDef + j);
        CL_ASSERT(pAlarmAttrDef);

        if (pmAttr->pAlarmData[j].pAlarmPayload != NULL)
        {
            rc = clAlarmUtilPayloadFlatten(pmAttr->pAlarmData[j].pAlarmPayload, &dataSize, &pBuf);
            if (rc != CL_OK)
            {
                clLogError("PM", "PMLISTWALK", "Failed to flatten the alarm payload. rc [0x%x]", rc);
                return rc;
            }
        }

        pAlarmInfo = clHeapAllocate(sizeof(ClAlarmInfoT) + dataSize);
        if (!pAlarmInfo)
        {
            clLogError("PM", "PMLISTWALK", "Failed to allocate memory.");
            if (pBuf) clHeapFree(pBuf);
            return CL_PM_RC(CL_ERR_NO_MEMORY);
        }

        memset(pAlarmInfo, 0, sizeof(ClAlarmInfoT) + dataSize);

        rc = _clPMAttrRangeCheck(pmAttr->pPMData, pmAttr->attrType, pmAttr->attrDataType, 
                pmAttr->index, pmAttr->size, pAlarmAttrDef->lowerBound, pAlarmAttrDef->upperBound);
        if (rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_COR_ERR_CLASS_ATTR_OUT_OF_RANGE)
        {
            clLogError("PM", "PMLISTWALK", "Failed to check the attribute range : classId [%d], attrId [%d] "
                    "attrType [%d], attrDataType [%d]. rc [0x%x]", classId, pmAttr->attrId, 
                    pmAttr->attrType, pmAttr->attrDataType, rc);
            if (pBuf) clHeapFree(pBuf);
            clHeapFree(pAlarmInfo);
            break;
        }
        
        if (rc == CL_OK)
        {
            /* Attribute value falls under this boundary, raise the alarm configured. */

            pAlarmInfo->moId = *pMoId;
            pAlarmInfo->probCause = pAlarmAttrDef->probableCause;
            pAlarmInfo->specificProblem = pAlarmAttrDef->specificProblem;
            pAlarmInfo->severity = pAlarmAttrDef->severity;
            pAlarmInfo->alarmState = CL_ALARM_STATE_ASSERT;
            
            if (dataSize > 0)
            {
                pAlarmInfo->len = dataSize;
                memcpy(pAlarmInfo->buff, pBuf, dataSize);
            }

            clLogNotice("PM", "PMLISTWALK", "Raising alarm with probCause : [%d], specificProblem : [%d], "
                    "severity : [%d]", pAlarmInfo->probCause, pAlarmInfo->specificProblem, pAlarmInfo->severity);

            rc = clAlarmRaise(pAlarmInfo, &alarmHandle);
            if (rc != CL_OK)
            {
                clLogError("PM", "UTILS", "Failed to raise alarm. rc [0x%x]", rc);
                if (pBuf) clHeapFree(pBuf);
                clHeapFree(pAlarmInfo);
                return rc;
            }

            /* Max severity alarm is raised, need to not process the rest */
            break;
        }
        else
        {
            /* Attribute value is not within this boundary, clear the alarm if it has been raised. */

            ClAlarmStateT alarmState = 0;

            pAlarmInfo->moId = *pMoId;
            pAlarmInfo->probCause = pAlarmAttrDef->probableCause;
            pAlarmInfo->specificProblem = pAlarmAttrDef->specificProblem;
            pAlarmInfo->severity = pAlarmAttrDef->severity;

            if (dataSize > 0)
            {
                pAlarmInfo->len = dataSize;
                memcpy(pAlarmInfo->buff, pBuf, dataSize);
            }

            clLogDebug("PM", "PMLISTWALK", "Query whether the alarm with probCause : [%d], specificProblem : [%d], "
                "severity : [%d] has already been raised.", pAlarmInfo->probCause, pAlarmInfo->specificProblem, pAlarmInfo->severity);
            rc = clCorMoIdServiceSet(&pAlarmInfo->moId, CL_COR_SVC_ID_ALARM_MANAGEMENT);
            CL_ASSERT(rc == CL_OK);

            rc = clAlarmStateQuery(pAlarmInfo, &alarmState);
            if (rc != CL_OK)
            {
                clLogError("PM", "UTILS", "Failed to get alarm state. rc [0x%x]", rc);
                if (pBuf) clHeapFree(pBuf);
                clHeapFree(pAlarmInfo);
                return rc;
            }

            if (alarmState == CL_ALARM_STATE_ASSERT)
            {
                pAlarmInfo->alarmState = CL_ALARM_STATE_CLEAR;

                clLogNotice("PM", "PMLISTWALK", "Clearing the alarm with probCause : [%d], specificProblem : [%d], "
                    "severity : [%d]", pAlarmInfo->probCause, pAlarmInfo->specificProblem, pAlarmInfo->severity);

                rc = clAlarmRaise(pAlarmInfo, &alarmHandle);
                if (rc != CL_OK)
                {
                    clLogError("PM", "UTILS", "Failed to clear the alarm. rc [0x%x]", rc);
                    if (pBuf) clHeapFree(pBuf);
                    clHeapFree(pAlarmInfo);
                    return rc;
                }
            }
        }

        if (pBuf) clHeapFree(pBuf);
        clHeapFree(pAlarmInfo);
    }

    return CL_OK;
}

ClRcT _clPMAttrRangeCheck(void* pValue, ClCorAttrTypeT attrType, ClCorTypeT attrDataType, ClInt32T index, 
        ClUint32T size, ClInt64T minVal, ClInt64T maxVal)
{
    ClInt64T val = 0;
    ClUint32T i = 0;
    ClRcT rc = CL_OK;

    if (!pValue)
    {
        clLogError("PM", "UTILS", "NULL value passed in parameter pValue.");
        return CL_PM_RC(CL_ERR_NULL_POINTER);
    }

    if (index == -1)
        index = 0;

    if (attrType == CL_COR_SIMPLE_ATTR)
    {
        switch (attrDataType)
        {
            case CL_COR_INT8:
                val = * ((ClInt8T *) pValue + index);

                if (val > minVal && val < maxVal)
                    return CL_OK;
                break;

            case CL_COR_UINT8:
                val = * ((ClUint8T *) pValue + index);

                if ( (ClUint64T) val > (ClUint64T) minVal  &&
                        (ClUint64T) val < (ClUint64T) maxVal)
                    return CL_OK;
                break;

            case CL_COR_INT16:
                val = *((ClInt16T *) pValue + index);

                if (val > minVal && val < maxVal)
                    return CL_OK;
                break;

            case CL_COR_UINT16:
                val = *((ClUint16T *) pValue + index);

                if ( (ClUint64T) val > (ClUint64T) minVal  &&
                        (ClUint64T) val < (ClUint64T) maxVal)
                    return CL_OK;
                break;

            case CL_COR_INT32:
                val = *((ClInt32T *) pValue + index);

                if (val > minVal && val < maxVal)
                    return CL_OK;
                break;

            case CL_COR_UINT32:
                val = *((ClUint32T *) pValue + index);

                if ( (ClUint64T) val > (ClUint64T) minVal  &&
                        (ClUint64T) val < (ClUint64T) maxVal)
                    return CL_OK;
                break;

            case CL_COR_INT64:
                val = *((ClInt64T *)pValue + index);

                if (val > minVal && val < maxVal)
                    return CL_OK;
                break;

            case CL_COR_UINT64:
                val = *((ClUint64T *) pValue + index);

                if ( (ClUint64T) val > (ClUint64T) minVal  &&
                        (ClUint64T) val < (ClUint64T) maxVal)
                    return CL_OK;
                break;

            default:
            {
                    clLogError("PM", "UTILS", "Invalid attribute data type [%d] specified.", attrDataType);
                    return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
            }
        }

        return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_OUT_OF_RANGE); 
    }
    else if (attrType == CL_COR_ARRAY_ATTR)
    {
        ClUint32T count = 0;
        ClUint32T elemSize = 0;

        /* Go through each element and check for the value range. */
        elemSize = _clPMAttrSizeGet(attrDataType);

        count = size / elemSize;

        for (i=0; i<count; i++)
        {
            rc = _clPMAttrRangeCheck(pValue, CL_COR_SIMPLE_ATTR, attrDataType, i, elemSize, minVal, maxVal);
            if (rc == CL_OK)
            {
                /* Attribute's value falls within the boundary (i.e, it crossed the threshold).
                 * Process the alarms.
                 */
                return rc; 
            }
        }

        /* Attribute's value is less than the threshold configured. */
        return rc;
    }
    else
    {
        clLogError("PM", "UTILS", "Invalid attribute type [%d] specified.", attrType);
        return CL_COR_SET_RC(CL_COR_ERR_CLASS_ATTR_INVALID_TYPE);
    }

    return CL_OK;
}

ClCorAttrDefT* _clPMAttrCachedGet(ClCorAttrDefT* pCorAttrDef, ClUint32T attrCount, ClCorAttrIdT attrId)
{
    ClUint32T i = 0;

    if (!pCorAttrDef)
    {
        return NULL; 
    }

    for (i=0; i<attrCount; i++)
    {
        if (attrId == (pCorAttrDef + i)->attrId)
        {
            clLogTrace("PM", "PMLISTWALK", "Found the attribute [%d] in the cached attribtue list.", attrId);
            return (pCorAttrDef + i);
        }
    }

    return NULL;
}

ClRcT clPMObjectDataFree(ClPMObjectDataPtrT pmObjData)
{
    ClUint32T i = 0;
    ClUint32T j = 0;
    ClPMAttrDataPtrT pAttrData = NULL;

    for (i=0; i < pmObjData->attrCount; i++)
    {
        pAttrData = (pmObjData->pAttrData + i);

        clHeapFree(pAttrData->pPMData);

        for (j=0; j < pAttrData->alarmCount; j++)
        {
            if (pAttrData->pAlarmData[j].pAlarmPayload)
                clAlarmUtilPayloadListFree(pAttrData->pAlarmData[j].pAlarmPayload);
        }

        clHeapFree(pAttrData->pAlarmData);
    }
    
    clHeapFree(pmObjData->pAttrData);

    return CL_OK;
}

ClRcT clPMStart(ClCorMOIdListPtrT pMoIdList)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;
    ClCorMOIdPtrT pMoId = NULL;

    /* Add the Moids into the linked list to periodically collect the statistics */
    clOsalMutexLock(&gClPMMutex);
    for (i=0; i<pMoIdList->moIdCnt; i++)
    {
        pMoId = clHeapAllocate(sizeof(ClCorMOIdT));
        if (!pMoId)
        {
            clLogError("PM", "PMSTART", "Failed to allocate memory.");
            rc = CL_PM_RC(CL_ERR_NO_MEMORY);
            goto out_unlock;
        }

        memcpy((void *) pMoId, (void *) &pMoIdList->moId[i], sizeof(ClCorMOIdT));

        rc = clCorMoIdServiceSet(pMoId, CL_COR_SVC_ID_PM_MANAGEMENT);
        if (rc != CL_OK)
        {
            clLogError("PM", "PMSTART", "Failed to set service-id in the moId. rc [0x%x]", rc);
            goto out_unlock;
        }

        rc = clClistFirstNodeAdd(gClPMMoIdList, pMoId);
        if (rc != CL_OK)
        {
            clLogError("PM", "PMSTART", "Failed to add the resource into PM MoId list. rc [0x%x]", rc);
            goto out_unlock;
        }
    }

    out_unlock:
    clOsalMutexUnlock(&gClPMMutex);
    return rc;
}

ClRcT clPMStop(ClCorMOIdListPtrT pMoIdList)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;
    ClCorMOIdT moId = {{{0}}};

    /* Add the Moids into the linked list to periodically the statistics */
    clOsalMutexLock(&gClPMMutex);
    for (i=0; i<pMoIdList->moIdCnt; i++)
    {
        memcpy(&moId, &pMoIdList->moId[i], sizeof(ClCorMOIdT));

        rc = clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_PM_MANAGEMENT);
        if (rc != CL_OK)
        {
            clLogError("PM", "PMSTOP", "Failed to set service-id of the moId. rc [0x%x]", rc);
            goto out_unlock;
        }

        rc = clPMMoIdListNodeDel(gClPMMoIdList, &moId);
        if (rc != CL_OK)
        {
            clLogError("PM", "PMSTOP", "Failed to remove the resource from PM MoId list. rc [0x%x]", rc);
            goto out_unlock;
        }
    }

    out_unlock:
    clOsalMutexUnlock(&gClPMMutex);
    return rc;
}

ClRcT clPMResourcesGet(ClCorMOIdListT** ppMoIdList)
{
    if (!pPMConfigMoIdList)
    {
        clLogDebug("PM", "RESLISTGET", "No PM resources configured.");
        return CL_OK;
    }

    *ppMoIdList = clHeapAllocate(sizeof(ClCorMOIdListT) + 
                        pPMConfigMoIdList->moIdCnt * sizeof(ClCorMOIdT));
    if (!(*ppMoIdList))
    {
        clLogError("PM", "RESLISTGET", "Failed to allocate memory.");
        return CL_PM_RC(CL_ERR_NO_MEMORY);
    }

    memcpy(*ppMoIdList, pPMConfigMoIdList, 
            sizeof(ClCorMOIdListT) + pPMConfigMoIdList->moIdCnt * sizeof(ClCorMOIdT));

    return CL_OK;
}
