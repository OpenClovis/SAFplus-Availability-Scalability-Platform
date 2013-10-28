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

/**
 * This file implements the boot manager.
 */

/*
 * Standard header files 
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * ASP header files 
 */
#include <clCommon.h>
#include <clCommonErrors.h>

#include <clDebugApi.h>
#include <clLogApi.h>
#include <clIocErrors.h>
#include <clOsalApi.h>
#include <clParserApi.h>
#include <clOsalErrors.h>
#include <clClmApi.h>

/*
 * CPM Internal header files 
 */
#include <clCpmInternal.h>
#include <clCpmCkpt.h>
#include <clCpmGms.h>
#include <clCpmLog.h>
#include <clCpmClient.h>
#include <clCpmCor.h>
#include <clCpmIpi.h>

/*
 * XDR header files 
 */
#include "xdrClCpmBootOperationT.h"
#include "xdrClCpmBmSetLevelResponseT.h"

/*
 * Bug 3610: Added fields
 * 1. abandoningBootLevel: Indicates if current boot level
 * has been abandoned.
 * 2. compTerminationCount: Keep count of number of components
 * to whom termination request has been sent while abandoning 
 * current boot level.
 */
static ClUint32T abandoningBootLevel = CL_FALSE;
static ClUint32T compTerminationCount = 0;
static ClInt32T bmRegRetries = 0;

#define CL_CPM_COR_OBJ_CREATE_RETRIES   5

/*
 * Forward Declaration, to keep compiler happy. 
 */
static ClRcT cpmBmStartup(cpmBMT *bmTable);
static ClRcT cpmBmStartNextLevel(cpmBMT *bmTable);
static ClRcT cpmBmStopCurrentLevel(cpmBMT *bmTable);

/*
 * Sets the bootlevel appropriately. 
 */
static ClRcT cpmBmSetLevel(cpmBMT *bmTable, ClUint32T bootLevel);

/*
 * Starts the CPM-BM thread. 
 */
static void *bmInitialize(void *threadArg);

/*
 * Creates and starts the CPM-BM thread. 
 */
ClRcT cpmBmInitialize(ClOsalTaskIdT *pTaskId, ClEoExecutionObjT *pThis)
{
    ClRcT rc = CL_OK;

    ClUint32T priority  = CL_OSAL_THREAD_PRI_NOT_APPLICABLE;
    ClUint32T stackSize = 0;

    rc = clOsalTaskCreateAttached("cpmBm",
                                  CL_OSAL_SCHED_OTHER,
                                  priority,
                                  stackSize,
                                  bmInitialize,
                                  pThis,
                                  pTaskId);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_TASK_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

  failure:
    return rc;
}

static ClRcT cpmGetMasterAddrIfRequired(ClIocNodeAddressT *nodeAddress)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_NULL_POINTER);

    if (!nodeAddress)
        goto failure;
    
    if (CPM_RESPOND_TO_ACTIVE == *nodeAddress)
    {
        rc = clCpmMasterAddressGet(nodeAddress);
        if (CL_OK != rc)
            goto failure;
    }

    return CL_OK;
    
 failure:
    return rc;
}
        
/*
 * Starts the CPM-BM thread. Remember, we can not exit from this thread till 
 * the CPM finalize is called.
 */
static void *bmInitialize(void *threadArg)
{
    ClRcT rc = CL_OK;

    ClEoExecutionObjT *pThis = (ClEoExecutionObjT *) threadArg;

    ClTimerTimeOutT timeOut = { 0, 0 };

    ClUint32T queueSize = 0;    /* number of elements in the set request queue 
                                 */
    ClCpmBootOperationT *pBootOp = NULL;    /* boot level will be sent to this 
                                             * variable */
    ClCpmBmSetLevelResponseT setLevelResponse = {{0}};

    rc = clEoMyEoIocPortSet(pThis->eoPort);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_IOC_MY_EO_IOC_PORT_GET_ERR, rc,
                   rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clEoMyEoObjectSet(pThis);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_EO_MY_OBJ_SET_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

#if 0
    display(gpClCpm->bmTable);
#endif

    /*
     * Boot up until default boot level is reached.  
     */
    rc = cpmBmStartup(gpClCpm->bmTable);
    if (rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                   CL_CPM_LOG_2_BM_START_ERR,
                   gpClCpm->bmTable->defaultBootLevel, rc);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Booting up process failure %x, not aborting at this point \n",
                        rc));
        cpmSelfShutDown();
    }

    while (gpClCpm->cpmShutDown == CL_TRUE)
    {
        rc = clOsalMutexLock(gpClCpm->bmTable->bmQueueCondVarMutex);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to Lock %x\n", rc));
            continue;
        }

        queueSize = 0;
        rc = clQueueSizeGet(gpClCpm->bmTable->setRequestQueueHead, &queueSize);
        if (rc != CL_OK)
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Unable to get queue Size %x\n", rc));

        /*
         * Wait indefinitely for the signal.
         */
        if (queueSize != 0 ||
            ((rc =
              clOsalCondWait(gpClCpm->bmTable->bmQueueCondVar,
                             gpClCpm->bmTable->bmQueueCondVarMutex,
                             timeOut)) == CL_OK))
        {
            /*
             * FIXME: Needs attention 
             */

            rc = clQueueSizeGet(gpClCpm->bmTable->setRequestQueueHead,
                                &queueSize);
            if (rc != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("Unable to get queue Size %x\n", rc));

            rc = clOsalMutexUnlock(gpClCpm->bmTable->bmQueueCondVarMutex);
            if (rc != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to unlock %x\n", rc));
            
            while (queueSize != 0)
            {
                /*
                 * BUG 3610:
                 * Added if condition to check for shutDown flag
                 * If shutdown flag is set, should not wait for other messages 
                 * in queue to be processed. Directly go to shutdown
                 */
                if(!gpClCpm->cpmShutDown)
                {
                    /* Go out of Upper while (check for shutdown flag loop)*/
                    CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Got Shutdown, Breaking out of while loop"));
                    break;
                }

                rc = clOsalMutexLock(gpClCpm->bmTable->bmQueueCondVarMutex);
                if (rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to Lock %x\n", rc));
                    continue;
                }
                
                rc = clQueueNodeDelete(gpClCpm->bmTable->setRequestQueueHead, 
                            (ClQueueDataT *)&pBootOp);   
                if(rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                            ("Unable to delete Queue node %x\n", rc));
                    continue;
                }
                rc = clOsalMutexUnlock(gpClCpm->bmTable->bmQueueCondVarMutex);
                if (rc != CL_OK)
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to unlock %x\n", rc));

                rc = cpmBmSetLevel(gpClCpm->bmTable, pBootOp->bootLevel);
                if (rc != CL_OK)
                {
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                               CL_CPM_LOG_2_BM_SET_LEVEL_ERR,
                               pBootOp->bootLevel, rc);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                   ("Unable to set boot level\n"));
                }
                if (pBootOp->srcAddress.portId != 0)
                {
                    ClRcT rc2 = cpmGetMasterAddrIfRequired(&pBootOp->
                                                    srcAddress.nodeAddress);
                    if (CL_OK != rc2)
                    {
                        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                                      "Master has died/not accessible "
                                      "when I am booting."
                                      "Getting master address returned "
                                      "error [%#x] doing self shutdown...",
                                      rc2);
                        cpmSelfShutDown();
                    }
                    
                    if(pBootOp->rmdNumber == CPM_NODE_GO_BACK_TO_REG)
                    {
                         ClTimerTimeOutT timeOut = {5, 0};
                        
                        /* Introducing a delay to fix the problem when
                         *  GMS on active system controller is killed
                         *  and there is no standby system
                         *  controller. Please see Bug 7285 for more.
                         */

                        clOsalTaskDelay(timeOut);

                        /*
                         * We block here on the registration with active
                         * This would block the BM thread instead of
                         * blocking the worker threads. The behaviour
                         * obtained would be pretty similar to what
                         * happens during worker node ASP startup which
                         * blocks the BM thread waiting for the active
                         * to come up. This would also make CPM responsive
                         * to debug CLI requests or other requests
                         * just like the startup phase.
                         */
                        if(gpClCpm->bmTable->currentBootLevel != CL_CPM_BOOT_LEVEL_2)
                        {
                            clLogWarning("REG", "ACT", 
                                         "Resetting boot level from [%d] to [%d] before "
                                         "trying to register with active",
                                         gpClCpm->bmTable->currentBootLevel,
                                         CL_CPM_BOOT_LEVEL_2);
                            gpClCpm->bmTable->currentBootLevel = CL_CPM_BOOT_LEVEL_2;
                        }
                        cpmRegisterWithActive();
                    }
                    else
                    {
                        setLevelResponse.bootLevel = pBootOp->bootLevel;
                        setLevelResponse.retCode = rc;
                        strcpy(setLevelResponse.nodeName.value, 
                               gpClCpm->pCpmLocalInfo->nodeName);
                        setLevelResponse.nodeName.length = 
                            strlen(gpClCpm->pCpmLocalInfo->nodeName);
                        /*
                         * send response to the caller 
                         */
                        rc = CL_CPM_CALL_RMD_ASYNC_NEW(pBootOp->srcAddress.
                                                       nodeAddress,
                                                       pBootOp->srcAddress.portId,
                                                       pBootOp->rmdNumber,
                                                       (ClUint8T *)
                                                       &setLevelResponse,
                                                       sizeof
                                                       (ClCpmBmSetLevelResponseT),
                                                       NULL,
                                                       NULL,
                                                       CL_RMD_CALL_ATMOST_ONCE,
                                                       0,
                                                       0,
                                                       0,
                                                       NULL,
                                                       NULL,
                                                       MARSHALL_FN(ClCpmBmSetLevelResponseT, 4, 0, 0));
                        if (rc != CL_OK)
                            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                           ("Response call failed. %x\n", rc));
                    }
                }
                
                rc = clQueueSizeGet(gpClCpm->bmTable->setRequestQueueHead,
                                    &queueSize);
                if (rc != CL_OK)
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                   ("Unable to get queue Size %x\n", rc));

                clHeapFree(pBootOp);
            } /* end while(queueSize !=0) */

        } /* end if clOsalCondWait */
        else
        {
            rc = clOsalMutexUnlock(gpClCpm->bmTable->bmQueueCondVarMutex);
            if (rc != CL_OK)
                CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Unable to unlock %x\n", rc));

            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("CondWait failed.\n"));
        }
    } /* end of while(gpClCpm->cpmShutDown) */

    /*
     * BUG 3610:
     * Added if condition.
     * We will be out of while loop only if shutDown is set CL_FALSE
     * Check for Boot level and if it is not 0, do shutdown
     */
    if(gpClCpm->bmTable->currentBootLevel != CL_CPM_BOOT_LEVEL_0) 
    {
        CL_DEBUG_PRINT(CL_DEBUG_INFO,
                ("Out of while loop, shutting down the node...\n"));
        rc = cpmBmSetLevel(gpClCpm->bmTable, CL_CPM_BOOT_LEVEL_0);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                       CL_CPM_LOG_2_BM_SET_LEVEL_ERR,
                       CL_CPM_BOOT_LEVEL_0, rc);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Unable to set boot level while shutting down\n"));
        }
    }
    
    gpClCpm->cpmShutDown = CL_TRUE;

  failure:
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, NULL,
               CL_CPM_LOG_0_BM_THREAD_RET_INFO);
    return NULL;
}

/*
 * Dummy queue callback functions.
 */
static void userDequeueCallBack(ClQueueDataT userData)
{
    ;
}

static void userDestroyCallback(ClQueueDataT userData)
{
    ;
}

/*
 * Allocates and initializes the CPM-BM submodule data structure.
 */
ClRcT cpmBmInitDS(void)
{
    ClRcT rc = CL_OK;

    cpmBMT *cpmBmTable = NULL;

    ClTimerTimeOutT timeOut = {0};

    timeOut.tsSec = CL_RMD_DEFAULT_TIMEOUT/1000 * CL_RMD_DEFAULT_RETRIES + 10;
    timeOut.tsMilliSec = 0;

    gpClCpm->bmTable = cpmBmTable = (cpmBMT *) clHeapAllocate(sizeof(cpmBMT));
    if (cpmBmTable == NULL)
    {
        CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                       CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                       CL_LOG_HANDLE_APP);
    }

    rc = clOsalCondCreate(&cpmBmTable->lcmCondVar);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_COND_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clOsalMutexCreate(&cpmBmTable->lcmCondVarMutex);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    cpmBmTable->countComponent = 0;

    rc = clOsalCondCreate(&cpmBmTable->bmQueueCondVar);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_COND_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clOsalMutexCreate(&cpmBmTable->bmQueueCondVarMutex);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clQueueCreate(0, userDequeueCallBack, userDestroyCallback,
                       &gpClCpm->bmTable->setRequestQueueHead);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_QUEUE_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clTimerCreate(timeOut,
                       CL_TIMER_ONE_SHOT,
                       CL_TIMER_SEPARATE_CONTEXT,
                       cpmBmRespTimerCallback,
                       NULL,
                       &gpClCpm->bmTable->bmRespTimer);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "Failed to create timer, error [%#x]",
                   rc);
        CL_ASSERT(0);
    }

    cpmBmTable->maxBootLevel = 0;
    cpmBmTable->defaultBootLevel = 0;
    cpmBmTable->currentBootLevel = CL_CPM_BOOT_LEVEL_0;
    cpmBmTable->numComponent = 0;
    cpmBmTable->lastBootStatus = CL_FALSE;

    cpmBmTable->table = NULL;

    return CL_OK;

  failure:
    return rc;
}

#if 0
void display(
    cpmBMT *ptable
)
{
    /*
     * Display contents of top level structure 
     */
    /*
     * Display contents of table 
     */

    bootTableT *p;
    bootRowT *q;

    /*
     * Cannot be NULL 
     */
    p = ptable->table;

    while (p != NULL)
    {
        clOsalPrintf("Boot Level : %d\n", p->bootLevel);
        clOsalPrintf("Number of components : %d\n", p->numComp);
        q = p->listHead;
        clOsalPrintf("Component(s) :");
        while (q != NULL)
        {
            clOsalPrintf(" %s", q->compName);
            q = q->pNext;
        }
        clOsalPrintf("\n");
        p = p->pDown;
    }
}

void show(
    cpmBMT *cpmBmTable
)
{
    /*
     * display contents of part of cpmBM data structure 
     */
    /*
     * showing booted up components 
     */

    /*
     * may be useful while debugging 
     */

    bootTableT *p = NULL;
    bootRowT *q = NULL;

    if (cpmBmTable->currentBootLevel == CL_CPM_BOOT_LEVEL_0)
    {
        clOsalPrintf("There are no booted components at present.\n");
        return;
    }

    p = cpmBmTable->table;      /* Cannot be NULL */

    while (p != NULL)
    {
        clOsalPrintf("Boot Level : %d\n", p->bootLevel);
        q = p->listHead;
        clOsalPrintf("Component(s) :");
        while (q != NULL)
        {
            clOsalPrintf(" %s", q->compName);
            q = q->pNext;
        }
        clOsalPrintf("\n");
        if (p->bootLevel == cpmBmTable->currentBootLevel)
            break;
        p = p->pDown;
    }
}
#endif

/*
 * Boots up all the components in upto and including
 * the default boot level.
 */
static ClRcT cpmBmStartup(cpmBMT *cpmBmTable)
{
    ClUint32T rc = CL_OK;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Booting up...\n"));

    if (cpmBmTable->defaultBootLevel > cpmBmTable->currentBootLevel)
    {
        cpmBmTable->lastBootStatus = CL_TRUE;
        /* By default, go till 2 only */
        rc = cpmBmSetLevel(gpClCpm->bmTable, CL_CPM_BOOT_LEVEL_2);
        CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_BM_START_ERR,
                       CL_CPM_BOOT_LEVEL_2, rc, rc, CL_LOG_DEBUG,
                       CL_LOG_HANDLE_APP);
    }
    else
    {
        /*
         * Do not boot any components. Just set the current boot level to 0 
         */
        /*
         * cpmBmTable->currentBootLevel = cpmBmTable->defaultBootLevel;
         */
        /*
         * booting is always successful 
         */
        cpmBmTable->lastBootStatus = CL_TRUE;
    }

    return CL_OK;               /* We have reached the default boot level. */

  failure:
    return rc;
}

/*
 * Checks if booting up request of service unit or component
 * was successful and updates the the number of components
 * booted accordingly. Provides a guaranteed response to the
 * CPM-BM thread.
 */
void *cpmBMResponse(ClCpmLcmResponseT *response)
{
    ClUint32T rc = CL_OK;

    cpmBMT *cpmBmTable = gpClCpm->bmTable;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("ENTERED FUNCTION: "
                "Request type [%d], retcode [0x%x], abandoningBootLevel = [%d]", 
                response->requestType, response->returnCode, 
                abandoningBootLevel));
    /*
     * Bug 3610:
     * Added abandoningBootLevel if condition
     */
    if (abandoningBootLevel == CL_TRUE)
    {
        if (response->requestType == CL_CPM_TERMINATE)
        {
            if (response->returnCode != CL_OK)
            {
                rc = _cpmComponentCleanup(response->name,
                                          NULL,
                                          gpClCpm->pCpmConfig->nodeName,
                                          NULL, 0, CL_CPM_CLEANUP);
                if (rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                            ("Unable to cleanup the component [%s], rc =[0x%x]\n",
                             response->name, rc));
                }
            }
            
            rc = clOsalMutexLock(cpmBmTable->lcmCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc, rc,
                    CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            
            compTerminationCount--;
            
            rc = clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR, rc,
                    rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            if (compTerminationCount == 0)
            {
                rc = clOsalCondSignal(cpmBmTable->lcmCondVar);
                CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_COND_SIGNAL_ERR,
                        rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
        }
    }
    else if (response->returnCode == CL_OK)
    {
        rc = clOsalMutexLock(cpmBmTable->lcmCondVarMutex);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

        cpmBmTable->countComponent++;

        rc = clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR, rc,
                       rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

        if (cpmBmTable->countComponent == cpmBmTable->numComponent)
        {
            rc = clOsalCondSignal(cpmBmTable->lcmCondVar);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_COND_SIGNAL_ERR,
                           rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
    }
    else
    {
        if ((response->requestType == CL_CPM_TERMINATE) && 
                (gpClCpm->cpmShutDown == CL_FALSE))
        {
            rc = clOsalMutexLock(cpmBmTable->lcmCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc,
                           rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            cpmBmTable->countComponent++;

            rc = clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR,
                           rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            if (cpmBmTable->countComponent == cpmBmTable->numComponent)
            {
                rc = clOsalCondSignal(cpmBmTable->lcmCondVar);
                CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_COND_SIGNAL_ERR,
                        rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
        } /* end if requestType CL_CPM_TERMINATE and cpmShutDown is FALSE */ 
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_WARN, ("Request Type [%d], failed", response->requestType));
            clOsalMutexLock(cpmBmTable->lcmCondVarMutex);
            clOsalCondSignal(cpmBmTable->lcmCondVar);
            clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_COND_SIGNAL_ERR, rc,
                    rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
    }/* end else if cpmBMResponse returnCode is not CL_OK */

  failure:
    return NULL;
}

/*
 * Deallocates the CPM-BM submodule data structure.
 */
ClRcT cpmBmCleanupDS(void)
{
    ClRcT rc = CL_OK;

    cpmBMT *cpmBmTable = gpClCpm->bmTable;

    /*
     * Temporary pointers for doubly linked list.
     */
    bootTableT *pDList = NULL;
    bootTableT *tDList = NULL;

    /*
     * Temporary pointers for singly linked list.
     */
    bootRowT *qList = NULL;
    bootRowT *rList = NULL;

    rc = clOsalCondDelete(cpmBmTable->lcmCondVar);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_COND_DELETE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clOsalMutexDelete(cpmBmTable->lcmCondVarMutex);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_DELETE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clOsalCondDelete(cpmBmTable->bmQueueCondVar);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_COND_DELETE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clOsalMutexDelete(cpmBmTable->bmQueueCondVarMutex);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_DELETE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clQueueDelete(&cpmBmTable->setRequestQueueHead);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_QUEUE_DELETE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    clTimerDelete(&gpClCpm->bmTable->bmRespTimer);
    gpClCpm->bmTable->bmRespTimer = CL_HANDLE_INVALID_VALUE;

    pDList = cpmBmTable->table;

    while (pDList != NULL)
    {
        /*
         * Freeing a doubly linked list 
         */
        qList = pDList->listHead;
        while (qList != NULL)
        {
            /*
             * Freeing a linked list 
             */
            rList = qList->pNext;
            clHeapFree(qList);
            qList = NULL;
            qList = rList;
        }
        tDList = pDList->pDown;
        clHeapFree(pDList);
        pDList = NULL;
        pDList = tDList;
    }

    rList = NULL;
    qList = NULL;
    tDList = NULL;
    pDList = NULL;

    clHeapFree((void *) cpmBmTable);
    cpmBmTable = NULL;

    return CL_OK;

  failure:
    return rc;
}

/*
 * Returns the current boot level.
 */
ClRcT VDECL(cpmBootLevelGet)(ClEoDataT data,
                             ClBufferHandleT inMsgHandle,
                             ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;

    ClCpmBootOperationT bootOp;
    ClUint32T currentBootLevel = CL_CPM_BOOT_LEVEL_0;

    rc = VDECL_VER(clXdrUnmarshallClCpmBootOperationT, 4, 0, 0)(inMsgHandle, (void *) &bootOp);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    if (!strcmp(bootOp.nodeName.value, gpClCpm->pCpmLocalInfo->nodeName))
    {
        currentBootLevel = gpClCpm->bmTable->currentBootLevel;

        rc = clXdrMarshallClUint32T((void *) &currentBootLevel, outMsgHandle,
                                    0);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        ClCpmLT *cpmL = NULL;
        ClUint32T size = sizeof(ClUint32T);
        clOsalMutexLock(gpClCpm->cpmTableMutex);
        rc = cpmNodeFindLocked(bootOp.nodeName.value, &cpmL);
        if(rc != CL_OK)
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            clLogError("CPM", "BOOT", "Unable to find node [%s]. Failure code [%#x]", 
                       bootOp.nodeName.value, rc);
            goto failure;
        }
        if (cpmL->pCpmLocalInfo != NULL &&
            cpmL->pCpmLocalInfo->status != CL_CPM_EO_DEAD)
        {
            ClIocPhysicalAddressT destNode = cpmL->pCpmLocalInfo->cpmAddress;
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            rc = CL_CPM_CALL_RMD_SYNC_NEW(destNode.nodeAddress,
                                          destNode.portId,
                                          CPM_BM_GET_CURRENT_LEVEL,
                                          (ClUint8T *) &bootOp,
                                          sizeof(ClCpmBootOperationT),
                                          (ClUint8T *) &(currentBootLevel),
                                          &size,
                                          (CL_RMD_CALL_NEED_REPLY),
                                          0,
                                          0,
                                          0,
                                          MARSHALL_FN(ClCpmBootOperationT, 4, 0, 0),
                                          clXdrUnmarshallClUint32T);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_RMD_CALL_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            rc = clXdrMarshallClUint32T((void *) &currentBootLevel,
                                        outMsgHandle, 0);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
        else
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            rc = CL_CPM_RC(CL_CPM_ERR_FORWARDING_FAILED);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_SERVER_FORWARD_ERR, rc,
                           rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
    }

    return CL_OK;

  failure:
    return rc;
}

/*
 * Sets the boot level to the required boot level.
 */
ClRcT VDECL(cpmBootLevelSet)(ClEoDataT data,
                             ClBufferHandleT inMsgHandle,
                             ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;

    ClCpmBootOperationT *bootOp = NULL;

    bootOp = (ClCpmBootOperationT *) clHeapAllocate(sizeof(ClCpmBootOperationT));
    if (bootOp == NULL)
    {
        CL_CPM_CHECK_0(CL_DEBUG_ERROR,
                       CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED,
                       CL_CPM_RC(CL_ERR_NO_MEMORY), CL_LOG_DEBUG,
                       CL_LOG_HANDLE_APP);
    }
    rc = VDECL_VER(clXdrUnmarshallClCpmBootOperationT, 4, 0, 0)(inMsgHandle, (void *) bootOp);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    if (!strcmp(bootOp->nodeName.value, gpClCpm->pCpmLocalInfo->nodeName))
    {
        rc = clOsalMutexLock(gpClCpm->bmTable->bmQueueCondVarMutex);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

        rc = clQueueNodeInsert(gpClCpm->bmTable->setRequestQueueHead,
                               (ClQueueDataT) bootOp);
        if (rc != CL_OK)
        {
            clOsalMutexUnlock(gpClCpm->bmTable->bmQueueCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_QUEUE_INSERT_ERR,
                           CL_CPM_RC(rc), CL_CPM_RC(rc), CL_LOG_DEBUG,
                           CL_LOG_HANDLE_APP);
        }

        rc = clOsalCondSignal(gpClCpm->bmTable->bmQueueCondVar);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_COND_SIGNAL_ERR, rc,
                       rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

        rc = clOsalMutexUnlock(gpClCpm->bmTable->bmQueueCondVarMutex);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR, rc,
                       rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    }
    else
    {
        ClCpmLT *cpmL = NULL;
        clOsalMutexLock(gpClCpm->cpmTableMutex);
        rc = cpmNodeFindLocked(bootOp->nodeName.value, &cpmL);
        if(rc != CL_OK)
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            clLogError("CPM", "BOOT", "Node [%s] not found. Failure code [%#x]",
                       bootOp->nodeName.value, rc);
            goto failure;
        }
        if (cpmL->pCpmLocalInfo != NULL &&
            cpmL->pCpmLocalInfo->status != CL_CPM_EO_DEAD)
        {
            ClIocPhysicalAddressT destNode = cpmL->pCpmLocalInfo->cpmAddress;
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            rc = CL_CPM_CALL_RMD_ASYNC_NEW(destNode.nodeAddress,
                                           destNode.portId,
                                           CPM_BM_SET_LEVEL,
                                           (ClUint8T *) bootOp,
                                           sizeof(ClCpmBootOperationT),
                                           NULL,
                                           NULL,
                                           0,
                                           0,
                                           0,
                                           CL_IOC_HIGH_PRIORITY,
                                           NULL,
                                           NULL,
                                           MARSHALL_FN(ClCpmBootOperationT, 4, 0, 0));
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_RMD_CALL_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
        else
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            rc = CL_CPM_RC(CL_CPM_ERR_FORWARDING_FAILED);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_SERVER_FORWARD_ERR, rc,
                           rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
        clHeapFree(bootOp);
    }

    return CL_OK;

  failure:
    return rc;
}

/*
 * Returns the maximum boot level.
 */
ClRcT VDECL(cpmBootLevelMax)(ClEoDataT data,
                             ClBufferHandleT inMsgHandle,
                             ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmBootOperationT bootOp;
    ClUint32T maxBootLevel = 0;

    rc = VDECL_VER(clXdrUnmarshallClCpmBootOperationT, 4, 0, 0)(inMsgHandle, (void *) &bootOp);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    if (!strcmp(bootOp.nodeName.value, gpClCpm->pCpmLocalInfo->nodeName))
    {
        maxBootLevel = gpClCpm->bmTable->maxBootLevel;

        rc = clXdrMarshallClUint32T((void *) &maxBootLevel, outMsgHandle, 0);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        ClCpmLT *cpmL = NULL;
        ClUint32T size = sizeof(ClUint32T);
        clOsalMutexLock(gpClCpm->cpmTableMutex);
        rc = cpmNodeFindLocked(bootOp.nodeName.value, &cpmL);
        if(rc != CL_OK)
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            clLogError("CPM", "BOOT", "Node [%s] not found. Failure code [%#x]",
                       bootOp.nodeName.value, rc);
            goto failure;
        }
        if (cpmL->pCpmLocalInfo != NULL &&
            cpmL->pCpmLocalInfo->status != CL_CPM_EO_DEAD)
        {
            ClIocPhysicalAddressT destNode = cpmL->pCpmLocalInfo->cpmAddress;
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            rc = CL_CPM_CALL_RMD_SYNC_NEW(destNode.nodeAddress,
                                          destNode.portId,
                                          CPM_BM_GET_MAX_LEVEL,
                                          (ClUint8T *) &bootOp,
                                          sizeof(ClCpmBootOperationT),
                                          (ClUint8T *) &(maxBootLevel),
                                          &size,
                                          (CL_RMD_CALL_NEED_REPLY),
                                          0,
                                          0,
                                          0,
                                          MARSHALL_FN(ClCpmBootOperationT, 4, 0, 0),
                                          clXdrUnmarshallClUint32T);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_RMD_CALL_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            rc = clXdrMarshallClUint32T((void *) &maxBootLevel, outMsgHandle,
                                        0);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
        else
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            rc = CL_CPM_RC(CL_CPM_ERR_FORWARDING_FAILED);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_SERVER_FORWARD_ERR, rc,
                           rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
    }

    return CL_OK;

  failure:
    return rc;
}

/*
 * Sets the bootlevel appropriately.
 */
static ClRcT cpmBmSetLevel(cpmBMT *bmTable, ClUint32T bootLevel)
{
    ClRcT rc = CL_OK;

    cpmBMT *cpmBmTable = gpClCpm->bmTable;

    if (cpmBmTable->currentBootLevel < bootLevel)
    {
        if (bootLevel > cpmBmTable->maxBootLevel)
        {
            while (cpmBmTable->currentBootLevel != cpmBmTable->maxBootLevel)
            {
                rc = cpmBmStartNextLevel(cpmBmTable);
                if (rc == CL_OK)
                    ;
                else
                {
                    CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                                   CL_CPM_LOG_2_BM_SET_LEVEL_ERR, bootLevel, rc,
                                   rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
                }
            }
        }
        else
        {
            while (cpmBmTable->currentBootLevel != bootLevel)
            {
                rc = cpmBmStartNextLevel(cpmBmTable);
                if (rc == CL_OK)
                {
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, NULL,
                               CL_CPM_LOG_1_BM_SET_LEVEL_INFO,
                               gpClCpm->bmTable->currentBootLevel);
                }
                else
                {
                    CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                                   CL_CPM_LOG_2_BM_SET_LEVEL_ERR, bootLevel, rc,
                                   rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
                }
            }
        }
    }
    else if (cpmBmTable->currentBootLevel > bootLevel)
    {
        while (cpmBmTable->currentBootLevel != bootLevel)
        {
            rc = cpmBmStopCurrentLevel(cpmBmTable);
            if (rc != CL_OK)
            {
                CL_CPM_CHECK_2(CL_DEBUG_ERROR,
                        CL_CPM_LOG_2_BM_SET_LEVEL_ERR, bootLevel, rc,
                        rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
        }
    }
    else
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("setlevel argument and current boot level [%d] are the same.\n",
                        cpmBmTable->currentBootLevel));
    }

    return CL_OK;

  failure:
    return rc;
}

/*
 * Bug 3610: Added the function cpmBmAbandonCurrentLevel.
 * The current boot process is abandoned and the components already
 * processed are terminated or cleaned up.
 * As SUs processing was halted in the middle, function need to look 
 * at the component level. Component status is either 
 * INSTANTIATED or INSTANTIATING. Terminate the instantiated
 * components and cleanup the instantiating components.
 * PARAMS:
 *  pDList is the list of components at the current boot level.
 */
static ClRcT cpmBmAbandonCurrentLevel(bootTableT *pDList)
{
    ClRcT rc = CL_OK;
    ClTimerTimeOutT timeOut = {0, 0};

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Entered function...\n"));

    abandoningBootLevel = CL_TRUE;

    clOsalMutexLock(gpClCpm->bmTable->lcmCondVarMutex);

    if (compTerminationCount != 0)
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
                ("CondWait for components response...\n"));
        if ((rc = clOsalCondWait(gpClCpm->bmTable->lcmCondVar, 
                        gpClCpm->bmTable->lcmCondVarMutex, timeOut)) == CL_OK)
        {
            clOsalMutexUnlock(gpClCpm->bmTable->lcmCondVarMutex);
            abandoningBootLevel = CL_FALSE;
            return CL_OK;
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("CondWait failed, rc=[0x%x]\n", rc));
            clOsalMutexUnlock(gpClCpm->bmTable->lcmCondVarMutex);
            abandoningBootLevel = CL_FALSE;
            return rc;
        }
    }
    clOsalMutexUnlock(gpClCpm->bmTable->lcmCondVarMutex);

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, 
            ("Returning CL_OK...\n"));

    abandoningBootLevel = CL_FALSE;
    return CL_OK;

}

ClRcT cpmBmRespTimerCallback(ClPtrT unused)
{
    ClRcT rc = CL_OK;
    
    clOsalMutexLock(&gpClCpm->clusterMutex);
    clOsalMutexLock(&gpClCpm->cpmMutex);

    if (2 == gpClCpm->bmTable->currentBootLevel)
    {
        if(++bmRegRetries >= 3
           ||
           !gpClCpm->polling 
           ||
           !gpClCpm->pCpmLocalInfo
           ||
           CL_CPM_IS_ACTIVE())
        {
            clLogMultiline(CL_LOG_CRITICAL,
                           CPM_LOG_AREA_CPM,
                           CPM_LOG_CTX_CPM_BOOT,
                           "CPM/G active did not respond to my "
                           "registration request -- \n"
                           "This indicates that there is some communication "
                           "problem between this node and the CPM/G active. \n"
                           "If you are running only one node (i.e. this node) "
                           "then the problem is more severe. \n"
                           "Shutting down...");
            cpmSelfShutDown();
        }
        else
        {
            ClCpmLocalInfoT localInfo = {{0}};
            memcpy(&localInfo, gpClCpm->pCpmLocalInfo, sizeof(localInfo));
            clOsalMutexUnlock(&gpClCpm->cpmMutex);
            clOsalMutexUnlock(&gpClCpm->clusterMutex);
            clLog(CL_LOG_CRITICAL,
                  CPM_LOG_AREA_CPM,
                  CPM_LOG_CTX_CPM_BOOT,
                  "CPM/G active did not respond to [%s] "
                  "registration request. Retrying again.",
                  localInfo.nodeName);
            rc = clCpmCpmLocalRegister(&localInfo);
            clOsalMutexLock(&gpClCpm->clusterMutex);
            clOsalMutexLock(&gpClCpm->cpmMutex);
            if(rc != CL_OK)
            {
                ClTimerTimeOutT delay = {.tsSec = 1, .tsMilliSec = 0};
                clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                           "CPM/G standby/worker blade registration with master failed with [%#x].  Restarting node", rc);
                cpmRestart(&delay, "registration");
            }
            else
            {
                cpmBmRespTimerStart();
            }
        }
    }
    else
    {
        bmRegRetries = 0;
    }

    clOsalMutexUnlock(&gpClCpm->cpmMutex);
    clOsalMutexUnlock(&gpClCpm->clusterMutex);

    return CL_OK;
}

void cpmBmRespTimerStop(void)
{
    ClRcT rc = CL_OK;

    bmRegRetries = 0;

    if (CL_HANDLE_INVALID_VALUE != gpClCpm->bmTable->bmRespTimer)
    {
        rc = clTimerStop(gpClCpm->bmTable->bmRespTimer);
        if (CL_OK != rc)
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                       "Unable to restart timer, error [%#x]",
                       rc);
        }
    }
}

void cpmBmRespTimerStart(void)
{
    ClRcT rc = CL_OK;
    
    if (CL_HANDLE_INVALID_VALUE != gpClCpm->bmTable->bmRespTimer)
    {
        rc = clTimerStart(gpClCpm->bmTable->bmRespTimer);
        if (CL_OK != rc)
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                       "Unable to restart timer, error [%#x]",
                       rc);
        }
    }
}

/*
 * Boots up the components at the next level.
 */
static ClRcT cpmBmStartNextLevel(cpmBMT *cpmBmTable)
{
    ClRcT rc = CL_OK;
    bootTableT *pDList = NULL;
    bootRowT *p = NULL;

    ClTimerTimeOutT timeOut = {0, 0};

    /*
     * Check if last boot up was a success. 
     */
    if (cpmBmTable->lastBootStatus == CL_FALSE)
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_STATE);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BM_SET_INVALID_STATE, rc,
                       rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }


    pDList = cpmBmTable->table;
    while (pDList != NULL)
    {
        if (pDList->bootLevel == cpmBmTable->currentBootLevel)
        {
            /*
             * Move to the next boot level. 
             */
            pDList = pDList->pDown;
            break;
        }
        pDList = pDList->pDown;
    }

    /*
     * Check if the booting is from halt state. 
     */
    if (pDList == NULL && cpmBmTable->currentBootLevel == CL_CPM_BOOT_LEVEL_0)
    {
        /*
         * Start component from first boot level. 
         */
        pDList = cpmBmTable->table;
    }

    if (pDList != NULL && pDList->numComp != 0)
    {
        /*
         * Move to the next boot level. 
         */
        /*
         * Bug 3610:
         * Boot level is set after we get response.
         */
        /* cpmBmTable->currentBootLevel = pDList->bootLevel; */
        cpmBmTable->numComponent = pDList->numComp;
        cpmBmTable->countComponent = 0;

        rc = clOsalMutexLock(cpmBmTable->lcmCondVarMutex);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

        p = pDList->listHead;
        while (p != NULL)
        {
            ClNameT compName={0}, nodeName={0};
            ClCpmLcmReplyT srcInfo = {0};

            strcpy(compName.value, p->compName);
            compName.length = strlen(compName.value);
            srcInfo.srcIocAddress = 0;
            srcInfo.srcPort = 0xFFFF;
            srcInfo.rmdNumber = 0;
            strcpy(nodeName.value, gpClCpm->pCpmConfig->nodeName);
            nodeName.length = strlen(nodeName.value);

            rc = clCpmComponentInstantiate(&compName, &nodeName, &srcInfo);
            if (rc != CL_OK)
            {
                clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
                CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_BM_SU_INST_ERR,
                               p->compName, rc, rc, CL_LOG_DEBUG,
                               CL_LOG_HANDLE_APP);
            }
            p = p->pNext;
        }

        /*
         * Conditinal wait for guaranteed response. 
         */

        /*
         * BUG 3610: Converted Cond wait to timed wait
         * Some Components may take long to come up. If there is shutDown 
         * request, it should not be delayed for long
         * Periodically check for pending shutDown.
         */
        timeOut.tsSec = 0;
        timeOut.tsMilliSec = CL_CPM_COMPONENT_DEFAULT_TIMEOUT * 2;
        while (((rc =
                 clOsalCondWait(cpmBmTable->lcmCondVar, cpmBmTable->lcmCondVarMutex,
                                timeOut)) == CL_OSAL_RC(CL_ERR_TIMEOUT)) && (gpClCpm->cpmShutDown == CL_TRUE))
        {
            continue;
        }

        /*
         * BUG 3610: Added if for handling shut down
         * If shutdown request is pending, Abandon the current boot process and start
         * booting Down the system.
         * As request for SUs Instantiation at current level has already been sent,
         * Terminate them or clean up. This can not be decided at SU level as some
         * components might have been instantiated while other may still be instantiating.
         * So do it at components level.
         */
        if (gpClCpm->cpmShutDown == CL_FALSE)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Out of TimedCondWait loop: Shutdown Flag is False"));
            
            rc = clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR,
                           rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            /*
             * Call the function to cleanup components
             * at this level.
             */
            rc = cpmBmAbandonCurrentLevel(pDList);

            return CL_CPM_RC(CL_CPM_ERR_OPER_ABANDONED);
        }

        if (rc == CL_OK)
        {
            /*
             * Bug 3610:
             * Setting the boot level after we got the response.
             */
            cpmBmTable->currentBootLevel = pDList->bootLevel;

            rc = clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR,
                           rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            if (cpmBmTable->countComponent == pDList->numComp)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Out of TimedCondWait loop: All SUs instantiated successfully"));
                /*
                 * Booting up of all components in this boot level 
                 * was a success. 
                 */
                cpmBmTable->lastBootStatus = CL_TRUE;
                cpmBmTable->countComponent = 0;
                rc = CL_OK;
            }
            else
            {
                /*
                 * Booting up of one of the components was a failure. 
                 */
                cpmBmTable->lastBootStatus = CL_FALSE;
                /*
                 * Set this to zero, else clOsalCondWait will block. 
                 */
                cpmBmTable->countComponent = 0;
                rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_FAILED);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("Out of TimedCondWait loop: Operation on one of the Service Unit with an error %x\n",
                                rc));
                CL_CPM_CHECK_1(CL_DEBUG_ERROR,
                               CL_CPM_LOG_1_SERVER_ERR_OPERATION_FAILED, rc, rc,
                               CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
        }
        else
        {
            /*
             * Bug 3610:
             * Cond wait failed. But we have already sent instantiation request
             * for this boot level. So set it here.
             */
            cpmBmTable->currentBootLevel = pDList->bootLevel;

            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Out of TimedCondWait loop: CondWait failed"));
            rc = clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR,
                           rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
    }
    else if (pDList != NULL && pDList->numComp == 0)
    {
        /*
         * Nothing to do. Simply move to the next boot level. 
         */
        cpmBmTable->currentBootLevel = pDList->bootLevel;
        cpmBmTable->lastBootStatus = CL_TRUE;
        cpmBmTable->numComponent = pDList->numComp;
        cpmBmTable->countComponent = 0;
        rc = CL_OK;
    }
    else
    {
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_FAILED);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BM_MAX_LEVEL_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

    if (gpClCpm->emUp && gpClCpm->emInitDone == 0)
    {
        rc = cpmEventInitialize();
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                       CL_CPM_LOG_1_EVT_CPM_OPER_ERR, rc);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("Failure in event Initialization %x\n", rc));
        }
        gpClCpm->emInitDone = 1;

        rc = clGmsCompUpNotify(0);
        if (rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                           ("Failed to notify GMS server about event up."));
        }
    }

    if (gpClCpm->pCpmConfig->cpmType == CL_CPM_LOCAL 
        && 
        gpClCpm->emUp == 1 
        &&
        gpClCpm->nodeEventPublished != 1 
        &&
        gpClCpm->bmTable->currentBootLevel == CL_CPM_BOOT_LEVEL_4)
    {
        ClNameT nodeName = {0};
        strcpy(nodeName.value, gpClCpm->pCpmConfig->nodeName);
        nodeName.length = strlen(gpClCpm->pCpmConfig->nodeName);
        /*
         * Node arrival event publish. 
         */
        rc = nodeArrivalDeparturePublish(clIocLocalAddressGet(), nodeName,
                                         CL_CPM_NODE_ARRIVAL);
        if (rc == CL_OK)
            gpClCpm->nodeEventPublished = 1;
        else
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                       CL_CPM_LOG_2_EVT_PUB_NODE_ARRIVAL_ERR, nodeName.value,
                       rc);
        }
    }
    
#ifdef CL_CPM_AMS
    /* CKPT must have come up Alive till now */
    if (cpmBmTable->currentBootLevel == CL_CPM_BOOT_LEVEL_4)
    {
        if(CL_CPM_IS_ACTIVE())
        {
            clLog(CL_LOG_INFO, CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
                  "Started as ACTIVE, so initialize checkpoint and create sections...");
            rc = cpmCpmLActiveCheckpointInitialize();
            if (rc == CL_OK)
            {
                /* Bug 4532:
                 * Checkpoint the data using server based checkpointing.
                 * This takes care of any checkpoint fail that happened 
                 * because checkpoint server was not up.
                 */
                cpmCkptCpmLDatsSet();
            }
            else
            {
                clLog(CL_LOG_ERROR, CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
                      "Checkpoint initialization failed, error [0x%x]", rc);
            }

            /* FIXME: What if the initialize fails */
            /* Inform AMS , that CKPT is up and running */
            if (gpClCpm->cpmToAmsCallback != NULL &&
                gpClCpm->cpmToAmsCallback->ckptServerReady != NULL)
            {
                gpClCpm->cpmToAmsCallback->ckptServerReady(gpClCpm->ckptHandle, CL_AMS_INSTANTIATE_MODE_ACTIVE);
            }

            /* Inform AMS , that Event is up and running */
            if (gpClCpm->cpmToAmsCallback != NULL &&
                gpClCpm->cpmToAmsCallback->eventServerReady != NULL)
            {
                gpClCpm->cpmToAmsCallback->eventServerReady(CL_TRUE);
            }

        }
        else if(clCpmIsSCCapable())
        {
            clLog(CL_LOG_INFO, CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
                  "Started as capable deputy. Initialize checkpoint...");
            rc = cpmCpmLStandbyCheckpointInitialize();
            //CL_ASSERT(rc == CL_OK);  /* GAS make sure that this does actually return success for the multi-sc work */
            if (rc != CL_OK)
	    {
	        clLog(CL_LOG_ERROR, CPM_LOG_AREA_CPM, CL_LOG_CONTEXT_UNSPECIFIED,
                  "Initializing checkpoint failed, rc=[%#x]. Exit", rc);
		exit(EXIT_FAILURE);
            }
            if (gpClCpm->cpmToAmsCallback != NULL &&
                gpClCpm->cpmToAmsCallback->ckptServerReady != NULL)
            {
                gpClCpm->cpmToAmsCallback->ckptServerReady(gpClCpm->ckptHandle, CL_AMS_INSTANTIATE_MODE_STANDBY);
            }

            /* Inform AMS , that Event is up and running */
            if (gpClCpm->cpmToAmsCallback != NULL &&
                gpClCpm->cpmToAmsCallback->eventServerReady != NULL)
            {
                gpClCpm->cpmToAmsCallback->eventServerReady(CL_TRUE);
            }

        }
    }
#endif
    
    if (cpmBmTable->currentBootLevel == CL_CPM_BOOT_LEVEL_1)
    {
        rc = clLogLibInitialize();
        CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED, "LOG",
                       rc, rc, CL_LOG_ERROR, CL_LOG_HANDLE_APP);
    }
    else if (cpmBmTable->currentBootLevel == CL_CPM_BOOT_LEVEL_2)
    {
        rc = cpmGmsInitialize();
        if (CL_OK != rc)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                       CL_CPM_LOG_1_GMS_INIT_ERR, rc);
            cpmShutdownHeartbeat();
            return rc;
        }

        /*
         * Register with the CPM/G active.  This is also called for
         * the CPM/G active itself, i.e. "self registration". This
         * must be called after the cpmGmsInitialize() or you are
         * screwed.
         */
        cpmRegisterWithActive();
    }
    else if (cpmBmTable->currentBootLevel ==
             gpClCpm->pCpmLocalInfo->defaultBootLevel)
    {
        cpmWriteNodeStatToFile("CPM", CL_YES);
    }

    return CL_OK;

    failure:
    return rc;
}

/*
 * Shuts down the components at the current boot level.
 */
static ClRcT cpmBmStopCurrentLevel(cpmBMT *cpmBmTable)
{
    ClRcT rc = CL_OK;

    bootTableT *pDList = NULL;
    bootRowT *p = NULL;

    ClTimerTimeOutT timeOut = { 0, 0 };

    if (cpmBmTable->currentBootLevel == CL_CPM_BOOT_LEVEL_0)
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_STATE);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BM_MIN_LEVEL_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
#ifdef CL_CPM_GMS
    /**
     * Check if the current boot level is 2.
     * If it is, then do GMS related processing.
     */

    if (cpmBmTable->currentBootLevel == CL_CPM_BOOT_LEVEL_2)
    {
        cpmGmsFinalize();
    }
#endif

#ifdef CL_CPM_AMS
    if (cpmBmTable->currentBootLevel == CL_CPM_BOOT_LEVEL_4)
    {
        if (gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL)
        {
            if (gpClCpm->ckptOpenHandle != CL_HANDLE_INVALID_VALUE)
            {
                rc = clCkptCheckpointClose(gpClCpm->ckptOpenHandle);
                /* Bug 5295:
                 * Ignoring the error from checkpoint close and finalize,
                 * since it is resulting CPM to shutdown leaving the other 
                 * ASP components alive.
                 */
                if (rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("CheckpointClose Failed while booting down rc=[0x%x]\n",rc));
                }
            }

            if (gpClCpm->ckptHandle != CL_HANDLE_INVALID_VALUE)
            {
                rc = clCkptFinalize(gpClCpm->ckptHandle);
                if (rc != CL_OK)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("CheckpointFinalize failed while booting down rc=[0x%x]\n",rc));
                }
            }
        }
    }
#endif

    pDList = cpmBmTable->table;
    while (pDList != NULL)
    {
        if (pDList->bootLevel == cpmBmTable->currentBootLevel)
            break;              /* start shutting down from current boot level */
        pDList = pDList->pDown;
    }

    if (pDList != NULL && pDList->numComp != 0)
    {
        cpmBmTable->currentBootLevel = pDList->bootLevel;
        cpmBmTable->numComponent = pDList->numComp;
        cpmBmTable->countComponent = 0;

        rc = clOsalMutexLock(cpmBmTable->lcmCondVarMutex);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_LOCK_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        p = pDList->listHead;
        while (p != NULL)
        {
            ClNameT compName = {0}, nodeName = {0};
            ClCpmLcmReplyT srcInfo = {0};

            strcpy(compName.value, p->compName);
            compName.length = strlen(compName.value);
            srcInfo.srcIocAddress = 0;
            srcInfo.srcPort = 0xFFFF;
            srcInfo.rmdNumber = 0;

            strcpy(nodeName.value, gpClCpm->pCpmConfig->nodeName);
            nodeName.length = strlen(nodeName.value);
            rc = clCpmComponentTerminate(&compName, &nodeName, &srcInfo);
            if (rc != CL_OK)
            {
                clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
                CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_BM_SU_TERM_ERR,
                               p->compName, rc, rc, CL_LOG_DEBUG,
                               CL_LOG_HANDLE_APP);
            }
            p = p->pNext;
        }

        /*
         * Conditinal wait for guaranteed response. 
         */
        if (clOsalCondWait
            (cpmBmTable->lcmCondVar, cpmBmTable->lcmCondVarMutex,
             timeOut) == CL_OK)
        {
            rc = clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR,
                           rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            if (cpmBmTable->countComponent == pDList->numComp)
            {
                /*
                 * Shutting down of all the components in this boot level 
                 * was a success.
                 */
                CL_DEBUG_PRINT(CL_DEBUG_INFO, ("Out of CondWait: All SUs terminated successfully"));
                cpmBmTable->lastBootStatus = CL_TRUE;
                cpmBmTable->countComponent = 0;
                /*
                 * Check if it is the last boot level. 
                 */
                if (pDList->pUp != NULL)
                    cpmBmTable->currentBootLevel = pDList->pUp->bootLevel;
                else
                    cpmBmTable->currentBootLevel = CL_CPM_BOOT_LEVEL_0;
            }
            else
            {
                /*
                 * Shutting down of one of the components was a failure. 
                 */
                cpmBmTable->lastBootStatus = CL_FALSE;
                /*
                 * Set this to zero, else clOsalCondWait will block. 
                 */
                cpmBmTable->countComponent = 0;
                /*
                 * Failure status. 
                 */
                rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_FAILED);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("Operation on one of the Service Unit with an error %x\n",
                                rc));
                CL_CPM_CHECK_1(CL_DEBUG_ERROR,
                               CL_CPM_LOG_1_SERVER_ERR_OPERATION_FAILED, rc, rc,
                               CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
        }
        else
        {
            rc = clOsalMutexUnlock(cpmBmTable->lcmCondVarMutex);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_UNLOCK_ERR,
                           rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
    }
    else if (pDList != NULL && pDList->numComp == 0)
    {
        /*
         * Nothing to do. Simply move to the next boot level. 
         */
        if (pDList->pUp != NULL)
            cpmBmTable->currentBootLevel = pDList->pUp->bootLevel;
        else
            cpmBmTable->currentBootLevel = CL_CPM_BOOT_LEVEL_0;
        cpmBmTable->lastBootStatus = CL_TRUE;
        cpmBmTable->numComponent = pDList->numComp;
        cpmBmTable->countComponent = 0;
    }
    else
    {
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_FAILED);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BM_MIN_LEVEL_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

    return CL_OK;

  failure:
    return rc;
}
