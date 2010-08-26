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
 * ModuleName  : alarm                                                         
 * File        : clAlarmDebug.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *******************************************************************************/

/* System includes */
#include <string.h>
#include <sys/time.h>

/* Clovis Common includes */
#include <clCommon.h>
#include <clDebugApi.h>
#include <clAlarmDefinitions.h>
#include <clAlarmDebug.h>
#include <clCorApi.h>
#include <clCorUtilityApi.h>
#include <clCorErrors.h>
#include <clAlarmCommons.h>
#include <clAlarmInfoCnt.h>
#include <ipi/clAlarmIpi.h>
#include <clAlarmServerAlarmUtil.h>

#include <clIdlApi.h>
#include <clXdrApi.h>
#include <xdrClAlarmInfoIDLT.h>

/******************************************************************************
 * Constants and extern declarations
 *****************************************************************************/

#define MAX_DISPLAY_SIZE 500
#define MAX_ALARM_MSO_BITMAP_LENGTH 100
#define MAX_ALL_ALARM_MSO_BITMAP_LENGTH 10000

/*****************************************************************************/
extern ClUint32T gClAlarmSeverityStringCount;

void clAlarmCliStrPrint(ClCharT* str, ClCharT**retStr)
{
    if (retStr != NULL)
    {
        *retStr = clHeapAllocate(strlen(str)+1);
        if(NULL == *retStr)
        {
            clLogWrite(CL_LOG_HANDLE_SYS, CL_LOG_CRITICAL,    CL_ALARM_SERVER_LIB,
                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            return;
        }
        sprintf(*retStr, str);
    }
    else
        clLogError("DBG", "STR", "The retStr passed is NULL");
    return;
}

/* Added the show command for displaying raised alarms */
ClRcT
clAlarmCliInfoShow(ClUint32T argc, ClCharT **argv, ClCharT** ret)
{
    ClRcT                 rc = CL_OK; 
    ClAlarmInfoT*         pAlarmInfo;
    ClAlarmHandleT        alarmHandle;
    ClCharT               displayStr[MAX_DISPLAY_SIZE];
    ClNameT moidName={0,"\0"};

    if(argc != 2)
    {
          clAlarmCliStrPrint("\nUsage: showAlarmInfo <AlarmHandle>\n",ret);
          return rc;
    }

    alarmHandle = atoi(argv[1]);
    rc = clAlarmAlarmInfoGet(alarmHandle,&pAlarmInfo);
    if(rc == CL_OK)
    { 
        clCorMoIdToMoIdNameGet(&(pAlarmInfo->moId),&moidName);
        memset(displayStr,'\0',MAX_DISPLAY_SIZE); 
        sprintf(displayStr,
                "\n ProbCause = %d \n SpecificProblem = %d \n alarmState = %d \n category = %d\n severity = %d  \n MoId = %s\n", 
                pAlarmInfo->probCause,
                pAlarmInfo->specificProblem,
                pAlarmInfo->alarmState,
                pAlarmInfo->category,
                pAlarmInfo->severity,
                moidName.value);

        clAlarmCliStrPrint(displayStr,ret);
    }
    else 
        clAlarmCliStrPrint("Invalid alarm Handle\n",ret);

    return rc;

}

/* Added the show command for displaying raised alarms */
ClRcT
clAlarmCliHandlesShow(ClUint32T argc, ClCharT **argv, ClCharT** ret)
{
    ClRcT                 rc = CL_OK; 
    ClAlarmHandleT        alarmHandle;
    ClUint32T             numRecords;
    ClUint32T             recordSize;
    void*                 buffer;                  
    void*                 temp;                  
    ClUint32T             bufLen;
    ClUint32T             index;
    ClCharT*              displayStr = NULL;
    ClCharT               tempStr[CL_MAX_NAME_LENGTH];
    
    if(argc != 1)
    {
          clAlarmCliStrPrint("\nUsage: showAllAlarmHandles \n",ret);
          return rc;
    }
    
    rc= clAlarmNumAlarmInfoEntriesGet(&numRecords);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clAlarmShow: clAlarmNumAlarmInfoEntriesGet failed w/rc :%x \n", rc));
        return rc;
    }

    if (!numRecords)
    {
        clLogTrace("CLI", "SHOW", "No alarms published by this server.");
        return CL_OK;
    }

    recordSize = sizeof(ClAlarmHandleT);
    bufLen = (numRecords+1)*recordSize; 
    
    buffer = clHeapAllocate(bufLen);
    if (NULL == buffer)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clAlarmShow: clHeapAllocate failed w/rc :%x \n", rc));
        return rc;
    }

    temp = buffer;

    clLogTrace("DBG", "AHS", "Got the no. of entries in the container. [%d] bufLen [%d]", numRecords, bufLen);

    rc= clAlarmDatabaseHandlesGet(buffer,bufLen);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clAlarmShow: clAlarmDatabaseGet failed w/rc :%x \n", rc));
        clHeapFree(buffer);
        return rc;
    }
  
    clLogTrace("DBG", "AHS", "Got the no. of entries in the container. [%d]", numRecords);

    /* allocate numRecords * string length of one handle information */
    displayStr = clHeapAllocate(numRecords * 25);
    if (!displayStr)
    {
        clLogError("DBG", "AHS", "Failed to allocate memory.");
        clHeapFree(buffer);
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    for(index=0;index<numRecords;index++)
    {
        alarmHandle = *((ClAlarmHandleT *)buffer);
        sprintf(tempStr,"\n Handle = %d",alarmHandle);
        strcat(displayStr,tempStr);
        buffer = ((ClCharT *)buffer + recordSize);
    } 
    
    clAlarmCliStrPrint(displayStr,ret);

    clHeapFree(temp);
    clHeapFree(displayStr);
    
    return rc;
}

/*****************************************************************************/

ClRcT _clAlarmCliShowRaisedAlarms(void* pData, void* cookie)
{
    ClCorObjectHandleT objH = 0;
    ClCharT alarmString[CL_MAX_NAME_LENGTH] = {0};
    ClBufferHandleT buffer = *(ClBufferHandleT *) cookie;
    ClCorServiceIdT svcId = CL_COR_INVALID_SVC_ID;
    ClAlarmAttrListT* pAttrList = NULL;
    ClCorMOIdT moId = {{{0}}};
    ClRcT rc = CL_OK;
    ClAlarmProbableCauseT probCause = 0;
    ClAlarmSpecificProblemT specProb = 0;
    ClNameT moIdname = {0};
    ClUint64T publishedalarms = 0;
    ClUint32T i = 0;

    if (!pData)
    {
        clLogError("DBG", "SHOWRAISEDALMS", "NULL value passed in pData");
        return CL_ALARM_RC(CL_ERR_NULL_POINTER);
    }

    objH = *(ClCorObjectHandleT *) pData;

    rc = clCorObjectHandleToMoIdGet(objH, &moId, &svcId);
    if (rc != CL_OK)
    {
        clLogError("DBG", "SHOWRAISEDALMS", 
                "Failed to get MoId from object handle. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    if (svcId != CL_COR_SVC_ID_ALARM_MANAGEMENT)
    {
        clCorObjectHandleFree(&objH);
        return CL_OK;
    }

    pAttrList = (ClAlarmAttrListT *) clHeapAllocate( sizeof(ClAlarmAttrListT) + 
                                            2 * sizeof(ClAlarmAttrInfoT));
    if (!pAttrList)
    {
        clLogError("DBG", "SHOWRAISEDALMS", "Failed to allocate memory.");
        clCorObjectHandleFree(&objH);
        return CL_ALARM_RC(CL_ERR_NO_MEMORY);
    }

    pAttrList->numOfAttr = 1;
    pAttrList->attrInfo[0].attrId = CL_ALARM_PUBLISHED_ALARMS;
    pAttrList->attrInfo[0].index = -1;
    pAttrList->attrInfo[0].pValue = &publishedalarms;
    pAttrList->attrInfo[0].size = sizeof(ClUint64T);

    rc = clAlarmAttrValueGet(objH, pAttrList);
    if (rc != CL_OK)
    {
        clLogError("DBG", "SHOWRAISEDALMS", "Failed to get alarm values. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        clHeapFree(pAttrList);
        return rc;
    }

    if (publishedalarms != 0)
    {
        rc = clCorMoIdToMoIdNameGet(&moId, &moIdname);
        if (rc != CL_OK)
        {
            clLogError("DBG", "SHOWRAISEDALMS", 
                    "Failed to get moid name from moid. rc [0x%x]", rc);
            clCorObjectHandleFree(&objH);
            clHeapFree(pAttrList);
            return rc;
        }

        snprintf (alarmString, CL_MAX_NAME_LENGTH, 
            "\n\nMoId :      [%s]", moIdname.value);
        rc = clBufferNBytesWrite(buffer, (ClUint8T *) alarmString, strlen(alarmString));
        if (rc != CL_OK)
        {
            clLogError("DBG", "SHOWRAISEDALMS", 
                    "Failed to write into buffer. rc [0x%x]", rc);
            clCorObjectHandleFree(&objH);
            clHeapFree(pAttrList);
            return rc;
        }

        for (i=0; i < CL_ALARM_MAX_ALARMS; i++)
        {
            if (publishedalarms & ((ClUint64T) 1 << i))
            {
                /*
                 * Get the values for PROBCAUSE and SPECPROB.
                 */
                pAttrList->numOfAttr = 2;
                pAttrList->attrInfo[0].attrId = CL_ALARM_PROBABLE_CAUSE;
                pAttrList->attrInfo[0].index = i;
                pAttrList->attrInfo[0].pValue = &probCause;
                pAttrList->attrInfo[0].size = sizeof(ClUint32T);

                pAttrList->attrInfo[1].attrId = CL_ALARM_SPECIFIC_PROBLEM;
                pAttrList->attrInfo[1].index = i;
                pAttrList->attrInfo[1].pValue = &specProb;
                pAttrList->attrInfo[1].size = sizeof(ClUint32T);

                rc = clAlarmAttrValueGet(objH, pAttrList);
                if (rc != CL_OK)
                {
                    clLogError("DBG", "SHOWRAISEDALMS", "Failed to get alarm values. rc [0x%x]", rc);
                    clCorObjectHandleFree(&objH);
                    clHeapFree(pAttrList);
                    return rc;
                }

                snprintf(alarmString, CL_MAX_NAME_LENGTH, "\nPROBCAUSE : [%-80s (%u)], SPECPROB : [%u]", 
                        clAlarmProbableCauseString[probCause], probCause, specProb);
                
                rc = clBufferNBytesWrite(buffer, (ClUint8T *) alarmString, strlen(alarmString));
                if (rc != CL_OK)
                {
                    clLogError("DBG", "SHOWRAISEDALMS", "Failed to write into buffer. rc [0x%x]", rc);
                    clCorObjectHandleFree(&objH);
                    clHeapFree(pAttrList);
                    return rc;
                }
            }
        }
    }

    clCorObjectHandleFree(&objH);
    clHeapFree(pAttrList);

    return CL_OK;
}

ClRcT
clAlarmCliShowRaisedAlarms(ClUint32T argc, ClCharT** argv, ClCharT** ret)
{
    ClBufferHandleT buffer = 0;
    ClRcT rc = CL_OK;
    ClCorMOIdT moId = {{{0}}};
    ClCorObjectHandleT objH = 0;
    ClUint32T size = 0;

    if (argc > 2)
    {
        clAlarmCliStrPrint("\nUsage: showRaisedAlarms <MoId> \n"
            "\tMoId (optional)       : This is the absolute path of MoId."
            " eg. \\Class_Chassis:0\\Class_GigeBlade:1 \n",
            ret);
        return CL_OK;
    }

    rc = clBufferCreate(&buffer);
    if (rc != CL_OK)
    {
        clLogError("DBG", "SHOWRAISEDALMS", "Failed to create buffer. rc [0x%x]", rc);
        clAlarmCliStrPrint("Execution Failed. \n", ret);
        return rc;
    }

    if (argc == 2)
    {
        rc = clAlarmCorXlateMOPath(argv[1], &moId);
        if (rc != CL_OK)
        {
            clLogError("DBG", "SHOWRAISEALMS", "Failed to convert string to moId. rc [0x%x]", rc);
            clAlarmCliStrPrint("Execution Failed. \n", ret);
            clBufferDelete(&buffer);
            return rc;
        }

        rc = clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_ALARM_MANAGEMENT);
        if (rc != CL_OK)
        {
            clLogError("DBG", "SHOWRAISEALMS", "Failed to set svcId in moId. rc [0x%x]", rc);
            clAlarmCliStrPrint("Execution Failed. \n", ret);
            clBufferDelete(&buffer);
            return rc;
        }

        /* 
         * This object handle will be freed by _clAlarmCliShowRaisedAlarms.
         */
        rc = clCorMoIdToObjectHandleGet(&moId, &objH);
        if (rc != CL_OK)
        {
            clLogError("DBG", "SHOWRAISEALMS", "Failed to get object handle from moId. rc [0x%x]", rc);
            clAlarmCliStrPrint("Execution Failed. \n", ret);
            clBufferDelete(&buffer);
            return rc;
        }

        rc = _clAlarmCliShowRaisedAlarms(&objH, &buffer);
        if (rc != CL_OK)
        {
            clLogError("DBG", "SHOWRAISEALMS", "Failed to get raised alarms info. rc [0x%x]", rc);
            clAlarmCliStrPrint("Execution Failed. \n", ret);
            clCorObjectHandleFree(&objH);
            clBufferDelete(&buffer);
            return rc;
        }
    }
    else
    {
        rc = clCorObjectWalk(NULL, NULL, _clAlarmCliShowRaisedAlarms, 
                CL_COR_MSO_WALK, &buffer);
        if (rc != CL_OK)
        {
            clLogError("DBG", "SHOWRAISEALMS", "Failed to get raised alarms info. rc [0x%x]", rc);
            clAlarmCliStrPrint("Execution Failed. \n", ret);
            clBufferDelete(&buffer);
            return rc;
        }
    }

    rc = clBufferLengthGet(buffer, &size);
    if (rc != CL_OK)
    {
        clLogError("DBG", "SHOWRAISEALMS", "Failed to get length of the buffer. rc [0x%x]", rc);
        clAlarmCliStrPrint("Execution Failed. \n", ret);
        clBufferDelete(&buffer);
        return rc;
    }

    if (size > 0)
    {
        ClUint8T* pBuf = NULL;

        pBuf = clHeapAllocate(size + 1);
        if (!pBuf)
        {
            clLogError("DBG", "SHOWRAISEDALMS", "Failed to allocate memory.");
            clAlarmCliStrPrint("Execution Failed. \n", ret);
            clBufferDelete(&buffer);
            return CL_ALARM_RC(CL_ERR_NO_MEMORY); 
        }

        memset(pBuf, 0, (size + 1));

        clBufferNBytesRead(buffer, (ClUint8T *) pBuf, &size);
        clAlarmCliStrPrint((ClCharT *) pBuf, ret);
        clHeapFree(pBuf);
    }

    clBufferDelete(&buffer);

    return CL_OK;
}

/*****************************************************************************/

ClRcT 
_clAlarmCliQuery(ClAlarmInfoT *alarmInfo,ClCorObjectHandleT objH)
{
    ClRcT rc=CL_OK;
    ClUint32T index=0;
    ClUint32T size=sizeof(ClUint32T);
    ClAlarmRuleEntryT alarmKey = {.probableCause = alarmInfo->probCause,
                              .specificProblem = alarmInfo->specificProblem};
    ClAlarmAttrListT    attrList = {0};

    rc = clAlarmUtilAlmIdxGet(objH, alarmKey, &index);
    if(CL_OK != rc)
    {
        clLogError("DBG", "AQY", "Failed to get the alarm index for "
                "Probable Cause [%d] and specific problem [%d]. rc[0x%x]", 
                alarmInfo->probCause, alarmInfo->specificProblem, rc);
        return rc;
    }

    size = sizeof(ClUint8T);

    attrList.numOfAttr = 1;
    attrList.attrInfo[0].attrId = CL_ALARM_ACTIVE;
    attrList.attrInfo[0].index = index;
    attrList.attrInfo[0].pValue = &alarmInfo->alarmState;
    attrList.attrInfo[0].size = size;

    rc = clAlarmAttrValueGet(objH, &attrList);
    if (CL_OK != rc)
        clLogError("DBG", "AQY", "Failed to get the State of the alarm. rc[0x%x]", rc);

    return rc;
}

ClRcT
clAlarmCliQuery(ClUint32T argc, ClCharT **argv, ClCharT** ret)
{
    ClRcT                 rc = CL_OK; 
    ClAlarmInfoT*         pAlarmInfo;
    ClCorMOIdT             moid;
    ClCorObjectHandleT     hMSOObj;    

    if ( argc < 3 || argc > 4 )
    {
        clAlarmCliStrPrint("\nUsage: queryAlarm <Moid> <Probable Cause> <Specific Problem>\n"
            "\tMoid [STRING]       : This is the absolute path of the MOID"
            " i.e \\Class_Chassis:0\\Class_GigeBlade:1 \n"
            "\tProbable Cause [DEC] : Probable cause of the alarm to query\n"
            "\tSpecific Problem [DEC] : Specific of the alarm to query\n",
            ret);
        return CL_OK;
    }

    pAlarmInfo = (ClAlarmInfoT *)clHeapAllocate(sizeof(ClAlarmInfoT));
	if(pAlarmInfo == NULL)
	{
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nMemory Allocation failed\n"));
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
	}


    rc = clAlarmCorXlateMOPath (argv[1], &moid );

    if ( CL_OK == rc)
    {
        rc = clCorMoIdServiceSet(&moid , CL_COR_SVC_ID_ALARM_MANAGEMENT);


        /*clCorMoIdShow(&moid);*/

        rc = clCorObjectHandleGet(&moid, &hMSOObj);

        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorObjectHandleGet failed w/rc :%x \n", rc));
            clHeapFree(pAlarmInfo);
            return rc;
        }

        pAlarmInfo->probCause = atoi(argv[2]);
        pAlarmInfo->specificProblem = argc == 4 ? atoi(argv[3]) : 0;
        pAlarmInfo->moId = moid;
        rc = _clAlarmCliQuery(pAlarmInfo,hMSOObj);
        if(rc == CL_OK)
        {
            if(pAlarmInfo->alarmState==CL_ALARM_STATE_CLEAR)
                clAlarmCliStrPrint("Status of the alarm : CL_ALARM_STATE_CLEAR",ret);
            else if(pAlarmInfo->alarmState==CL_ALARM_STATE_ASSERT)
                clAlarmCliStrPrint("Status of the alarm : CL_ALARM_STATE_ASSERT",ret);
        }
        else
        {
            ClCharT     tempStr [CL_MAX_NAME_LENGTH] = {0};
            sprintf (tempStr, "Execution Failed. rc[0x%x]", rc);
            clAlarmCliStrPrint(tempStr,ret);
            rc = CL_OK;
        }
    }
    else
    {
        ClCharT tempStr[CL_MAX_NAME_LENGTH] = {0};
        sprintf(tempStr, "Execution failed. rc[0x%x]", rc);
        clAlarmCliStrPrint(tempStr, ret);
    }

clHeapFree(pAlarmInfo);
    return rc;
}
/*****************************************************************************/

static ClRcT alarmCliProcess(ClUint32T argc, ClCharT** argv, ClCharT** ret, ClAlarmStateT alarmState)
{
    ClRcT                 rc = CL_OK; 
    ClAlarmInfoIDLT       *pAlarmInfoIdl=NULL;
    ClCorMOIdT             moid;
    ClCorObjectHandleT     hMSOObj;    
    ClBufferHandleT inMsgHandle;
    ClIocAddressT          iocAddress;
    ClRmdOptionsT           opts;
    ClCorAddrT         addr;    
    struct timeval alarmTime;

    rc = clAlarmCorXlateMOPath (argv[1], &moid );

    if ( CL_OK == rc)
    {
        rc = clCorMoIdServiceSet(&moid , CL_COR_SVC_ID_ALARM_MANAGEMENT);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorMoIdServiceSet failed w/rc :%x \n", rc));
            return rc;
        }

        rc = clCorObjectHandleGet(&moid, &hMSOObj);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorObjectHandleGet failed w/rc :%x \n", rc));
            return rc;
        }

		pAlarmInfoIdl = clHeapAllocate(sizeof(ClAlarmInfoIDLT));
		if(pAlarmInfoIdl == NULL)
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\nMemory Allocation failed\n"));
			return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
		}

        pAlarmInfoIdl->probCause = clAlarmProbCauseStringToValueGet(argv[2]);
        pAlarmInfoIdl->specificProblem = atoi(argv[3]);
        pAlarmInfoIdl->severity = clAlarmSeverityStringToValueGet(argv[4]);
        pAlarmInfoIdl->alarmState = alarmState; 

		pAlarmInfoIdl->compName.length = 0;
        pAlarmInfoIdl->moId = moid;
        gettimeofday(&alarmTime,NULL);
        pAlarmInfoIdl->eventTime = alarmTime.tv_sec;
		
        rc = clCorMoIdToComponentAddressGet(&(pAlarmInfoIdl->moId), &addr);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCorMoIdToComponentAddressGet failed with rc 0x:%x\n", rc));
            clHeapFree(pAlarmInfoIdl);
            return rc;
        }

        opts.timeout = CL_ALARM_RMDCALL_TIMEOUT;
        opts.priority = CL_RMD_DEFAULT_PRIORITY;
        opts.retries = CL_ALARM_RMDCALL_RETRIES;

        rc = clBufferCreate(&inMsgHandle);
        if (rc != CL_OK )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not create buffer message\n"));
            clHeapFree(pAlarmInfoIdl);
            return rc;
        }

        rc = VDECL_VER(clXdrMarshallClAlarmInfoIDLT, 4, 0, 0)((void *)pAlarmInfoIdl,inMsgHandle,0);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Could not write into Buffer with rc:0x%x\n",rc));
            clBufferDelete(&inMsgHandle);
			clHeapFree(pAlarmInfoIdl);
            return rc;
        }

		clHeapFree(pAlarmInfoIdl);

        iocAddress.iocPhyAddress.nodeAddress = addr.nodeAddress;
        iocAddress.iocPhyAddress.portId    = addr.portId;

        rc = clRmdWithMsg (iocAddress,
                           CL_ALARM_COMP_ALARM_RAISE_IPI,
                           inMsgHandle,
                           (ClBufferHandleT)NULL,
                           CL_RMD_CALL_ATMOST_ONCE,
                           &opts,
                           NULL);
        if(rc != CL_OK)
        {
            clLogError("DBG", "ALR", "Failed while raising the alarm. rc[0x%x]", rc);
        }

        clBufferDelete(&inMsgHandle);
    }
    else
    {
        rc = CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID);
    }

    return rc;
}

ClRcT
clAlarmCliRaise(ClUint32T argc, ClCharT **argv, ClCharT** ret)
{
    ClRcT                 rc = CL_OK; 
    ClCharT               alarmStr[CL_MAX_NAME_LENGTH] = {0};

    if ( argc != 5 )
    {
        clAlarmCliStrPrint("\nUsage: raiseAlarm <MoId> <ProbableCause> <Specific Problem> <Severity>\n"
                    "\tMoId [STRING]             : This is the absolute path of the MOID"
                    " i.e \\Class_Chassis:0\\Class_GigeBlade:1 \n"
                    "\tProbableCause [STRING]    : Probable cause of the alarm to be raised\n"
                    "\tSpecific Problem [DEC]    : Specific Problem of the alarm to be raised.\n"
                    "\tSeverity [STRING]         : Severity of the alarm.\n", ret);
        return CL_OK;
    }

    rc = alarmCliProcess(argc, argv, ret, CL_ALARM_STATE_ASSERT);
    if (rc != CL_OK)
    {
        clLogError("CLI", "ALARMRAISE", "Failed to process alarm. rc [0x%x]", rc);
        sprintf(alarmStr, "Failed to process alarm. rc [0x%x]", rc);
        clAlarmCliStrPrint(alarmStr, ret);
        return rc;
    }

    return CL_OK;
}

ClRcT
clAlarmCliClear(ClUint32T argc, ClCharT **argv, ClCharT** ret)
{
    ClRcT                 rc = CL_OK; 
    ClCharT               alarmStr[CL_MAX_NAME_LENGTH] = {0};

    if ( argc != 5 )
    {
        clAlarmCliStrPrint("\nUsage: clearAlarm <Moid> <ProbableCause> <Specific Problem> <Severity>\n"
                    "\tMoId [STRING]             : This is the absolute path of the MOID"
                    " i.e \\Class_Chassis:0\\Class_GigeBlade:1 \n"
                    "\tProbableCause [STRING]    : Probable cause of the alarm to be raised\n"
                    "\tSpecific Problem [DEC]    : Specific Problem of the alarm to be raised.\n"
                    "\tSeverity [STRING]         : Severity of the alarm.\n", ret);
        return CL_OK;
    }

    rc = alarmCliProcess(argc, argv, ret, CL_ALARM_STATE_CLEAR);
    if (rc != CL_OK)
    {
        clLogError("CLI", "ALARMRAISE", "Failed to process alarm. rc [0x%x]", rc);
        sprintf(alarmStr, "Failed to process alarm. rc [0x%x]", rc);
        clAlarmCliStrPrint(alarmStr, ret);
        return rc;
    }

    return CL_OK;
}

static ClRcT _clAlarmCliShowAssociatedAlarms(void* pData, void* cookie)
{
    ClRcT rc = CL_OK;
    ClBufferHandleT buffer = *(ClBufferHandleT *) cookie;
    ClCorObjectHandleT objH = *(ClCorObjectHandleT *) pData;
    ClAlarmAttrListT* pAttrList = NULL;
    ClAlarmInfoT alarmInfo[CL_ALARM_MAX_ALARMS] = {{0}};
    ClUint8T alarmEnable[CL_ALARM_MAX_ALARMS] = {0};
    ClUint32T attrIndex = 0;
    ClUint32T index = 0;
    ClCharT alarmString[CL_MAX_NAME_LENGTH] = {0};
    ClUint64T publishedAlarms = 0;
    ClCorMOIdT moId = {{{0}}};
    ClCorServiceIdT svcId = CL_COR_INVALID_SVC_ID;
    ClNameT moIdname = {0};

    rc = clCorObjectHandleToMoIdGet(objH, &moId, &svcId);
    if (rc != CL_OK)
    {
        clLogError("DBG", "ASSOCALARMS", "Failed to object handle from MoId. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        return rc;
    }

    if (svcId != CL_COR_SVC_ID_ALARM_MANAGEMENT)
    {   
        clCorObjectHandleFree(&objH);
        return CL_OK;
    }

    pAttrList = clHeapAllocate(sizeof(ClAlarmAttrListT) + 
                    (4 * CL_ALARM_MAX_ALARMS) * sizeof(ClAlarmAttrInfoT));
    if (!pAttrList)
    {
        clLogError("DBG", "ASSOCALARMS", "Failed to allocate memory.");
        clCorObjectHandleFree(&objH);
        return CL_ALARM_RC(CL_ERR_NO_MEMORY); 
    }

    attrIndex = 0;

    for (index=0; index < CL_ALARM_MAX_ALARMS; index++)
    {
        pAttrList->attrInfo[attrIndex].attrId = CL_ALARM_PROBABLE_CAUSE;
        pAttrList->attrInfo[attrIndex].index = index;
        pAttrList->attrInfo[attrIndex].pValue = &(alarmInfo[index].probCause);
        pAttrList->attrInfo[attrIndex].size = sizeof(ClUint32T);
        attrIndex++;

        pAttrList->attrInfo[attrIndex].attrId = CL_ALARM_SPECIFIC_PROBLEM; 
        pAttrList->attrInfo[attrIndex].index = index;
        pAttrList->attrInfo[attrIndex].pValue = &(alarmInfo[index].specificProblem);
        pAttrList->attrInfo[attrIndex].size = sizeof(ClUint32T);
        attrIndex++;

        pAttrList->attrInfo[attrIndex].attrId = CL_ALARM_CATEGORY;
        pAttrList->attrInfo[attrIndex].index = index;
        pAttrList->attrInfo[attrIndex].pValue = &(alarmInfo[index].category);
        pAttrList->attrInfo[attrIndex].size = sizeof(ClUint8T);
        attrIndex++;

        pAttrList->attrInfo[attrIndex].attrId = CL_ALARM_ENABLE;
        pAttrList->attrInfo[attrIndex].index = index;
        pAttrList->attrInfo[attrIndex].pValue = &(alarmEnable[index]);
        pAttrList->attrInfo[attrIndex].size = sizeof(ClUint8T);
        attrIndex++;
    }

    /* Get published alarms bitmap */
    pAttrList->attrInfo[attrIndex].attrId = CL_ALARM_PUBLISHED_ALARMS;
    pAttrList->attrInfo[attrIndex].index = -1;
    pAttrList->attrInfo[attrIndex].pValue = &(publishedAlarms);
    pAttrList->attrInfo[attrIndex].size = sizeof(ClUint64T);
    attrIndex++;

    pAttrList->numOfAttr = attrIndex; 

    rc = clAlarmAttrValueGet(objH, pAttrList);
    if (rc != CL_OK)
    {
        clLogError("DBG", "ASSOCALARMS", "Failed to get alarm attributes from COR. rc [0x%x]", rc);
        clCorObjectHandleFree(&objH);
        clHeapFree(pAttrList);
        return rc; 
    }

    clCorObjectHandleFree(&objH);
    clHeapFree(pAttrList);

    rc = clCorMoIdToMoIdNameGet(&moId, &moIdname);
    if (rc != CL_OK)
    {
        clLogError("DBG", "ASSOCALARMS", "Failed to get moid name from moid. rc [0x%x]", rc);
        return rc;
    }
    
    snprintf(alarmString, CL_MAX_NAME_LENGTH, "\n\n\nMoId : [%s]\n", moIdname.value);

    rc = clBufferNBytesWrite(buffer, (ClUint8T *) alarmString, strlen(alarmString));
    if (rc != CL_OK)
    {
        clLogError("DBG", "ASSOCALARMS", "Failed to get moid name from moid. rc [0x%x]", rc);
        return rc;
    }

    for (index=0; index < CL_ALARM_MAX_ALARMS; index++)
    {
        if (alarmInfo[index].probCause == CL_ALARM_ID_INVALID)
            break;

        snprintf(alarmString, CL_MAX_NAME_LENGTH, 
                "\nPROBABLE CAUSE     : %s\nSPECIFIC PROBLEM   : %u\n"
                "CATEGORY           : %s\nENABLED            : %s\nSTATE              : %s\n",
                clAlarmProbableCauseString[alarmInfo[index].probCause], 
                alarmInfo[index].specificProblem, 
                clAlarmCategoryString[alarmInfo[index].category], 
                (alarmEnable[index] == 1) ? "YES" : "NO",
                (publishedAlarms & ((ClUint64T) 1 << index)) ? "RAISED" : "CLEARED");

        rc = clBufferNBytesWrite(buffer, (ClUint8T *) alarmString, strlen(alarmString));
        if (rc != CL_OK)
        {
            clLogError("DBG", "ASSOCALARMS", "Failed to write into buffer. rc [0x%x]", rc);
            return rc;
        }
    }

    return CL_OK; 
}

ClRcT clAlarmCliShowAssociatedAlarms(ClUint32T argc, ClCharT** argv, ClCharT** ret)
{
    ClRcT rc = CL_OK;
    ClCorMOIdT moId = {{{0}}};
    ClCorObjectHandleT objH = 0;
    ClBufferHandleT buffer = 0;
    ClUint32T size = 0;
    ClCharT alarmStr[CL_MAX_NAME_LENGTH] = {0};

    if (argc > 2)
    {
        clAlarmCliStrPrint("\nUsage : showAssociatedAlarms <MoId>\n"
                    "\tMoId [STRING] (optional) : Absolute path of the MOID\n", ret);
        return CL_OK;
    }

    rc = clBufferCreate(&buffer);
    if (rc != CL_OK)
    {
        clLogError("DBG", "ASSOCALARMS", "Failed to create buffer. rc [0x%x]", rc);
        return rc;
    }

    if (argc == 2)
    {
        rc = clAlarmCorXlateMOPath(argv[1], &moId);
        if (rc != CL_OK)
        {
            clLogError("DBG", "ASSOCALARMS", "Failed to convert MoId string to MoId. rc [0x%x]", rc);
            clBufferDelete(&buffer);
            sprintf(alarmStr, "Failed to convert MoId string to MoId. rc [0x%x]", 
                    CL_COR_SET_RC(CL_COR_INST_ERR_INVALID_MOID));
            clAlarmCliStrPrint(alarmStr, ret);
            return rc;
        }

        rc = clCorMoIdServiceSet(&moId, CL_COR_SVC_ID_ALARM_MANAGEMENT);
        if (rc != CL_OK)
        {
            clLogError("DBG", "ASSOCALARMS", "Failed to set MoId service-Id. rc [0x%x]", rc);
            clBufferDelete(&buffer);
            return rc;
        }

        rc = clCorObjectHandleGet(&moId, &objH);
        if (rc != CL_OK)
        {
            clLogError("DBG", "ASSOCALARMS", "Failed to get object handle. rc [0x%x]", rc);
            clBufferDelete(&buffer);
            return rc;
        }

        rc = _clAlarmCliShowAssociatedAlarms(&objH, &buffer);
        if (rc != CL_OK)
        {
            clLogError("DBG", "ASSOCALARMS", "Failed to get the associated alarms. rc [0x%x]", rc);
            clBufferDelete(&buffer);
            return rc;
        }
    }
    else
    {
        rc = clCorObjectWalk(NULL, NULL, _clAlarmCliShowAssociatedAlarms, CL_COR_MSO_WALK, &buffer);
        if (rc != CL_OK)
        {
            clLogError("DBG", "ASSOCALARMS", "Failed to walk COR objects. rc [0x%x]", rc);
            clBufferDelete(&buffer);
            return rc;
        }
    }

    rc = clBufferLengthGet(buffer, &size);
    if (rc != CL_OK)
    {
        clLogError("DBG", "ASSOCALARMS", "Failed to get buffer length. rc [0x%x]", rc);
        clBufferDelete(&buffer);
        return rc;
    }

    if (size > 0)
    {
        ClUint8T* pBuf = NULL;

        pBuf = clHeapAllocate(size + 1);
        if (!pBuf)
        {
            clLogError("DBG", "ASSOCALARMS", "Failed to allocate memory.");
            clBufferDelete(&buffer);
            return CL_ALARM_RC(CL_ERR_NO_MEMORY); 
        }

        memset(pBuf, 0, (size + 1));

        rc = clBufferNBytesRead(buffer, (ClUint8T *) pBuf, &size);
        if (rc != CL_OK)
        {
            clLogError("DBG", "ASSOCALARMS", "Failed to reads bytes from the buffer. rc [0x%x]", rc);
            clBufferDelete(&buffer);
            return rc; 
        }

        clAlarmCliStrPrint((ClCharT *) pBuf, ret);
        clHeapFree(pBuf);
    }

    clBufferDelete(&buffer);

    return CL_OK;
}

ClRcT clAlarmCliShowAlarmSeverityList(ClUint32T argc, ClCharT** argv, ClCharT** ret)
{
    ClRcT rc = CL_OK;
    ClCharT alarmSeverity[CL_MAX_NAME_LENGTH] = {0};
    ClUint32T size = 0;
    ClBufferHandleT buffer = 0;
    ClUint32T i = 0;
    ClUint8T* pBuf = NULL;

    if (argc != 1)
    {
        clAlarmCliStrPrint("\nUsage : showAlarmSeverityList\n", ret);
        return CL_OK;
    }

    rc = clBufferCreate(&buffer);
    if (rc != CL_OK)
    {
        clLogError("DBG", "SHOWALARMSEVLIST", "Failed to create buffer. rc [0x%x]", rc);
        return rc;
    }

    sprintf(alarmSeverity, "\n%-50s -->  %s\n\n", "STRING", "VALUE");

    rc = clBufferNBytesWrite(buffer, (ClUint8T *) alarmSeverity, strlen(alarmSeverity));
    if (rc != CL_OK)
    {
        clLogError("DBG", "SHOWALARMSEVLIST", "Failed to write into buffer. rc [0x%x]", rc);
        clBufferDelete(&buffer);
        return rc;
    }

    for (i=0; i < gClAlarmSeverityStringCount; i++)
    {
        sprintf(alarmSeverity, "%-50s -->  %u\n", clAlarmSeverityString[i], i);

        rc = clBufferNBytesWrite(buffer, (ClUint8T *) alarmSeverity, strlen(alarmSeverity));
        if (rc != CL_OK)
        {
            clLogError("DBG", "SHOWALARMSEVLIST", "Failed to write into buffer. rc [0x%x]", rc);
            clBufferDelete(&buffer);
            return rc;
        }
    }

    rc = clBufferLengthGet(buffer, &size);
    if (rc != CL_OK)
    {
        clLogError("DBG", "SHOWALARMSEVLIST", "Failed to get length of the buffer. rc [0x%x]", rc);
        clBufferDelete(&buffer);
        return rc;
    }

    pBuf = clHeapAllocate(size + 1);
    if (!pBuf)
    {
        clLogError("DBG", "SHOWALARMSEVLIST", "Failed to allocate memory.");
        clBufferDelete(&buffer);
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY); 
    }

    memset(pBuf, 0, (size + 1));

    clBufferNBytesRead(buffer, (ClUint8T *) pBuf, &size);
    clAlarmCliStrPrint((ClCharT *) pBuf, ret);

    clHeapFree(pBuf);
    clBufferDelete(&buffer);

    return CL_OK;
}
