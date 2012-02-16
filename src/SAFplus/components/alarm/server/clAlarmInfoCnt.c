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
 * File        : clAlarmInfoCnt.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file provide implemantation of the API used for Alarm Info
 * Container.
 * This C file SHOULD NOT BE MODIFIED BY THE USER
 *
 *****************************************************************************/


#include <string.h>
#include <stdlib.h>
#include <clCommon.h>
#include <clEoApi.h>

#include <clCorApi.h>
#include <clDebugApi.h>
#include <clCntApi.h>
#include <clAlarmCommons.h>
#include <clAlarmInfoCnt.h>
#include <clCorUtilityApi.h>

static ClCntHandleT gAlarmInfoListHandle;
static ClOsalMutexIdT gClAlarmInfoCntMutex;
static ClUint32T gClAlarmHandle;
static ClUint32T offset=0; /* tells current positon in the dummy buffer */

ClInt32T clAlarmInfoCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return((ClWordT)key1-(ClWordT)key2);
}

void clAlarmInfoEntryDeleteCallback(ClCntKeyHandleT key, ClCntDataHandleT userData)
{
    clHeapFree((void*)userData);
}

ClRcT
clAlarmAlarmInfoCntCreate()
{
    ClRcT        rc = CL_OK;

    rc = clOsalMutexCreate(&gClAlarmInfoCntMutex);    
    
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clAlarmAlarmInfoCntCreate: clOsalMutexCreate failed with rc 0x%x\n", rc));
        return rc;
    }
    
    rc = clCntLlistCreate(clAlarmInfoCompare,
                clAlarmInfoEntryDeleteCallback,
                clAlarmInfoEntryDeleteCallback,
                CL_CNT_UNIQUE_KEY,
                &gAlarmInfoListHandle);

    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clAlarmAlarmInfoCntCreate: clCntListCreate failed with rc 0x%x\n", rc));
        return rc;
    }

    return rc;
}

ClRcT
clAlarmAlarmInfoCntDestroy()
{
    ClRcT        rc = CL_OK;


    rc = clCntAllNodesDelete(gAlarmInfoListHandle);
    
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clAlarmAlarmInfoCntDestroy: clCntAllNodesDelete failed with rc 0x%x\n", rc));
        return rc;
    }

    rc = clCntDelete(gAlarmInfoListHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clAlarmAlarmInfoCntDestroy: clCntDelete failed with rc 0x%x\n", rc));
    }
    
    return rc;
}

ClRcT 
clAlarmAlarmInfoAdd(ClAlarmInfoT *pAlarmInfo,ClAlarmHandleT *pHandle)
{
    ClRcT        rc = CL_OK;
	ClAlarmInfoT *alarmInfo= NULL;
    
    alarmInfo = (ClAlarmInfoT *)clHeapAllocate(sizeof(ClAlarmInfoT));
    if(alarmInfo == NULL)
    {
        clLogError("INF", "GEN", "Failed while allocating the alarm info to put into container");
        return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }
	
    *alarmInfo=*pAlarmInfo;

    clOsalMutexLock(gClAlarmInfoCntMutex);

    /* Generate a Unique key for the Alarm Handle */
    gClAlarmHandle++;
    if(gClAlarmHandle == 65535)
        gClAlarmHandle =1;

    *pHandle = gClAlarmHandle;

    clLogTrace("INF", "GEN", "The alarm handle generated is [%d]", gClAlarmHandle);

    rc =clCntNodeAdd((ClCntHandleT)gAlarmInfoListHandle,
			(ClCntKeyHandleT)(ClWordT)(*pHandle),
			(ClCntDataHandleT)alarmInfo,NULL);

    clOsalMutexUnlock(gClAlarmInfoCntMutex);

    if (CL_OK != rc)
    {
        clLogError("INF", "GEN", 
                "Failed while adding the alarm handle in the container. rc 0x%x\n", rc);
    }
        
    return rc;
}

ClRcT
clAlarmAlarmInfoDelete(ClAlarmHandleT handle)
{
    ClRcT        rc = CL_OK;

    clOsalMutexLock(gClAlarmInfoCntMutex);
    rc = clCntAllNodesForKeyDelete(gAlarmInfoListHandle,(ClCntKeyHandleT)(ClWordT)handle);
    clOsalMutexUnlock(gClAlarmInfoCntMutex);

    if (CL_OK != rc)
    {
         CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clAlarmAlarmInfoDelete: clCntAllNodesForKeyDelete failed w rc:0x%x\n", rc));
    }
    
    return rc;
}

ClRcT
clAlarmAlarmInfoGet(ClAlarmHandleT handle,ClAlarmInfoT **pAlarmInfo)
{
    ClRcT        rc = CL_OK;
   
    rc = clCntDataForKeyGet(gAlarmInfoListHandle, (ClCntKeyHandleT)(ClWordT)handle, (ClCntDataHandleT*) pAlarmInfo);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_INFO,("clAlarmAlarmInfoGet: Data not found with rc 0x%x\n", rc));
    }

    return rc;
}

ClRcT
clAlarmNumAlarmInfoEntriesGet(ClUint32T *pNumEntries)
{
    ClRcT        rc = CL_OK;
   
    clOsalMutexLock(gClAlarmInfoCntMutex);
    rc = clCntSizeGet(gAlarmInfoListHandle, pNumEntries);    
    clOsalMutexUnlock(gClAlarmInfoCntMutex);

    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clAlarmNumAlarmInfoEntriesGet: clCntSizeGet failed !!!  rc => [0x%x]",rc));
    }

    return rc;
}

ClRcT
clAlarmDatabaseHandlesGet(void *buf,ClUint32T length)
{
    ClRcT        rc = CL_OK;

    offset = 0;
    clOsalMutexLock(gClAlarmInfoCntMutex);
    rc = clCntWalk(gAlarmInfoListHandle,clAlarmDatabaseToBufferCopy, buf,length);
    clOsalMutexUnlock(gClAlarmInfoCntMutex);
    
     if (CL_OK != rc)
     {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clAlarmDatabaseGet: clCntWalk failed with rc 0x%x \n", rc));
     }

    return rc;
}

ClRcT
clAlarmDatabaseToBufferCopy(ClCntKeyHandleT     key,
            ClCntDataHandleT    pData,
            void                *dummy,
            ClUint32T           dataLength)
{
    ClRcT        rc = CL_OK;
    ClUint32T tempLength;
    ClAlarmHandleT alarmHandle = (ClAlarmHandleT)(ClWordT)key;

    tempLength = sizeof(ClAlarmHandleT);


    if(offset+tempLength <= dataLength)
    {
        clLogTrace("DBG", "AHS", "Copying the buffer handle information [%d]", alarmHandle);
        /* copy alarm info into dummy buffer from offset */
        memcpy(((ClUint8T *)dummy+offset), &alarmHandle,tempLength);
        offset += tempLength;
    }
    else
    {
        rc= CL_ERR_BUFFER_OVERRUN;
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clAlarmDatabaseToBufferCopy: buffer copy failed with rc 0x%x \n", rc));
    }

    return rc;
}


/**
 * Function to search the alarm handle given the alarm information. 
 */
ClRcT
clAlarmAlarmInfoToHandleGet(ClAlarmInfoT *pAlarmInfo,ClAlarmHandleT *pAlarmHandle)
{
    ClRcT        rc = CL_OK;
    ClCntNodeHandleT containerNode;
    ClCntKeyHandleT  userKey ; 
    ClAlarmInfoT *pTempAlarmInfo;
    ClUint8T bFound=CL_FALSE;

    *pAlarmHandle = 0;
    
    clOsalMutexLock(gClAlarmInfoCntMutex);
    rc = clCntFirstNodeGet (gAlarmInfoListHandle, &containerNode);
    if (CL_OK != rc)
    {
        if (CL_GET_ERROR_CODE(rc) != CL_ERR_NOT_EXIST)
            clLogError("ACT", "SRH", 
                    "Failed to get the alarm information from the in-memory container. rc [0x%x]", rc);
        clOsalMutexUnlock(gClAlarmInfoCntMutex);
        return rc;
    }

    clLogTrace("ACT", "SRH", "Searching the alarm info. ProbCause [%d], Specific Problem [%d]", 
            pAlarmInfo->probCause, pAlarmInfo->specificProblem);

    while(1)
    {
        rc = clCntNodeUserKeyGet (gAlarmInfoListHandle, containerNode, &userKey);
        if (CL_OK != rc)
        {
            clLogError("ACT", "SRH", 
                    "Failed to get the user key, while getting the alarm info \
                    for an alarm handle. rc[0x%x]", rc);
            break;
        }

        rc = clCntDataForKeyGet(gAlarmInfoListHandle, 
                (ClCntKeyHandleT)userKey, 
                (ClCntDataHandleT*)&pTempAlarmInfo);
        if (CL_OK != rc)
        {
            clLogError("ACT", "GET", " Failed while getting the Key information from container. rc [0x%x]", rc);
            break;
        }

        clLogTrace("ACT", "SRH", "Getting the probable cause [%d], Specific Problem [%d]", 
                pTempAlarmInfo->probCause, pTempAlarmInfo->specificProblem);

        if((pTempAlarmInfo->probCause == pAlarmInfo->probCause) && 
                (pTempAlarmInfo->specificProblem == pAlarmInfo->specificProblem) &&
                (clCorMoIdCompare(&(pTempAlarmInfo->moId),&(pAlarmInfo->moId))== 0) )
        {
            *pAlarmHandle = (ClWordT)userKey;
            clLogTrace("ACT", "SRH", "Found the alarm handle [%d] for PC [%d], SP[%d]",
                    *pAlarmHandle, pTempAlarmInfo->probCause, pTempAlarmInfo->specificProblem);
            bFound = CL_TRUE;
            break;
        }

         if(clCntNextNodeGet (gAlarmInfoListHandle, containerNode, &containerNode) != CL_OK)
         {
             clLogTrace("ACT", "SRH", "Reached the end of alarm information list. So breaking. ");
             break;
         }
     }

     if(bFound == CL_FALSE)
     {
            clLogError("ACT", "SRH", 
                    "Failed to get the alarm handle for P-C[%d], S-P[%d]",
                    pTempAlarmInfo->probCause, pTempAlarmInfo->specificProblem);
            rc = CL_ERR_DOESNT_EXIST;
     }

     clOsalMutexUnlock(gClAlarmInfoCntMutex);

    return rc;
}
