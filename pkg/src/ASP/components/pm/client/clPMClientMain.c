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
 * File        : clPMClientMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *      This module contains the PM library implementation. 
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
#include <clHash.h>

#include <clPMClientUtils.h>
#include <clPMErrors.h>

ClMsoConfigCallbacksT gClPMClientMsoCallbacks = {
    .fpMsoJobPrepare = clPMTxnPrepare, 
    .fpMsoJobCommit  = clPMTxnUpdate, 
    .fpMsoJobRollback = clPMTxnRollback, 
    .fpMsoJobRead = clPMRead
};

ClOsalMutexT gClPMMutex;

extern ClPMCallbacksT gClPMCallbacks;

ClClistT gClPMMoIdList = NULL; 
ClUint32T gClPMTimerInt = 0;
ClTimerHandleT gClPMTimerHandle = 0;
ClCorMOIdListT* pPMConfigMoIdList = NULL;

/**********************************************************************************
 * Alarm Class Hash Table definitions.
 *********************************************************************************/

static struct hashStruct *gpPMAlarmClassTable[CL_PM_ALMCLASSTABLE_BUCKETS];

static __inline__ ClUint32T clPMAlarmClassTableHash(ClCorClassTypeT classId)
{
    return ( (ClUint32T ) (classId-1) & CL_PM_ALMCLASSTABLE_MASK );
}

static __inline__ ClRcT clPMAlarmClassTableHashAdd(ClPMAlmTableClassDefT* pClassInfo)
{
    ClUint32T key = clPMAlarmClassTableHash(pClassInfo->classId);

    return hashAdd(gpPMAlarmClassTable, key, &pClassInfo->hash);
}

static __inline__ void clPMAlarmClassTableHashDel(ClPMAlmTableClassDefT* pClassInfo)
{
    hashDel(&pClassInfo->hash);
}

ClRcT clPMAlarmClassInfoGet(ClCorClassTypeT classId, ClPMAlmTableClassDefT** ppClassInfo)
{
    ClUint32T key = clPMAlarmClassTableHash(classId);
    register struct hashStruct *pTemp = NULL;

    for (pTemp = gpPMAlarmClassTable[key]; pTemp; pTemp = pTemp->pNext)
    {
        ClPMAlmTableClassDefT* pTempClassInfo = hashEntry(pTemp, ClPMAlmTableClassDefT, hash);
        if(pTempClassInfo->classId == classId)
        {
            *ppClassInfo = pTempClassInfo;
            return CL_OK;
        }
    }
    return CL_PM_RC(CL_ERR_NOT_EXIST);
}

/**********************************************************************************
 * Alarm Attribute Hash Table definitions.
 *********************************************************************************/

static __inline__ ClUint32T clPMAlarmAttrTableHash(ClCorAttrIdT attrId)
{

    return ( (ClUint32T ) (attrId-1) & CL_PM_ALMATTRTABLE_MASK );
}

static __inline__ ClRcT clPMAlarmAttrTableHashAdd(struct hashStruct **pPMAlarmAttrTable, ClPMAlmTableAttrDefT *pAttrInfo)
{
    ClUint32T key = clPMAlarmAttrTableHash(pAttrInfo->attrId);

    return hashAdd(pPMAlarmAttrTable, key, &pAttrInfo->hash);
}

static __inline__ void clPMAlarmAttrTableHashDel(ClPMAlmTableAttrDefT* pAttrInfo)
{
    hashDel(&pAttrInfo->hash);
}

ClRcT clPMAlarmAttrInfoGet(struct hashStruct **pPMAlarmAttrTable, ClCorAttrIdT attrId, ClPMAlmTableAttrDefT** ppAttrInfo)
{
    ClUint32T key = 0;
    register struct hashStruct* pTemp = NULL;

    key = clPMAlarmAttrTableHash(attrId);

    for (pTemp = pPMAlarmAttrTable[key]; pTemp; pTemp = pTemp->pNext)
    {
        ClPMAlmTableAttrDefT* pTempAttr = hashEntry(pTemp, ClPMAlmTableAttrDefT, hash);
        if (pTempAttr->attrId == attrId)
        {
            *ppAttrInfo = pTempAttr;
            return CL_OK;
        }
    }

    return CL_PM_RC(CL_ERR_NOT_EXIST);
}
/**************************************************************************/

ClRcT clPMAlarmClassInfoAdd(ClCorClassTypeT classId, ClCorAttrIdT pmResetAttrId)
{
    ClRcT rc = CL_OK;
    ClPMAlmTableClassDefT* pClassInfo = NULL; 

    rc = clPMAlarmClassInfoGet(classId, &pClassInfo);
    if (rc != CL_OK)
    {
        pClassInfo = clHeapAllocate(sizeof(ClPMAlmTableClassDefT));
        if (!pClassInfo)
        {
            clLogError("PM", "ALMTABLE", "Failed to allocate memory.");
            return CL_PM_RC(CL_ERR_NO_MEMORY);
        }

        pClassInfo->classId = classId;
        pClassInfo->pmResetAttrId = pmResetAttrId;

        rc = clPMAlarmClassTableHashAdd(pClassInfo);
    }

    return rc; 
}

ClRcT clPMAlarmAttrInfoAdd(ClCorClassTypeT classId, ClPMAlmTableAttrDefT* pAttrInfo)
{
    ClRcT rc = CL_OK;
    ClPMAlmTableClassDefT* pClassInfo = NULL; 

    rc = clPMAlarmClassInfoGet(classId, &pClassInfo);
    if (rc != CL_OK)
    {
        clLogError("PM", "ALMTABLE", "Failed to get Alarm Class Info for classId [%d].", classId);
        return rc;
    }

    rc = clPMAlarmAttrTableHashAdd(pClassInfo->pPMAlarmAttrTable, pAttrInfo);

    return rc;
}

ClRcT clPMAlarmInfoGet(ClCorClassTypeT classId, ClCorAttrIdT attrId, ClPMAlmTableAttrDefT** ppAttrInfo)
{
    ClRcT rc = CL_OK;
    ClPMAlmTableClassDefT* pClassInfo = NULL;

    rc = clPMAlarmClassInfoGet(classId, &pClassInfo);
    if (rc != CL_OK)
    {
        return rc;
    }

    rc = clPMAlarmAttrInfoGet(pClassInfo->pPMAlarmAttrTable, attrId, ppAttrInfo);
    if (rc != CL_OK)
    {
        return rc;
    }

    return CL_OK;
}

void clPMAlarmInfoFinalize()
{
    ClUint32T i = 0;
    ClUint32T j = 0;
    struct hashStruct* pTempClass = NULL;
    struct hashStruct* pTempAttr = NULL;
    ClPMAlmTableClassDefT* pClassInfo = NULL;
    ClPMAlmTableAttrDefT* pAttrInfo = NULL;

    for (i = 0; i < CL_PM_ALMCLASSTABLE_BUCKETS; i++)
    {
        struct hashStruct *classNext = NULL;

        for (pTempClass = gpPMAlarmClassTable[i]; pTempClass; pTempClass = classNext)
        {
            classNext = pTempClass->pNext;

            pClassInfo = hashEntry(pTempClass, ClPMAlmTableClassDefT ,hash);

            /* Remove the attr info */
            for(j = 0 ; j < CL_PM_ALMATTRTABLE_BUCKETS; j++)
            {
                struct hashStruct *attrNext = NULL;

                for(pTempAttr = pClassInfo->pPMAlarmAttrTable[j]; pTempAttr; pTempAttr = attrNext)
                {
                    attrNext = pTempAttr->pNext;
                    pAttrInfo = hashEntry(pTempAttr, ClPMAlmTableAttrDefT, hash);
                    clPMAlarmAttrTableHashDel(pAttrInfo);
                    clHeapFree(pAttrInfo);
                }
            }

            clPMAlarmClassTableHashDel(pClassInfo);
            clHeapFree(pClassInfo);
        }
    }

    memset(gpPMAlarmClassTable, 0, sizeof(gpPMAlarmClassTable));

    return;
}

ClRcT clPMLibInitialize()
{
    ClRcT rc = CL_OK;
    ClTimerTimeOutT timerInterval = {0, 0};
    ClCorAddrT compAddr = {0};
    ClOampRtResourceArrayT resourcesArray = {0};
    ClUint32T i = 0;
    ClCorMOIdT moId = {{{0}}};
    ClCorMOClassPathT moPath = {{0}};

    /* Create the linked list to store the moIds for the PM operation.
     */
    rc = clOsalMutexInit(&gClPMMutex);
    if(rc != CL_OK)
    {
        clLogError("PM", "INI", "PM mutex initialize failed with [%#x]", rc);
        return rc;
    }

    rc = clClistCreate(0, CL_NO_DROP, clPMMoIdListDeleteCB, clPMMoIdListDeleteCB, &gClPMMoIdList);
    if (rc != CL_OK)
    {
        clLogError("PM", "INI", "Failed to create the PM MoId List. rc [0x%x]", rc);
        return rc;
    }

    rc = clMsoConfigRegister(CL_COR_SVC_ID_PM_MANAGEMENT, gClPMClientMsoCallbacks);
    if (rc != CL_OK)
    {
        clLogError("PM", "INI", "Failed to register callbacks with mso library. rc [0x%x]", rc);
        return rc;
    }

    /* Go through the resource list and register with COR. */
    rc = clMsoConfigResourceInfoGet(&compAddr, &resourcesArray);
    if (rc != CL_OK)
    {
        clLogError("PM", "INI", "Failed to get the resource information. rc [0x%x]", rc);
        return rc;
    }

    /* Allocate memory for PM resource list. */
    pPMConfigMoIdList = clHeapAllocate(sizeof(ClCorMOIdListT) + 
                            resourcesArray.noOfResources * sizeof(ClCorMOIdT));
    if (!pPMConfigMoIdList)
    {
        clLogError("PM", "INI", "Failed to allocate memory.");
        if (resourcesArray.noOfResources != 0)
            clHeapFree(resourcesArray.pResources);
        return CL_PM_RC(CL_ERR_NO_MEMORY);
    }

    memset(pPMConfigMoIdList, 0, sizeof(ClCorMOIdListT) + 
                                    resourcesArray.noOfResources * sizeof(ClCorMOIdT));

    for (i=0; i<resourcesArray.noOfResources; i++)
    {
        rc = clCorMoIdNameToMoIdGet(&resourcesArray.pResources[i].resourceName, &moId);
        if (rc != CL_OK)
        {
            clLogError("PM", "INI", "Failed to get moId from moId name. rc [0x%x]", rc);
            goto exit;
        }

        rc = clCorMoIdToMoClassPathGet(&moId, &moPath);
        if (rc != CL_OK)
        {
            clLogError("PM", "INI", "Failed to get Mo class path from MoId. rc [0x%x]", rc);
            goto exit;
        }

        rc = clCorMSOClassExist(&moPath, CL_COR_SVC_ID_PM_MANAGEMENT);
        if (rc != CL_OK)
        {
            /* PM is not configured for this resource.
             * Continue with the next one.
             */
            clLogDebug("PM", "INI", "PM is not configured for the resource : [%s], "
                    "continue with the next one..", 
                    resourcesArray.pResources[i].resourceName.value);
            rc = CL_OK;
            continue;
        }

        rc = clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_PM_MANAGEMENT);
        if (rc != CL_OK)
        {
            clLogError("PM", "INI", "Failed to set the service id. rc [0x%x]", rc);
            goto exit;
        }

        memcpy(&pPMConfigMoIdList->moId[pPMConfigMoIdList->moIdCnt], &moId, sizeof(ClCorMOIdT));
        pPMConfigMoIdList->moIdCnt++;

        rc = clCorOIRegister(&moId, &compAddr);
        if (rc != CL_OK)
        {
            clLogError("PM", "INI", "Failed to register as OI for the resource [%s]. rc [0x%x]",
                            resourcesArray.pResources[i].resourceName.value, rc);
            goto exit;
        }

        if (resourcesArray.pResources[i].primaryOIFlag == CL_TRUE)
        {
            rc = clCorPrimaryOISet(&moId, &compAddr);
            if (rc != CL_OK)
            {
                clLogError("PM", "INI", "Failed to set the primary OI for a moId. rc [0x%x]", rc);
                goto exit;
            }
        }
    }

    /*
     * Create PM -> alarm mapping table.
     */
    rc = clPMAlarmTableCreate();
    if (rc != CL_OK)
    {
        clLogError("PM", "INI", "Failed to create PM alarm mapping table. rc [0x%x]", rc);
        goto exit;
    }
    
    timerInterval.tsSec = 0;
    timerInterval.tsMilliSec = gClPMTimerInt;

    /* 
     * Create the PM engine timer to collect the data from POI and update it in COR/raise alarms.
     */
    rc = clTimerCreate(timerInterval, CL_TIMER_REPETITIVE, CL_TIMER_SEPARATE_CONTEXT, 
            _clPMTimerCallback, NULL, &gClPMTimerHandle);
    if (rc != CL_OK)
    {
        clLogError("PM", "INI", "Failed to create PM engine timer. rc [0x%x]", rc);
        goto exit;
    }

    rc = clTimerStart(gClPMTimerHandle);
    if (rc != CL_OK)
    {
        clLogError("PM", "INI", "Failed to start the PM engine timer. rc [0x%x]", rc);
        goto exit;
    }

exit:
    if (resourcesArray.noOfResources != 0)
        clHeapFree(resourcesArray.pResources);

    if (rc != CL_OK)
        clHeapFree(pPMConfigMoIdList);

    return rc;
}

static ClInt32T pmAlarmAttrDefCompare(const void *attrDef1, const void *attrDef2)
{
    return ((ClPMAlmAttrDefT*)attrDef1)->severity - ((ClPMAlmAttrDefT*)attrDef2)->severity;
}

ClRcT clPMAlarmTableCreate()
{
    ClRcT rc = CL_OK;
    ClCharT* configPath = NULL;
    ClParserPtrT top = NULL;
    ClCorClassTypeT classId = 0;
    ClCorAttrIdT pmResetAttrId = 0;
    ClInt64T lowerBound = 0;
    ClInt64T upperBound = 0;
    ClAlarmProbableCauseT probableCause = 0;
    ClAlarmSpecificProblemT specificProblem = 0;
    ClAlarmSeverityTypeT severity = 0;
    ClParserPtrT pmClassInfo = NULL;
    ClParserPtrT pmConfigInterval = NULL;
    ClCharT* pTemp = NULL;
    ClParserPtrT pmAttrInfo = NULL;
    ClParserPtrT pmAlarmInfo = NULL;
    ClParserPtrT compInst = NULL;
    ClNameT compName = {0};
    const ClCharT *pConfigFile = CL_PM_CONFIG_FILE;

    configPath = getenv("ASP_CONFIG");
    if (!configPath)
    {
        clLogError("PM", "ALARMTABLE", "Failed to get value of env variable ASP_CONFIG.");
        return CL_PM_RC(CL_ERR_NULL_POINTER);
    }

    if(gClOIConfig.oiDBReload)
    {
        if(!gClOIConfig.pOIPMFile)
        {
            clLogWarning("PM", "ALARMTABLE", "OI has db reload and PM enabled but has not "
                         "specified the PM config file. Continuing without PM enabled");
            return CL_OK;
        }
        pConfigFile = gClOIConfig.pOIPMFile;
        if(gClOIConfig.pOIPMPath)
            configPath = (ClCharT*)gClOIConfig.pOIPMPath;
    }
    top = clParserOpenFile(configPath, pConfigFile);
    if (!top)
    {
        clLogError("PM", "ALARMTABLE", "Failed to open the PM Configuration file [%s] at path [%s]",
                   pConfigFile, configPath);
        return CL_PM_RC(CL_ERR_NULL_POINTER);
    }

    rc = clCpmComponentNameGet(0, &compName);
    if (rc != CL_OK)
    {
        clLogError("PM", "ALMTABLE", "Failed to get component name. rc [0x%x]", rc);
        clParserFree(top);
        return rc; 
    }

    pmConfigInterval = clParserChild(top, "pmConfigIntervalT");
    if (!pmConfigInterval)
    {
        clLogError("PM", "ALMTABLE", "Failed to parse pmConfigT tag.");
        clParserFree(top);
        return CL_PM_RC(CL_ERR_NULL_POINTER);
    }

    compInst = clParserChild(pmConfigInterval, "compInstT");
    for (; compInst; compInst = compInst->next)
    {
        pTemp = (ClCharT *) clParserAttr(compInst, "compName");
        if (!pTemp)
        {
            clLogError("PM", "ALMTABLE", "Failed to parse compName attribute from compInstT tag.");
            clParserFree(top);
            return CL_PM_RC(CL_ERR_NULL_POINTER);
        }

        if (!strncmp(pTemp, compName.value, compName.length))
        {
            pTemp = (ClCharT *) clParserAttr(compInst, "interval");
            if (!pTemp)
            {
                clLogError("PM", "ALMTABLE", "Failed to parse interval attribute from compInstT tag.");
                clParserFree(top);
                return CL_PM_RC(CL_ERR_NULL_POINTER);
            }

            gClPMTimerInt = (ClUint32T) pmAtoI(pTemp);
            break;
        }
    }

    clLogTrace("PM", "ALMTABLE", "PM configuration interval : [%u]", gClPMTimerInt);

    pmClassInfo = clParserChild(top, "pmClassInfoT");
    for (; pmClassInfo; pmClassInfo = pmClassInfo->next)
    {
        ClPMAlmTableAttrDefT* pAttrDef = NULL;

        pTemp = (ClCharT *) clParserAttr(pmClassInfo, "classId");
        if (!pTemp)
        {
            clLogError("PM", "ALARMTABLE", "Failed to parse classId attribute from the config file.");
            clParserFree(top);
            return CL_PM_RC(CL_ERR_NULL_POINTER);
        }

        classId = (ClCorClassTypeT) pmAtoI(pTemp);

        pTemp = (ClCharT *) clParserAttr(pmClassInfo, "pmResetAttrId");
        if (!pTemp)
        {
            clLogError("PM", "ALARMTABLE", "Failed to parse pmResetAttrId attribute from the config file.");
            clParserFree(top);
            return CL_PM_RC(CL_ERR_NULL_POINTER);
        }

        pmResetAttrId = (ClCorAttrIdT) pmAtoI(pTemp); 

        rc = clPMAlarmClassInfoAdd(classId, pmResetAttrId);
        if (rc != CL_OK)
        {
            clLogError("PM", "ALARMTABLE", "Failed to add PM class info configurations. rc [0x%x].", rc);
            clParserFree(top);
            return rc; 
        }

        pmAttrInfo = clParserChild(pmClassInfo, "pmAttrInfoT");
        for (; pmAttrInfo; pmAttrInfo = pmAttrInfo->next)
        {
            /* Add the values into PM Alarm Table */
            pAttrDef = clHeapCalloc(1, sizeof(ClPMAlmTableAttrDefT));
            if (!pAttrDef)
            {
                clLogError("PM", "ALARMTABLE", "Failed to allocate memory.");
                clParserFree(top);
                return CL_PM_RC(CL_ERR_NULL_POINTER);
            }

            pTemp = (ClCharT *) clParserAttr(pmAttrInfo, "attrId");
            if (!pTemp)
            {
                clLogError("PM", "ALARMTABLE", "Failed get parse attrId attribute from the config file.");
                clParserFree(top);
                return CL_PM_RC(CL_ERR_NULL_POINTER);
            }

            pAttrDef->attrId = pmAtoI(pTemp);
            
            pmAlarmInfo = clParserChild(pmAttrInfo, "pmAlarmInfoT");
            for (; pmAlarmInfo; pmAlarmInfo = pmAlarmInfo->next)
            {
                ClPMAlmAttrDefT tempAttrDef = {0};

                pTemp = (ClCharT *) clParserAttr(pmAlarmInfo, "probableCause");
                if (!pTemp)
                {
                    clLogError("PM", "ALARMTABLE", "Failed parse probableCause attribute from the config file.");
                    clParserFree(top);
                    return CL_PM_RC(CL_ERR_NULL_POINTER);
                }

                probableCause = pmAlarmProbCauseGet(pTemp);

                pTemp = (ClCharT *) clParserAttr(pmAlarmInfo, "specificProblem");
                if (!pTemp)
                {
                    clLogError("PM", "ALARMTABLE", "Failed parse specificProblem attribute from the config file.");
                    clParserFree(top);
                    return CL_PM_RC(CL_ERR_NULL_POINTER);
                }

                specificProblem = pmAtoI(pTemp);

                pTemp = (ClCharT *) clParserAttr(pmAlarmInfo, "severity");
                if (!pTemp)
                {
                    clLogError("PM", "ALARMTABLE", "Failed parse severity attribute from the config file.");
                    clParserFree(top);
                    return CL_PM_RC(CL_ERR_NULL_POINTER);
                }

                severity = pmAlarmSeverityGet(pTemp);

                pTemp = (ClCharT*) clParserAttr(pmAlarmInfo, "thresholdValue");
                if(!pTemp)
                {
                    clLogWarning("PM", "ALARMTABLE", 
                                 "thresholdValue tag not found. Checking for lowerBound/upperBound tags");
                    pTemp = (ClCharT *) clParserAttr(pmAlarmInfo, "lowerBound");
                    if (!pTemp)
                    {
                        clLogError("PM", "ALARMTABLE", "Failed parse lowerBound attribute from the config file.");
                        clParserFree(top);
                        return CL_PM_RC(CL_ERR_NULL_POINTER);
                    }

                    lowerBound = pmAtoI(pTemp);

                    pTemp = (ClCharT *) clParserAttr(pmAlarmInfo, "upperBound");
                    if (!pTemp)
                    {
                        clLogError("PM", "ALARMTABLE", "Failed parse upperBound attribute from the config file.");
                        clParserFree(top);
                        return CL_PM_RC(CL_ERR_NULL_POINTER);
                    }

                    upperBound = pmAtoI(pTemp);
                }
                else
                {
                    ClInt64T thresholdValue = 0;
                    thresholdValue = pmAtoI(pTemp);
                    lowerBound = thresholdValue - 1;
                    upperBound = ~0U; /* 32 bit max*/
                }
                tempAttrDef.probableCause = probableCause;
                tempAttrDef.specificProblem = specificProblem;
                tempAttrDef.severity = severity;
                tempAttrDef.lowerBound = lowerBound;
                tempAttrDef.upperBound = upperBound;

                /* Realloc attribute definitions */
                if (! (pAttrDef->alarmCount & 3))
                {
                    pAttrDef->pAlarmAttrDef = clHeapRealloc(pAttrDef->pAlarmAttrDef, 
                                                            (pAttrDef->alarmCount + 4) * sizeof(ClPMAlmAttrDefT));
                    if (! pAttrDef->pAlarmAttrDef)
                    {
                        clLogError("PM", "ALARMTABLE", "Failed to allocate memory.");
                        clParserFree(top);
                        return CL_PM_RC(CL_ERR_NO_MEMORY);
                    }

                    memset(pAttrDef->pAlarmAttrDef + pAttrDef->alarmCount, 0, 
                           (4 * sizeof(ClPMAlmAttrDefT)));
                }

                memcpy(pAttrDef->pAlarmAttrDef + pAttrDef->alarmCount, &tempAttrDef, sizeof(ClPMAlmAttrDefT));
                pAttrDef->alarmCount++;
            }
            /*
             * Sort in decreasing order of severity.
             */
            if(pAttrDef->pAlarmAttrDef)
                qsort(pAttrDef->pAlarmAttrDef, pAttrDef->alarmCount, sizeof(*pAttrDef->pAlarmAttrDef),
                      pmAlarmAttrDefCompare);
            rc = clPMAlarmAttrInfoAdd(classId, pAttrDef);
            if (rc != CL_OK)
            {
                clLogError("PM", "ALARMTABLE", "Failed to add to PM attr info to PM Alarm Table. rc [0x%x]", rc);
                clHeapFree(pAttrDef);
                clParserFree(top);
                return rc;
            }
        }
    }

    clParserFree(top);
    return CL_OK;
}

ClInt64T pmAtoI(ClCharT* pTemp)
{
    ClInt64T val = 0;

    if (!pTemp)
    {
        clLogError("PM", "UTIL", "NULL pointer passed.");
        return 0;
    }

    sscanf(pTemp, "%lld", &val);

    return val;
}

ClAlarmSeverityTypeT pmAlarmSeverityGet(ClCharT* severity)
{
    ClAlarmSeverityTypeT value = -1;

    if (!severity)
    {
        clLogError("PM", "UTILS", "NULL pointer passed.");
        return -1;
    }

    if (!strcmp(severity, "CL_ALARM_SEVERITY_CRITICAL"))
        value = CL_ALARM_SEVERITY_CRITICAL;
    else if (!strcmp(severity, "CL_ALARM_SEVERITY_MAJOR"))
        value = CL_ALARM_SEVERITY_MAJOR;
    else if (!strcmp(severity, "CL_ALARM_SEVERITY_MINOR"))
        value = CL_ALARM_SEVERITY_MINOR;
    else if (!strcmp(severity, "CL_ALARM_SEVERITY_WARNING"))
        value = CL_ALARM_SEVERITY_WARNING;
    else if (!strcmp(severity, "CL_ALARM_SEVERITY_INDETERMINATE"))
        value = CL_ALARM_SEVERITY_INDETERMINATE;
    else if (!strcmp(severity, "CL_ALARM_SEVERITY_CLEAR"))
        value = CL_ALARM_SEVERITY_CLEAR;

    return value;
}   

ClAlarmProbableCauseT pmAlarmProbCauseGet(ClCharT* probCause)
{
    ClAlarmProbableCauseT value = -1;

    if (!probCause)
    {
        clLogError("PM", "UTILS", "NULL pointer passed.");
        return -1;
    }

    if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_LOSS_OF_SIGNAL"))
        value = CL_ALARM_PROB_CAUSE_LOSS_OF_SIGNAL;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_LOSS_OF_FRAME"))
        value = CL_ALARM_PROB_CAUSE_LOSS_OF_FRAME;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_FRAMING_ERROR"))
        value = CL_ALARM_PROB_CAUSE_FRAMING_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_LOCAL_NODE_TRANSMISSION_ERROR"))
        value = CL_ALARM_PROB_CAUSE_LOCAL_NODE_TRANSMISSION_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_REMOTE_NODE_TRANSMISSION_ERROR"))
        value = CL_ALARM_PROB_CAUSE_REMOTE_NODE_TRANSMISSION_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_CALL_ESTABLISHMENT_ERROR"))
        value = CL_ALARM_PROB_CAUSE_CALL_ESTABLISHMENT_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_DEGRADED_SIGNAL"))
        value = CL_ALARM_PROB_CAUSE_DEGRADED_SIGNAL;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_COMMUNICATIONS_SUBSYSTEM_FAILURE"))
        value = CL_ALARM_PROB_CAUSE_COMMUNICATIONS_SUBSYSTEM_FAILURE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_COMMUNICATIONS_PROTOCOL_ERROR"))
        value = CL_ALARM_PROB_CAUSE_COMMUNICATIONS_PROTOCOL_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_LAN_ERROR"))
        value = CL_ALARM_PROB_CAUSE_LAN_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_DTE"))
        value = CL_ALARM_PROB_CAUSE_DTE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_RESPONSE_TIME_EXCESSIVE"))
        value = CL_ALARM_PROB_CAUSE_RESPONSE_TIME_EXCESSIVE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_QUEUE_SIZE_EXCEEDED"))
        value = CL_ALARM_PROB_CAUSE_QUEUE_SIZE_EXCEEDED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_BANDWIDTH_REDUCED"))
        value = CL_ALARM_PROB_CAUSE_BANDWIDTH_REDUCED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_RETRANSMISSION_RATE_EXCESSIVE"))
        value = CL_ALARM_PROB_CAUSE_RETRANSMISSION_RATE_EXCESSIVE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_THRESHOLD_CROSSED"))
        value = CL_ALARM_PROB_CAUSE_THRESHOLD_CROSSED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_PERFORMANCE_DEGRADED"))
        value = CL_ALARM_PROB_CAUSE_PERFORMANCE_DEGRADED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_CONGESTION"))
        value = CL_ALARM_PROB_CAUSE_CONGESTION;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_RESOURCE_AT_OR_NEARING_CAPACITY"))
        value = CL_ALARM_PROB_CAUSE_RESOURCE_AT_OR_NEARING_CAPACITY;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_STORAGE_CAPACITY_PROBLEM"))
        value = CL_ALARM_PROB_CAUSE_STORAGE_CAPACITY_PROBLEM;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_VERSION_MISMATCH"))
        value = CL_ALARM_PROB_CAUSE_VERSION_MISMATCH;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_CORRUPT_DATA"))
        value = CL_ALARM_PROB_CAUSE_CORRUPT_DATA;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_CPU_CYCLES_LIMIT_EXCEEDED"))
        value = CL_ALARM_PROB_CAUSE_CPU_CYCLES_LIMIT_EXCEEDED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_SOFWARE_ERROR"))
        value = CL_ALARM_PROB_CAUSE_SOFWARE_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_SOFTWARE_PROGRAM_ERROR"))
        value = CL_ALARM_PROB_CAUSE_SOFTWARE_PROGRAM_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_SOFWARE_PROGRAM_ABNORMALLY_TERMINATED"))
        value = CL_ALARM_PROB_CAUSE_SOFWARE_PROGRAM_ABNORMALLY_TERMINATED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_FILE_ERROR"))
        value = CL_ALARM_PROB_CAUSE_FILE_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_OUT_OF_MEMORY"))
        value = CL_ALARM_PROB_CAUSE_OUT_OF_MEMORY;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE"))
        value = CL_ALARM_PROB_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_APPLICATION_SUBSYSTEM_FAILURE"))
        value = CL_ALARM_PROB_CAUSE_APPLICATION_SUBSYSTEM_FAILURE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_CONFIGURATION_OR_CUSTOMIZATION_ERROR"))
        value = CL_ALARM_PROB_CAUSE_CONFIGURATION_OR_CUSTOMIZATION_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_POWER_PROBLEM"))
        value = CL_ALARM_PROB_CAUSE_POWER_PROBLEM;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_TIMING_PROBLEM"))
        value = CL_ALARM_PROB_CAUSE_TIMING_PROBLEM;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_PROCESSOR_PROBLEM"))
        value = CL_ALARM_PROB_CAUSE_PROCESSOR_PROBLEM;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_DATASET_OR_MODEM_ERROR"))
        value = CL_ALARM_PROB_CAUSE_DATASET_OR_MODEM_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_MULTIPLEXER_PROBLEM"))
        value = CL_ALARM_PROB_CAUSE_MULTIPLEXER_PROBLEM;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_RECEIVER_FAILURE"))
        value = CL_ALARM_PROB_CAUSE_RECEIVER_FAILURE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_TRANSMITTER_FAILURE"))
        value = CL_ALARM_PROB_CAUSE_TRANSMITTER_FAILURE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_RECEIVE_FAILURE"))
        value = CL_ALARM_PROB_CAUSE_RECEIVE_FAILURE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_TRANSMIT_FAILURE"))
        value = CL_ALARM_PROB_CAUSE_TRANSMIT_FAILURE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_OUTPUT_DEVICE_ERROR"))
        value =  CL_ALARM_PROB_CAUSE_OUTPUT_DEVICE_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_INPUT_DEVICE_ERROR"))
        value = CL_ALARM_PROB_CAUSE_INPUT_DEVICE_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_INPUT_OUTPUT_DEVICE_ERROR"))
        value = CL_ALARM_PROB_CAUSE_INPUT_OUTPUT_DEVICE_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_EQUIPMENT_MALFUNCTION"))
        value = CL_ALARM_PROB_CAUSE_EQUIPMENT_MALFUNCTION;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_ADAPTER_ERROR"))
        value = CL_ALARM_PROB_CAUSE_ADAPTER_ERROR;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_TEMPERATURE_UNACCEPTABLE"))
        value = CL_ALARM_PROB_CAUSE_TEMPERATURE_UNACCEPTABLE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_HUMIDITY_UNACCEPTABLE"))
        value = CL_ALARM_PROB_CAUSE_HUMIDITY_UNACCEPTABLE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM"))
        value = CL_ALARM_PROB_CAUSE_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_FIRE_DETECTED"))
        value = CL_ALARM_PROB_CAUSE_FIRE_DETECTED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_FLOOD_DETECTED"))
        value = CL_ALARM_PROB_CAUSE_FLOOD_DETECTED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_TOXIC_LEAK_DETECTED"))
        value = CL_ALARM_PROB_CAUSE_TOXIC_LEAK_DETECTED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_LEAK_DETECTED"))
        value = CL_ALARM_PROB_CAUSE_LEAK_DETECTED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_PRESSURE_UNACCEPTABLE"))
        value = CL_ALARM_PROB_CAUSE_PRESSURE_UNACCEPTABLE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_EXCESSIVE_VIBRATION"))
        value = CL_ALARM_PROB_CAUSE_EXCESSIVE_VIBRATION;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_MATERIAL_SUPPLY_EXHAUSTED"))
        value = CL_ALARM_PROB_CAUSE_MATERIAL_SUPPLY_EXHAUSTED;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_PUMP_FAILURE"))
        value = CL_ALARM_PROB_CAUSE_PUMP_FAILURE;
    else if (!strcmp(probCause, "CL_ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN"))
        value = CL_ALARM_PROB_CAUSE_ENCLOSURE_DOOR_OPEN;

    return value;
}

ClRcT pmCorTypeGet(ClCharT* dataTypeName, ClCorTypeT* dataTypeId)
{
    ClRcT rc = CL_OK;

    if (!dataTypeName || !dataTypeId)
    {
        clLogError("PM", "UTILS", "NULL pointer passed.");
        return CL_PM_RC(CL_ERR_NULL_POINTER);
    }

    if (strcmp(dataTypeName, "CL_COR_INVALID_DATA_TYPE") == 0)
        *dataTypeId = CL_COR_INVALID_DATA_TYPE;
    else if (strcmp(dataTypeName, "CL_COR_VOID") == 0)
        *dataTypeId = CL_COR_VOID;
    else if (strcmp(dataTypeName, "CL_COR_INT8") == 0)
        *dataTypeId = CL_COR_INT8;
    else if (strcmp(dataTypeName, "CL_COR_UINT8") == 0)
        *dataTypeId = CL_COR_UINT8;
    else if (strcmp(dataTypeName, "CL_COR_INT16") == 0)
        *dataTypeId = CL_COR_INT16;
    else if (strcmp(dataTypeName, "CL_COR_UINT16") == 0)
        *dataTypeId = CL_COR_UINT16;
    else if (strcmp(dataTypeName, "CL_COR_INT32") == 0)
        *dataTypeId = CL_COR_INT32;
    else if (strcmp(dataTypeName, "CL_COR_UINT32") == 0)
        *dataTypeId = CL_COR_UINT32;
    else if (strcmp(dataTypeName, "CL_COR_INT64") == 0)
        *dataTypeId = CL_COR_INT64;
    else if (strcmp(dataTypeName, "CL_COR_UINT64") == 0)
        *dataTypeId = CL_COR_UINT64;
    else if (strcmp(dataTypeName, "CL_COR_FLOAT") == 0)
        *dataTypeId = CL_COR_FLOAT;
    else if (strcmp(dataTypeName, "CL_COR_DOUBLE") == 0)
        *dataTypeId = CL_COR_DOUBLE;
    else if (strcmp(dataTypeName, "CL_COR_COUNTER32") == 0)
        *dataTypeId = CL_COR_COUNTER32;
    else if (strcmp(dataTypeName, "CL_COR_COUNTER64") == 0)
        *dataTypeId = CL_COR_COUNTER64;
    else if (strcmp(dataTypeName, "CL_COR_SEQUENCE32") == 0)
        *dataTypeId = CL_COR_SEQUENCE32;
    else 
        rc = CL_COR_SET_RC(CL_COR_ERR_NOT_SUPPORTED);

    return rc;
}


ClRcT _clPMTimerCallback(ClPtrT arg)
{
    ClRcT rc = CL_OK;
    ClCorTxnSessionIdT corTxnId = 0;

    /* Traverse through the PM List and invoke the POI callback function. */
    clOsalMutexLock(&gClPMMutex);
    rc = clClistWalk(gClPMMoIdList, _clPMMoIdListWalkCB, &corTxnId);
    clOsalMutexUnlock(&gClPMMutex);
    if (rc != CL_OK)
    {
        clLogError("PM", "TIMER", "Failed to walk the PM MoId list. rc [0x%x]", rc);
        clCorTxnSessionFinalize(corTxnId);
        return rc;
    }

    if (corTxnId)
    {
        rc = clCorTxnSessionCommit(corTxnId);
        if (rc != CL_OK)
        {
            clLogError("PM", "TIMER", "Failed to commit the transaction. rc [0x%x]", rc);
            clCorTxnSessionFinalize(corTxnId);
            return rc;
        }
    }

    return CL_OK; 
}

ClRcT clPMLibFinalize()
{
    ClRcT rc = CL_OK;

    rc = clTimerDelete(&gClPMTimerHandle);
    if (rc != CL_OK)
    {
        clLogError("PM", "FINALIZE", "Failed to stop the PM engine timer. rc [0x%x]", rc);
        return rc;
    }

    clOsalMutexLock(&gClPMMutex);
    rc = clClistDelete(&gClPMMoIdList);

    if (rc != CL_OK)
    {
        clOsalMutexUnlock(&gClPMMutex);
        clLogError("PM", "FINALIZE", "Failed to delete the PM MoId List. rc [0x%x]", rc);
        return rc;
    }

    clPMAlarmInfoFinalize();
    clOsalMutexUnlock(&gClPMMutex);

    clHeapFree(pPMConfigMoIdList);

    return CL_OK;
}

ClRcT
clPMTxnPrepare ( CL_IN ClTxnTransactionHandleT txnHandle,
                   CL_IN       ClTxnJobDefnHandleT     jobDefn,
                   CL_IN       ClUint32T               jobDefnSize,
                   CL_IN       ClCorTxnIdT             corTxnId) 
{
    /* No need to validate the values.
     * It is used only to reset the PM attribtues values.
     */
    return CL_OK;
}

ClRcT
clPMTxnRollback ( CL_IN ClTxnTransactionHandleT txnHandle,
                   CL_IN       ClTxnJobDefnHandleT     jobDefn,
                   CL_IN       ClUint32T               jobDefnSize,
                   CL_IN       ClCorTxnIdT             corTxnId) 
{
    /* No need to rollback as it is assumed that _Prepare() 
     * will always succeed.
     */
    return CL_OK;
}

ClRcT
clPMTxnUpdate ( CL_IN ClTxnTransactionHandleT txnHandle,
                   CL_IN       ClTxnJobDefnHandleT     jobDefn,
                   CL_IN       ClUint32T               jobDefnSize,
                   CL_IN       ClCorTxnIdT             corTxnId) 
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId = {{{0}}};
    ClPMObjectDataT pmObjectData = {0};
    ClPMOpT pmOp = CL_PM_RESET;
    ClCorOpsT corOp = 0;
    ClUint32T attrIndex = 0;
    ClCorTxnJobIdT jobId = 0;
    ClPtrT jobWalkArg[] = {
                        (void *) &pmObjectData,
                        (void *) &pmOp,
                        (void *) &moId
    };
    ClPtrT arg[] = { 
                        (void *) &pmObjectData,
                        (void *) &rc,
                        (void *) &attrIndex
    };

    /*
     * Traverse through the jobs and call POI callback function.
     */
    rc = clCorTxnFirstJobGet(corTxnId, &jobId);
    if (rc != CL_OK)
    {
        clLogError("PM", "TXNUPDATE", "Failed to get the first cor job from txn job. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorTxnJobOperationGet(corTxnId, jobId, &corOp);
    if (rc != CL_OK)
    {
        clLogError("PM", "TXNUPDATE", "Failed to get job operation. rc [0x%x]", rc);
        return rc;
    }

    /* Process only the SET job. */
    if (corOp == CL_COR_OP_SET)
    {
        rc = clCorTxnJobMoIdGet(corTxnId, &moId);
        if (rc != CL_OK)
        {
            clLogError("PM", "TXNUPDATE", "Failed to get the moId from corTxnId. rc [0x%x]", rc);
            return rc;
        }

        pmObjectData.pMoId = &moId;

        rc = clCorTxnJobWalk(corTxnId, clPMTxnJobWalk, (void *) jobWalkArg);
        if (rc != CL_OK && rc != CL_COR_SET_RC(CL_COR_TXN_ERR_JOB_WALK_TERMINATE))
        {
            clLogError("PM", "TXNUPDATE", "Failed to walk the txn job. rc [0x%x]", rc);
            return rc;
        }

        if (rc == CL_COR_SET_RC(CL_COR_TXN_ERR_JOB_WALK_TERMINATE))
        {
            /* No need to call PM Reset callback. */
            clLogTrace("PM", "TXNUPDATE", "Non Reset attribute is SET.");
            clPMObjectDataFree(&pmObjectData);
            return CL_OK;
        }

        /* Invoke the user callback function */
        if (gClPMCallbacks.fpPMObjectReset)
        {
            rc = gClPMCallbacks.fpPMObjectReset(txnHandle, &pmObjectData);
            if (rc != CL_OK)
            {
                clLogError("PM", "TXNUPDATE", "PM Object Reset OI callback failed. rc [0x%x]", rc);
            }
        }
        else
        {
            clLogError("PM", "TXNUPDATE", "PM Object Reset OI callback is not registered.");
            rc = CL_PM_RC(CL_ERR_NULL_POINTER);
        }

        /* Update the values in network format and job status back into COR job */
        rc = clCorTxnJobWalk(corTxnId, clPMTxnJobUpdate, (void *) arg);
        if (rc != CL_OK)
        {
            clLogError("PM", "TXNREAD", "Failed to walk txn job. rc [0x%x]", rc);
            clPMObjectDataFree(&pmObjectData);
            return rc;
        }

        rc = clCorTxnJobDefnHandleUpdate(jobDefn, corTxnId);
        if (rc != CL_OK)
        {
            clLogError("PM", "TXNREAD", "Failed to update txn job definition. rc [0x%x]", rc);
            clPMObjectDataFree(&pmObjectData);
            return rc;
        }

        clPMObjectDataFree(&pmObjectData);
    }

    return CL_OK; 
}

ClRcT
clPMRead ( CL_IN ClTxnTransactionHandleT txnHandle,
           CL_IN       ClTxnJobDefnHandleT     jobDefn,
           CL_IN       ClUint32T               jobDefnSize,
           CL_IN       ClCorTxnIdT             corTxnId) 
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId = {{{0}}};
    ClPMObjectDataT pmObjectData = {0};
    ClUint32T attrIndex = 0;
    ClUint32T i = 0;
    ClCorClassTypeT classId = 0;
    ClPMOpT pmOp = CL_PM_READ;
    ClPtrT arg[] = { 
        (void *) &pmObjectData,
            (void *) &rc,
            (void *) &attrIndex
    };
    ClPtrT jobWalkArg[] = {
        (void *) &pmObjectData,
            (void *) &pmOp,
            (void *) &moId
    };

    /*
     * Traverse through the jobs and call POI callback function.
     */
    rc = clCorTxnJobMoIdGet(corTxnId, &moId);
    if (rc != CL_OK)
    {
        clLogError("PM", "TXNREAD", "Failed to get the moId from corTxnId. rc [0x%x]", rc);
        return rc;
    }

    pmObjectData.pMoId = &moId;

    rc = clCorTxnJobWalk(corTxnId, clPMTxnJobWalk, (void *) jobWalkArg);
    if (rc != CL_OK)
    {
        clLogError("PM", "TXNREAD", "Failed to walk the txn job. rc [0x%x]", rc);
        return rc;
    }

    /* Invoke the user callback function */
    if (gClPMCallbacks.fpPMObjectRead)
    {
        rc = gClPMCallbacks.fpPMObjectRead(txnHandle, &pmObjectData);
        if (rc != CL_OK)
        {
            clLogError("PM", "TXNREAD", "PM Object Read OI callback failed. rc [0x%x]", rc);
            rc = CL_COR_SET_RC(CL_COR_ERR_GET_DATA_NOT_FOUND);
        }
    }
    else
    {
        clLogError("PM", "TXNREAD", "PM Object Read OI callback is not registered.");
        rc = CL_PM_RC(CL_ERR_NULL_POINTER);
    }

    /* Update the values and job status back into COR job */
    rc = clCorTxnJobWalk(corTxnId, clPMTxnJobUpdate, (void *) arg);
    if (rc != CL_OK)
    {
        clLogError("PM", "TXNREAD", "Failed to walk txn job. rc [0x%x]", rc);
        clPMObjectDataFree(&pmObjectData);
        return rc;
    }

    rc = clCorMoIdToClassGet(pmObjectData.pMoId, CL_COR_MSO_CLASS_GET, &classId);
    if (rc != CL_OK)
    {
        clLogError("PM", "TXNREAD", "Failed to get classId from MoId. rc [0x%x]", rc);
        clPMObjectDataFree(&pmObjectData);
        return rc;
    }

    /* Process the attribute jobs and raise/clear alarms. */
    for (i=0; i<pmObjectData.attrCount; i++)
    {
        ClPMAttrDataPtrT pAttrData = NULL;

        pAttrData = (pmObjectData.pAttrData + i);
        CL_ASSERT(pAttrData);

        if(pAttrData->pAlarmData)
        {
            rc = clPMAttrAlarmProcess(pmObjectData.pMoId, classId, pAttrData);
            if (rc != CL_OK)
            {
                clLogError("PM", "TXNREAD", "Failed to process alarms for classId [%d], attrId [%d]. rc [0x%x]", 
                           classId, pAttrData->attrId, rc);
                break;
            }
        }
    }

    rc = clCorTxnJobDefnHandleUpdate(jobDefn, corTxnId);
    if (rc != CL_OK)
    {
        clLogError("PM", "TXNREAD", "Failed to update txn job definition. rc [0x%x]", rc);
        clPMObjectDataFree(&pmObjectData);
        return rc;
    }

    clPMObjectDataFree(&pmObjectData);
    return CL_OK; 
}

ClRcT clPMTxnJobUpdate(ClCorTxnIdT corTxnId, ClCorTxnJobIdT jobId, void* arg)
{
    ClRcT rc = CL_OK;
    ClPMObjectDataT* pmObjectData = NULL;
    ClUint32T index = 0;
    ClRcT jobStatus = CL_OK;

    if (! arg)
    {
        clLogError("PM", "JOBUPDATE", "NULL pointer passed.");
        return CL_PM_RC(CL_ERR_NULL_POINTER);
    }
    
    pmObjectData = (ClPMObjectDataT *) ((void **) arg)[0] ;
    jobStatus = *(ClRcT *) ((void **) arg)[1];
    index = *(ClUint32T *) ((void **) arg)[2];

    rc = clCorBundleAttrValueSet(corTxnId, jobId, (pmObjectData->pAttrData + index)->pPMData);
    if (rc != CL_OK)
    {
        clLogError("PM", "JOBUPDATE", "Failed to update value back into cor txn job. rc [0x%x]", rc);
        return rc;
    }

    clCorTxnJobStatusSet(corTxnId, jobId, jobStatus);

    ++index;

    *(ClUint32T *) ((void **) arg)[2] = index;

    return CL_OK;
}

ClRcT clPMTxnJobWalk(ClCorTxnIdT corTxnId, ClCorTxnJobIdT jobId, void* arg)
{
    ClRcT rc = CL_OK;
    ClPMObjectDataT* pmObjectData = NULL;
    ClPMAttrDataPtrT pAttrData = NULL;
    void* pValue = NULL;
    ClPMOpT pmOp = 0;
    ClCorMOIdT* pMoId = NULL;
    ClCorClassTypeT classId = 0;
    ClPMAlmTableClassDefT* pClassInfo = NULL;
    ClPMAlmTableAttrDefPtrT pAlmTableAttrDef = NULL;
    ClUint32T i = 0;

    if (!arg)
    {
        clLogError("PM", "JOBWALK", "NULL pointer passed.");
        return CL_PM_RC(CL_ERR_NULL_POINTER);
    }

    pmObjectData = (ClPMObjectDataT *) ((void **) arg)[0];
    pmOp         = *(ClPMOpT *) ((void **) arg)[1];
    pMoId        = (ClCorMOIdT *) ((void **) arg)[2];

    if (! (pmObjectData->attrCount & 3))
    {
        pmObjectData->pAttrData = clHeapRealloc(pmObjectData->pAttrData, (4 + pmObjectData->attrCount) * 
                                        sizeof(ClPMAttrDataT));
        if (!pmObjectData->pAttrData)
        {
            clLogError("PM", "JOBWALK", "Failed to allocate memory.");
            return CL_PM_RC(CL_ERR_NO_MEMORY);
        }

        memset(pmObjectData->pAttrData + pmObjectData->attrCount, 0, 
                4 * sizeof(ClPMAttrDataT));
    }

    pAttrData = (pmObjectData->pAttrData + pmObjectData->attrCount);

    pmObjectData->attrCount++;

    rc = clCorTxnJobSetParamsGet(corTxnId, jobId, 
            &pAttrData->attrId, &pAttrData->index, &pValue, &pAttrData->size);
    if (rc != CL_OK)
    {
        clLogError("PM", "JOBWALK", "Failed to get the params from txn job. rc [0x%x]", rc);
        return rc;
    }

    pAttrData->pPMData = clHeapAllocate(pAttrData->size);
    if (! pAttrData->pPMData)
    {
        clLogError("PM", "JOBWALK", "Failed to allocate memory.");
        return CL_PM_RC(CL_ERR_NO_MEMORY);
    }

    rc = clCorMoIdToClassGet(pMoId, CL_COR_MSO_CLASS_GET, &classId);
    if (rc != CL_OK)
    {
        clLogError("PM", "TXNUPDATE", "Failed to get classId from moId. rc [0x%x]", rc);
        return rc;
    }

    if (pmOp == CL_PM_RESET)
    {
        /* Check if it is the RESET attribute being SET. */
        rc = clPMAlarmClassInfoGet(classId, &pClassInfo);
        if (rc != CL_OK)
        {
            clLogError("PM", "TXNUPDATE", "Failed to get alarm class info for classId [%d]. rc [0x%x]", 
                    classId, rc);
            return rc;
        }

        if (pAttrData->attrId != pClassInfo->pmResetAttrId)
        {
            clLogTrace("PM", "TXNUPDATE", "PM Attribute is being SET, POI Reset callback should not be called.");
            /* Set the attribute's value back into network format */
            clCorBundleAttrValueSet(corTxnId, jobId, pValue);
            return CL_COR_SET_RC(CL_COR_TXN_ERR_JOB_WALK_TERMINATE);
        }

        /* PM Reset attribute is being SET */
        if (!pValue)
        {
            clLogError("PM", "JOBWALK", "NULL value passed in txn job.");
            return CL_PM_RC(CL_ERR_NULL_POINTER);
        }

        memcpy(pAttrData->pPMData, pValue, pAttrData->size);
    }
    else
    {
        /*
         * Fill up the alarm info to get the payload info.
         */
        rc = clPMAlarmInfoGet(classId, pAttrData->attrId, &pAlmTableAttrDef);
        if (rc != CL_OK)
        {
            clLogError("PM", "JOBWALK", "Failed to get alarm info for the attribute [%d] of class [%d]. "
                    "rc [0x%x]", pAttrData->attrId, classId, rc);
            return rc;
        }

        pAttrData->alarmCount = pAlmTableAttrDef->alarmCount;

        pAttrData->pAlarmData = clHeapAllocate(pAttrData->alarmCount * 
                                        sizeof(ClPMAlarmDataT));
        if (!pAttrData->pAlarmData)
        {
            clLogError("PM", "JOBWALK", "Failed to allocate memory.");
            return CL_PM_RC(CL_ERR_NO_MEMORY);
        }

        for (i=0; i < pAttrData->alarmCount; i++)
        {
            pAttrData->pAlarmData[i].probableCause = pAlmTableAttrDef->pAlarmAttrDef[i].probableCause; 
            pAttrData->pAlarmData[i].specificProblem = pAlmTableAttrDef->pAlarmAttrDef[i].specificProblem; 
            pAttrData->pAlarmData[i].pAlarmPayload = NULL;
        }
    }

    rc = clCorTxnJobAttributeTypeGet(corTxnId, jobId, &pAttrData->attrType, &pAttrData->attrDataType);
    if (rc != CL_OK)
    {
        clLogError("PM", "JOBWALK", "Failed to get attrType and attrDataType from txn job. rc [0x%x]", rc);
        return rc;
    }

    rc = clCorTxnJobAttrPathGet(corTxnId, jobId, &pAttrData->pAttrPath);
    if (rc != CL_OK)
    {
        clLogError("PM", "JOBWALK", "Failed to get attribute path from txn job. rc [0x%x]", rc);
        return rc;
    }
    
    return CL_OK; 
}
