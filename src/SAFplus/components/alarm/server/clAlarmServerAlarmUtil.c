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
 * File        : clAlarmServerAlarmUtil.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This file provide implemantation of the Eonized application main function
 *          This C file SHOULD NOT BE MODIFIED BY THE USER
 *
 *****************************************************************************/


#include <string.h>
#include <stdlib.h>
#include <clCommon.h>
#include <sys/time.h>
#include <clEoApi.h>

#include <clCorApi.h>
#include <clDebugApi.h>
#include <clOmApi.h>
#include <clCorMetaData.h>
#include <clCorUtilityApi.h>
#include <clCorServiceId.h>
#include <clCorTxnApi.h>
#include <clAlarmDefinitions.h>
#include <clAlarmCommons.h>
#include <clAlarmErrors.h>
#include <clAlarmServerAlarmUtil.h>
#include <clAlarmServerEngine.h>
#include <clAlarmInfoCnt.h>
#include <clAlarmPayloadCont.h>

#include <ipi/clAlarmIpi.h>
#include "xdrClAlarmIdIDLT.h"
#include "xdrClCorMOIdT.h"

ClRcT 
clAlarmAttrValueGet(ClCorObjectHandleT hMSOObj, ClAlarmAttrListT* pAlarmAttrList) 
{
    ClRcT                   rc = CL_OK;
    ClUint32T               index=0;
    ClUint32T               i = 0;
    ClCorBundleHandleT      bundleHandle = CL_HANDLE_INVALID_VALUE;
    ClCorBundleConfigT      bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrValueDescriptorListT attrList = {0};
                                                                                       
    if (pAlarmAttrList == NULL)
    {
        clLogError("ASU", "IDG", "pAlarmAttrList passed is NULL.");
        return CL_ALARM_RC(CL_ALARM_ERR_NULL_POINTER);
    }

    if (pAlarmAttrList->numOfAttr <= 0)
    {
        clLogError("ASU", "IDG", "No. of attributes passed is zero.");
        return CL_ALARM_RC(CL_ALARM_ERR_INVALID_PARAM);
    }

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if (rc != CL_OK)
    {
        clLogError("ASU", "IDG", "Failed to initialize the bundle. rc [0x%x]", rc);
        return rc;
    }

    attrList.numOfDescriptor = pAlarmAttrList->numOfAttr; 
    attrList.pAttrDescriptor = (ClCorAttrValueDescriptorPtrT) clHeapAllocate(
            attrList.numOfDescriptor * sizeof(ClCorAttrValueDescriptorT));
    if (attrList.pAttrDescriptor == NULL)
    {
        clLogError("ASU", "IDG", "Failed to allocate memory. rc [0x%x]", 
                CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }

    memset(attrList.pAttrDescriptor, 0, attrList.numOfDescriptor * sizeof(ClCorAttrValueDescriptorT));

    for (i=0; i<pAlarmAttrList->numOfAttr; i++)
    {
        if ( (pAlarmAttrList->attrInfo[i].attrId > CL_ALARM_SIMPLE_ATTR_START) &&
             (pAlarmAttrList->attrInfo[i].attrId < CL_ALARM_SIMPLE_ATTR_END) )
        {
            index = CL_COR_INVALID_ATTR_IDX;
        }

        if ( (pAlarmAttrList->attrInfo[i].attrId == CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY) || 
                ( (pAlarmAttrList->attrInfo[i].attrId > CL_ALARM_CONT_ATTR_START) &&
                  (pAlarmAttrList->attrInfo[i].attrId < CL_ALARM_CONT_ATTR_END)) )
        {
            rc = clCorAttrPathAlloc(&(attrList.pAttrDescriptor[i].pAttrPath));
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n clCorAttrPathAlloc, not able to get the\
                                       alm COR attr value for attrId:%d, rc:%0x \n", 
                                       pAlarmAttrList->attrInfo[i].attrId, rc));
                goto free_and_exit;
            }
            
            rc = clCorAttrPathAppend(attrList.pAttrDescriptor[i].pAttrPath, CL_ALARM_INFO_CONT_CLASS, 
                    pAlarmAttrList->attrInfo[i].index);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n clCorAttrPathAppend, not able to get the\
                                          alm COR attr value for attrId:%d, rc:%0x \n", 
                                          pAlarmAttrList->attrInfo[i].attrId, rc));
                goto free_and_exit;
            }        

            index = CL_COR_INVALID_ATTR_IDX; 
        }
        else if ( (pAlarmAttrList->attrInfo[i].attrId > CL_ALARM_ARRAY_ATTR_START) && 
                (pAlarmAttrList->attrInfo[i].attrId < CL_ALARM_ARRAY_ATTR_END) )
        {
            index = pAlarmAttrList->attrInfo[i].index;
        }

        attrList.pAttrDescriptor[i].attrId = pAlarmAttrList->attrInfo[i].attrId;
        attrList.pAttrDescriptor[i].index = index;
        attrList.pAttrDescriptor[i].bufferPtr = pAlarmAttrList->attrInfo[i].pValue;
        attrList.pAttrDescriptor[i].bufferSize = pAlarmAttrList->attrInfo[i].size;
        attrList.pAttrDescriptor[i].pJobStatus = clHeapAllocate(sizeof(ClCorJobStatusT));
        if (attrList.pAttrDescriptor[i].pJobStatus == NULL)
        {
            clLogError("ASU", "IDG", "Failed to allocate memory. rc [0x%x]",
                    CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY));
            rc = CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
            goto free_and_exit;
        }
    }

    rc = clCorBundleObjectGet(bundleHandle, &hMSOObj, &attrList);
    if (rc != CL_OK)
    {
        clLogError("ASU", "IDG", "Failed to add the get job to the bundle. rc [0x%x]", rc);
        goto free_and_exit;
    }

    rc = clCorBundleApply(bundleHandle);
    if (rc != CL_OK)
    {
        clLogError("ASU", "IDG", "Failed to apply the bundle. rc [0x%x]", rc);
        goto free_and_exit;
    }

free_and_exit:

    for (i=0; i<attrList.numOfDescriptor; i++)
    {
        clCorAttrPathFree(attrList.pAttrDescriptor[i].pAttrPath);

        if (attrList.pAttrDescriptor[i].pJobStatus)
            clHeapFree(attrList.pAttrDescriptor[i].pJobStatus);
    }

    clHeapFree(attrList.pAttrDescriptor);
    clCorBundleFinalize(bundleHandle);

    return rc;
}

ClRcT clAlarmAttrValueSet(ClCorObjectHandleT hMSOObj, ClAlarmAttrListT* pAttrList) 
{
    ClRcT rc = CL_OK;
    ClCorAttrPathT tempContAttrPath = {{{0}}};
    ClUint32T       index=0;    
    ClCorTxnSessionIdT txnId = 0;
    ClUint32T i = 0;
    
    if (pAttrList == NULL)
    {
        clLogError("UTL", "SET", "pAttrList is NULL.");
        return CL_ALARM_RC(CL_ALARM_ERR_NULL_POINTER);
    }

    if (pAttrList->numOfAttr <= 0)
    {
        clLogError("UTL", "SET", "No. of attributes is zero.");
        return CL_ALARM_RC(CL_ALARM_ERR_INVALID_PARAM);
    }

    for (i=0; i<pAttrList->numOfAttr; i++)
    {
        rc = clCorAttrPathInitialize(&tempContAttrPath);
        if (rc != CL_OK)
        {
            clLogError("UTL", "SET", "clCorAttrPathAlloc failed for attrId:%d, rc [0x%x]", 
                        pAttrList->attrInfo[i].attrId, rc);
            clCorTxnSessionFinalize(txnId);
            return rc;
        }        

        if ( (pAttrList->attrInfo[i].attrId > CL_ALARM_SIMPLE_ATTR_START) &&
             (pAttrList->attrInfo[i].attrId < CL_ALARM_SIMPLE_ATTR_END) )
        {
            index = CL_COR_INVALID_ATTR_IDX; 
        }

        if ( (pAttrList->attrInfo[i].attrId == CL_ALARM_CONTAINMENT_ATTR_VALID_ENTRY) || 
                ( (pAttrList->attrInfo[i].attrId > CL_ALARM_CONT_ATTR_START) &&
                ( pAttrList->attrInfo[i].attrId < CL_ALARM_CONT_ATTR_END)) )
        {
            rc = clCorAttrPathAppend(&tempContAttrPath, CL_ALARM_INFO_CONT_CLASS, pAttrList->attrInfo[i].index);
            if (rc != CL_OK)
            {
                clLogError("UTL", "SET", "clCorAttrPathAppend failed for attrId: %d. rc [0x%x]", 
                            pAttrList->attrInfo[i].attrId, rc);
                clCorTxnSessionFinalize(txnId);
                return rc;
            }

            index = CL_COR_INVALID_ATTR_IDX;
        }
        else if ( (pAttrList->attrInfo[i].attrId > CL_ALARM_ARRAY_ATTR_START) && 
                (pAttrList->attrInfo[i].attrId < CL_ALARM_ARRAY_ATTR_END) )
        {
            index = pAttrList->attrInfo[i].index;
        }

        rc = clCorObjectAttributeSet(&txnId, hMSOObj, &tempContAttrPath, pAttrList->attrInfo[i].attrId, 
                index, pAttrList->attrInfo[i].pValue, pAttrList->attrInfo[i].size);

        if (rc != CL_OK)
        {
            clLogError("UTL", "SET", "clCorObjectAttributeSet failed for attrId:%d, rc [0x%x]", 
                    pAttrList->attrInfo[i].attrId, rc);
            clCorTxnSessionFinalize(txnId);
            return rc;
        }
    }

    rc = clCorTxnSessionCommit(txnId);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTL", "Failed to commit the transaction. rc [0x%x]", rc);
        clCorTxnSessionFinalize(txnId);
        return rc;
    }
     
    return rc;
}

/**
 * Function to get the alarm index given the alarm probable cause and specific problem field.
 */

ClRcT 
clAlarmUtilAlmIdxGet(ClCorObjectHandleT objH,  ClAlarmRuleEntryT alarmKey, 
                        ClUint32T* index)
{
    ClRcT                   rc = CL_ERR_NOT_EXIST;
    ClAlarmRuleEntryT       alarmKeyList[CL_ALARM_MAX_ALARMS] = {{0}};
    ClAlarmAttrListT*       pAttrList = NULL;
    ClUint32T               i = 0;

    CL_FUNC_ENTER();

    clLogDebug("ASU", "IDG", 
            "Querying the almInfo structure for "
            " probable cause [%d], Specific Problem [%d]", 
            alarmKey.probableCause, alarmKey.specificProblem);
                   
    pAttrList = clHeapAllocate(sizeof(ClAlarmAttrListT) + 
            (2* CL_ALARM_MAX_ALARMS) * sizeof(ClAlarmAttrInfoT));
    if (pAttrList == NULL)
    {
        clLogError("ASU", "IDG", "pAttrList passed is NULL.");
        return CL_ALARM_RC(CL_ALARM_ERR_NULL_POINTER);
    }
     
    pAttrList->numOfAttr = (2 * CL_ALARM_MAX_ALARMS);

    for (i=0; i<CL_ALARM_MAX_ALARMS; i++)
    {
        /* Get Probable Cause */
        pAttrList->attrInfo[i].attrId = CL_ALARM_PROBABLE_CAUSE;
        pAttrList->attrInfo[i].index = i;
        pAttrList->attrInfo[i].pValue = &(alarmKeyList[i].probableCause);
        pAttrList->attrInfo[i].size = sizeof(ClUint32T);

        /* Get Specific Problem */
        pAttrList->attrInfo[i + CL_ALARM_MAX_ALARMS].attrId = CL_ALARM_SPECIFIC_PROBLEM;
        pAttrList->attrInfo[i + CL_ALARM_MAX_ALARMS].index = i;
        pAttrList->attrInfo[i + CL_ALARM_MAX_ALARMS].pValue = &(alarmKeyList[i].specificProblem);
        pAttrList->attrInfo[i + CL_ALARM_MAX_ALARMS].size = sizeof(ClUint32T);
    }

    rc = clAlarmAttrValueGet(objH, pAttrList);
    if (rc != CL_OK)
    {
        clLogError("ASU", "IDG", "Failed to get the probabale cause and specific problem values from COR. "
                "rc [0x%x]", rc);
        clHeapFree(pAttrList);
        return rc;
    }

    clHeapFree(pAttrList);

    for(i=0; i<CL_ALARM_MAX_ALARMS; i++)
    {
        if((alarmKey.probableCause == alarmKeyList[i].probableCause) && 
                (alarmKey.specificProblem == alarmKeyList[i].specificProblem)) 
        {
            clLogTrace("ASU", "IDG", 
                    "The alarm entry found for probable cause [%d], specific Problem"
                    "[%d] : idx [%d] ", alarmKey.probableCause, alarmKey.specificProblem, i);
            *index = i;
            return CL_OK;
        }
    }

    rc = CL_ALARM_RC(CL_ERR_NOT_EXIST);

    clLogError("ASU", "IDG", "Failed to get the alarm index for Probable "
            "Cause [%d], Specific Problem [%d] combo. rc [0x%x]", 
            alarmKey.probableCause, alarmKey.specificProblem, rc);

    return rc;
}

ClRcT VDECL_VER(clAlarmServerAlarmReset, 4, 1, 0) (ClEoDataT data, 
                              ClBufferHandleT  inMsgHandle, 
                              ClBufferHandleT  outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClVersionT version = {0};
    VDECL_VER(ClAlarmIdIDLT, 4, 1, 0) alarmIdIDL = {0};
    VDECL_VER(ClAlarmIdT, 4, 1, 0)* pAlarmId = NULL;
    ClCorBundleHandleT bundleHandle = 0;
    ClCorBundleConfigT bundleConfig = {CL_COR_BUNDLE_NON_TRANSACTIONAL};
    ClCorAttrValueDescriptorT attrDesc[CL_ALARM_MAX_ALARMS] = {{0}};
    ClCorAttrValueDescriptorListT attrDescList = {0};
    ClUint64T almsAfterSoakingBM = 0;
    ClUint64T publishedAlmsBM = 0;
    ClUint64T suppressedAlmsBM = 0;
    ClAlarmCategoryTypeT category[CL_ALARM_MAX_ALARMS] = {0}; 
    ClUint8T alarmCategoryBM = 0;
    ClAlarmAttrListT* pAttrList = NULL; 
    ClUint32T idx = 0;
    ClCorJobStatusT jobStatus = 0;
    ClCorMOIdT moId = {{{0}}};
    ClCorObjectHandleT objH = 0;
    ClCorAttrPathT attrPath[CL_ALARM_MAX_ALARMS] = {{{{0}}}};
    ClUint32T i = 0;
    ClUint8T alarmActive = 0;

    rc = clXdrUnmarshallClVersionT(inMsgHandle, &version);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to unmarshall ClVersionT. rc [0x%x]", rc);
        return rc;
    }

    clAlarmClientToServerVersionValidate(version);

    rc = VDECL_VER(clXdrUnmarshallClCorMOIdT, 4, 0, 0) (inMsgHandle, &moId);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to unmarshall ClCorMOIdT. rc [0x%x]", rc);
        return rc;
    }

    rc = VDECL_VER(clXdrUnmarshallClAlarmIdIDLT, 4, 1, 0) (inMsgHandle, &alarmIdIDL);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to unmarshall ClAlarmIdIDLT. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorMoIdToObjectHandleGet(&moId, &objH);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to get object handle from moId. rc [0x%x]", rc);
        clHeapFree(alarmIdIDL.pAlarmId);
        return rc;
    }

    rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to initialize the bundle. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        clHeapFree(alarmIdIDL.pAlarmId);
        return rc;
    }

    attrDesc[0].pAttrPath = NULL;
    attrDesc[0].attrId = CL_ALARM_ALMS_AFTER_SOAKING_BITMAP;
    attrDesc[0].index = -1;
    attrDesc[0].bufferPtr = &almsAfterSoakingBM;
    attrDesc[0].bufferSize = sizeof(ClUint64T);
    attrDesc[0].pJobStatus = &jobStatus;

    attrDesc[1].pAttrPath = NULL;
    attrDesc[1].attrId = CL_ALARM_PUBLISHED_ALARMS;
    attrDesc[1].index = -1;
    attrDesc[1].bufferPtr = &publishedAlmsBM;
    attrDesc[1].bufferSize = sizeof(ClUint64T);
    attrDesc[1].pJobStatus = &jobStatus;

    attrDesc[2].pAttrPath = NULL;
    attrDesc[2].attrId = CL_ALARM_SUPPRESSED_ALARMS;
    attrDesc[2].index = -1;
    attrDesc[2].bufferPtr = &suppressedAlmsBM;
    attrDesc[2].bufferSize = sizeof(ClUint64T);
    attrDesc[2].pJobStatus = &jobStatus;

    attrDescList.numOfDescriptor = 3;
    attrDescList.pAttrDescriptor = attrDesc;

    rc = clCorBundleObjectGet(bundleHandle, &objH, &attrDescList);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to add GET job to the bundle. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        clHeapFree(alarmIdIDL.pAlarmId);
        clCorBundleFinalize(bundleHandle);
        return rc;
    }

    rc = clCorBundleApply(bundleHandle);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to apply GET bundle job. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        clCorBundleFinalize(bundleHandle);
        clHeapFree(alarmIdIDL.pAlarmId);
        return rc;
    }

    rc = clCorBundleFinalize(bundleHandle);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to finalize the bundle. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        clHeapFree(alarmIdIDL.pAlarmId);
        return rc;
    }

    pAttrList = (ClAlarmAttrListT *) clHeapAllocate(sizeof(ClAlarmAttrListT) + 
            CL_ALARM_MAX_ALARMS * sizeof(ClAlarmAttrInfoT));
    if (!pAttrList)
    {
        clLogError("ALM", "UTIL", "Failed to allocate memory.");
        clCorObjectHandleFree(&objH);
        clHeapFree(alarmIdIDL.pAlarmId);
        return CL_ALARM_RC(CL_ERR_NO_MEMORY);
    }

    for (i=0; i<alarmIdIDL.numEntries; i++)
    {
        ClUint32T almIndex = 0;

        pAlarmId = (alarmIdIDL.pAlarmId + i);
        CL_ASSERT(pAlarmId);

        rc = clAlarmUtilAlmIdxGet(objH, *pAlarmId, &almIndex);
        if (rc != CL_OK)
        {
            clLogError("ALM", "UTIL", "Failed to get alarm index for pc [%d], sp [%u]. rc [0x%x]", 
                    pAlarmId->probableCause, pAlarmId->specificProblem, rc);
            goto free_and_exit;
        }

        almsAfterSoakingBM &= ~((ClUint64T) 1 << almIndex);
        publishedAlmsBM &= ~((ClUint64T) 1 << almIndex);

        /* 
         * No need to do anything if it is already suppressed. 
         */
        if ((suppressedAlmsBM & ((ClUint64T) 1 << almIndex)) != 0)
        {
            continue;
        }

        rc = clCorAttrPathAppend(&attrPath[idx], 0x3, idx);
        if (rc != CL_OK)
        {
            clLogError("ALM", "UTIL", "Failed to SET index in the attribute path. rc [0x%x]", rc);
            goto free_and_exit;
        }

        /* Set the alarm active value using alarm wrapper function. */
        alarmActive = CL_ALARM_STATE_CLEAR;
        pAttrList->attrInfo[idx].attrId = CL_ALARM_ACTIVE;
        pAttrList->attrInfo[idx].index = almIndex; 
        pAttrList->attrInfo[idx].pValue = &alarmActive;
        pAttrList->attrInfo[idx].size = sizeof(ClUint8T);

        /* Get the alarm category using cor api. */
        attrDesc[idx].pAttrPath = &attrPath[idx];
        attrDesc[idx].attrId = CL_ALARM_CATEGORY;
        attrDesc[idx].index = -1;
        attrDesc[idx].bufferPtr = &category[idx];
        attrDesc[idx].bufferSize = sizeof(ClUint8T);
        attrDesc[idx].pJobStatus = &jobStatus;

        idx++;
    }

    /*
     * Set the bitmaps into COR.
     */
    pAttrList->attrInfo[idx].attrId = CL_ALARM_ALMS_AFTER_SOAKING_BITMAP;
    pAttrList->attrInfo[idx].index = 0;
    pAttrList->attrInfo[idx].pValue = &almsAfterSoakingBM;
    pAttrList->attrInfo[idx].size = sizeof(ClUint64T);

    pAttrList->attrInfo[idx+1].attrId = CL_ALARM_PUBLISHED_ALARMS;
    pAttrList->attrInfo[idx+1].index = 0;
    pAttrList->attrInfo[idx+1].pValue = &publishedAlmsBM;
    pAttrList->attrInfo[idx+1].size = sizeof(ClUint64T);

    pAttrList->numOfAttr = (idx+2);

    rc = clAlarmAttrValueSet(objH, pAttrList);
    if (rc != CL_OK)
    {
        clLogError("ALM", "UTIL", "Failed to SET alarm attribute values. rc [0x%x]", rc);
        goto free_and_exit;
    }

    /*
     * Only if there is any unsuppressed alarm to be reset. 
     * Walk the children and reset the PEF entry.
     */
    if (idx > 0)
    {
        PefInfoT pefInfo = {0};

        /*
         * Get alarm category values from COR.
         */
        rc = clCorBundleInitialize(&bundleHandle, &bundleConfig);
        if (rc != CL_OK)
        {
            clLogError("ALM", "UTIL", "Failed to initialize the bundle. rc [0x%x]", rc);
            goto free_and_exit;
        }

        attrDescList.numOfDescriptor = idx;
        attrDescList.pAttrDescriptor = attrDesc;

        rc = clCorBundleObjectGet(bundleHandle, &objH, &attrDescList);
        if (rc != CL_OK)
        {
            clLogError("ALM", "UTIL", "Failed to add object job to the bundle. rc [0x%x]", rc);
            clCorBundleFinalize(bundleHandle);
            goto free_and_exit;
        }

        rc = clCorBundleApply(bundleHandle);
        if (rc != CL_OK)
        {
            clLogError("ALM", "UTIL", "Failed to apply the bundle. rc [0x%x]", rc);
            clCorBundleFinalize(bundleHandle);
            goto free_and_exit;
        }

        rc = clCorBundleFinalize(bundleHandle);
        if (rc != CL_OK)
        {
            clLogError("ALM", "UTIL", "Failed to finalize the bundle. rc [0x%x]", rc);
            goto free_and_exit;
        }

        while (idx)
            alarmCategoryBM |= clAlarmCategory2BitmapTranslate(category[--idx]);

        memcpy(&pefInfo.moID, &moId, sizeof(ClCorMOIdT));
        pefInfo.isPEFpresent = alarmCategoryBM;

        rc = clCorObjectWalk(&moId, NULL, _clAlarmResetChildWalk, CL_COR_MSO_WALK, &pefInfo);
        if (rc != CL_OK)
        {
            clLogError("ALM", "UTIL", "Failed to do cor object walk. rc [0x%x]", rc);
            goto free_and_exit;
        }
    }

    for (i=0; i<alarmIdIDL.numEntries; i++)
    {
        ClAlarmInfoT alarmInfo = {0};
        ClAlarmHandleT alarmHandle = 0;

        pAlarmId = (alarmIdIDL.pAlarmId + i);
        
        alarmInfo.probCause = pAlarmId->probableCause;
        alarmInfo.specificProblem = pAlarmId->specificProblem;
        memcpy(&alarmInfo.moId, &moId, sizeof(ClCorMOIdT));

        /* Clear the alarm handle information. */
        rc = clAlarmAlarmInfoToHandleGet(&alarmInfo, &alarmHandle);
        if (rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        {
            clLogError("ALM", "UTIL", "Failed to get alarm handle from the container. rc [0x%x]", rc);
            goto free_and_exit;
        }

        if (rc == CL_OK)
        {
            rc = clAlarmAlarmInfoDelete(alarmHandle);
            if (rc != CL_OK)
            {
                clLogError("ALM", "UTIL", "Failed to delete alarm information from the container. rc [0x%x]", rc);
                goto free_and_exit;
            }
        }

        /* Clear the alarm payload information. */
        rc = clAlarmPayloadCntDelete(&moId, pAlarmId->probableCause, pAlarmId->specificProblem);
        if (rc != CL_OK && CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
        {
            clLogError("ALM", "UTIL", "Failed to remove the payload information from the container. rc [0x%x]", rc);
            goto free_and_exit;
        }
    }

    rc = CL_OK;

free_and_exit:    
    clHeapFree(pAttrList);
    clHeapFree(alarmIdIDL.pAlarmId);
    clCorObjectHandleFree(&objH);

    return rc;
}

ClRcT _clAlarmResetChildWalk(void* pData, void* pCookie)
{
    ClCorObjectHandleT objH = *(ClCorObjectHandleT *) pData;
    ClCorMOIdT moId = {{{0}}};
    PefInfoT pefInfo = {0};
    ClUint8T pefEntry = 0;
    ClRcT rc = CL_OK;
    ClCorServiceIdT svcId = CL_COR_INVALID_SVC_ID;
    ClAlarmAttrListT attrList = {0};

    if (!pCookie)
    {
        clLogError("ALM", "WALK", "NULL pointer passed in pCookie.");
        clCorObjectHandleFree(&objH);
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    pefInfo = *(PefInfoT *) pCookie;

    rc = clCorObjectHandleToMoIdGet(objH, &moId, &svcId);
    if (rc != CL_OK)
    {
        clLogError("ALM", "WALK", "Failed to get MoId from object handle. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    if ((svcId != CL_COR_SVC_ID_ALARM_MANAGEMENT) || (!clCorMoIdCompare(&pefInfo.moID, &moId)))
    {
        clCorObjectHandleFree(&objH);
        return CL_OK;
    }

    attrList.numOfAttr = 1; 
    attrList.attrInfo[0].attrId = CL_ALARM_PARENT_ENTITY_FAILED;
    attrList.attrInfo[0].index = 0; 
    attrList.attrInfo[0].pValue = &(pefEntry);
    attrList.attrInfo[0].size = sizeof(ClUint8T);

    rc = clAlarmAttrValueGet(objH, &attrList);
    if (CL_OK != rc)
    {    
        clLogError("ALM", "UTIL", "Failed while getting the value of the PEF attribute. rc[0x%x]", rc); 
        clCorObjectHandleFree(&objH);
        return rc;
    }    

    pefEntry &= (~pefInfo.isPEFpresent);

    attrList.numOfAttr = 1;
    attrList.attrInfo[0].attrId = CL_ALARM_PARENT_ENTITY_FAILED;
    attrList.attrInfo[0].index = 0;
    attrList.attrInfo[0].pValue = &pefEntry;
    attrList.attrInfo[0].size = sizeof(ClUint8T);

    rc  = clAlarmAttrValueSet(objH, &attrList);
    if (CL_OK != rc)
    {
        clLogError("ALM", "UTIL", "Failed while setting the PEF attribute. rc[0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    clCorObjectHandleFree(&objH);
    return CL_OK;
}
