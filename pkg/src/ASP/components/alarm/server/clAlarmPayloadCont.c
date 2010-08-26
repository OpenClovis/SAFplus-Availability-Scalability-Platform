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
 * Build: 2.2
 */
/*******************************************************************************
 * ModuleName  : alarm                                                         
 * File        : clAlarmPayloadCont.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file provides the cache implementation of the statically configured 
 * information.
 *
 *****************************************************************************/
/* Standard Inculdes */
#include<string.h>

/* ASP Includes */
#include <clCommon.h>
#include <clCntApi.h>
#include <clDebugApi.h>
#include <clAlarmErrors.h>

/* cor apis */
#include <clCorApi.h>
#include <clCorUtilityApi.h>

/* alarm includes */
#include <clAlarmServer.h>
#include <clAlarmPayloadCont.h>
#include <ipi/clAlarmIpi.h>

/******************************************************************************
 *  Global
 *****************************************************************************/

/*************************** Mutex variables ********************************/
/*
 * The mutex used to protect the container
 */
static ClOsalMutexIdT gClAlarmPayloadCntMutex;
/*
 * The handle returned identifying the container
 */
ClCntHandleT     gPayloadCntHandle = 0;

/*
 * The container creation api
 */
ClRcT clAlarmPayloadCntListCreate()
{
    ClRcT rc = CL_OK;

    clOsalMutexCreate (&gClAlarmPayloadCntMutex);
    
    rc = clCntLlistCreate(clAlarmPayloadCntCompare, 
                          clAlarmPayloadCntEntryDeleteCallback,
                          clAlarmPayloadCntEntryDestroyCallback, 
                          CL_CNT_UNIQUE_KEY, 
                          &gPayloadCntHandle );
    return rc;
}

/*
 * The key compare callback
 */
ClInt32T clAlarmPayloadCntCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClAlarmPayloadCntKeyT *payloadKey1 = (ClAlarmPayloadCntKeyT *)key1;
    ClAlarmPayloadCntKeyT *payloadKey2 = (ClAlarmPayloadCntKeyT *)key2;
	if(clCorMoIdCompare((ClCorMOIdPtrT)&(payloadKey1->moId),(ClCorMOIdPtrT)&(payloadKey2->moId)) == 0 &&
        payloadKey1->probCause == payloadKey2->probCause &&
        payloadKey1->specificProblem == payloadKey2->specificProblem)
        return 0;
    else
        return 1;
}

/*
 * The cnt delete callback
 */
void clAlarmPayloadCntEntryDeleteCallback(ClCntKeyHandleT key, ClCntDataHandleT userData)
{
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("clCntNodeAdd deleting \n"));
    clHeapFree((ClAlarmPayloadCntT *)userData);
	clHeapFree((ClAlarmPayloadCntKeyT *)key);
	return;
}

/*
 * The cnt destroy callback
 */
void clAlarmPayloadCntEntryDestroyCallback(ClCntKeyHandleT key, ClCntDataHandleT userData)
{
    clHeapFree((ClAlarmPayloadCntT *)userData);
	clHeapFree((ClAlarmPayloadCntKeyT *)key);
	return;
}

/*
 * The cnt add function
 * The information is read from COR and then populated in the container
 * This api would be called as and when the object is created and the corresponding entry 
 * within the container needs to be added.
 */
ClRcT clAlarmPayloadCntAdd(ClAlarmInfoT *pAlarmInfo)
{
	ClRcT rc = CL_OK;
	ClCntNodeHandleT nodeH;
    ClAlarmPayloadCntT *payloadInfo;
    
    ClAlarmPayloadCntKeyT *pCntKey = clHeapAllocate(sizeof(ClAlarmPayloadCntKeyT));
    if(NULL == pCntKey)
    {
          CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,("Memory allocation failed with rc 0x%x ", rc));
          return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }    

    payloadInfo = NULL;
    pCntKey->probCause = pAlarmInfo->probCause;
    pCntKey->specificProblem = pAlarmInfo->specificProblem;
    pCntKey->moId = pAlarmInfo->moId;

    payloadInfo = clHeapAllocate(sizeof(ClAlarmPayloadCntT)+pAlarmInfo->len);
    if(NULL == payloadInfo)
    {
          CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,("Memory allocation failed with rc 0x%x ", rc));
          clHeapFree(pCntKey);
          return CL_ALARM_RC(CL_ALARM_ERR_NO_MEMORY);
    }    
    payloadInfo->len = pAlarmInfo->len;
    memcpy(payloadInfo->buff, pAlarmInfo->buff, payloadInfo->len);

	clOsalMutexLock(gClAlarmPayloadCntMutex);        
    rc = clCntNodeFind(gPayloadCntHandle,(ClCntKeyHandleT)pCntKey,&nodeH);
	if(rc != CL_OK)
	{
		rc = clCntNodeAdd((ClCntHandleT)gPayloadCntHandle,
							(ClCntKeyHandleT)pCntKey,
							(ClCntDataHandleT)payloadInfo,
							NULL);
		if (CL_OK != rc)
		{
			CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCntNodeAdd failed with rc = %x\n", rc));
            clHeapFree(payloadInfo);
            clHeapFree(pCntKey);
        }
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("clCntNodeAdd adding payloadInfo->len : %d\n",payloadInfo->len));
	}
	else
	{
		CL_DEBUG_PRINT(CL_DEBUG_INFO,("Node already exist\n"));
        clHeapFree(payloadInfo);
        clHeapFree(pCntKey);
	}
	clOsalMutexUnlock(gClAlarmPayloadCntMutex);
	return rc;
}

/*
 * This api would be called as and when the object is no more and the corresponding entry 
 * within the container needs to be deleted.
 */
ClRcT clAlarmPayloadCntDelete(ClCorMOIdPtrT pMoId, ClAlarmProbableCauseT probCause, ClAlarmSpecificProblemT specificProblem)
{
	ClRcT rc = CL_OK;
	ClCntNodeHandleT nodeH;
    ClAlarmPayloadCntKeyT *pCntKey = clHeapAllocate(sizeof(ClAlarmPayloadCntKeyT));
    if(NULL == pCntKey)
    {
          CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,("Memory allocation failed with rc 0x%x ", rc));
          return (rc);
    }    
    pCntKey->probCause = probCause;
    pCntKey->specificProblem = specificProblem;
    pCntKey->moId = *pMoId;

    clOsalMutexLock(gClAlarmPayloadCntMutex);        
    rc = clCntNodeFind(gPayloadCntHandle,(ClCntKeyHandleT)pCntKey,&nodeH);
	if(CL_OK == rc)
	{
    	rc = clCntAllNodesForKeyDelete(gPayloadCntHandle,(ClCntKeyHandleT)pCntKey);
    	if (CL_OK != rc)
	    {
        	CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCntAllNodesForKeyDelete failed w rc:0x%x\n", rc));
	    }        
	}
	else
	{
		CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Node does not exist with rc:0x%x\n", rc));
	}
    clHeapFree(pCntKey);
	clOsalMutexUnlock(gClAlarmPayloadCntMutex);    
	return rc;
}

/*
 * The cnt data get api
 */

ClRcT clAlarmPayloadCntDataGet(ClAlarmProbableCauseT probCause, 
                                    ClAlarmSpecificProblemT specificProblem,
                                    ClCorMOIdPtrT pMoId, 
                                    ClAlarmPayloadCntT **payloadInfo)
{
	ClRcT rc = CL_OK;
	ClCntNodeHandleT nodeH;
    ClAlarmPayloadCntKeyT *pCntKey = clHeapAllocate(sizeof(ClAlarmPayloadCntKeyT));
    if(NULL == pCntKey)
    {
          CL_DEBUG_PRINT (CL_DEBUG_CRITICAL,("Memory allocation failed with rc 0x%x ", rc));
          return (rc);
    }    
    pCntKey->probCause = probCause;
    pCntKey->specificProblem = specificProblem;
    pCntKey->moId = *pMoId;

    clOsalMutexLock(gClAlarmPayloadCntMutex);        
    rc = clCntNodeFind(gPayloadCntHandle,(ClCntKeyHandleT)pCntKey,&nodeH);
	if(CL_OK == rc)
	{
        rc = clCntDataForKeyGet(gPayloadCntHandle, (ClCntKeyHandleT)pCntKey,
									(ClCntDataHandleT*)payloadInfo);
        if (CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clCntDataForKeyGet failed with rc 0x%x\n", rc));
        }
    }
	else
	{
		CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Node does not exist with rc:0x%x\n", rc));
	}
    clHeapFree(pCntKey);
    clOsalMutexUnlock(gClAlarmPayloadCntMutex);    
	return rc;
}

/*
 * This api would be called when alarm library is being finalize
 */
ClRcT clAlarmPayloadCntDeleteAll(void)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(gClAlarmPayloadCntMutex);
    rc = clCntDelete(gPayloadCntHandle);
    clOsalMutexUnlock(gClAlarmPayloadCntMutex);    
    clOsalMutexDelete(gClAlarmPayloadCntMutex);

    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("container deletion failed [%x]",rc));
        return rc;
    }
	return rc;
}
