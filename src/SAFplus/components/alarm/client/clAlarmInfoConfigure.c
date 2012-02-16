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
 * File        : clAlarmInfoConfigure.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This module contains alarm Service related APIs
 *****************************************************************************/

#include <clCommon.h>
#include <clAlarmUtil.h>
#include <clCorMetaData.h>
#include <clAlarmErrors.h>
#include <clAlarmClient.h>

#define HASH_NUM_BUCKETS 256
ClCntHandleT gClAlarmConfigTable = 0;

ClRcT clAlarmClientConfigTableCreate()
{
    ClRcT rc = CL_OK;

    rc = clCntHashtblCreate(HASH_NUM_BUCKETS, _clAlarmInfoKeyCompare, _clAlarmInfoHashFunc, 0, 0, 
            CL_CNT_UNIQUE_KEY, &gClAlarmConfigTable);
    if (rc != CL_OK)
    {
        clLogError("ALM", "HASH", "Failed to create hash table container. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

ClRcT clAlarmClientConfigTableDelete()
{
    ClRcT rc = CL_OK;

    rc = clCntDelete(gClAlarmConfigTable);
    if (rc != CL_OK)
    {
        clLogError("ALM", "HASH", "Failed to delete the alarm configuration table. rc [0x%x]", rc);
        return rc;
    }

    return CL_OK;
}

ClInt32T _clAlarmInfoKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
   return ((ClWordT) key1 - (ClWordT) key2);
}

ClUint32T _clAlarmInfoHashFunc(ClCntKeyHandleT key)
{
    return ((ClWordT) key & (HASH_NUM_BUCKETS - 1));
}

ClRcT clAlarmClientConfigTableClassAdd(ClCorClassTypeT classId, VDECL_VER(ClCorAlarmResourceDataIDLT, 4,1,0)* pResData)
{
    ClRcT rc = CL_OK;

    rc = clCntNodeAdd(gClAlarmConfigTable, (ClCntKeyHandleT) (ClWordT) classId, (ClCntDataHandleT) pResData, 0);
    if (rc != CL_OK)
    {
        clLogError("ALM", "HASH", "Failed to add into the container hash table. rc [0x%x]", rc);
        return rc;
    }
    
    return CL_OK;
}

ClRcT clAlarmClientConfigTableResDataGet(ClCorClassTypeT classId, VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)** pAlarmConfig)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT nodeHandle = 0;

    rc = clCntNodeFind(gClAlarmConfigTable, (ClCntKeyHandleT) (ClWordT) classId, (ClCntNodeHandleT *) &nodeHandle);
    if (rc != CL_OK)
    {
        clLogError("ALM", "HASH", "Failed to find the node in the container. rc [0x%x]", rc);
        return rc;
    }

    rc = clCntNodeUserDataGet(gClAlarmConfigTable, nodeHandle, (ClCntDataHandleT *) pAlarmConfig);
    if (rc != CL_OK)
    {
        clLogError("ALM", "HASH", "Failed to get node user data from the container. rc [0x%x]", rc);
        return rc;
    }

    return rc; 
}

ClRcT clAlarmClientConfigTableResAdd(ClCorClassTypeT classId, ClCorMOIdPtrT pMoId)
{
    ClCntNodeHandleT nodeHandle = 0;
    VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)* pResData = NULL;
    ClRcT rc = CL_OK;

    rc = clCntNodeFind(gClAlarmConfigTable, (ClCntKeyHandleT) (ClWordT) classId, (ClCntNodeHandleT *) &nodeHandle);
    if (rc != CL_OK)
    {
        clLogError("ALM", "HASH", "Failed to find the node in the container. rc [0x%x]", rc);
        return rc;
    }

    rc = clCntNodeUserDataGet(gClAlarmConfigTable, nodeHandle, (ClCntDataHandleT *) &pResData);
    if (rc != CL_OK)
    {
        clLogError("ALM", "HASH", "Failed to get node user data from the container. rc [0x%x]", rc);
        return rc;
    }

    if (!pResData)
    {
        clLogError("ALM", "HASH", "No configuration found in the table for classId [0x%x]", classId);
        return rc;
    }

    if (! (pResData->noOfInst & 3))
    {
        pResData->pAlarmInstList = clHeapRealloc(pResData->pAlarmInstList,
                (pResData->noOfInst + 4) * sizeof(ClCorMOIdT));
        CL_ASSERT(pResData->pAlarmInstList);
        memset((void *) (pResData->pAlarmInstList + pResData->noOfInst), 0, (4 * sizeof(ClCorMOIdT)));
    }

    memcpy( (void *) (pResData->pAlarmInstList + pResData->noOfInst), (void *) pMoId, sizeof(ClCorMOIdT));
    pResData->noOfInst++;

    return CL_OK;
}

ClRcT clAlarmClientConfigDataFree(VDECL_VER(ClCorAlarmConfiguredDataIDLT, 4, 1, 0)* pAlarmData)
{
    ClUint32T i = 0;
    VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)* pResourceData = NULL;

    if (!pAlarmData)
    {
        clLogError("ALM", "INT", "NULL pointer passed.");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    for (i=0; i<pAlarmData->noOfRes; i++)
    {
        pResourceData = (pAlarmData->pResourceData + i);
        
        if(pResourceData)
        {
            if (pResourceData->noOfAlarms && pResourceData->pAlarmProfile)
                clHeapFree(pResourceData->pAlarmProfile);

            if (pResourceData->noOfInst && pResourceData->pAlarmInstList)
                clHeapFree(pResourceData->pAlarmInstList);
        }
    }

    clHeapFree(pAlarmData->pResourceData);

    return CL_OK;
}

ClRcT clAlarmClientConfigDataGet(VDECL_VER(ClCorAlarmConfiguredDataIDLT, 4, 1, 0)* pAlarmData)
{
    ClRcT rc = CL_OK;
    ClUint32T index = 0;

    if (!pAlarmData)
    {
        clLogError("ALM", "INT", "NULL pointer passed.");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    while (appAlarms[index++].moClassID != 0);

    pAlarmData->noOfRes = index-1;
    clLogDebug("ALM", "INT", "No. of Alarm resources configured : %u", pAlarmData->noOfRes);

    pAlarmData->pResourceData = clHeapAllocate(pAlarmData->noOfRes * sizeof(VDECL_VER(ClCorAlarmResourceDataIDLT,4,1,0)));
    CL_ASSERT(pAlarmData->pResourceData != NULL);

    for (index = 0; index < pAlarmData->noOfRes; index++)
    {
        CL_ASSERT(pAlarmData->pResourceData + index);

        rc = clAlarmClientConfigMOAlarmsGet(&(appAlarms[index]), pAlarmData->pResourceData + index);
        if (rc != CL_OK)
        {
            clLogError("ALM", "INT", "Failed to get alarms configured for the class [0x%x]. rc [0x%x]", 
                    appAlarms[index].moClassID, rc);
            return rc;
        }

        rc = clAlarmClientConfigAlarmRuleSet(&(appAlarms[index]), pAlarmData->pResourceData + index);
        if (rc != CL_OK)
        {
            clLogError("ALM", "INT", "Failed to correlate alarms in the class [0x%x]. rc [0x%x]", 
                    appAlarms[index].moClassID, rc);
            return rc;
        }

        rc = clAlarmClientConfigTableClassAdd((pAlarmData->pResourceData + index)->classId, 
                (pAlarmData->pResourceData + index));
        if (rc != CL_OK)
        {
            clLogError("ALM", "INT", "Failed to add class info [0x%x] into hash table. rc [0x%x]", 
                    appAlarms[index].moClassID, rc);
            return rc;
        }
    }

    return CL_OK;
}


ClRcT clAlarmClientConfigAlarmRuleSet(ClAlarmComponentResAlarmsT* pAlarmMO, VDECL_VER(ClCorAlarmResourceDataIDLT, 4,1,0)* pResData)
{
    ClAlarmRuleEntryT alarmKeyGen = {0};
    ClAlarmRuleEntryT alarmKeySup = {0};
    ClUint32T curAlmIdx = 0;
    ClUint32T almRuleIdx = 0;
    ClUint32T almDepIdx = 0;

    if (!pAlarmMO || !pResData)
    {
        clLogError("ALM", "INT", "NULL pointer passed.");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    for (curAlmIdx=0; curAlmIdx < pResData->noOfAlarms; curAlmIdx++)
    {
        clLogTrace("ALM", "INT", "Configuring Alarm [%u] [probCause : %u, specificProb : %u] of class [0x%x].",
                curAlmIdx, pAlarmMO->MoAlarms[curAlmIdx].probCause,
                pAlarmMO->MoAlarms[curAlmIdx].specificProblem, pAlarmMO->moClassID);

        clLogTrace("", "", "Initial values : Alarm [probCause : %u, specificProb : %u] of class [0x%x]."
                "Generation Rule [%llu], Sup Rule [%llu], Gen Rule Rel [%u], Sup Rule Rel [%u], affected alarms [%llu]",
                pResData->pAlarmProfile[curAlmIdx].probCause,
                pResData->pAlarmProfile[curAlmIdx].specProb, pResData->classId,
                pResData->pAlarmProfile[curAlmIdx].genRule, pResData->pAlarmProfile[curAlmIdx].supRule,
                pResData->pAlarmProfile[curAlmIdx].genRuleRel, pResData->pAlarmProfile[curAlmIdx].supRuleRel,
                pResData->pAlarmProfile[curAlmIdx].affectedAlarms);

        clLogTrace("ALM", "INT", "Configuring Alarm [%u] [probCause : %u, specificProb : %u] of class [0x%x].",
                curAlmIdx, pResData->pAlarmProfile[curAlmIdx].probCause,
                pResData->pAlarmProfile[curAlmIdx].specProb, pResData->classId);

        for (almRuleIdx=0; almRuleIdx<CL_ALARM_MAX_ALARM_RULE_IDS; almRuleIdx++)
        {
            if ((pAlarmMO->MoAlarms[curAlmIdx].generationRule == NULL) && 
                    (pAlarmMO->MoAlarms[curAlmIdx].suppressionRule == NULL))
            {
                clLogTrace("ALM", "INT", "The Alarm [probCause : %u, specificProb : %u] of class [0x%x] "
                        "doesn't have generation or suppression rules configured.", pAlarmMO->MoAlarms[curAlmIdx].probCause,
                        pAlarmMO->MoAlarms[curAlmIdx].specificProblem, pAlarmMO->moClassID);
                break;
            }

            if (pAlarmMO->MoAlarms[curAlmIdx].generationRule != NULL)
            {
                alarmKeyGen.probableCause = pAlarmMO->MoAlarms[curAlmIdx].generationRule->alarmIds[almRuleIdx].probableCause;
                alarmKeyGen.specificProblem = pAlarmMO->MoAlarms[curAlmIdx].generationRule->alarmIds[almRuleIdx].specificProblem;
            }

            if (pAlarmMO->MoAlarms[curAlmIdx].suppressionRule != NULL)
            {
                alarmKeySup.probableCause = pAlarmMO->MoAlarms[curAlmIdx].suppressionRule->alarmIds[almRuleIdx].probableCause;
                alarmKeySup.specificProblem = pAlarmMO->MoAlarms[curAlmIdx].suppressionRule->alarmIds[almRuleIdx].specificProblem;
            }

            if (alarmKeyGen.probableCause == CL_ALARM_ID_INVALID &&
                    alarmKeySup.probableCause == CL_ALARM_ID_INVALID)
                break;

            /*
             * Set the affected alarm got from generation rule.
             */
            if (alarmKeyGen.probableCause != CL_ALARM_ID_INVALID)
            {
                clLogTrace("ALM", "INT", "Finding the alarm index for pc [%u, sp [%u] of gen rule.", 
                        alarmKeyGen.probableCause, alarmKeyGen.specificProblem);

                for (almDepIdx=0; almDepIdx < pResData->noOfAlarms; almDepIdx++)
                {
                    if (pResData->pAlarmProfile[almDepIdx].probCause == alarmKeyGen.probableCause &&
                            pResData->pAlarmProfile[almDepIdx].specProb == alarmKeyGen.specificProblem)
                        break;
                }

                clLogTrace("ALM", "INT", "Index found for pc [%u], sp [%u] is : [%u]",
                        alarmKeyGen.probableCause, alarmKeyGen.specificProblem, 
                        almDepIdx);

                pResData->pAlarmProfile[almDepIdx].affectedAlarms |= ((ClUint64T) 1 << curAlmIdx);
                pResData->pAlarmProfile[curAlmIdx].genRule |= ((ClUint64T) 1 << almDepIdx);

                clLogTrace("ALM", "INT", "Generation rule : [%llu]", pResData->pAlarmProfile[curAlmIdx].genRule);
            }

            if (alarmKeySup.probableCause != CL_ALARM_ID_INVALID)
            {
                clLogTrace("ALM", "INT", "Finding the alarm index for pc [%u, sp [%u] of sup rule.", 
                        alarmKeySup.probableCause, alarmKeySup.specificProblem);

                for (almDepIdx=0; almDepIdx<pResData->noOfAlarms; almDepIdx++)
                {
                    if (pResData->pAlarmProfile[almDepIdx].probCause == alarmKeySup.probableCause &&
                            pResData->pAlarmProfile[almDepIdx].specProb == alarmKeySup.specificProblem)
                        break;
                }

                clLogTrace("ALM", "INT", "Index found for pc [%u], sp [%u] is : [%u]",
                        alarmKeySup.probableCause, alarmKeySup.specificProblem, 
                        almDepIdx);

                pResData->pAlarmProfile[almDepIdx].affectedAlarms |= ((ClUint64T) 1 << curAlmIdx);
                pResData->pAlarmProfile[curAlmIdx].supRule |= ((ClUint64T) 1 << almDepIdx);
            }
        }

        if (pAlarmMO->MoAlarms[curAlmIdx].generationRule)
            pResData->pAlarmProfile[curAlmIdx].genRuleRel = pAlarmMO->MoAlarms[curAlmIdx].generationRule->relation;  

        if (pAlarmMO->MoAlarms[curAlmIdx].suppressionRule)
            pResData->pAlarmProfile[curAlmIdx].supRuleRel = pAlarmMO->MoAlarms[curAlmIdx].suppressionRule->relation; 

        clLogTrace("ALM", "INT", "Configured rules for the Alarm [probCause : %u, specificProb : %u] of class [0x%x]."
                "Generation Rule [%llu], Sup Rule [%llu], Gen Rule Rel [%u], Sup Rule Rel [%u], affected alarms [%llu]",
                pResData->pAlarmProfile[curAlmIdx].probCause,
                pResData->pAlarmProfile[curAlmIdx].specProb, pResData->classId,
                pResData->pAlarmProfile[curAlmIdx].genRule, pResData->pAlarmProfile[curAlmIdx].supRule,
                pResData->pAlarmProfile[curAlmIdx].genRuleRel, pResData->pAlarmProfile[curAlmIdx].supRuleRel,
                pResData->pAlarmProfile[curAlmIdx].affectedAlarms);
    }

    return CL_OK;
}

ClRcT clAlarmClientConfigMOAlarmsGet(ClAlarmComponentResAlarmsT* pAlarmMO, VDECL_VER(ClCorAlarmResourceDataIDLT, 4,1,0)* pResData)
{
    ClRcT rc = CL_OK;
    ClUint32T i = 0;

    if (!pResData || !pAlarmMO)
    {
        clLogError("ALM", "INT", "NULL pointer passed.");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    pResData->classId = pAlarmMO->moClassID;

    for (i=0; pAlarmMO->MoAlarms[i].probCause != CL_ALARM_ID_INVALID; i++)
    {
        if (! (pResData->noOfAlarms & 3))
        {
            pResData->pAlarmProfile = clHeapRealloc(pResData->pAlarmProfile, 
                    (pResData->noOfAlarms + 4) * sizeof(VDECL_VER(ClCorAlarmProfileDataIDLT, 4,1,0)));
            CL_ASSERT(pResData->pAlarmProfile);
        }
        
        memset((void *) (pResData->pAlarmProfile+i), 0, sizeof(VDECL_VER(ClCorAlarmProfileDataIDLT, 4,1,0)));

        rc = clAlarmClientConfigAlarmsGet(&(pAlarmMO->MoAlarms[i]), pResData->pAlarmProfile+i);
        if (rc != CL_OK)
        {
            clLogError("ALM", "INT", "Failed to get configured alarm profiles data. rc [0x%x]", rc);
            return rc;
        }

        pResData->noOfAlarms++;
    }

    clLogDebug("ALM", "INT", "No. of alarm profiles configured for the class [0x%x] is : [%u]", 
            pResData->classId, pResData->noOfAlarms);

    return CL_OK;
}

ClRcT clAlarmClientConfigAlarmsGet(ClAlarmProfileT* pAlarmConfig, VDECL_VER(ClCorAlarmProfileDataIDLT, 4,1,0)* pAlarmData)
{
    if (!pAlarmConfig || !pAlarmData)
    {
        clLogError("ALM", "INT", "NULL pointer passed.");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    pAlarmData->probCause = pAlarmConfig->probCause;
    pAlarmData->category = pAlarmConfig->category;
    pAlarmData->severity = pAlarmConfig->severity;
    pAlarmData->specProb = pAlarmConfig->specificProblem;
    pAlarmData->contValidEntry = 1;  

#ifdef ALARM_POLL
    if (pAlarmConfig->fetchMode == CL_ALARM_POLL)
        pAlarmData->pollAlarm = 1;
    else
        pAlarmData->pollAlarm = 0;
#endif

    return CL_OK;
}
