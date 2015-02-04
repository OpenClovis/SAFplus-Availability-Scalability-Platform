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
 * This file implements all the component related functionality of
 * CPM.
 */

/*
 * Standard header files 
 */
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <clIocIpi.h>

/*
 * ASP header files 
 */
#include <clCpmApi.h>
#include <clDebugApi.h>
#include <clDbg.h>
#include <clLogApi.h>
#include <clCpmLog.h>
#include <clHandleApi.h>
#include <clAms.h>
#include <clJobQueue.h>
/*
 * CPM internal header files 
 */
#include <clVersionApi.h>
#include <clCpmClient.h>
#include <clCpmInternal.h>
#include <clCpmCor.h>
#include <clCpmMgmt.h>

/*
 * XDR header files 
 */
#include "xdrClCpmCompInitSendT.h"
#include "xdrClCpmCompInitRecvT.h"
#include "xdrClCpmCompFinalizeSendT.h"
#include "xdrClCpmCompRegisterT.h"
#include "xdrClCpmLifeCycleOprT.h"
#include "xdrClCpmComponentStateT.h"
#include "xdrClIocAddressIDLT.h"
#include "xdrClCpmLcmResponseT.h"
#include "xdrClCpmLifeCycleOprT.h"
#include "xdrClCpmClientCompTerminateT.h"
#include "xdrClCpmCompLAUpdateT.h"
#include "xdrClCpmCompRequestTypeT.h"
#include "xdrClErrorReportT.h"
#include "xdrClCpmEventPayLoadT.h"
#include "xdrClCpmCompHealthcheckT.h"
#include "xdrClCpmCompSpecInfoRecvT.h"
#ifdef VXWORKS_BUILD
#define CL_RTP_STACK_SIZE (1<<20)
#endif

#define ASP_PRECLEANUP_SCRIPT "asp_precleanup.sh"

#define CPM_TIMED_WAIT(millisec) do {                               \
    ClTimerTimeOutT _delay = {.tsSec = 0, .tsMilliSec = millisec }; \
    clOsalTaskDelay(_delay);                                        \
}while(0)

extern int gCompHealthCheckFailSendSignal;
extern ClBoolT gCpmShuttingDown;

/*
 * Versions supported
 */
static ClVersionT versions_supported[] =
{
    {'B', 0x01, 0x01},
    {'B', 0x02, 0x00},
};

static ClVersionDatabaseT version_database =
{
    sizeof(versions_supported) / sizeof(ClVersionT),
    versions_supported
};

typedef struct ClEventPublishData
{
    ClCpmComponentT comp;
    ClCpmCompEventT compEvent;
    ClBoolT eventCleanup;
} ClEventPublishDataT;

static ClJobQueueT eventPublishQueue;

/*
 * Forward Declaratiion 
 */
static ClRcT cpmCompRespondToCaller(ClCpmComponentT *comp,
                                    ClCpmCompRequestTypeT requestType,
                                    ClRcT retCode);

/*
 * CPM Timer related implementation 
 */

/*
 * Timer CB: Check the component Presence State 
 * This function will be invoked in case when the timer pops up. This timer 
 * should popup only when a requested life cycle management function [CB] is 
 * not completed within the timespan specified as per the configuration 
 */
static ClRcT cpmTimerCallback(void *arg)
{
    ClRcT rc = CL_OK;
    ClCpmComponentT *comp = ((ClCpmComponentT *) arg);

    clEoMyEoIocPortSet(gpClCpm->cpmEoObj->eoPort);
    clEoMyEoObjectSet(gpClCpm->cpmEoObj);
    /*
     * No Need to call the response here, as the Cleanup/Instantiate function
     * will take care of it 
     */
    switch (comp->requestType)
    {
        case CL_CPM_INSTANTIATE:
        case CL_CPM_PROXIED_INSTANTIATE:
        {
            if (comp->compPresenceState == CL_AMS_PRESENCE_STATE_INSTANTIATING)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                           CL_CPM_LOG_2_LCM_COMP_OPER_ERR,
                           comp->compConfig->compName, "instantiated");
                clLogError("CMP","MGT","Component %s did not start within the specified limit. ",comp->compConfig->compName);
                rc = _cpmComponentCleanup(comp->compConfig->compName,
                                          NULL,
                                          gpClCpm->pCpmConfig->nodeName,
                                          &(comp->requestSrcAddress),
                                          comp->requestRmdNumber,
                                          CL_CPM_INSTANTIATE);
            }
            break;
        }
        case CL_CPM_TERMINATE:
        {
            if (comp->compPresenceState == CL_AMS_PRESENCE_STATE_TERMINATING)
            {
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                           CL_CPM_LOG_2_LCM_COMP_OPER_ERR,
                           comp->compConfig->compName, "terminated");
                clLogError("CMP","MGT","Component [%s] did not terminate within the specified limit. ",comp->compConfig->compName);
                rc = _cpmComponentCleanup(comp->compConfig->compName,
                                          NULL,
                                          gpClCpm->pCpmConfig->nodeName,
                                          &(comp->requestSrcAddress),
                                          comp->requestRmdNumber,
                                          CL_CPM_TERMINATE);

                if (!(comp->restartPending))
                {
                    ;
                    /*
                     * CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to respond to
                     * caller %x\n", rc), rc);
                     */
                }
                else
                {
                    /*
                     * Wait atleast 4000 milli seconds 
                     */
                    CPM_TIMED_WAIT(CL_CPM_RESTART_SLEEP);

                    _cpmComponentInstantiate(comp->compConfig->compName,
                                             NULL,
                                             0,
                                             gpClCpm->pCpmConfig->nodeName,
                                             &(comp->requestSrcAddress),
                                             comp->requestRmdNumber);
                }
            }
            break;
        }

        case CL_CPM_HEALTHCHECK:
        case CL_CPM_PROXIED_CLEANUP:
        case CL_CPM_CLEANUP:
        case CL_CPM_RESTART:
        case CL_CPM_EXTN_HEALTHCHECK:
            break;
    }
    /*
     * clOsalPrintf("Inside cpmTimerCallback for component %s %d \n",
     * comp->compConfig->compName, comp->requestType);
     */
    return rc;

    /*
     * failure: return rc;
     */

}

/*
 * Modification for Bug 4028.
 * Commenting cpmTimerWait and cpmCompInstTimerCallback.
 * Using CPM_TIMED_WAIT now.
 */
#if 0
static ClRcT cpmCompInstTimerCallback(void *arg)
{
    ClRcT rc = CL_OK;
    ClCpmComponentT *comp = ((ClCpmComponentT *) arg);

    _cpmComponentInstantiate(comp->compConfig->compName,
                             gpClCpm->pCpmConfig->nodeName,
                             0,
                             &(comp->requestSrcAddress),
                             comp->requestRmdNumber);
    return rc;

}

ClRcT cpmTimerWait(ClCpmComponentT *comp, ClUint32T milliSeconds)
{
    ClRcT rc = CL_OK;
    ClTimerTimeOutT cpmTimeOut = { 0, 0 };
    ClTimerHandleT  timerHandle = 0;

    cpmTimeOut.tsSec = 0;
    cpmTimeOut.tsMilliSec = milliSeconds;
    rc = clTimerCreate(cpmTimeOut, CL_TIMER_ONE_SHOT, CL_TIMER_SEPARATE_CONTEXT,
                       cpmCompInstTimerCallback, (void *)comp,
                       &timerHandle);

    if (timerHandle)
    {
        rc = clTimerStop(timerHandle);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_STOP_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        rc = clTimerStart(timerHandle);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_START_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_NOT_INITIALIZED);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_INIT_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

  failure:
    return rc;

    return rc;
}
#endif

/*
 * Following four functions start the time. FIXME: Strangly timer needs to be 
 * stopped before it can be started
 */
static ClRcT clCompTimerInstantiate(ClCpmComponentT *comp)
{
    ClRcT rc = CL_OK;

    if (comp->cpmInstantiateTimerHandle)
    {
        rc = clTimerStop(comp->cpmInstantiateTimerHandle);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_STOP_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        rc = clTimerStart(comp->cpmInstantiateTimerHandle);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_START_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_NOT_INITIALIZED);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_INIT_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

  failure:
    return rc;
}

static ClRcT clCompTimerTerminate(ClCpmComponentT *comp)
{
    ClRcT rc = CL_OK;

    if (comp->cpmTerminateTimerHandle)
    {
        rc = clTimerStop(comp->cpmTerminateTimerHandle);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_STOP_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        rc = clTimerStart(comp->cpmTerminateTimerHandle);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_START_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_NOT_INITIALIZED);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_INIT_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

  failure:
    return rc;
}

static ClRcT cpmHealthCheckTimerStart(ClCpmComponentT *comp)
{
    ClRcT rc = CL_OK;

    if (comp->cpmHealthcheckTimerHandle)
    {
        clTimerStop(comp->cpmHealthcheckTimerHandle);

        rc = clTimerStart(comp->cpmHealthcheckTimerHandle);
        if (CL_OK != rc)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                       "Unable to start healthcheck timer, "
                       "error [%#x]", rc);
            goto failure;
        }
    }

    return CL_OK;
failure:
    return rc;
}

static ClRcT cpmHealthCheckTimerCallback(void *arg)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)arg;
    ClNameT compName = {0};

    clNameSet(&compName, comp->compConfig->compName);

    /*
     * Publish death notifications for non SAF components which
     * cannot be caught from our ioc notification layer.
     */
    if(comp->compConfig->compProperty != CL_AMS_COMP_PROPERTY_SA_AWARE)
    {
        cpmComponentEventPublish(comp, CL_CPM_COMP_DEATH, CL_FALSE);
    }

    clLogNotice("HEALTH", "CHECK", "Health check failure detected for component [%.*s], instantiate cookie [%lld]",
                compName.length, compName.value, comp->instantiateCookie);

    comp->hbFailureDetected = CL_TRUE;
    clCpmCompPreCleanupInvoke(comp);
    clCpmComponentFailureReportWithCookie(0,
                                          &compName,
                                          comp->instantiateCookie,
                                          0,
                                          comp->compConfig->
                                          healthCheckConfig.recommendedRecovery,
                                          0);
    clOsalMutexLock(comp->compMutex);
    comp->hbInvocationPending = CL_NO;
    clTimerStop(comp->hbTimerHandle);
    clTimerDeleteAsync(&comp->hbTimerHandle);
    comp->hbTimerHandle = CL_HANDLE_INVALID_VALUE;
    clOsalMutexUnlock(comp->compMutex);
    
    return CL_OK;
}

static ClRcT cpmHealthCheckTimerCreate(ClCpmComponentT *comp)
{
    ClRcT rc = CL_OK;
    ClTimerTimeOutT timeOut =
    {
        0,
        comp->compConfig->healthCheckConfig.maxDuration
    };

    rc = clTimerCreate(timeOut,
                       CL_TIMER_ONE_SHOT,
                       CL_TIMER_SEPARATE_CONTEXT,
                       cpmHealthCheckTimerCallback,
                       comp,
                       &comp->cpmHealthcheckTimerHandle);
    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                   "Unable to create health check timer, error [%#x]",
                   rc);
        goto failure;
    }

    return CL_OK;

failure:
    return rc;
}

/*
 * Bug 4381:
 * Commented functions to suppress compiler warnings.
 * FIXME: Need to uncomment them in future when CPM will also
 * manage proxied components.
 */
#if 0
static ClRcT clCompTimerProxiedInstantiate(ClCpmComponentT *comp)
{
    ClRcT rc = CL_OK;

    if (comp->cpmProxiedInstTimerHandle)
    {
        rc = clTimerStop(comp->cpmProxiedInstTimerHandle);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_STOP_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        rc = clTimerStart(comp->cpmProxiedInstTimerHandle);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_START_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_NOT_INITIALIZED);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_INIT_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

  failure:
    return rc;
}

static ClRcT clCompTimerProxiedCleanup(ClCpmComponentT *comp)
{
    ClRcT rc = CL_OK;

    if (comp->cpmProxiedCleanupTimerHandle)
    {
        rc = clTimerStop(comp->cpmProxiedCleanupTimerHandle);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_STOP_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        rc = clTimerStart(comp->cpmProxiedCleanupTimerHandle);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_START_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_NOT_INITIALIZED);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_TIMER_INIT_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

  failure:
    return rc;
}
#endif

static ClRcT eventPublishQueueCallback(ClCallbackT cb, ClPtrT data, ClPtrT arg)
{
    ClEventPublishDataT *jobTemp = arg;
    ClEventPublishDataT *job = data;
    ClCpmComponentT *compTemp = &jobTemp->comp;
    ClCpmComponentT *comp = &job->comp;

    if ((jobTemp->compEvent == CL_CPM_COMP_DEATH) && (job->compEvent == CL_CPM_COMP_DEATH) && (comp->eoID == compTemp->eoID)
                    && (comp->eoPort == compTemp->eoPort))
    {
        /*
         * Combine event publishing and cleanup with one job and ignore the other one
         */
        job->eventCleanup = CL_TRUE;
        compTemp->compEventPublished = CL_TRUE;
    }

    return CL_OK;
}

/*
 * Free the Component structure completely including all substructures 
 */
ClRcT cpmCompFinalize(ClCpmComponentT *comp)
{
    if (comp != NULL)
    {
        if (comp->compConfig != NULL)
        {
            clHeapFree(comp->compConfig);
            comp->compConfig = NULL;
        }
        clHeapFree(comp);
        comp = NULL;
    }
    return CL_OK;
}

void cpmCompConfigCpy(ClCpmCompConfigT *toConfig,
                      const ClCpmCompConfigT *fromConfig)
{
    ClUint32T i = 0;
    
    strncpy(toConfig->compName, fromConfig->compName, CL_MAX_NAME_LENGTH-1);
    toConfig->isAspComp = fromConfig->isAspComp;
    toConfig->compProperty = fromConfig->compProperty;
    toConfig->compProcessRel = fromConfig->compProcessRel;
    strncpy(toConfig->instantiationCMD,
            fromConfig->instantiationCMD,
            CL_MAX_NAME_LENGTH-1);

    for (i = 0; (i < CPM_MAX_ARGS -1) && fromConfig->argv[i]; ++i)
    {
        toConfig->argv[i] = clHeapAllocate(strlen(fromConfig->argv[i]) + 1);
        if (!toConfig->argv[i])
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                       "Unable to allocate memory");
            break;
        }

        strncpy(toConfig->argv[i],
                fromConfig->argv[i],
                strlen(fromConfig->argv[i]));
    }
    toConfig->argv[i] = NULL;

    for (i = 0; fromConfig->env[i]; ++i)
    {
        toConfig->env[i] = clHeapAllocate(sizeof(ClCpmEnvVarT));
        if (!toConfig->env[i])
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                       "Unable to allocate memory");
            break;
        }
        
        strncpy(toConfig->env[i]->envName,
                fromConfig->env[i]->envName,
                CL_MAX_NAME_LENGTH-1);
        strncpy(toConfig->env[i]->envValue,
                fromConfig->env[i]->envValue,
                CL_MAX_NAME_LENGTH-1);
    }
    toConfig->env[i] = NULL;

    strncpy(toConfig->terminationCMD,
            fromConfig->terminationCMD,
            CL_MAX_NAME_LENGTH-1);
    strncpy(toConfig->cleanupCMD,
            fromConfig->cleanupCMD,
            CL_MAX_NAME_LENGTH-1);

    toConfig->compInstantiateTimeout = fromConfig->compInstantiateTimeout;
    toConfig->compCleanupTimeout = fromConfig->compCleanupTimeout;

    toConfig->compTerminateCallbackTimeOut =
    fromConfig->compTerminateCallbackTimeOut;

    toConfig->compProxiedCompInstantiateCallbackTimeout =
    fromConfig->compProxiedCompInstantiateCallbackTimeout;

    toConfig->compProxiedCompCleanupCallbackTimeout =
    fromConfig->compProxiedCompCleanupCallbackTimeout;

    toConfig->healthCheckConfig.period =
    fromConfig->healthCheckConfig.period;
    toConfig->healthCheckConfig.maxDuration =
    fromConfig->healthCheckConfig.maxDuration;
    toConfig->healthCheckConfig.recommendedRecovery =
    fromConfig->healthCheckConfig.recommendedRecovery;
}
    
/*
 * Configure the Component structure with User Configured values 
 */
ClRcT cpmCompConfigure(ClCpmCompConfigT *compCfg, ClCpmComponentT **component)
{
    ClCpmComponentT *tmpComp = NULL;
    ClRcT rc = CL_OK;
    ClTimerTimeOutT cpmTimeOut = { 0, 0 };

    /*
     * Memory Allocation 
     */
    tmpComp = (ClCpmComponentT *) clHeapAllocate(sizeof(ClCpmComponentT));
    if (tmpComp == NULL)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "Unable to allocate memory");
        rc = CL_CPM_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }

    tmpComp->compConfig =
        (ClCpmCompConfigT *) clHeapAllocate(sizeof(ClCpmCompConfigT));
    if (tmpComp->compConfig == NULL)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_BOOT,
                   "Unable to allocate memory");
        rc = CL_CPM_RC(CL_ERR_NULL_POINTER);
        goto failure;
    }
    
    cpmCompConfigCpy(tmpComp->compConfig, compCfg);
    
    tmpComp->compId = ++(gpClCpm->compIdAlloc);
    /*
     * Component State Initialization 
     */
    tmpComp->compPresenceState = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;
    if (tmpComp->compConfig->compProperty ==
        CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE &&
        tmpComp->compConfig->compProperty ==
        CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)
    {
        tmpComp->compOperState = CL_AMS_OPER_STATE_ENABLED;
    }
    else
        tmpComp->compOperState = CL_AMS_OPER_STATE_DISABLED;
    tmpComp->compReadinessState = CL_AMS_READINESS_STATE_OUTOFSERVICE;

    if (IS_ASP_COMP(tmpComp))
    {
        /*
         * Create one shot timers for component instantiation and
         * termination.
         *
         * The default value for clDbgCompTimeoutOverride is
         * zero. Overriding this helps debug apps, since the app won't
         * be killed if you set this variable to high value.
         */

        cpmTimeOut.tsSec = clDbgCompTimeoutOverride;
        cpmTimeOut.tsMilliSec = tmpComp->compConfig->compInstantiateTimeout;
        rc = clTimerCreate(cpmTimeOut, CL_TIMER_ONE_SHOT, CL_TIMER_SEPARATE_CONTEXT,
                           cpmTimerCallback, (void *) tmpComp,
                           &(tmpComp->cpmInstantiateTimerHandle));

        cpmTimeOut.tsSec = clDbgCompTimeoutOverride;
        cpmTimeOut.tsMilliSec = tmpComp->compConfig->compTerminateCallbackTimeOut;
        rc = clTimerCreate(cpmTimeOut, CL_TIMER_ONE_SHOT, CL_TIMER_SEPARATE_CONTEXT,
                           cpmTimerCallback, (void *) tmpComp,
                           &(tmpComp->cpmTerminateTimerHandle));

        tmpComp->cpmProxiedInstTimerHandle = CL_HANDLE_INVALID_VALUE;
        tmpComp->cpmProxiedCleanupTimerHandle = CL_HANDLE_INVALID_VALUE;
        tmpComp->hbFailureDetected = CL_FALSE;
    }
    else
    {
        tmpComp->cpmInstantiateTimerHandle = CL_HANDLE_INVALID_VALUE;
        tmpComp->cpmTerminateTimerHandle = CL_HANDLE_INVALID_VALUE;
        tmpComp->cpmHealthcheckTimerHandle = CL_HANDLE_INVALID_VALUE;
        tmpComp->hbTimerHandle = CL_HANDLE_INVALID_VALUE;
        tmpComp->cpmProxiedInstTimerHandle = CL_HANDLE_INVALID_VALUE;
        tmpComp->cpmProxiedCleanupTimerHandle = CL_HANDLE_INVALID_VALUE;

        tmpComp->hbInvocationPending = CL_NO;
        tmpComp->hcConfirmed = CL_NO;
        tmpComp->hbFailureDetected = CL_FALSE;
    }

    rc = clOsalMutexCreate(&(tmpComp->compMutex));
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_OSAL_MUTEX_CREATE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    
    *component = tmpComp;

    return CL_OK;

  failure:
    cpmCompFinalize(tmpComp);
    return rc;
}

ClRcT cpmCompFindWithLock(ClCharT *name, ClCntHandleT compTable, ClCpmComponentT **comp)
{
    ClUint16T compKey = 0;
    ClCntNodeHandleT hNode = 0;
    ClCpmComponentT *tempComp = NULL;
    ClRcT rc = CL_OK;
    ClUint32T numNode = 0;

    rc = clCksm16bitCompute((ClUint8T *) name, strlen((ClCharT *) name),
                            &compKey);
    CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_CNT_CKSM_ERR, name, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clCntNodeFind(compTable, (ClCntKeyHandleT)(ClWordT)compKey, &hNode);
    if(rc != CL_OK)
        goto failure;

    rc = clCntKeySizeGet(compTable, (ClCntKeyHandleT)(ClWordT)compKey, &numNode);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_CNT_KEY_SIZE_GET_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    while (numNode > 0)
    {
        rc = clCntNodeUserDataGet(compTable, hNode,
                                  (ClCntDataHandleT *) &tempComp);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR,
                       rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

        if (!strcmp
            ((ClCharT *) tempComp->compConfig->compName, (ClCharT *) name))
        {
            *comp = tempComp;
            goto done;
        }
        if(--numNode)
        {
            rc = clCntNextNodeGet(compTable,hNode,&hNode);
            if(rc != CL_OK)
            {
                break;
            }
        }
    }

    rc = CL_CPM_RC(CL_ERR_DOESNT_EXIST);
    goto failure;

done:
    return CL_OK;
    
failure:
    return rc;
}

ClRcT cpmCompFind(ClCharT *name, ClCntHandleT compTable, ClCpmComponentT **comp)
{
    ClRcT rc = CL_OK;
    clOsalMutexLock(gpClCpm->compTableMutex);
    rc = cpmCompFindWithLock(name, compTable, comp);
    clOsalMutexUnlock(gpClCpm->compTableMutex);
    return rc;
}
 
static ClRcT cpmCompRespondToCaller(ClCpmComponentT *comp,
                                    ClCpmCompRequestTypeT requestType,
                                    ClRcT retCode)
{
    ClRcT rc = CL_OK;
    ClCpmLcmResponseT response = { {0, 0}, 0 };

    if (retCode == CL_OK)
    {
        ;
    }
    else
    {
        if (requestType == CL_CPM_INSTANTIATE ||
            requestType == CL_CPM_PROXIED_INSTANTIATE)
        {
            /*
             * Modified for Bug 4028.
             * Added top level if statement.
             * Should not change state if component state was INSTANTIATING.
             *
             * FIXME: Code not very clean, remarshall the if conditions.
             * Also check for other request types if return code is 
             * CL_CPM_ERR_OPERATION_IN_PROGRESS.
             */
            if (CL_GET_ERROR_CODE(retCode) != CL_CPM_ERR_OPERATION_IN_PROGRESS)
            {
                comp->compPresenceState = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;

                clOsalMutexLock(comp->compMutex);
                comp->compOperState = CL_AMS_OPER_STATE_DISABLED;
                comp->compReadinessState = CL_AMS_READINESS_STATE_OUTOFSERVICE;
                clOsalMutexUnlock(comp->compMutex);
            }
        }
        else if (requestType == CL_CPM_TERMINATE ||
                 requestType == CL_CPM_CLEANUP ||
                 requestType == CL_CPM_PROXIED_CLEANUP)
        {
            clOsalMutexLock(comp->compMutex);
            comp->compOperState = CL_AMS_OPER_STATE_DISABLED;
            comp->compReadinessState = CL_AMS_READINESS_STATE_OUTOFSERVICE;
            clOsalMutexUnlock(comp->compMutex);
        }
    }

    response.returnCode = retCode;

    if (comp->restartPending && requestType == CL_CPM_INSTANTIATE)
    {
        if(gpClCpm->bmTable->currentBootLevel < gpClCpm->pCpmLocalInfo->defaultBootLevel
           &&
           retCode == CL_OK)
        {
            comp->requestSrcAddress.portId = 0xffff; /* return bmresponse*/
        }
        response.requestType = CL_CPM_RESTART;
        comp->restartPending = 0;
    }
    else if(comp->restartPending && requestType == CL_CPM_TERMINATE) 
    {
        response.requestType = requestType;
        return CL_OK;
    }
    else 
    {
        response.requestType = requestType;
    }

    strcpy(response.name, comp->compConfig->compName);
    if (comp->requestSrcAddress.portId == 0xFFFF)
    {
        cpmBMResponse(&response);
    }
    else if (comp->requestSrcAddress.portId == 0x0)
    {
        ;
    }
    else
    {
        ClUint8T priority = 0;
        ClNameT compName = {0};
        clNameSet(&compName, comp->compConfig->compName);
        if(requestType == CL_CPM_INSTANTIATE)
        {
            priority = CL_IOC_CPM_INSTANTIATE_PRIORITY;
        }
        else if(requestType == CL_CPM_TERMINATE)
        {
            priority = CL_IOC_CPM_TERMINATE_PRIORITY;
        }

        rc = CL_CPM_CALL_RMD_ASYNC_NEW(comp->requestSrcAddress.nodeAddress,
                                       comp->requestSrcAddress.portId,
                                       comp->requestRmdNumber,
                                       (ClUint8T *) &response,
                                       sizeof(ClCpmLcmResponseT),
                                       NULL,
                                       NULL,
                                       CL_RMD_CALL_ATMOST_ONCE,
                                       0,
                                       0,
                                       priority,
                                       NULL,
                                       NULL,
                                       MARSHALL_FN(ClCpmLcmResponseT, 4, 0, 0));
        if(rc != CL_OK 
           &&
           comp->requestSrcAddress.portId == CL_IOC_CPM_PORT
           &&
           comp->requestSrcAddress.nodeAddress 
           &&
           comp->requestSrcAddress.nodeAddress != CL_IOC_BROADCAST_ADDRESS
           &&
           !cpmIsInfrastructureComponent(&compName)
           )
        {
            ClInt32T tries = 0;
            ClTimerTimeOutT delay = { .tsSec = 1, .tsMilliSec = 0};
            /*
             * We could be in the middle of a master transitioning phase
             */
            do
            {
                if(clCpmMasterAddressGet(&comp->requestSrcAddress.nodeAddress) != CL_OK)
                    break;
                rc = CL_CPM_CALL_RMD_ASYNC_NEW(comp->requestSrcAddress.nodeAddress,
                                               comp->requestSrcAddress.portId,
                                               comp->requestRmdNumber,
                                               (ClUint8T *) &response,
                                               sizeof(ClCpmLcmResponseT),
                                               NULL,
                                               NULL,
                                               CL_RMD_CALL_ATMOST_ONCE,
                                               0,
                                               0,
                                               priority,
                                               NULL,
                                               NULL,
                                               MARSHALL_FN(ClCpmLcmResponseT, 4, 0, 0));
            } while (rc != CL_OK && ++tries < 5 && clOsalTaskDelay(delay) == CL_OK);
            
        }
    }

    return rc;
}


void cpmResponse(ClRcT retCode,
                 ClCpmComponentT *comp,
                 ClCpmCompRequestTypeT requestType)
{
    ClRcT rc = CL_OK;

    if (retCode == CL_OK)
    {
        switch (requestType)
        {
        case CL_CPM_HEALTHCHECK:
            {
                clOsalMutexLock(comp->compMutex);
                comp->hbInvocationPending = CL_NO;
                if(comp->cpmHealthcheckTimerHandle != CL_HANDLE_INVALID_VALUE)
                {
                    clTimerStop(comp->cpmHealthcheckTimerHandle);
                }
                clOsalMutexUnlock(comp->compMutex);
                break;
            }
        case CL_CPM_TERMINATE:
            {

                /*
                 * We got the response, hence stop the timer, don't care of the 
                 * failure here, as even if times pops, the corresponsing
                 * function will first look at the stats and then do something
                 */
                if (IS_ASP_COMP(comp))
                {
                    clTimerStop(comp->cpmTerminateTimerHandle);
                }
                else
                {
                    if (CL_HANDLE_INVALID_VALUE != comp->cpmHealthcheckTimerHandle)
                    {
                        clTimerStop(comp->cpmHealthcheckTimerHandle);
                        clTimerDelete(&comp->cpmHealthcheckTimerHandle);
                    }
                    if (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)
                    {
                        clTimerStop(comp->hbTimerHandle);
                        clTimerDelete(&comp->hbTimerHandle);
                    }
                }
                
                /*
                 * Update the component States 
                 */
                comp->compPresenceState = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;

                clOsalMutexLock(comp->compMutex);
                comp->compOperState = CL_AMS_OPER_STATE_DISABLED;
                comp->compReadinessState = CL_AMS_READINESS_STATE_OUTOFSERVICE;
                clOsalMutexUnlock(comp->compMutex);

                cpmCompRespondToCaller(comp, requestType, CL_OK);
                /*
                 * Added the if block for Bug 4028.
                 *  If restart is pending, then instantiate here as termination
                 * is complete now.
                 */
                if (comp->restartPending)
                {
                    CPM_TIMED_WAIT(CL_CPM_RESTART_SLEEP);

                    _cpmComponentInstantiate(comp->compConfig->compName,
                                             NULL,
                                             0,
                                             gpClCpm->pCpmConfig->nodeName,
                                             &(comp->requestSrcAddress),
                                             comp->requestRmdNumber);
                }
                break;
            }
        case CL_CPM_PROXIED_INSTANTIATE:
            {
                /*
                 * We got the response, hence stop the timer, don't care of the 
                 * failure here, as even if times pops, the corresponsing
                 * function will first look at the stats and then do something
                 */
                /*
                 * Bug 4381:
                 * Commenting out the code for stopping the timer in case of proxied
                 * component. If a ASP component can be a proxied component then this
                 * needs to be changed accordingly.
                 */
                /*rc = clTimerStop(comp->cpmProxiedInstTimerHandle);*/

                /*
                 * Update the component States 
                 */
                comp->compPresenceState = CL_AMS_PRESENCE_STATE_INSTANTIATED;

                clOsalMutexLock(comp->compMutex);
                comp->compOperState = CL_AMS_OPER_STATE_ENABLED;
                comp->compReadinessState = CL_AMS_READINESS_STATE_INSERVICE;
                clOsalMutexUnlock(comp->compMutex);

                cpmCompRespondToCaller(comp, requestType, CL_OK);
                break;
            }
        case CL_CPM_PROXIED_CLEANUP:
            {
                /*
                 * We got the response, hence stop the timer, don't care of the 
                 * failure here, as even if times pops, the corresponsing
                 * function will first look at the stats and then do something
                 */
                /*
                 * Bug 4381:
                 * Commenting out the code for stopping the timer in case of proxied
                 * component. If a ASP component can be a proxied component then this
                 * needs to be changed accordingly.
                 */
                /*rc = clTimerStop(comp->cpmProxiedCleanupTimerHandle);*/

                /*
                 * Update the component States 
                 */
                comp->compPresenceState = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;

                clOsalMutexLock(comp->compMutex);
                comp->compOperState = CL_AMS_OPER_STATE_DISABLED;
                comp->compReadinessState = CL_AMS_READINESS_STATE_OUTOFSERVICE;
                clOsalMutexUnlock(comp->compMutex);

                cpmCompRespondToCaller(comp, requestType, CL_OK);
                break;
            }
        default:
            {
                rc = CL_CPM_RC(CL_CPM_ERR_INVALID_ARGUMENTS);
                cpmCompRespondToCaller(comp, requestType, CL_OK);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                               ("Invalid Response Received \n"));
            }
        }
    }
    else
    {
        /*
         * 3610: Termination failure.
         * Added switch statement and case for CL_CPM_TERMINATE.
         * Stop timer as the component will be cleaned up unconditionally,
         * let the caller take appropriate action.
         */
        switch(requestType)
        {
        case CL_CPM_HEALTHCHECK:
            {
                ClNameT compName = {0};

                clNameSet(&compName, comp->compConfig->compName);
                comp->hbFailureDetected = CL_TRUE;
                clCpmCompPreCleanupInvoke(comp);
                clCpmComponentFailureReportWithCookie(0,
                                                      &compName,
                                                      comp->instantiateCookie,
                                                      0,
                                                      comp->compConfig->
                                                      healthCheckConfig.recommendedRecovery,
                                                      0);
                
                clOsalMutexLock(comp->compMutex);
                comp->hbInvocationPending = CL_NO;
                if (CL_HANDLE_INVALID_VALUE != comp->cpmHealthcheckTimerHandle)
                {
                    clTimerStop(comp->cpmHealthcheckTimerHandle);
                }
                if (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)
                {
                    clTimerStop(comp->hbTimerHandle);
                }
                clOsalMutexUnlock(comp->compMutex);
                break;
            }
        case CL_CPM_TERMINATE:
            {
                /*
                 * We got the response, hence stop the timer, don't care of the 
                 * failure here, as even if times pops, the corresponsing
                 * function will first look at the stats and then do something
                 * VERIFY THIS:
                 */
                if (IS_ASP_COMP(comp))
                {
                    clTimerStop(comp->cpmTerminateTimerHandle);
                }
                else
                {
                    if (CL_HANDLE_INVALID_VALUE != comp->cpmHealthcheckTimerHandle)
                    {
                        clTimerStop(comp->cpmHealthcheckTimerHandle);
                        clTimerDelete(&comp->cpmHealthcheckTimerHandle);
                    }
                    if (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)
                    {
                        clTimerStop(comp->hbTimerHandle);
                        clTimerDelete(&comp->hbTimerHandle);
                    }
                }

                if (comp->restartPending)
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                   ("Component %s restart pending, termination failed\n",
                                    comp->compConfig->compName));
                    rc = _cpmComponentCleanup(comp->compConfig->compName,
                                              NULL,
                                              gpClCpm->pCpmConfig->nodeName,
                                              &(comp->requestSrcAddress),
                                              comp->requestRmdNumber,
                                              CL_CPM_TERMINATE);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                   ("Unable to cleanup component while restarting %x\n",
                                    rc));

                    CPM_TIMED_WAIT(CL_CPM_RESTART_SLEEP);

                    _cpmComponentInstantiate(comp->compConfig->compName,
                                             NULL,
                                             0,
                                             gpClCpm->pCpmConfig->nodeName,
                                             &(comp->requestSrcAddress),
                                             comp->requestRmdNumber);
                }
                break;
            }
        default:
            {
                break;
            }
        }

        cpmCompRespondToCaller(comp, requestType, retCode);
    }
}

ClRcT cpmRequestFailedResponse(ClCharT *name, 
                               ClCharT *nodeName,
                               ClIocPhysicalAddressT *srcAddress,
                               ClUint32T rmdNumber,
                               ClCpmCompRequestTypeT requestType,
                               ClRcT retCode)
{
    ClRcT rc = CL_OK;
    ClCpmLcmResponseT response = { {0, 0}, 0 };

    if(!srcAddress) return rc;

    if (srcAddress->portId == 0xFFFF)
    {
        /*
         * Ideally Should Never Reach here 
         */
        response.returnCode = retCode;
        response.requestType = requestType;
        strcpy(response.name, name);
        if (srcAddress->portId == 0xFFFF)
        {
            cpmBMResponse(&response);
        }
    }
    else if (srcAddress->portId == 0x0)
    {
        ;                       /* No Reply required in this case */
    }
    else
    {
        response.returnCode = retCode;
        strcpy(response.name, name);
        response.requestType = requestType;
        rc = CL_CPM_CALL_RMD_ASYNC_NEW(srcAddress->nodeAddress,
                                       srcAddress->portId,
                                       rmdNumber,
                                       (ClUint8T *) &response,
                                       sizeof(ClCpmLcmResponseT),
                                       NULL,
                                       NULL,
                                       CL_RMD_CALL_ATMOST_ONCE,
                                       0,
                                       0,
                                       0,
                                       NULL,
                                       NULL,
                                       MARSHALL_FN(ClCpmLcmResponseT, 4, 0, 0));
        CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_LCM_RESPONSE_FAILED,
                       srcAddress->nodeAddress, srcAddress->portId, rc, rc,
                       CL_LOG_ERROR, CL_LOG_HANDLE_APP);
    }
  failure:
    return rc;
}

static ClRcT cpmRouteRequestWithCookie(ClCharT *compName,
                                       ClCharT *proxyCompName,
                                       ClUint64T instantiateCookie,
                                       ClCharT *nodeName,
                                       ClIocPhysicalAddressT *srcAddress,
                                       ClUint32T rmdNumber,
                                       ClUint32T requestRmdNumber)
{
    ClCpmLifeCycleOprT compInstantiate = {{0}};
    ClCpmLT *cpmL = NULL;

    /*
     * ClCpmComponentT *comp = NULL;
     */
    ClRcT rc = CL_OK;

    /*
     * Find the node Info 
     */
    clOsalMutexLock(gpClCpm->cpmTableMutex);
    rc = cpmNodeFindLocked(nodeName, &cpmL);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        clLogError("COMP", "INST", "Node [%s] not found. Failure code [%#x]",
                   nodeName, rc);
        goto failure;
    }
    
    compInstantiate.instantiateCookie = instantiateCookie;
    strncpy((ClCharT *) compInstantiate.name.value, (ClCharT *) compName,CL_MAX_NAME_LENGTH);
    compInstantiate.name.value[CL_MAX_NAME_LENGTH-1] = 0;
    compInstantiate.name.length =
        strlen((ClCharT *) compInstantiate.name.value);
    strncpy((ClCharT *) compInstantiate.proxyCompName.value, (ClCharT *) proxyCompName,CL_MAX_NAME_LENGTH);
    compInstantiate.proxyCompName.value[CL_MAX_NAME_LENGTH-1] = 0;
    compInstantiate.proxyCompName.length =
        strlen((ClCharT *) compInstantiate.proxyCompName.value);
    strncpy((ClCharT *) compInstantiate.nodeName.value, (ClCharT *) nodeName,CL_MAX_NAME_LENGTH);
    compInstantiate.nodeName.value[CL_MAX_NAME_LENGTH-1] = 0;
    compInstantiate.nodeName.length =
        strlen((ClCharT *) compInstantiate.nodeName.value);
    /*
     * Copy the srcInfo so that, cpm\L of that node can respond to it directly 
     */
    compInstantiate.srcAddress.nodeAddress = srcAddress->nodeAddress;
    compInstantiate.srcAddress.portId = srcAddress->portId;
    compInstantiate.rmdNumber = rmdNumber;

    if (cpmL->pCpmLocalInfo && cpmL->pCpmLocalInfo->status != CL_CPM_EO_DEAD)
    {
        ClIocPhysicalAddressT destNode = cpmL->pCpmLocalInfo->cpmAddress;
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        /*
         * Send the request to the respective CPM/L 
         */
        rc = CL_CPM_CALL_RMD_ASYNC_NEW(destNode.nodeAddress,
                                       destNode.portId,
                                       requestRmdNumber,
                                       (ClUint8T *) &compInstantiate,
                                       sizeof(ClCpmLifeCycleOprT),
                                       NULL,
                                       NULL,
                                       CL_RMD_CALL_ATMOST_ONCE,
                                       0,
                                       0,
                                       0,
                                       NULL,
                                       NULL,
                                       MARSHALL_FN(ClCpmLifeCycleOprT, 4, 0, 0));
        CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_LCM_OPER_FWD_ERR,
                       destNode.nodeAddress,
                       destNode.portId, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        clOsalMutexUnlock(gpClCpm->cpmTableMutex);
        /*
         * Bug 4522:
         * Print diffrent messages depending on whether the node to
         * which LCM request is sent is actually down or the node 
         * has not registered yet.
         */
        rc = CL_CPM_RC(CL_CPM_ERR_FORWARDING_FAILED);
        CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_LCM_NODE_UNAVAILABLE_ERR,
                       nodeName, rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP)
    }
    return rc;

  failure:
    return rc;
}

ClRcT cpmRouteRequest(ClCharT *compName,
                      ClCharT *proxyCompName,
                      ClCharT *nodeName,
                      ClIocPhysicalAddressT *srcAddress,
                      ClUint32T rmdNumber,
                      ClUint32T requestRmdNumber)
{
    return cpmRouteRequestWithCookie(compName, proxyCompName, 0, nodeName,
                                     srcAddress, rmdNumber, requestRmdNumber);
}

ClRcT VDECL(cpmClientInitialize)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle)
{
    ClCpmComponentT *comp = NULL;
    ClCpmCompInitSendT info = { 0 };
    ClCpmCompInitRecvT outBuff = { 0 };
    ClUint32T rc = CL_OK;

    rc = VDECL_VER(clXdrUnmarshallClCpmCompInitSendT, 4, 0, 0)(inMsgHandle, (void *) &info);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = cpmCompFind(info.compName, gpClCpm->compTable, &comp);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "component", info.compName, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                   ("Client Version is RC = %c\t MJC = %c\t MNC = %c\n",
                    info.cpmVersion.releaseCode, info.cpmVersion.majorVersion,
                    info.cpmVersion.minorVersion));

    /*
     * XXX NOT USED 
     */
    outBuff.cpmHandle = 0;

    /*
     * Do the Version checking 
     */
    memcpy(&(outBuff.cpmVersion), &(info.cpmVersion), sizeof(ClVersionT));
    /*
     * Verify the version information 
     */
    rc = clVersionVerify(&version_database, &(outBuff.cpmVersion));
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_VERSION_MISMATCH, rc, rc,
                   CL_LOG_WARNING, CL_LOG_HANDLE_APP);

    rc = VDECL_VER(clXdrMarshallClCpmCompInitRecvT, 4, 0, 0)((void *) &outBuff, outMsgHandle, 0);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    /*
     * If version is supported then move ahead with the initialization 
     */
    /*
     * comp->eoPort = info.eoPort;
     */

    /*
     * TODO: Multiple process, hence multiple PIDs 
     */
    comp->processId = info.myPid;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                   ("Inside cpmClientInitialize CompName %s\t compId %d\t eoPort %x\n",
                    comp->compConfig->compName, comp->compId, comp->eoPort));

    return rc;

  failure:
    return rc;
}

ClRcT VDECL(cpmClientFinalize)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmComponentT *comp = NULL;
    ClCpmCompFinalizeSendT info;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Inside cpmClientFinalize \n"));

    rc = VDECL_VER(clXdrUnmarshallClCpmCompFinalizeSendT, 4, 0, 0)(inMsgHandle, (void *) &info);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = cpmCompFind(info.compName, gpClCpm->compTable, &comp);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "component", info.compName, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    return rc;

  failure:
    return rc;
}

ClRcT VDECL(cpmComponentRegister)(ClEoDataT data,
                                  ClBufferHandleT inMsgHandle,
                                  ClBufferHandleT outMsgHandle)
{
    ClCpmComponentT *comp = NULL;
    ClCpmComponentT *proxyComp = NULL;
    ClUint32T rc = CL_OK;
    ClCpmCompRegisterT info = {{0}};
    ClBoolT isProxied = CL_FALSE;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Inside cpmComponentRegister \n"));

    rc = VDECL_VER(clXdrUnmarshallClCpmCompRegisterT, 4, 0, 0)(inMsgHandle, (void *) &info);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    if (info.proxyCompName.length)
        isProxied = CL_TRUE;

    rc = cpmCompFind(info.compName.value, gpClCpm->compTable, &comp);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "component", info.compName.value, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    if (isProxied)
    {
        if (! ((comp->compConfig->compProperty ==
                CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE) ||
               (comp->compConfig->compProperty ==
                CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)))
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                       "Attempt to register [%.*s] as proxied component with "
                       "[%.*s] as its proxy. "
                       "But [%.*s] is not modelled as a proxied component.",
                       info.compName.length, info.compName.value,
                       info.proxyCompName.length, info.proxyCompName.value,
                       info.compName.length, info.compName.value);
            
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = cpmCompFind(info.proxyCompName.value, gpClCpm->compTable, &proxyComp);
        CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                       "component", info.proxyCompName.value, rc, rc, CL_LOG_DEBUG,
                       CL_LOG_HANDLE_APP);

        if (proxyComp->compConfig->compProperty != CL_AMS_COMP_PROPERTY_SA_AWARE)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                       "Attempt to register [%.*s] as proxied component with "
                       "[%.*s] as its proxy. "
                       "But [%.*s] is not a saf aware component, so it "
                       "cannot act as a proxy.",
                       info.compName.length, info.compName.value,
                       info.proxyCompName.length, info.proxyCompName.value,
                       info.proxyCompName.length, info.proxyCompName.value);
            
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }
        
        if (comp->proxyCompRef != NULL)
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_LCM_REG_MULTI_ERR,
                           info.compName.value, CL_CPM_RC(CL_CPM_ERR_EXIST),
                           CL_LOG_ERROR, CL_LOG_HANDLE_APP);
        /*
         * proxyComp is the component proxying the comp 
         */
        comp->proxyCompRef = proxyComp;

        strcpy(comp->proxyCompName, info.proxyCompName.value);

        /*
         * Increase the references 
         */
        comp->proxyCompRef->numProxiedComps += 1;

        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFO, NULL,
                   CL_CPM_LOG_4_LCM_PROXY_COMP_REG_INFO, "Registering",
                   info.compName.value, info.proxyCompName.value, info.eoPort);
    }
    else
    {
        if (comp->compPresenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED)
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_LCM_REG_MULTI_ERR,
                           info.compName.value, CL_CPM_RC(CL_CPM_ERR_EXIST),
                           CL_LOG_ERROR, CL_LOG_HANDLE_APP);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFO, NULL,
                   CL_CPM_LOG_3_LCM_COMP_REG_INFO, "Registering",
                   info.compName.value, info.eoPort);

        if (IS_ASP_COMP(comp)) clTimerStop(comp->cpmInstantiateTimerHandle);

        /*
         * Update the component States 
         */
        comp->compPresenceState = CL_AMS_PRESENCE_STATE_INSTANTIATED;

        clOsalMutexLock(comp->compMutex);
        comp->compOperState = CL_AMS_OPER_STATE_ENABLED;
        comp->compReadinessState = CL_AMS_READINESS_STATE_INSERVICE;

        /*
         * Remember the context/source from where the request is originated 
         */
        comp->eoPort = info.eoPort;
        clOsalMutexUnlock(comp->compMutex);

#ifdef CL_CPM_GMS
        /* If the regisration is for GMS and restart count is not zero, 
           create the timer, so that the join can heppen */
        if((gpClCpm->cpmGmsHdl != CL_HANDLE_INVALID_VALUE) &&
           (comp->eoPort == CL_IOC_GMS_PORT))
        {
            ClTimerTimeOutT cpmTimeOut = { 0, 0};
            
            cpmTimeOut.tsSec = 0;
            cpmTimeOut.tsMilliSec = 2000;

            rc = clTimerCreateAndStart(cpmTimeOut,
                                       CL_TIMER_ONE_SHOT,
                                       CL_TIMER_SEPARATE_CONTEXT,
                                       cpmGmsTimerCallback,
                                       (void *) comp,
                                       &(gpClCpm->gmsTimerHandle));
        }
#endif
        /*
         * Check if it is Event 
         */
        if (comp->eoPort == CL_IOC_EVENT_PORT)
        {
            gpClCpm->emUp = 1;
        }

        if(comp->eoPort == CL_IOC_COR_PORT)
        {
            gpClCpm->corUp = 1;
        }

        if (comp->eoPort == CL_IOC_CKPT_PORT 
            &&
            gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL)
        {
            if (gpClCpm->nodeEventPublished != 1)
            {
                ClNameT nodeName = {0};

                strcpy(nodeName.value, gpClCpm->pCpmConfig->nodeName);
                nodeName.length = strlen(gpClCpm->pCpmConfig->nodeName);

                /*
                 * Node Arrival Event Publish 
                 */
                rc = nodeArrivalDeparturePublish(clIocLocalAddressGet(),
                                                 nodeName, CL_CPM_NODE_ARRIVAL);
                if (rc == CL_OK)
                    gpClCpm->nodeEventPublished = 1;

            }
        }
        rc = cpmCompRespondToCaller(comp, CL_CPM_INSTANTIATE, CL_OK);
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to respond to caller %x\n", rc),
                     rc);
    }

    cpmComponentEventPublish(comp, CL_CPM_COMP_ARRIVAL, CL_FALSE);
    
    return rc;

    failure:
    return rc;
}

ClRcT VDECL(cpmComponentUnregister)(ClEoDataT data,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmComponentT *comp = NULL;
    ClCpmComponentT *proxyComp = NULL;
    ClCpmCompRegisterT info;
    ClBoolT isProxied = CL_FALSE;
    ClCpmEOListNodeT *ptr = NULL;
    ClCpmEOListNodeT *pTemp = NULL;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Inside cpmComponentUnregister \n"));

    rc = VDECL_VER(clXdrUnmarshallClCpmCompRegisterT, 4, 0, 0)(inMsgHandle, (void *) &info);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    if (info.proxyCompName.length)
        isProxied = CL_TRUE;

    rc = cpmCompFind(info.compName.value, gpClCpm->compTable, &comp);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "component", info.compName.value, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    comp->compTerminated = CL_TRUE;
    if (isProxied)
    {
        if (! ((comp->compConfig->compProperty ==
                CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE) ||
               (comp->compConfig->compProperty ==
                CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)))
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                       "Attempt to unregister [%.*s] as proxied component with "
                       "[%.*s] as its proxy. "
                       "But [%.*s] is not modelled as a proxied component.",
                       info.compName.length, info.compName.value,
                       info.proxyCompName.length, info.proxyCompName.value,
                       info.compName.length, info.compName.value);
            
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }

        rc = cpmCompFind(info.proxyCompName.value, gpClCpm->compTable,
                         &proxyComp);
        CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                       "component", info.compName, rc, rc, CL_LOG_DEBUG,
                       CL_LOG_HANDLE_APP);
        /*
         * Check whether it is the right proxy, which is unregistering the
         * proxied component 
         */
        if (strcmp(info.proxyCompName.value, comp->proxyCompName) != 0)
        {
            rc = CL_CPM_RC(CL_CPM_ERR_BAD_OPERATION);
            CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_LCM_INVALID_PROXY_ERR,
                           info.proxyCompName.value, info.compName,
                           comp->proxyCompName, rc, CL_LOG_ERROR,
                           CL_LOG_HANDLE_APP);
            goto failure;
        }

        /*
         * Decrease the references 
         */
        comp->proxyCompRef->numProxiedComps -= 1;
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFO, NULL,
                   CL_CPM_LOG_4_LCM_PROXY_COMP_REG_INFO, "Unregistering",
                   info.compName.value, info.proxyCompName.value, info.eoPort);
        /*
         * ProxyComp is the component proxying the comp 
         */
        comp->proxyCompRef = NULL;
        strcpy(comp->proxyCompName, "\0");

        cpmComponentEventPublish(comp, CL_CPM_COMP_DEPARTURE, CL_FALSE);
    }
#if 0
    else if (comp->compPresenceState == CL_AMS_PRESENCE_STATE_UNINSTANTIATED)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                       ("Already unregistered this component %s\n",
                        info.compName.value));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_WARNING, NULL,
                   CL_CPM_LOG_1_LCM_REG_MULTI_ERR, info.compName.value);
    }
#endif
    else
    {
        if (comp->numProxiedComps != 0)
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_LCM_NOT_UNREG_PROXY_ERR,
                           info.compName.value,
                           CL_CPM_RC(CL_CPM_ERR_BAD_OPERATION), CL_LOG_ERROR,
                           CL_LOG_HANDLE_APP);

        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFO, NULL,
                   CL_CPM_LOG_3_LCM_COMP_REG_INFO, "Unregistering",
                   info.compName.value, info.eoPort);

        cpmComponentEventPublish(comp, CL_CPM_COMP_DEPARTURE, CL_FALSE);
        /*
         * Cleanup the components event channel if any.
         */
        cpmComponentEventCleanup(comp);
        /*
         * Cleanup the EOs of that Component 
         */
        rc = clOsalMutexLock(gpClCpm->eoListMutex);
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to lock mutex %x\n", rc), rc);

        ptr = comp->eoHandle;
        while (ptr != NULL && ptr->eoptr != NULL)
        {
            if (ptr->eoptr->eoPort == CL_IOC_EVENT_PORT)
            {
                gpClCpm->emUp = 0;

            }
            pTemp = ptr;
            ptr = ptr->pNext;
            clHeapFree(pTemp->eoptr);
            pTemp->eoptr = NULL;
            clHeapFree(pTemp);
            pTemp = NULL;
        }
        comp->eoHandle = NULL;
        comp->eoCount = 0;
        comp->eoPort = 0;

        rc = clOsalMutexUnlock(gpClCpm->eoListMutex);
        CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to unlock mutex %x\n", rc), rc);

        if (comp->restartPending)
        {
            /*
             * FIXME: Start the timer and after expiry of timer execute the
             * instantiate 
             */
            /*
             * Wait atleast 4000 milliSeconds 
             */

            
            /*
             * Modified for bug 4028.
             *  Moved instantiation to cpmResponse().
             *  Also reverting back to timed wait.
             */
#if 0
            cpmTimerWait(comp, CL_CPM_RESTART_SLEEP);
#endif
            /*
             * End of modification for Bug 4028.
             */
#if 0
            CPM_TIMED_WAIT(CL_CPM_RESTART_SLEEP);

            _cpmComponentInstantiate(comp->compConfig->compName,
                                     gpClCpm->pCpmConfig->nodeName,
                                     0,
                                     &(comp->requestSrcAddress),
                                     comp->requestRmdNumber);
#endif
        }
    }

    return rc;

  failure:
    return rc;
}

static void getAspBinPath(ClCpmComponentT* comp, ClCharT *path, ClInt32T maxSize)
{
    ClCharT* t1  = NULL;
    ClInt32T bytes = 0;
    path[0] = 0;
    if (comp->compConfig->instantiationCMD[0] != '/') 
    {
        t1 = getenv(CL_ASP_BINDIR_PATH);
        if (t1)
        {
            strncpy(path, t1, maxSize-1);  /* Its relative to our binary directory */
            strcat(path, "/");                  
        }
            
    }
    bytes = CL_MIN((ClInt32T)strlen(path), maxSize);
    maxSize -= bytes;
    strncat(path, comp->compConfig->instantiationCMD, maxSize);
    /* Ok, now we have the full name of the executable.  Find the last / and chop */
    t1 = strrchr(path,'/');
    if (t1) *t1 = 0;
    else path[0] = 0; /* If there is not /, I guess there is no path */
}

void cpmSetEnvs(ClCpmComponentT *comp)
{
    ClCharT iocAddress[12] = { 0 };
    ClUint32T i = 0;

    setenv("ASP_COMPNAME", comp->compConfig->compName, 1);

    setenv("ASP_NODENAME", gpClCpm->pCpmConfig->nodeName, 1);

    sprintf(iocAddress, "%d", (ClInt32T) clIocLocalAddressGet());
    setenv("ASP_NODEADDR", iocAddress, 1);

    if (1)
    {
        ClCharT temp[CL_MAX_NAME_LENGTH*2];
        getAspBinPath(comp, temp, sizeof(temp)-1);
        setenv("ASP_APP_BINDIR", temp, 1);
    }
    
    /*
     * Set component specific environment variables.
     */
    for (i = 0; comp->compConfig->env[i]; ++i)
    {
        setenv(comp->compConfig->env[i]->envName,
               comp->compConfig->env[i]->envValue, 1);
    }
}

#ifndef POSIX_BUILD
void cpmSaAwareExecImage(void *arg)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)arg;

    ClCharT imageName[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *imagePath = NULL;

    cpmSetEnvs(comp);

    imagePath = getenv(CL_ASP_BINDIR_PATH);
    if (imagePath != NULL)
    {
        if (cpmIsValgrindBuild(comp->compConfig->instantiationCMD)
            ||
            comp->compConfig->argv[0][0] == '/')
        {
            strncpy(imageName,comp->compConfig->argv[0],CL_MAX_NAME_LENGTH);
        }
        else
        {
            snprintf(imageName, CL_MAX_NAME_LENGTH, "%s/%s", imagePath, comp->compConfig->argv[0]);
        }

        
        if (-1 == execvp(imageName, comp->compConfig->argv))
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                       "Unable to launch binary image [%s] "
                       "for component [%s] : [%s]",
                       imageName,
                       comp->compConfig->compName,
                       strerror(errno));
            exit(1);
        }
    }
    else
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                      "The ASP_BINDIR environmental variable is not set !");
        exit(1);
    }
}
#else
void cpmCopyEnv(ClCharT **old_envp, ClCharT ***new_envp)
{
    ClInt32T i;
    ClInt32T j;
    ClInt32T nenv;
    ClInt32T skipIndex = -1;
    ClCharT **newenvp = NULL;
    
    for (i = 0, nenv = 0; old_envp[i]; ++i)
    {
        if(!strncmp(old_envp[i], "ASP_COMPNAME=", strlen("ASP_COMPNAME=")))
        {
            skipIndex = i;
            continue;
        }
        ++nenv;
    }

    newenvp = clHeapAllocate((nenv+1) * sizeof(ClCharT *));
    CL_ASSERT(newenvp != NULL);

    for (i = 0, j = 0; old_envp[i]; ++i)
    {
        int len ;
        if(i == skipIndex) continue;
        len = strlen(old_envp[i]);
        
        newenvp[j] = clHeapAllocate(len+1);
        CL_ASSERT(newenvp[j] != NULL);
        
        strcpy(newenvp[j++], old_envp[i]);
    }
    newenvp[j] = NULL;

    *new_envp = newenvp;

    return;
}

void cpmAppendEnv(ClCharT ***env, ClCharT *name, ClCharT *value)
{                                
    ClInt32T i;                       
    ClCharT buf[BUFSIZ];           
    ClInt32T len;
    ClCharT **p = *env;

    for (i = 0; p[i]; ++i);
                                 
    *env = clHeapRealloc(*env, (i+2) * sizeof(ClCharT *));
    CL_ASSERT(*env != NULL);                           

    p = *env;
    
    snprintf(buf, BUFSIZ, "%s=%s", name, value);
    len = strlen(buf);
                                                   
    p[i] = clHeapAllocate(len+1);                        
    CL_ASSERT(p[i] != NULL);                      

    strcpy(p[i], buf);
                                                   
    p[++i] = NULL;
}

void cpmFreeEnv(ClCharT **env)
{
    ClInt32T i;
    ClCharT **p = env;

    for(i = 0 ; p[i]; ++i) clHeapFree(p[i]);
    clHeapFree(env);
}


void cpmSaAwareExecImage(void *arg)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)arg;

    ClCharT imageName[CL_MAX_NAME_LENGTH] = {0};
    ClCharT *imagePath = NULL;

    ClCharT **new_envp = NULL;

    extern ClCharT **gEnvp;

    gEnvp = environ;

    cpmCopyEnv(gEnvp, &new_envp);

    if (!new_envp)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                   "Unable to copy the parent environment variables");
        return;
    }

    cpmAppendEnv(&new_envp, "ASP_COMPNAME", comp->compConfig->compName);
    cpmAppendEnv(&new_envp, "ASP_NODENAME", gpClCpm->pCpmConfig->nodeName);
    {
        ClCharT iocAddress[CL_MAX_NAME_LENGTH];
     
        sprintf(iocAddress, "%d", (ClInt32T) clIocLocalAddressGet());
        cpmAppendEnv(&new_envp,"ASP_NODEADDR", iocAddress);
    }

    {
        int i;

        /*
         * Append component specific environment variables.
         */
        for (i = 0; comp->compConfig->env[i]; ++i)
        {
            cpmAppendEnv(&new_envp,
                       comp->compConfig->env[i]->envName,
                       comp->compConfig->env[i]->envValue);
        }
    }

    imagePath = getenv(CL_ASP_BINDIR_PATH);
    if (imagePath != NULL)
    {
        ClCharT temp[CL_MAX_NAME_LENGTH*2];
        getAspBinPath(comp, temp, sizeof(temp)-1);
        cpmAppendEnv(&new_envp, "ASP_APP_BINDIR", temp);

        if (comp->compConfig->argv[0][0] == '/')
        {
            strncpy(imageName,comp->compConfig->argv[0],CL_MAX_NAME_LENGTH);
        }
        else
        {
            snprintf(imageName, CL_MAX_NAME_LENGTH, "%s/%s", imagePath, comp->compConfig->argv[0]);
        }

#ifdef VXWORKS_BUILD
        if (ERROR == (comp->processId = rtpSpawn(imageName, (const char **)comp->compConfig->argv, 
                                                 (const char **)new_envp, 
                                                 200, CL_RTP_STACK_SIZE, 0, VX_FP_TASK)))
#elif defined(QNX_BUILD)
        if (-1 == (comp->processId = spawnvpe(P_NOWAITO,imageName, comp->compConfig->argv, new_envp)))
#endif
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                       "Unable to launch binary image [%s] "
                       "for component [%s] because of errno [%d], error [%s]",
                       comp->compConfig->argv[0],
                       comp->compConfig->compName,
                       errno, strerror(errno));
            exit(1);
        }
    }
    else
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                      "The ASP_BINDIR environmental variable is not set !");
        exit(1);
    }
    cpmFreeEnv(new_envp);
}
#endif



ClRcT _cpmSaAwareComponentInstantiate(ClCharT *compName,
                                      ClCpmComponentT *comp)
{
    ClRcT rc = CL_OK;
    ClOsalPidT childPid = 0;

    comp->requestType = CL_CPM_INSTANTIATE;
    
    if ((CL_CPM_COMP_SINGLE_PROCESS == comp->compConfig->compProcessRel) ||
        (CL_CPM_COMP_MULTI_PROCESS == comp->compConfig->compProcessRel))
    {
        ClCharT binary[CL_MAX_NAME_LENGTH] = {0};
        ClCharT *pBinPath = getenv(CL_ASP_BINDIR_PATH);

        clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                    "Launching binary image [%s] as component [%s]...",
                    comp->compConfig->argv[0],
                    comp->compConfig->compName);

        if (!pBinPath)
        {
            clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                          "The ASP_BINDIR environmental variable is not set !");

            rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_FAILED);

            goto failure;
        }

        if (!cpmIsValgrindBuild(comp->compConfig->instantiationCMD))
        {
            /* If it is an absolute path, just use it, otherwise prepend the ASP_BINDIR */
            if (comp->compConfig->argv[0][0] == '/')
                strncpy(binary,comp->compConfig->argv[0],sizeof(binary)-1);
            else
              snprintf(binary,
                     sizeof(binary),
                     "%s/%s",
                     pBinPath,
                     comp->compConfig->argv[0]);

            if (access(binary, F_OK|X_OK))
            {
                clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                              "Binary [%s] access error [%s] !",
                              binary,
                              strerror(errno));

                rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_FAILED);
                goto failure;
            }
        }
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_NOT_IMPLEMENTED);
        goto failure;
    }

    if (IS_ASP_COMP(comp)) clCompTimerInstantiate(comp);
    
    rc = clOsalProcessCreate(cpmSaAwareExecImage, 
                             comp, 
                             CL_OSAL_PROCESS_WITH_NEW_SESSION |
                             CL_OSAL_PROCESS_WITH_NEW_GROUP,
                             &childPid);

    if (CL_OK != rc)
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                      "Failed to start [%s], error [%#x]",
                      comp->compConfig->argv[0],
                      rc);
        if (IS_ASP_COMP(comp))clTimerStop(comp->cpmInstantiateTimerHandle);
        goto failure;
    }
        
#ifndef POSIX_BUILD
    comp->processId = childPid;
#endif

    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
              "Component [%s] started, PID [%d]",
              comp->compConfig->compName,
              comp->processId);

    return CL_OK;

    failure:
    cpmCompRespondToCaller(comp, comp->requestType, rc);

    return CL_OK;
}

ClRcT _cpmProxiedComponentInstantiate(ClCharT *compName,
                                      ClCharT *proxyCompName,
                                      ClCpmComponentT *comp)
{
    ClCpmClientCompTerminateT compInstantiate;
    ClCpmComponentT *proxyComp = NULL;
    ClRcT rc = CL_OK;

    if ((compName == NULL) || (proxyCompName == NULL) ||
        (strcmp(compName, proxyCompName) == 0))
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_LOG_MESSAGE_1_INVALID_PARAMETER,
                __FUNCTION__, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }

    strcpy(compInstantiate.compName.value, compName);
    compInstantiate.compName.length = strlen(compInstantiate.compName.value);
    compInstantiate.requestType = CL_CPM_PROXIED_INSTANTIATE;
    
    rc = cpmInvocationAdd(CL_CPM_PROXIED_INSTANTIATE_CALLBACK, (void *) comp,
                          &compInstantiate.invocation,
                          CL_CPM_INVOCATION_CPM | CL_CPM_INVOCATION_DATA_SHARED);

    rc = cpmCompFind(proxyCompName, gpClCpm->compTable, &proxyComp);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "component", proxyCompName, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_INFORMATIONAL, NULL,
            CL_CPM_LOG_3_LCM_PROXY_OPER_INFO, "instantiating",
            comp->compConfig->compName, proxyComp->eoPort);

    /*
     * TimeOut has been taken care of by provoding the RMD timeout as half
     * of the actual timeout and one retry.
     * Also make RMD to the eoPort of the proxy component.
     */
    rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                   proxyComp->eoPort,
                                   CPM_PROXIED_COMP_INSTANTIATION_FN_ID,
                                   (ClUint8T *) &(compInstantiate),
                                   sizeof(ClCpmClientCompTerminateT),
                                   NULL,
                                   NULL,
                                   CL_RMD_CALL_ATMOST_ONCE,
                                   (proxyComp->compConfig->
                                    compProxiedCompInstantiateCallbackTimeout
                                    / 2),
                                   1,
                                   0,
                                   NULL,
                                   NULL,
                                   MARSHALL_FN(ClCpmClientCompTerminateT, 4, 0, 0));
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD to proxy component failed %x\n", rc),
            rc);

    return rc;

  failure:
    comp->compPresenceState = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;

    clOsalMutexLock(comp->compMutex);
    comp->compOperState = CL_AMS_OPER_STATE_DISABLED;
    comp->compReadinessState = CL_AMS_READINESS_STATE_OUTOFSERVICE;
    clOsalMutexUnlock(comp->compMutex);

    cpmCompRespondToCaller(comp, CL_CPM_INSTANTIATE, rc);
    return rc;
}

#ifndef POSIX_BUILD
void cpmNonProxiedNonPreinstantiableExecImage(void *arg)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)arg;

    cpmSetEnvs(comp);

    if (-1 == execvp(comp->compConfig->argv[0], comp->compConfig->argv))
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                   "Unable to launch binary image [%s] "
                   "for component [%s] : [%s]",
                   comp->compConfig->argv[0],
                   comp->compConfig->compName,
                   strerror(errno));
        exit(1);
    }
}
#else
void cpmNonProxiedNonPreinstantiableExecImage(void *arg)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)arg;
    ClCharT **new_envp = NULL;
    extern ClCharT **gEnvp;
    gEnvp = environ;
    cpmCopyEnv(gEnvp, &new_envp);
    if (!new_envp)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                   "Unable to copy the parent environment variables");
        return;
    }

    cpmAppendEnv(&new_envp, "ASP_COMPNAME", comp->compConfig->compName);
    cpmAppendEnv(&new_envp, "ASP_NODENAME", gpClCpm->pCpmConfig->nodeName);
    {
        ClCharT iocAddress[CL_MAX_NAME_LENGTH];
     
        sprintf(iocAddress, "%d", (ClInt32T) clIocLocalAddressGet());
        cpmAppendEnv(&new_envp,"ASP_NODEADDR", iocAddress);
    }
    if (1)
    {
        ClCharT temp[CL_MAX_NAME_LENGTH*2];
        getAspBinPath(comp, temp, sizeof(temp)-1);
        cpmAppendEnv(&new_envp, "ASP_APP_BINDIR", temp);
    }

    {
        int i;

        /*
         * Append component specific environment variables.
         */
        for (i = 0; comp->compConfig->env[i]; ++i)
        {
            cpmAppendEnv(&new_envp,
                       comp->compConfig->env[i]->envName,
                       comp->compConfig->env[i]->envValue);
        }
    }

#ifdef VXWORKS_BUILD
    if (ERROR == (comp->processId =  rtpSpawn(comp->compConfig->argv[0], (const char **)comp->compConfig->argv, 
                                              (const char **)new_envp, 150, CL_RTP_STACK_SIZE, 0, VX_FP_TASK)))
#elif defined(QNX_BUILD)
    if (-1 == (comp->processId = spawnvpe(P_NOWAITO, comp->compConfig->argv[0], comp->compConfig->argv, new_envp)))
#endif
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                   "Unable to launch binary image [%s] "
                   "for component [%s] because of errno [%d], error [%s]",
                   comp->compConfig->argv[0],
                   comp->compConfig->compName,
                   errno,
                   strerror(errno));
        exit(1);
    }
}
#endif

ClRcT cpmNonProxiedNonPreinstantiableCompInstantiate(ClCpmComponentT *comp)
{
    ClRcT rc = CL_OK;
    ClOsalPidT childPid = 0;

    clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
               "Starting non proxied non preinstantiable component [%s]...",
               comp->compConfig->compName);
    
    clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                "Launching binary image [%s] as component [%s]...",
                comp->compConfig->argv[0],
                comp->compConfig->compName);

    rc = clOsalProcessCreate(cpmNonProxiedNonPreinstantiableExecImage,
                             comp, 
                             CL_OSAL_PROCESS_WITH_NEW_SESSION |
                             CL_OSAL_PROCESS_WITH_NEW_GROUP,
                             &childPid);
    if (CL_OK != rc)
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                      "Failed to start [%s], error [%#x]",
                      comp->compConfig->argv[0],
                      rc);
        goto failure;
    }

#ifndef POSIX_BUILD        
    comp->processId = childPid;
#endif

 failure:
    return rc;

}
                                                     
ClRcT _cpmComponentInstantiate(ClCharT *compName,
                               ClCharT *proxyCompName,
                               ClUint64T instantiateCookie,
                               ClCharT *nodeName,
                               ClIocPhysicalAddressT *srcAddress,
                               ClUint32T rmdNumber)
{
    ClCpmComponentT *comp = NULL;
    ClUint32T rc = CL_OK;

    if ((gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL) &&
        (strcmp
         ((ClCharT *) nodeName, (ClCharT *) gpClCpm->pCpmConfig->nodeName)))
    {
        rc = cpmRouteRequestWithCookie(compName, proxyCompName, instantiateCookie, nodeName, 
                                       srcAddress, rmdNumber, CPM_COMPONENT_INSTANTIATE);
        if (rc != CL_OK)
        {
            cpmRequestFailedResponse(compName, nodeName, srcAddress, rmdNumber,
                                     CL_CPM_INSTANTIATE, rc);
            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("Request Route failed for component %s to node %s\n",
                          compName, nodeName), rc);
        }
    }
    else
    {
        /*
         * Find the component Name from the component Hash Table 
         * Its a local component, do the needful 
         */
        ClIocPortT lastEoPort = 0;

        rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
        if (rc != CL_OK)
        {
            /*
             * Try checking if its a dynamic comp. added on the master.
             */
            if(cpmComponentAddDynamic(compName) == CL_OK)
            {
                rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
            }
            if(rc != CL_OK)
            {
                cpmRequestFailedResponse(compName, nodeName, srcAddress, rmdNumber,
                                         CL_CPM_INSTANTIATE, rc);
                CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                               "component", compName, rc, rc, CL_LOG_DEBUG,
                               CL_LOG_HANDLE_APP);
            }
        }

        if (srcAddress != NULL)
            memcpy(&(comp->requestSrcAddress), srcAddress,
                   sizeof(ClIocPhysicalAddressT));
        comp->requestRmdNumber = rmdNumber;
        comp->compEventPublished = CL_FALSE;
        lastEoPort = comp->eoPort;
        comp->eoPort = 0;
        comp->instantiateCookie = instantiateCookie;
        comp->compTerminated = CL_FALSE;
        switch (comp->compPresenceState)
        {
            case CL_AMS_PRESENCE_STATE_UNINSTANTIATED:
            {
                comp->compPresenceState = CL_AMS_PRESENCE_STATE_INSTANTIATING;
                if (comp->compConfig->compProperty ==
                    CL_AMS_COMP_PROPERTY_SA_AWARE)
                {
                    rc = _cpmSaAwareComponentInstantiate(compName, comp);
                }
                else if (comp->compConfig->compProperty ==
                         CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE)
                {
                    comp->requestType = CL_CPM_PROXIED_INSTANTIATE;
                    rc = _cpmProxiedComponentInstantiate(compName, proxyCompName, comp);
                }
                else if (comp->compConfig->compProperty ==
                         CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)
                {
                    rc = CL_CPM_RC(CL_ERR_OP_NOT_PERMITTED);
                    goto failure;
                }
                else if (comp->compConfig->compProperty ==
                         CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE)
                {
                    rc = cpmNonProxiedNonPreinstantiableCompInstantiate(comp);
                    if (CL_OK != rc)
                    {
                        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                                   "Unable to instantiate [%s], error [%#x]",
                                   comp->compConfig->compName,
                                   rc);
                        goto failure;
                    }
                    comp->compPresenceState = CL_AMS_PRESENCE_STATE_INSTANTIATED;
                }
                break;
            }
            case CL_AMS_PRESENCE_STATE_INSTANTIATED:
            {
                comp->eoPort = lastEoPort;
                cpmCompRespondToCaller(comp, CL_CPM_INSTANTIATE, CL_OK);
                rc = CL_OK;
                break;
            }
            case CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED:
            case CL_AMS_PRESENCE_STATE_TERMINATION_FAILED:
            {
                rc = CL_CPM_RC(CL_ERR_INVALID_STATE);
                cpmCompRespondToCaller(comp, CL_CPM_INSTANTIATE, rc);
                break;
            }
            case CL_AMS_PRESENCE_STATE_INSTANTIATING:
            case CL_AMS_PRESENCE_STATE_TERMINATING:
            case CL_AMS_PRESENCE_STATE_RESTARTING:
            {
                rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_IN_PROGRESS);
                cpmCompRespondToCaller(comp, CL_CPM_INSTANTIATE, rc);
                break;
            }
            default:
            {
                rc = CL_CPM_RC(CL_ERR_INVALID_STATE);
                break;
            }
        }
    }
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_LCM_COMP_OPER1_ERR,
                   "instantiate", comp->compConfig->compName, rc, rc,
                   CL_LOG_ERROR, CL_LOG_HANDLE_APP);
    return rc;

failure:
    return rc;
}

ClRcT VDECL(cpmComponentInstantiate)(ClEoDataT data,
                                     ClBufferHandleT inMsgHandle,
                                     ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmLifeCycleOprT info = {{0}};

    rc = VDECL_VER(clXdrUnmarshallClCpmLifeCycleOprT, 4, 0, 0)(inMsgHandle, (void *) &info);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = _cpmComponentInstantiate((info.name).value,
                                  (info.proxyCompName).value,
                                  info.instantiateCookie,
                                  (info.nodeName).value,
                                  &(info.srcAddress), 
                                  info.rmdNumber);

    return rc;

 failure:
    return rc;
}

#ifdef __linux__
/*
 * Utility function to check if a component PID is valid.
 * Specific to linux.
 */
ClBoolT cpmCompIsValidPid(ClCpmComponentT *comp)
{
    FILE *fp = NULL;

    ClBoolT valid = CL_NO;

    ClCharT procFileName[CL_MAX_NAME_LENGTH] = {0};
    ClCharT procCmdLine[CL_MAX_NAME_LENGTH] = {0};

    ClCharT *compName = comp->compConfig->argv[0];

    snprintf(procFileName, CL_MAX_NAME_LENGTH, "%s/%d/%s", 
             "/proc", comp->processId, "cmdline");
    fp = fopen(procFileName, "r");
    if (!fp)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                   "Unable to open file [%s] : [%s]",
                   procFileName,
                   strerror(errno));
        goto invalid;
    }

    if(!fgets(procCmdLine, CL_MAX_NAME_LENGTH-1, fp)) procCmdLine[0] = 0;
    if (!strncmp(procCmdLine, compName, strlen(compName)))
    {
        valid = CL_YES;
    }
    else
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                   "The /proc command line is [%s], but "
                   "the component executable name is [%s]",
                   procCmdLine, 
                   comp->compConfig->argv[0]);
    }

    fclose(fp);

    return valid;

 invalid :
    return CL_NO;
}
#else
ClBoolT cpmCompIsValidPid(ClCpmComponentT *comp)
{
    return CL_YES;
}
#endif

static ClRcT cpmNonProxiedNonPreinstantiableCompTerminate(ClCpmComponentT *comp, 
                                                          ClBoolT cleanup)
{
    ClRcT rc = CL_CPM_RC(CL_ERR_LIBRARY);

    clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
               "Stopping non proxied non preinstantiable component [%s]...",
               comp->compConfig->compName);

    if (comp->processId)
    {
        if (cpmCompIsValidPid(comp))
        {
            ClCharT *command = NULL;

            if(cleanup)
            {
                command = comp->compConfig->cleanupCMD;
            }
            else
            {
                command = comp->compConfig->terminationCMD;
            }
                
            /*
             * Invoke the terminate command if its defined.
             */
            if(command[0])
            {
                /*
                 * Just run the system command which is POSIX. 
                 */
                if(system(command))
                {
                    clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                               "%s command [%s] error [%s]",
                               cleanup ? "Cleanup" : "Terminate",
                               command, strerror(errno));
                    goto out;
                }
                else
                {
                    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                              "%s command [%s] successful.",
                              cleanup ? "Cleanup" : "Terminate",
                              command);
                }
            }
            else
            {
                /*
                 * No terminate command defined. So issue a SIGKILL.
                 */
                clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "No %s command specified. Sending SIGKILL " "to component [%s]", 
                          cleanup ? "cleanup" : "terminate", comp->compConfig->compName);

                if (comp->hbFailureDetected)
                {
                    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Sending Signal[%d]to component [%s] due to health check failure", gCompHealthCheckFailSendSignal, comp->compConfig->compName);
                    rc = kill(comp->processId, gCompHealthCheckFailSendSignal);
                    comp->hbFailureDetected = CL_FALSE;
                }
                else
                {
                    rc = kill(comp->processId, SIGKILL);
                }
                if (-1 == rc)
                {
                    rc = CL_CPM_RC(CL_ERR_LIBRARY);
                    clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Unable to stop component [%s] : [%s]", comp->compConfig->compName, strerror(errno));
                    goto out;
                }
            }
        }
        else
        {
            rc = CL_CPM_RC(CL_ERR_NOT_EXIST);

            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                       "Component [%s]'s PID [%d] is invalid.",
                       comp->compConfig->compName,
                       comp->processId);
            clLogMultiline(CL_LOG_ERROR, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                           "Possible reasons for these are : \n"
                           "1. CPM was not able to start the component.\n"
                           "2. Component was started, but it "
                           "exited gracefully.\n"
                           "3. Component was started and it "
                           "crashed.");
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                       "Please note that for non proxied non preinstantiable "
                       "components only starting and stopping functionalities "
                       "are supported.");
            goto out;
        }
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_NOT_EXIST);
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                   "Hmm... I think something is wrong here, "
                   "tried to kill component [%s], with PID 0 !",
                   comp->compConfig->compName);
        goto out;
    }
    
    rc = CL_OK;

    out:
    return rc;
}

ClRcT _cpmComponentTerminate(ClCharT *compName,
                             ClCharT *proxyCompName,
                             ClCharT *nodeName,
                             ClIocPhysicalAddressT *srcAddress,
                             ClUint32T rmdNumber)
{
    ClCpmComponentT *comp = NULL;
    ClCpmComponentT *proxyComp = NULL; /* PROXIED Support: Added Var */
    ClCpmClientCompTerminateT compTerminate = { 0 };
    ClUint32T rc = CL_OK;

    if ((gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL) &&
        (strcmp
         ((ClCharT *) nodeName, (ClCharT *) gpClCpm->pCpmConfig->nodeName)))
    {
        rc = cpmRouteRequest(compName, proxyCompName, nodeName, srcAddress, rmdNumber,
                             CPM_COMPONENT_TERMINATE);
        if (rc != CL_OK)
        {
            cpmRequestFailedResponse(compName, nodeName, srcAddress, rmdNumber,
                                     CL_CPM_TERMINATE, rc);
            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("\n Unable to terminate component %s 0x%x\n",
                          compName, rc), rc);
        }
    }
    else
    {
        /*
         * Find the component Name from the component Hash Table 
         */
        rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
        if (rc != CL_OK)
        {
            cpmRequestFailedResponse(compName, nodeName, srcAddress, rmdNumber,
                                     CL_CPM_TERMINATE, rc);
            CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                           "component", compName, rc, rc, CL_LOG_DEBUG,
                           CL_LOG_HANDLE_APP);
        }
        if (strcmp(comp->compConfig->compName, gpClCpm->corServerName) == 0)
        {
            gpClCpm->corUp = 0;
        }
        if (strcmp(comp->compConfig->compName, gpClCpm->eventServerName) == 0)
        {
            /*
             * Don't care of the result here 
             */
            rc = clEventFree(gpClCpm->cpmEventHandle);
            rc = clEventFree(gpClCpm->cpmEventNodeHandle);
            
            rc = clEventChannelClose(gpClCpm->cpmEvtChannelHandle); 
            rc = clEventChannelClose(gpClCpm->cpmEvtNodeChannelHandle);
            
            rc = clEventFinalize(gpClCpm->cpmEvtHandle);
            gpClCpm->emUp = 0;
            gpClCpm->emInitDone = 0;
        }

        if (srcAddress != NULL)
        {
            memcpy(&(comp->requestSrcAddress),
                   srcAddress,
                   sizeof(ClIocPhysicalAddressT));
        }
        else
        {
            memset(&(comp->requestSrcAddress),
                   0,
                   sizeof(ClIocPhysicalAddressT));
        }
            
        comp->requestRmdNumber = rmdNumber;

        strcpy((ClCharT *) compTerminate.compName.value, (ClCharT *) compName);
        compTerminate.compName.length =
            strlen((ClCharT *) compTerminate.compName.value);
        compTerminate.requestType = CL_CPM_TERMINATE;

        switch (comp->compPresenceState)
        {
            case CL_AMS_PRESENCE_STATE_UNINSTANTIATED:
            {
                rc = CL_OK;
                cpmCompRespondToCaller(comp, CL_CPM_TERMINATE, rc);
                break;
            }
            case CL_AMS_PRESENCE_STATE_INSTANTIATING:
            {
                rc = CL_CPM_RC(CL_ERR_INVALID_STATE);
                cpmCompRespondToCaller(comp, CL_CPM_TERMINATE, rc);
                break;
            }
            case CL_AMS_PRESENCE_STATE_INSTANTIATED:
            {
                comp->requestType = CL_CPM_TERMINATE;
                comp->compPresenceState = CL_AMS_PRESENCE_STATE_TERMINATING;
                
                if (comp->compConfig->compProperty ==
                    CL_AMS_COMP_PROPERTY_SA_AWARE)
                {
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
                               CL_CPM_LOG_2_LCM_COMP_TERM_INFO,
                               comp->compConfig->compName, comp->eoPort);

                    rc = cpmInvocationAdd(CL_CPM_TERMINATE_CALLBACK, (void *) comp,
                                          &compTerminate.invocation,
                                          CL_CPM_INVOCATION_CPM | CL_CPM_INVOCATION_DATA_SHARED);

                    if (IS_ASP_COMP(comp)) clCompTimerTerminate(comp);
                    rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                                   comp->eoPort,
                                                   CPM_TERMINATE_FN_ID,
                                                   (ClUint8T *) &compTerminate,
                                                   sizeof
                                                   (ClCpmClientCompTerminateT),
                                                   NULL,
                                                   NULL,
                                                   CL_RMD_CALL_ATMOST_ONCE,
                                                   (comp->compConfig->
                                                    compTerminateCallbackTimeOut
                                                    / 2),
                                                   1,
                                                   0,
                                                   NULL,
                                                   NULL,
                                                   MARSHALL_FN(ClCpmClientCompTerminateT, 4, 0, 0));
                    CL_CPM_CHECK(CL_DEBUG_ERROR, ("RMD failed \n"), rc);
                }
                else if (comp->compConfig->compProperty ==
                         CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE)
                {
                    
                    rc = cpmCompFind(proxyCompName, gpClCpm->compTable, &proxyComp);
                    if (rc != CL_OK)
                    {
                        cpmRequestFailedResponse(compName, nodeName, &comp->requestSrcAddress, rmdNumber,
                                                 CL_CPM_TERMINATE, rc);
                        CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                                       "Proxy component", proxyCompName, rc, rc, CL_LOG_DEBUG,
                                       CL_LOG_HANDLE_APP);
                    }

                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
                               CL_CPM_LOG_3_LCM_PROXY_OPER_INFO,
                               "terminating", comp->compConfig->compName,
                               proxyComp->eoPort);
                    
                    rc = cpmInvocationAdd(CL_CPM_TERMINATE_CALLBACK, 
                                          (void *) comp,
                                          &compTerminate.invocation,
                                          CL_CPM_INVOCATION_CPM | CL_CPM_INVOCATION_DATA_SHARED);

                    /*
                     * TimeOut has been taken care of by providing the RMD 
                     * timeout as half of the actual timeout and one retry 
                     */
                    rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                                   proxyComp->eoPort,
                                                   CPM_TERMINATE_FN_ID,
                                                   (ClUint8T *)
                                                   &compTerminate,
                                                   sizeof
                                                   (ClCpmClientCompTerminateT),
                                                   NULL,
                                                   NULL,
                                                   CL_RMD_CALL_ATMOST_ONCE,
                                                   (proxyComp->compConfig->
                                                    compTerminateCallbackTimeOut
                                                    / 2),
                                                   1,
                                                   0,
                                                   NULL,
                                                   NULL,
                                                   MARSHALL_FN(ClCpmClientCompTerminateT, 4, 0, 0));
                    CL_CPM_CHECK(CL_DEBUG_ERROR,
                            ("Sending RMD to proxy component %s failed \n",
                             proxyCompName), rc);
                }
                else if (comp->compConfig->compProperty ==
                         CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)
                {
                    cpmCompRespondToCaller(comp, CL_CPM_TERMINATE, CL_OK);
                }
                else if (comp->compConfig->compProperty ==
                         CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE)
                {
                    if( (rc = cpmNonProxiedNonPreinstantiableCompTerminate(comp, CL_FALSE)) == CL_OK)
                    {
                        comp->compPresenceState = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;
                    }
                    cpmCompRespondToCaller(comp, CL_CPM_TERMINATE, rc);
                }
                break;
            }
            case CL_AMS_PRESENCE_STATE_TERMINATING:
            {
                rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_IN_PROGRESS);
                cpmCompRespondToCaller(comp, CL_CPM_TERMINATE, rc);
                break;
            }
            case CL_AMS_PRESENCE_STATE_RESTARTING:
            case CL_AMS_PRESENCE_STATE_TERMINATION_FAILED:
            case CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED:
            {
                rc = CL_CPM_RC(CL_ERR_INVALID_STATE);
                cpmCompRespondToCaller(comp, CL_CPM_TERMINATE, rc);
                break;
            }
            default:
                break;
        }
    }

    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_LCM_COMP_OPER1_ERR, "terminate",
                   comp->compConfig->compName, rc, rc, CL_LOG_ERROR,
                   CL_LOG_HANDLE_APP);
    return rc;

failure:
    return rc;
}

ClRcT VDECL(cpmComponentTerminate)(ClEoDataT data,
                                   ClBufferHandleT inMsgHandle,
                                   ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmLifeCycleOprT info = {{0}};

    rc = VDECL_VER(clXdrUnmarshallClCpmLifeCycleOprT, 4, 0, 0)(inMsgHandle, (void *) &info);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = _cpmComponentTerminate((info.name).value,
                                (info.proxyCompName).value, 
                                (info.nodeName).value,
                                &(info.srcAddress), 
                                info.rmdNumber);

    return rc;

  failure:
    return rc;
}

ClRcT clCpmCompPreCleanupInvoke(ClCpmComponentT *comp)
{
    ClRcT rc = CL_OK;
    ClCharT cmdBuf[CL_MAX_NAME_LENGTH];
    static ClCharT script[CL_MAX_NAME_LENGTH];
    static ClInt32T cachedState;
    static ClCharT *cachedConfigLoc;
    ClInt32T status;

    if(!comp || !comp->processId) return CL_OK;

    if(!cachedConfigLoc)
    {
        cachedConfigLoc = getenv("ASP_CONFIG");
        if(!cachedConfigLoc)
            cachedConfigLoc = "/root/asp/etc";
        snprintf(script, sizeof(script), "%s/asp.d/%s", cachedConfigLoc, ASP_PRECLEANUP_SCRIPT);
        /*
         * Script exists?
         */
        if(!access(script, F_OK))
        {
            cachedState = 1;
        }
    }

    if(!cachedState) goto out;
 
    snprintf(cmdBuf, sizeof(cmdBuf), "ASP_COMPNAME=%s %s", comp->compConfig->compName, script);
    clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Invoking precleanup command [%s] for Component [%s]", cmdBuf, comp->compConfig->compName);

    status = system(cmdBuf);
    (void)status; /*unused*/

    out:
    return rc;
}

static ClRcT compCleanupInvoke(ClCpmComponentT *comp)
{
    ClRcT rc = CL_OK;

    if(!comp->processId) return CL_OK;

    if(comp->compConfig->cleanupCMD[0])
    {
        ClCharT cleanupCmdBuf[CL_MAX_NAME_LENGTH];
        snprintf(cleanupCmdBuf, sizeof(cleanupCmdBuf), "ASP_COMPNAME=%s EFLAG=%d %s",
                 comp->compConfig->compName, !comp->hbFailureDetected, comp->compConfig->cleanupCMD);
        clLogNotice(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, 
                   "Invoking cleanup command [%s] for Component [%s]",
                   cleanupCmdBuf, comp->compConfig->compName);
        if(system(cleanupCmdBuf))
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, 
                       "Cleanup command [%s] returned error [%s] for Component [%s]",
                       cleanupCmdBuf, strerror(errno), comp->compConfig->compName);
            rc = CL_CPM_RC(CL_ERR_LIBRARY);
        }
    }
    /*
     * Issue an unconditional sigkill incase cleanup didn't terminate 
     * component.
     */
    if (comp->hbFailureDetected)
    {
        clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Sending Signal[%d] to component [%s]with process id[%d]", gCompHealthCheckFailSendSignal, comp->compConfig->compName, comp->processId);
        kill(comp->processId, gCompHealthCheckFailSendSignal);
        comp->hbFailureDetected = CL_FALSE;
    }
    else
    {
        clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Sending SIGKILL signal to component [%s]with process id[%d]", comp->compConfig->compName, comp->processId);
        kill(comp->processId, SIGKILL);
    }

    return rc;
}

ClRcT cpmProxiedHealthcheckStop(ClNameT *pCompName)
{
    ClCharT compName[CL_MAX_NAME_LENGTH];
    ClCpmComponentT *comp = NULL;
    ClRcT rc = CL_OK;
    if(!pCompName)
        return CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
    compName[0] = 0;
    strncat(compName, pCompName->value, pCompName->length);
    rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
    if(rc != CL_OK)
    {
        clLogError("HB", "STOP", "Proxied component [%s] not found in CPM component table",
                   compName);
        return rc;
    }
    clLogNotice("HB", "STOP", "Stopping healthcheck for proxied comp [%s]", compName);
    clOsalMutexLock(comp->compMutex);
    if (CL_HANDLE_INVALID_VALUE != comp->cpmHealthcheckTimerHandle)
    {
        clTimerStop(comp->cpmHealthcheckTimerHandle);
        clTimerDeleteAsync(&comp->cpmHealthcheckTimerHandle);
        comp->cpmHealthcheckTimerHandle = CL_HANDLE_INVALID_VALUE;
    }
    if (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)
    {
        clTimerStop(comp->hbTimerHandle);
        clTimerDeleteAsync(&comp->hbTimerHandle);
        comp->hbTimerHandle = CL_HANDLE_INVALID_VALUE;
    }
                        
    comp->hbInvocationPending = CL_NO;
    comp->hcConfirmed = CL_NO;
    clOsalMutexUnlock(comp->compMutex);
    return CL_OK;
}


ClRcT cpmCompCleanup(ClCharT *compName)
{

    ClCpmComponentT *comp = NULL;
    ClNameT compInstance = {0};
    ClCpmEOListNodeT *ptr = NULL;
    ClCpmEOListNodeT *pTemp = NULL;
    ClRcT rc;
    rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
    if (rc != CL_OK)
    {
        return rc;
    }
    if (strcmp(comp->compConfig->compName, gpClCpm->corServerName) == 0)
        gpClCpm->corUp = 0;
    if (strcmp(comp->compConfig->compName, gpClCpm->eventServerName) == 0)
        gpClCpm->emUp = 0;

    clNameSet(&compInstance, comp->compConfig->compName);
    cpmInvocationClearCompInvocation(&compInstance);
    switch (comp->compPresenceState)
    {
    case CL_AMS_PRESENCE_STATE_UNINSTANTIATED:
        {
            rc = CL_OK;
            /*
             * Its success, so inform caller about successful execution 
             */
            break;
        }
    case CL_AMS_PRESENCE_STATE_INSTANTIATING:
    case CL_AMS_PRESENCE_STATE_INSTANTIATED:
    case CL_AMS_PRESENCE_STATE_TERMINATING:
    case CL_AMS_PRESENCE_STATE_RESTARTING:
    case CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED:
    case CL_AMS_PRESENCE_STATE_TERMINATION_FAILED:
    default:
        {
            if (comp->compConfig->compProperty ==
                CL_AMS_COMP_PROPERTY_SA_AWARE)
            {
                clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                           "Cleaning up SA aware component [%s]...",
                           comp->compConfig->compName);

                /*
                 * Cleanup the EOs of that Component 
                 */
                clOsalMutexLock(gpClCpm->eoListMutex);
                ptr = comp->eoHandle;
                while (ptr != NULL && ptr->eoptr != NULL)
                {
                    if (ptr->eoptr->eoPort == CL_IOC_EVENT_PORT)
                    {
                        gpClCpm->emUp = 0;
                    }
                    pTemp = ptr;
                    ptr = ptr->pNext;
                    clHeapFree(pTemp->eoptr);
                    pTemp->eoptr = NULL;
                    clHeapFree(pTemp);
                    pTemp = NULL;
                }
                comp->eoHandle = NULL;
                comp->eoCount = 0;
                clOsalMutexUnlock(gpClCpm->eoListMutex);
                if (comp->compConfig->compProcessRel ==
                    CL_CPM_COMP_SINGLE_PROCESS)
                {
                    /*
                     * Send SIGKILL
                     */
                    if (comp->processId != 0)
                    {
                        compCleanupInvoke(comp);
                    }
                    else
                    {
                        clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Skipped cleanup for component [%s]",
                            comp->compConfig->compName);
                    }
                    comp->processId = 0;
                    comp->compPresenceState =
                        CL_AMS_PRESENCE_STATE_UNINSTANTIATED;

                    clOsalMutexLock(comp->compMutex);
                    comp->compOperState = CL_AMS_OPER_STATE_DISABLED;
                    comp->compReadinessState =
                        CL_AMS_READINESS_STATE_OUTOFSERVICE;
                    if (CL_HANDLE_INVALID_VALUE != comp->cpmHealthcheckTimerHandle)
                    {
                        clTimerStop(comp->cpmHealthcheckTimerHandle);
                        clTimerDeleteAsync(&comp->cpmHealthcheckTimerHandle);
                        comp->cpmHealthcheckTimerHandle = CL_HANDLE_INVALID_VALUE;
                    }
                    if (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)
                    {
                        clTimerStop(comp->hbTimerHandle);
                        clTimerDeleteAsync(&comp->hbTimerHandle);
                        comp->hbTimerHandle = CL_HANDLE_INVALID_VALUE;
                    }
                        
                    comp->hbInvocationPending = CL_NO;
                    comp->hcConfirmed = CL_NO;
                    clOsalMutexUnlock(comp->compMutex);
                }
                else if (comp->compConfig->compProcessRel ==
                         CL_CPM_COMP_MULTI_PROCESS)
                {
                    /*
                     * All the process must have done the cpmClientInit,
                     * then we know the pid, send SIGKILL to all the
                     * process 
                     */
                    rc = CL_ERR_NOT_IMPLEMENTED;
                }
                else if (comp->compConfig->compProcessRel ==
                         CL_CPM_COMP_THREADED)
                {
                    rc = CL_ERR_NOT_IMPLEMENTED;
                }
                else if (comp->compConfig->compProcessRel ==
                         CL_CPM_COMP_NONE)
                {
                    rc = CL_ERR_NOT_IMPLEMENTED;
                }
            }
            else if (comp->compConfig->compProperty ==
                     CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE)
            {
                if( (rc = cpmNonProxiedNonPreinstantiableCompTerminate(comp, CL_TRUE)) == CL_OK)
                {
                    comp->compPresenceState = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;
                }
            }
            else if(comp->processId) 
            {
                if (comp->hbFailureDetected)
                {
                    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Sending Signal[%d] to component [%s]with process id[%d]", gCompHealthCheckFailSendSignal, comp->compConfig->compName, comp->processId);
                    kill(comp->processId, gCompHealthCheckFailSendSignal);
                    comp->hbFailureDetected = CL_FALSE;
                }
                else
                {
                    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Sending SIGKILL signal to component [%s]with process id[%d]",
                              comp->compConfig->compName, comp->processId);
                    kill(comp->processId, SIGKILL);
               }
            }
            else
            {
                    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Ignored cleanup for component [%s]", comp->compConfig->compName);
            }

        }
    }

    return rc;
}

ClRcT _cpmLocalComponentCleanup(ClCpmComponentT *comp,
                                ClCharT *compName,
                                ClCharT *proxyCompName,
                                ClCharT *nodeName)
{
    ClUint32T rc = CL_OK;
    ClCpmEOListNodeT *ptr = NULL;
    ClCpmEOListNodeT *pTemp = NULL;
    ClNameT compInstance = {0};

    if (clDbgNoKillComponents)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("In debug mode (clDbgNoKillComponents is set), so will not clean up component %s", compName));
        return CL_CPM_ERR_OPERATION_FAILED;
    }

    if(!comp)
    {
        rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
        if (rc != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                       CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR, "component",
                       compName, rc);
            return rc;
        }
    }

    memset(&comp->requestSrcAddress, 0, sizeof(comp->requestSrcAddress));
    clNameSet(&compInstance, comp->compConfig->compName);
    cpmInvocationClearCompInvocation(&compInstance);
    switch (comp->compPresenceState)
    {
    case CL_AMS_PRESENCE_STATE_UNINSTANTIATED:
        {
            rc = CL_OK;
            /*
             * Its success, so inform caller about successful execution 
             */
            break;
        }
    case CL_AMS_PRESENCE_STATE_INSTANTIATING:
    case CL_AMS_PRESENCE_STATE_INSTANTIATED:
    case CL_AMS_PRESENCE_STATE_TERMINATING:
    case CL_AMS_PRESENCE_STATE_RESTARTING:
    case CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED:
    case CL_AMS_PRESENCE_STATE_TERMINATION_FAILED:
    default:
        {
            if (comp->compConfig->compProperty ==
                CL_AMS_COMP_PROPERTY_SA_AWARE)
            {
                clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                           "Cleaning up SA aware component [%s]...",
                           comp->compConfig->compName);

                ptr = comp->eoHandle;
                while (ptr != NULL && ptr->eoptr != NULL)
                {
                    if (ptr->eoptr->eoPort == CL_IOC_EVENT_PORT)
                    {
                        gpClCpm->emUp = 0;
                    }
                    pTemp = ptr;
                    ptr = ptr->pNext;
                    clHeapFree(pTemp->eoptr);
                    pTemp->eoptr = NULL;
                    clHeapFree(pTemp);
                    pTemp = NULL;
                }
                comp->eoHandle = NULL;
                comp->eoCount = 0;
                if(!comp->compEventPublished && comp->eoPort && !comp->compTerminated)
                    cpmComponentEventPublish(comp, CL_CPM_COMP_DEATH, CL_TRUE);
                if (comp->compConfig->compProcessRel ==
                    CL_CPM_COMP_SINGLE_PROCESS)
                {
                    /*
                     * Send SIGKILL
                     */
                    if (comp->processId != 0)
                    {
                        compCleanupInvoke(comp);
                    }
                    else
                    {
                        clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Skipped cleanup for component [%s]",
                            comp->compConfig->compName);
                    }
                    comp->processId = 0;
                    comp->compPresenceState =
                        CL_AMS_PRESENCE_STATE_UNINSTANTIATED;

                    clOsalMutexLock(comp->compMutex);
                    comp->compOperState = CL_AMS_OPER_STATE_DISABLED;
                    comp->compReadinessState =
                        CL_AMS_READINESS_STATE_OUTOFSERVICE;
                    if (CL_HANDLE_INVALID_VALUE != comp->cpmHealthcheckTimerHandle)
                    {
                        clTimerStop(comp->cpmHealthcheckTimerHandle);
                        clTimerDeleteAsync(&comp->cpmHealthcheckTimerHandle);
                        comp->cpmHealthcheckTimerHandle = CL_HANDLE_INVALID_VALUE;
                    }
                    if (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)
                    {
                        clTimerStop(comp->hbTimerHandle);
                        clTimerDeleteAsync(&comp->hbTimerHandle);
                        comp->hbTimerHandle = CL_HANDLE_INVALID_VALUE;
                    }
                        
                    comp->hbInvocationPending = CL_NO;
                    comp->hcConfirmed = CL_NO;
                    clOsalMutexUnlock(comp->compMutex);
                }
                else if (comp->compConfig->compProcessRel ==
                         CL_CPM_COMP_MULTI_PROCESS)
                {
                    /*
                     * All the process must have done the cpmClientInit,
                     * then we know the pid, send SIGKILL to all the
                     * process 
                     */
                    rc = CL_ERR_NOT_IMPLEMENTED;
                    CL_CPM_CHECK(CL_DEBUG_ERROR,
                                 ("COMP MULTITHREAD is not implemented \n"),
                                 rc);
                }
                else if (comp->compConfig->compProcessRel ==
                         CL_CPM_COMP_THREADED)
                {
                    rc = CL_ERR_NOT_IMPLEMENTED;
                    CL_CPM_CHECK(CL_DEBUG_ERROR,
                                 ("COMP MULTITHREAD is not implemented \n"),
                                 rc);
                }
                else if (comp->compConfig->compProcessRel ==
                         CL_CPM_COMP_NONE)
                {
                    rc = CL_ERR_NOT_IMPLEMENTED;
                    CL_CPM_CHECK(CL_DEBUG_ERROR,
                                 ("COMP Script based instantiation is not implemented \n"),
                                 rc);
                }
            }
            else if (comp->compConfig->compProperty ==
                     CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE ||
                     comp->compConfig->compProperty ==
                     CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)
            {
                /* PROXIED:
                   1. Request sent to Proxy component.
                */
                ClCpmClientCompTerminateT compCleanup;

                clOsalMutexLock(comp->compMutex);
                if (CL_HANDLE_INVALID_VALUE != comp->cpmHealthcheckTimerHandle)
                {
                    clTimerStop(comp->cpmHealthcheckTimerHandle);
                    clTimerDeleteAsync(&comp->cpmHealthcheckTimerHandle);
                    comp->cpmHealthcheckTimerHandle = CL_HANDLE_INVALID_VALUE;
                }
                if (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)
                {
                    clTimerStop(comp->hbTimerHandle);
                    clTimerDeleteAsync(&comp->hbTimerHandle);
                    comp->hbTimerHandle = CL_HANDLE_INVALID_VALUE;
                }
                        
                comp->hbInvocationPending = CL_NO;
                comp->hcConfirmed = CL_NO;
                clOsalMutexUnlock(comp->compMutex);

                strcpy(compCleanup.compName.value, compName);
                compCleanup.compName.length =
                    strlen(compCleanup.compName.value);
                compCleanup.requestType = CL_CPM_PROXIED_CLEANUP;

                if(proxyCompName)
                {
                    ClCpmComponentT *proxyComp = NULL;
                    rc = cpmCompFind(proxyCompName, gpClCpm->compTable, &proxyComp);
                    CL_CPM_CHECK_3(CL_DEBUG_ERROR, 
                                   CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                                   "proxy component", proxyCompName, rc, rc, CL_LOG_DEBUG,
                                   CL_LOG_HANDLE_APP);

                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
                               CL_CPM_LOG_3_LCM_PROXY_OPER_INFO, "cleaning",
                               comp->compConfig->compName, proxyComp->eoPort);

                    /*
                     * Invoke the proxy component registered function 
                     */

                    /*
                     * Bug 4381:
                     * Commenting out the code for starting the timer in case of proxied
                     * component. If a ASP component can be a proxied component then this
                     * needs to be changed accordingly.
                     */
                    /*clCompTimerProxiedCleanup(comp);*/

                    /*
                     * Allocate the Invocation Id 
                     */
                    rc = cpmInvocationAdd(CL_CPM_PROXIED_CLEANUP_CALLBACK, 
                                          (void *) comp,
                                          &compCleanup.invocation,
                                          CL_CPM_INVOCATION_CPM | CL_CPM_INVOCATION_DATA_SHARED);
                    rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                                   proxyComp->eoPort,
                                                   CPM_PROXIED_CL_CPM_CLEANUP_FN_ID,
                                                   (ClUint8T *)&compCleanup,
                                                   sizeof
                                                   (ClCpmClientCompTerminateT),
                                                   NULL,
                                                   NULL,
                                                   CL_RMD_CALL_ATMOST_ONCE,
                                                   (proxyComp->
                                                    compConfig->
                                                    compProxiedCompCleanupCallbackTimeout
                                                    / 2),
                                                   1,
                                                   0,
                                                   NULL,
                                                   NULL,
                                                   MARSHALL_FN(ClCpmClientCompTerminateT, 4, 0, 0));
                }
            }
            else if (comp->compConfig->compProperty ==
                     CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE)
            {
                if( (rc = cpmNonProxiedNonPreinstantiableCompTerminate(comp, CL_TRUE)) == CL_OK)
                {
                    comp->compPresenceState = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;
                }
                else
                {
                    goto failure;
                }
            }
            else
            {
                    clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Ignored Local Component Cleanup for component [%s]", comp->compConfig->compName);
            }
        }
    }

    failure:
    return rc;
}

ClRcT _cpmComponentCleanup(ClCharT *compName,
                           ClCharT *proxyCompName,
                           ClCharT *nodeName,
                           ClIocPhysicalAddressT *srcAddress,
                           ClUint32T rmdNumber,
                           ClCpmCompRequestTypeT requestType)
{

    ClCpmComponentT *comp = NULL;
    ClCpmComponentT *proxyComp = NULL;
    ClUint32T rc = CL_OK;
    ClCpmEOListNodeT *ptr = NULL;
    ClCpmEOListNodeT *pTemp = NULL;

    if (clDbgNoKillComponents)
    {
        CL_DEBUG_PRINT(CL_DEBUG_CRITICAL, ("In debug mode (clDbgNoKillComponents is set), so will not clean up component %s", compName));
        return CL_CPM_ERR_OPERATION_FAILED;
    }

    if ((gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL) &&
        (strcmp
         ((ClCharT *) nodeName, (ClCharT *) gpClCpm->pCpmConfig->nodeName)))
    {
        rc = cpmRouteRequest(compName, proxyCompName, nodeName, srcAddress, rmdNumber,
                             CPM_COMPONENT_CLEANUP);
        if (rc != CL_OK)
        {
            cpmRequestFailedResponse(compName, nodeName, srcAddress, rmdNumber,
                                     CL_CPM_CLEANUP, rc);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                           ("\n Unable to cleanup component %s 0x%x\n",
                            compName, rc));
            return rc;
        }
        else
        {
            /*
             * Other node would have returned the response, so nothing is
             * required here 
             */
            return rc;
        }
    }
    else
    {
        ClNameT compInstance = {0};
        rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
        if (rc != CL_OK)
        {
            cpmRequestFailedResponse(compName, nodeName, srcAddress, rmdNumber,
                                     CL_CPM_CLEANUP, rc);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                       CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR, "component",
                       compName, rc);
            return rc;
        }
        if (strcmp(comp->compConfig->compName, gpClCpm->corServerName) == 0)
            gpClCpm->corUp = 0;
        if (strcmp(comp->compConfig->compName, gpClCpm->eventServerName) == 0)
            gpClCpm->emUp = 0;

        if (srcAddress != NULL)
            memcpy(&(comp->requestSrcAddress), srcAddress,
                   sizeof(ClIocPhysicalAddressT));
        else
        {
            /*
             * No response required. Reset the request src.
             */
            memset(&comp->requestSrcAddress, 0, sizeof(comp->requestSrcAddress));
        }
        comp->requestRmdNumber = rmdNumber;
        clNameSet(&compInstance, comp->compConfig->compName);
        cpmInvocationClearCompInvocation(&compInstance);
        switch (comp->compPresenceState)
        {
        case CL_AMS_PRESENCE_STATE_UNINSTANTIATED:
            {
                rc = CL_OK;
                /*
                 * Its success, so inform caller about successful execution 
                 */
                break;
            }
        case CL_AMS_PRESENCE_STATE_INSTANTIATING:
        case CL_AMS_PRESENCE_STATE_INSTANTIATED:
        case CL_AMS_PRESENCE_STATE_TERMINATING:
        case CL_AMS_PRESENCE_STATE_RESTARTING:
        case CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED:
        case CL_AMS_PRESENCE_STATE_TERMINATION_FAILED:
        default:
            {
                if (comp->compConfig->compProperty ==
                    CL_AMS_COMP_PROPERTY_SA_AWARE)
                {
                    clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                               "Cleaning up SA aware component [%s]...",
                               comp->compConfig->compName);

                    /*
                     * Cleanup the EOs of that Component 
                     */
                    rc = clOsalMutexLock(gpClCpm->eoListMutex);
                    CL_CPM_CHECK(CL_DEBUG_ERROR,
                                 ("Unable to lock mutex %x\n", rc), rc);
                    ptr = comp->eoHandle;
                    while (ptr != NULL && ptr->eoptr != NULL)
                    {
                        if (ptr->eoptr->eoPort == CL_IOC_EVENT_PORT)
                        {
                            gpClCpm->emUp = 0;
                        }
                        pTemp = ptr;
                        ptr = ptr->pNext;
                        clHeapFree(pTemp->eoptr);
                        pTemp->eoptr = NULL;
                        clHeapFree(pTemp);
                        pTemp = NULL;
                    }
                    comp->eoHandle = NULL;
                    comp->eoCount = 0;
                    rc = clOsalMutexUnlock(gpClCpm->eoListMutex);
                    CL_CPM_CHECK(CL_DEBUG_ERROR,
                                 ("Unable to unlock mutex %x\n", rc), rc);
                    if(!comp->compEventPublished && comp->eoPort && !comp->compTerminated)
                        cpmComponentEventPublish(comp, CL_CPM_COMP_DEATH, CL_TRUE);
                    if (comp->compConfig->compProcessRel ==
                        CL_CPM_COMP_SINGLE_PROCESS)
                    {
                        /*
                         * Send SIGKILL
                         */
                        if (comp->processId != 0)
                        {
                            compCleanupInvoke(comp);
                        }
                        else
                        {
                            clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Skipped cleanup for component [%s]",
                               comp->compConfig->compName);
                        }
                        comp->processId = 0;
                        comp->compPresenceState =
                            CL_AMS_PRESENCE_STATE_UNINSTANTIATED;

                        clOsalMutexLock(comp->compMutex);
                        comp->compOperState = CL_AMS_OPER_STATE_DISABLED;
                        comp->compReadinessState =
                            CL_AMS_READINESS_STATE_OUTOFSERVICE;
                        if (CL_HANDLE_INVALID_VALUE != comp->cpmHealthcheckTimerHandle)
                        {
                            clTimerStop(comp->cpmHealthcheckTimerHandle);
                            clTimerDeleteAsync(&comp->cpmHealthcheckTimerHandle);
                            comp->cpmHealthcheckTimerHandle = CL_HANDLE_INVALID_VALUE;
                        }
                        if (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)
                        {
                            clTimerStop(comp->hbTimerHandle);
                            clTimerDeleteAsync(&comp->hbTimerHandle);
                            comp->hbTimerHandle = CL_HANDLE_INVALID_VALUE;
                        }
                        
                        comp->hbInvocationPending = CL_NO;
                        comp->hcConfirmed = CL_NO;
                        clOsalMutexUnlock(comp->compMutex);
                    }
                    else if (comp->compConfig->compProcessRel ==
                             CL_CPM_COMP_MULTI_PROCESS)
                    {
                        /*
                         * All the process must have done the cpmClientInit,
                         * then we know the pid, send SIGKILL to all the
                         * process 
                         */
                        rc = CL_ERR_NOT_IMPLEMENTED;
                        CL_CPM_CHECK(CL_DEBUG_ERROR,
                                     ("COMP MULTITHREAD is not implemented \n"),
                                     rc);
                    }
                    else if (comp->compConfig->compProcessRel ==
                             CL_CPM_COMP_THREADED)
                    {
                        rc = CL_ERR_NOT_IMPLEMENTED;
                        CL_CPM_CHECK(CL_DEBUG_ERROR,
                                     ("COMP MULTITHREAD is not implemented \n"),
                                     rc);
                    }
                    else if (comp->compConfig->compProcessRel ==
                             CL_CPM_COMP_NONE)
                    {
                        rc = CL_ERR_NOT_IMPLEMENTED;
                        CL_CPM_CHECK(CL_DEBUG_ERROR,
                                     ("COMP Script based instantiation is not implemented \n"),
                                     rc);
                    }
                }
                else if (comp->compConfig->compProperty ==
                         CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE ||
                         comp->compConfig->compProperty ==
                         CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)
                {
                    /* PROXIED:
                       1. Request sent to Proxy component.
                    */
                    ClCpmClientCompTerminateT compCleanup;

                    clOsalMutexLock(comp->compMutex);
                    if (CL_HANDLE_INVALID_VALUE != comp->cpmHealthcheckTimerHandle)
                    {
                        clTimerStop(comp->cpmHealthcheckTimerHandle);
                        clTimerDeleteAsync(&comp->cpmHealthcheckTimerHandle);
                        comp->cpmHealthcheckTimerHandle = CL_HANDLE_INVALID_VALUE;
                    }
                    if (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)
                    {
                        clTimerStop(comp->hbTimerHandle);
                        clTimerDeleteAsync(&comp->hbTimerHandle);
                        comp->hbTimerHandle = CL_HANDLE_INVALID_VALUE;
                    }
                        
                    comp->hbInvocationPending = CL_NO;
                    comp->hcConfirmed = CL_NO;
                    clOsalMutexUnlock(comp->compMutex);

                    strcpy(compCleanup.compName.value, compName);
                    compCleanup.compName.length =
                        strlen(compCleanup.compName.value);
                    compCleanup.requestType = CL_CPM_PROXIED_CLEANUP;

                    rc = cpmCompFind(proxyCompName, gpClCpm->compTable, &proxyComp);
                    CL_CPM_CHECK_3(CL_DEBUG_ERROR, 
                                   CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                                   "proxy component", proxyCompName, rc, rc, CL_LOG_DEBUG,
                                   CL_LOG_HANDLE_APP);

                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
                               CL_CPM_LOG_3_LCM_PROXY_OPER_INFO, "cleaning",
                               comp->compConfig->compName, proxyComp->eoPort);

                    /*
                     * Invoke the proxy component registered function 
                     */

                    /*
                     * Bug 4381:
                     * Commenting out the code for starting the timer in case of proxied
                     * component. If a ASP component can be a proxied component then this
                     * needs to be changed accordingly.
                     */
                    /*clCompTimerProxiedCleanup(comp);*/

                    /*
                     * Allocate the Invocation Id 
                     */
                    rc = cpmInvocationAdd(CL_CPM_PROXIED_CLEANUP_CALLBACK, 
                                          (void *) comp,
                                          &compCleanup.invocation,
                                          CL_CPM_INVOCATION_CPM | CL_CPM_INVOCATION_DATA_SHARED);
                    rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                                   proxyComp->eoPort,
                                                   CPM_PROXIED_CL_CPM_CLEANUP_FN_ID,
                                                   (ClUint8T *)&compCleanup,
                                                   sizeof
                                                   (ClCpmClientCompTerminateT),
                                                   NULL,
                                                   NULL,
                                                   CL_RMD_CALL_ATMOST_ONCE,
                                                   (proxyComp->
                                                    compConfig->
                                                    compProxiedCompCleanupCallbackTimeout
                                                    / 2),
                                                   1,
                                                   0,
                                                   NULL,
                                                   NULL,
                                                   MARSHALL_FN(ClCpmClientCompTerminateT, 4, 0, 0));
                }
                else if (comp->compConfig->compProperty ==
                         CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE)
                {
                    if( (rc = cpmNonProxiedNonPreinstantiableCompTerminate(comp, CL_TRUE)) == CL_OK)
                    {
                        comp->compPresenceState = CL_AMS_PRESENCE_STATE_UNINSTANTIATED;
                    }
                    else
                    {
                        goto failure;
                    }
                }
                else
                {
                        clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM, "Ignored cleanup for component [%s]", comp->compConfig->compName);
                }
            }
        }
    }

    /* Modified for Bug 4028.
     * Added && condition (requestType != CL_CPM_INSTANTIATE).
     *  Request CL_CPM_INSTANTIATE is error in case of restart,
     *  respond to caller and reset restartPending flag
     */
    if (comp->restartPending && (requestType != CL_CPM_INSTANTIATE))
    {
        /*
         * Request was for the restart, so no need to respond to the caller
         * yet 
         */
        return CL_OK;
    }
    /*
     * If the request was from cleanup, rspond to it 
     */
    else if (requestType == CL_CPM_CLEANUP)
        cpmCompRespondToCaller(comp, CL_CPM_CLEANUP, CL_OK);
    /*
     * If the request was from terminate or instantiate , rspond to it with
     * that type 
     */
    else if (requestType == CL_CPM_TERMINATE)
        cpmCompRespondToCaller(comp, requestType, CL_OK);
    /*
     * If the request was from instantiate, but instantiation migth have
     * failed, which eventually caused the clean, but for the caller it was a
     * instantiation request 
     */
    else
        cpmCompRespondToCaller(comp, requestType,
                               CL_CPM_RC(CL_CPM_ERR_OPERATION_FAILED));

    return rc;
    failure:
    if (comp->restartPending)
    {
        cpmCompRespondToCaller(comp, CL_CPM_RESTART, rc);
        /*
         * we have responded so set restart pending to zero 
         */
        comp->restartPending = 0;
    }
    /*
     * If the request was from cleanup, rspond to it 
     */
    else if (requestType == CL_CPM_CLEANUP)
        cpmCompRespondToCaller(comp, CL_CPM_CLEANUP, rc);
    /*
     * If the request was from terminate or instantiate , rspond to it with
     * that type 
     */
    else if (requestType == CL_CPM_TERMINATE)
        cpmCompRespondToCaller(comp, requestType, rc);
    else
        cpmCompRespondToCaller(comp, requestType,
                               CL_CPM_RC(CL_CPM_ERR_OPERATION_FAILED));

    return rc;
}

ClRcT VDECL(cpmComponentCleanup)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmLifeCycleOprT info = {{0}};

    rc = VDECL_VER(clXdrUnmarshallClCpmLifeCycleOprT, 4, 0, 0)(inMsgHandle, (void *) &info);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = _cpmComponentCleanup((info.name).value,
                              (info.proxyCompName).value, 
                              (info.nodeName).value,
                              &(info.srcAddress), 
                              info.rmdNumber,
                              CL_CPM_CLEANUP);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_LCM_COMP_OPER1_ERR, "cleanup",
                   info.name.value, rc, rc, CL_LOG_ERROR, CL_LOG_HANDLE_APP);

  failure:
    return rc;
}

ClRcT _cpmComponentRestart(ClCharT *compName,
                           ClCharT *proxyCompName,
                           ClCharT *nodeName,
                           ClIocPhysicalAddressT *srcAddress,
                           ClUint32T rmdNumber)
{
    ClRcT rc = CL_OK;
    ClCpmComponentT *comp = NULL;

    if ((gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL) &&
        (strcmp
         ((ClCharT *) nodeName, (ClCharT *) gpClCpm->pCpmConfig->nodeName)))
    {
        rc = cpmRouteRequest(compName, proxyCompName, nodeName, srcAddress, rmdNumber,
                             CPM_COMPONENT_RESTART);
        if (rc != CL_OK)
        {
            rc = cpmRequestFailedResponse(compName, nodeName, srcAddress,
                                          rmdNumber, CL_CPM_RESTART, rc);
            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("\n Unable to route the Restart Request component %s 0x%x\n",
                          compName, rc), rc);
        }
    }
    else
    {
        rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
        if (rc != CL_OK)
        {
            cpmRequestFailedResponse(compName, nodeName, srcAddress, rmdNumber,
                                     CL_CPM_RESTART, rc);
            CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                           "component", compName, rc, rc, CL_LOG_DEBUG,
                           CL_LOG_HANDLE_APP);
        }

        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ALERT, NULL,
                   CL_CPM_LOG_1_LCM_COMP_RESTART_INFO,
                   comp->compConfig->compName);
        /* Modified for Bug 4028.
         * Added the if block.
         */
        if (comp->restartPending == 1)
        {
            cpmCompRespondToCaller(comp, CL_CPM_RESTART, CL_CPM_RC(CL_CPM_ERR_OPERATION_IN_PROGRESS));
            /*
             * Bug 4350:
             * Return if the restart request is pending.
             */
            return CL_CPM_RC(CL_CPM_ERR_OPERATION_IN_PROGRESS);
        }
        comp->restartPending = 1;
        comp->compRestartCount += 1;

        clOsalMutexLock(comp->compMutex);
        if (comp->compOperState == CL_AMS_OPER_STATE_ENABLED)
        {
            clOsalMutexUnlock(comp->compMutex);
            clLogWarning("CMP","MGT","Cleanup component [%s] in preparation for restart. Oper state is ENABLED.",comp->compConfig->compName);
            rc = _cpmComponentTerminate(compName, NULL, nodeName, srcAddress, rmdNumber);
            CL_CPM_CHECK(CL_DEBUG_ERROR,("Unable to terminate component while restarting. Error [%x]",rc), rc);
        }
        else if (comp->compOperState == CL_AMS_OPER_STATE_DISABLED)
        {
            clOsalMutexUnlock(comp->compMutex);
            clLogWarning("CMP","MGT","Cleanup component [%s] in preparation for restart. Oper state is DISABLED.",comp->compConfig->compName);
            rc = _cpmComponentCleanup(compName, NULL, nodeName, srcAddress, rmdNumber,CL_CPM_CLEANUP);
            if(CL_GET_ERROR_CODE(rc) != CL_OK)
            {
                clLogError("CMP","MGT","Unable to cleanup component [%s] while restarting. Error [%x]",comp->compConfig->compName,rc);
            }
            
            /*
             * Wait atleast 4 milliSeconds 
             */
            CPM_TIMED_WAIT(CL_CPM_RESTART_SLEEP);

            rc = _cpmComponentInstantiate(compName, NULL, 0,nodeName, srcAddress, rmdNumber);
        }
        else
        {
            clOsalMutexUnlock(comp->compMutex);
        }
    }

  failure:
    return rc;
}

ClRcT VDECL(cpmComponentRestart)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmLifeCycleOprT info = {{0}};

    rc = VDECL_VER(clXdrUnmarshallClCpmLifeCycleOprT, 4, 0, 0)(inMsgHandle, (void *) &info);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = _cpmComponentRestart((info.name).value, (info.proxyCompName).value, 
                              (info.nodeName).value, &(info.srcAddress), 
                              info.rmdNumber);
    CL_CPM_CHECK(CL_DEBUG_ERROR, ("Unable to restart component %x\n", rc), rc);

  failure:
    return rc;
}

ClCharT *_cpmReadinessStateNameGet(
    ClAmsReadinessStateT readinessState
)
{
    switch (readinessState)
    {
        case CL_AMS_READINESS_STATE_OUTOFSERVICE:
        {
            return "OUT_OF_SERVICE";
        }
        case CL_AMS_READINESS_STATE_INSERVICE:
        {
            return "IN_SERVICE";
        }
        case CL_AMS_READINESS_STATE_STOPPING:
        {
            return "STOPPING";
        }
        default:
        {
            return "Corrupt";
        }
    }
}

ClCharT *_cpmPresenceStateNameGet(
    ClAmsPresenceStateT presenceState
)
{
    switch (presenceState)
    {
        case CL_AMS_PRESENCE_STATE_UNINSTANTIATED:
        {
            return "UNINSTANTIATED";
        }
        case CL_AMS_PRESENCE_STATE_INSTANTIATING:
        {
            return "INSTANTIATING";
        }
        case CL_AMS_PRESENCE_STATE_INSTANTIATED:
        {
            return "INSTANTIATED";
        }
        case CL_AMS_PRESENCE_STATE_TERMINATING:
        {
            return "TERMINATING";
        }
        case CL_AMS_PRESENCE_STATE_RESTARTING:
        {
            return "RESTARTING";
        }
        case CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED:
        {
            return "INSTANTIATION_FAILED";
        }
        case CL_AMS_PRESENCE_STATE_TERMINATION_FAILED:
        {
            return "TERMINATION_FAILED";
        }
        default:
        {
            return "Corrupt";
        }
    }
}

ClCharT *_cpmOperStateNameGet(
    ClAmsOperStateT operState
)
{
    switch (operState)
    {
        case CL_AMS_OPER_STATE_ENABLED:
        {
            return "ENABLED";
        }
        case CL_AMS_OPER_STATE_DISABLED:
        {
            return "DISABLED";
        }
        default:
        {
            return "Corrupt";
        }
    }
}

ClRcT VDECL(cpmComponentIdGet)(ClEoDataT data,
                               ClBufferHandleT inMsgHandle,
                               ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClNameT compName = { 0 };
    ClCpmComponentT *comp = NULL;
    ClUint32T compId = 0;

    rc = clXdrUnmarshallClNameT(inMsgHandle, (void *) &compName);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = cpmCompFind(compName.value, gpClCpm->compTable, &comp);
    if(rc != CL_OK)
    {
        compId = ++gpClCpm->compIdAlloc;
    }
    else
    {
        compId = comp->compId;
    }

    rc = clXdrMarshallClUint32T((void *) &compId, outMsgHandle, 0);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

  failure:
    return rc;
}

ClRcT VDECL(cpmComponentStatusGet)(ClEoDataT data,
                                   ClBufferHandleT inMsgHandle,
                                   ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmLifeCycleOprT status = {{0}};
    ClCpmComponentT *comp = NULL;
    ClCpmComponentStateT state;

    rc = VDECL_VER(clXdrUnmarshallClCpmLifeCycleOprT, 4, 0, 0)(inMsgHandle, (void *) &status);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    if (status.nodeName.length == 0 ||
        !(strcmp(status.nodeName.value, gpClCpm->pCpmLocalInfo->nodeName)))
    {
        rc = cpmCompFind(status.name.value, gpClCpm->compTable, &comp);
        CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                       "component", status.name.value, rc, rc, CL_LOG_DEBUG,
                       CL_LOG_HANDLE_APP);

        clOsalMutexLock(comp->compMutex);
        state.compPresenceState = comp->compPresenceState;
        state.compOperState = comp->compOperState;
        clOsalMutexUnlock(comp->compMutex);

        /*
         * clOsalPrintf("Pres State %d\t Oper State %d\n",
         * state.compPresenceState, state.compOperState);
         */

        rc = VDECL_VER(clXdrMarshallClCpmComponentStateT, 4, 0, 0)((void *) &state, outMsgHandle,
                                               0);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        ClCpmLT *cpmL = NULL;
        ClUint32T size = sizeof(ClUint32T);
        clOsalMutexLock(gpClCpm->cpmTableMutex);
        rc = cpmNodeFindLocked(status.nodeName.value, &cpmL);
        if(rc != CL_OK)
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            clLogError("STATUS", "GET", "Node [%s] not found. Failure code [%#x]",
                       status.nodeName.value, rc);
            goto failure;
        }

        if (cpmL->pCpmLocalInfo != NULL &&
            cpmL->pCpmLocalInfo->status != CL_CPM_EO_DEAD)
        {
            ClIocPhysicalAddressT destNode = cpmL->pCpmLocalInfo->cpmAddress;
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            rc = CL_CPM_CALL_RMD_SYNC_NEW(destNode.nodeAddress,
                                          destNode.portId,
                                          CPM_COMPONENT_STATUS_GET,
                                          (ClUint8T *) &status,
                                          sizeof(ClCpmLifeCycleOprT),
                                          (ClUint8T *) &state, &size,
                                          CL_RMD_CALL_NEED_REPLY, 0, 0, 0,
                                          MARSHALL_FN(ClCpmLifeCycleOprT, 4, 0, 0),
                                          UNMARSHALL_FN(ClCpmComponentStateT, 4, 0, 0)
            );

            CL_CPM_CHECK(CL_DEBUG_ERROR,
                         ("Unable to get Response from the other Node %x\n",
                          rc), rc);
            rc = VDECL_VER(clXdrMarshallClCpmComponentStateT, 4, 0, 0)((void *) &state,
                                                   outMsgHandle, 0);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
        else
        {
            clOsalMutexUnlock(gpClCpm->cpmTableMutex);
            rc = CL_CPM_RC(CL_CPM_ERR_FORWARDING_FAILED);
        }
    }
  failure:
    return rc;
}

ClRcT VDECL(cpmComponentAddressGet)(ClEoDataT data,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClNameT compName = { 0 };
    ClCpmComponentT *comp = NULL;
    ClIocAddressIDLT idlIocAddress;

    rc = clXdrUnmarshallClNameT(inMsgHandle, (void *) &compName);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = cpmCompFind(compName.value, gpClCpm->compTable, &comp);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "component", compName.value, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    /*
     * Return physical address 
     */
    idlIocAddress.discriminant = CLIOCADDRESSIDLTIOCPHYADDRESS;
    idlIocAddress.clIocAddressIDLT.iocPhyAddress.nodeAddress =
        clIocLocalAddressGet();
    idlIocAddress.clIocAddressIDLT.iocPhyAddress.portId = comp->eoPort;

    rc = VDECL_VER(clXdrMarshallClIocAddressIDLT, 4, 0, 0)((void *) &idlIocAddress, outMsgHandle,
                                       0);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

  failure:
    return rc;
}

ClRcT VDECL(cpmComponentPIDGet)(ClEoDataT data,
                                ClBufferHandleT inMsgHandle,
                                ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClNameT compName = { 0 };
    ClCpmComponentT *comp = NULL;
    ClUint32T pid = 0;

    rc = clXdrUnmarshallClNameT(inMsgHandle, (void *) &compName);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = cpmCompFind(compName.value, gpClCpm->compTable, &comp);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "component", compName.value, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    pid = comp->processId;

    rc = clXdrMarshallClUint32T((void *) &pid, outMsgHandle, 0);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

  failure:
    return rc;
}

ClRcT VDECL(cpmComponentLAUpdate)(ClEoDataT data,
                                  ClBufferHandleT inMsgHandle,
                                  ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmComponentT *comp = NULL;
    ClCpmCompLAUpdateT cpmLA;


    rc = VDECL_VER(clXdrUnmarshallClCpmCompLAUpdateT, 4, 0, 0)(inMsgHandle, (void *) &cpmLA);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = cpmCompFind(cpmLA.compName, gpClCpm->compTable, &comp);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "component", cpmLA.compName, rc, rc, CL_LOG_DEBUG,
                   CL_LOG_HANDLE_APP);

    /*
     * TODO: Add this LA to the component Ref 
     */
    /*
     * clOsalPrintf("Logical Address is %d.%d\n",
     * cpmLA.logicalAddress.iocLogicalAddress.DWord.high,
     * cpmLA.logicalAddress.iocLogicalAddress.DWord.low);
     */

  failure:
    return rc;
}

static ClRcT cpmFailureReportTask(void *data)
{
    if(gpClCpm->cpmToAmsCallback 
       &&
       gpClCpm->cpmToAmsCallback->compErrorReport)
    {
        ClErrorReportT *errorReport = data;
        ClAmsEntityT entity = {0};
        gpClCpm->cpmToAmsCallback->compErrorReport(
                                                   &errorReport->compName, 
                                                   errorReport->time, 
                                                   errorReport->recommendedRecovery,
                                                   errorReport->handle,
                                                   errorReport->instantiateCookie);
        memcpy(&entity.name, &errorReport->compName, sizeof(entity.name));
        clAmsFaultQueueDelete(&entity);
    }
    if(data) clHeapFree(data);
    return CL_OK;
}

ClRcT VDECL(cpmComponentFailureReport)(ClEoDataT data,
                                       ClBufferHandleT inMsgHandle,
                                       ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClErrorReportT *errorReport = NULL;
#if 0
    ClCpmComponentT *comp = NULL;
#endif

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Inside cpmComponentFailureReport \n"));

    errorReport = clHeapCalloc(1, sizeof(*errorReport));
    CL_ASSERT(errorReport != NULL);
    
    rc = VDECL_VER(clXdrUnmarshallClErrorReportT, 4, 0, 0)(inMsgHandle, (void *)errorReport);
    CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    clLogInfo("COMP", "FAILURE", "Component failure reported for component [%s], "
               "instantiate cookie [%lld]", errorReport->compName.value, 
               errorReport->instantiateCookie);

    /* BUG 3868: COMMENTED compFind()
     *  The error report might be for a component on another node, searching
     *  only local component table is faulty logic
     *  Anyway AMS needs the component name and will take care of component 
     *  lookup in the global component data it has
     */
#if 0
    rc = cpmCompFind(errorReport->compName.value, gpClCpm->compTable, &comp);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "component", errorReport->compName.value, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
#endif

    /*clOsalPrintf("compName %s time %d recovery %d \n",
        errorReport->compName.value,
        (ClUint32T)errorReport->time,
        (ClUint32T)errorReport->recommendedRecovery);*/

    if (gpClCpm->cpmToAmsCallback != NULL && gpClCpm->cpmToAmsCallback->compErrorReport != NULL)
    {
        ClAmsEntityT entity = {0};
        errorReport->compName.length += 1;
        memcpy(&entity.name, &errorReport->compName, sizeof(entity.name));
        clAmsFaultQueueAdd(&entity);
        clTaskPoolRun(gCpmFaultPool, cpmFailureReportTask, (void*)errorReport);
        errorReport = NULL;
    }
    else
    {
        rc = CL_CPM_RC(CL_CPM_ERR_OPERATION_NOT_ALLOWED);
        clLogAlert(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,"Cannot handle component failure report: error [%#x]",rc);
    }
    
        
  failure:
    if(errorReport) clHeapFree(errorReport);
    return rc;
}

ClRcT VDECL(cpmComponentFailureClear)(ClEoDataT data,
                                      ClBufferHandleT inMsgHandle,
                                      ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClErrorReportT *errorReport = NULL;
    ClUint32T msgLength = 0;
    ClCpmComponentT *comp = NULL;

    CL_DEBUG_PRINT(CL_DEBUG_TRACE, ("Inside cpmComponentF`ailureReport \n"));

    rc = clBufferLengthGet(inMsgHandle, &msgLength);
    if (msgLength >= sizeof(ClErrorReportT))
    {
        errorReport = (ClErrorReportT *) clHeapAllocate(msgLength);
        if (errorReport == NULL)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                       "Unable to allocate memory");
            rc = CL_CPM_RC(CL_ERR_NULL_POINTER);
            goto failure;
        }

        rc = clBufferNBytesRead(inMsgHandle, (ClUint8T *) errorReport,
                                &msgLength);
        CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_READ_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
    }
    else
    {
        rc = CL_CPM_RC(CL_ERR_INVALID_BUFFER);
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_AMS,
                   "Invalid buffer passed, error [%#x]",
                   rc);
        goto failure;
    }

    rc = cpmCompFind(errorReport->compName.value, gpClCpm->compTable, &comp);
    CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
                   "component", errorReport->compName.value, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

  failure:
    if (errorReport != NULL)
    {
        clHeapFree(errorReport);
        errorReport = NULL;
    }
    return rc;
}

ClRcT VDECL(cpmComponentListDebugAll)(ClEoDataT data,
                                      ClBufferHandleT inMsgHandle,
                                      ClBufferHandleT outMsgHandle)
{
    ClCntNodeHandleT hNode = 0;
    ClCpmComponentT *comp = NULL;
    ClUint32T rc = CL_OK, count;
    ClCharT tempStr[256];

    count = gpClCpm->noOfComponent;

    rc = clCntFirstNodeGet(gpClCpm->compTable, &hNode);
    CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_CNT_FIRST_NODE_GET_ERR,
                   "component", rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                              (ClCntDataHandleT *) &comp);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR, rc,
                   rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    while (count)
    {
        if (comp->compConfig->compProperty == CL_AMS_COMP_PROPERTY_SA_AWARE &&
            comp->compPresenceState == CL_AMS_PRESENCE_STATE_INSTANTIATED)
        {
            clOsalMutexLock(comp->compMutex);
            if (comp->compOperState == CL_AMS_OPER_STATE_ENABLED)
            {
                clOsalMutexUnlock(comp->compMutex);
                sprintf(tempStr, "%s\n", comp->compConfig->compName);
                rc = clBufferNBytesWrite(outMsgHandle, (ClUint8T *) tempStr,
                                         strlen(tempStr));
                CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                               CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
            }
            else
            {
                clOsalMutexUnlock(comp->compMutex);
            }
        }

        count--;
        if (count)
        {
            rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
            CL_CPM_CHECK_2(CL_DEBUG_ERROR, CL_CPM_LOG_2_CNT_NEXT_NODE_GET_ERR,
                           "CPM-L", rc, rc, CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

            rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                      (ClCntDataHandleT *) &comp);
            CL_CPM_CHECK_1(CL_DEBUG_ERROR,
                           CL_CPM_LOG_1_CNT_NODE_USR_DATA_GET_ERR, rc, rc,
                           CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
        }
    }
    /*
     * NULL terminate the string 
     */
    sprintf(tempStr, "%s", "\0");
    rc = clBufferNBytesWrite(outMsgHandle, (ClUint8T *) tempStr, 1);
    CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                   CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

    return CL_OK;

  failure:
    return rc;
}

ClRcT cpmComponentEventCleanup(ClCpmComponentT *comp)
{
    ClCpmEventPayLoadT cpmPayLoadData = {{0}};
    ClRcT rc = CL_OK;

    if (!gpClCpm->emUp)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                   "Event server is not up, "
                   "hence not cleaning up");
        return CL_OK;
    }
        
    strcpy(cpmPayLoadData.compName.value, comp->compConfig->compName);
    cpmPayLoadData.compName.length = strlen(cpmPayLoadData.compName.value);

    strcpy(cpmPayLoadData.nodeName.value, gpClCpm->pCpmConfig->nodeName);
    cpmPayLoadData.nodeName.length = strlen(cpmPayLoadData.nodeName.value);

    cpmPayLoadData.compId = comp->compId;
    cpmPayLoadData.eoId = comp->eoID;
    cpmPayLoadData.nodeIocAddress = clIocLocalAddressGet();
    cpmPayLoadData.eoIocPort = comp->eoPort;

    /* Inform the event server about the failed component,
     * so that it can cleanup any data structures related
     * to the component.
     */
    clLogNotice("EVT", "CLEANUP", "Event cleanup done for component [%s], "
              "compId [%#x], eoId [%#llx], eoPort [%#x]", comp->compConfig->compName,
              comp->compId, comp->eoID, comp->eoPort);
    rc = clEventCpmCleanup(&cpmPayLoadData);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                   "Unable to inform event server about "
                   "the failed component [%s], error [%#x]",
                   comp->compConfig->compName,
                   rc);
    }

    return rc;
}

static ClRcT _cpmComponentEventPublish(ClCpmComponentT *comp,
                               ClCpmCompEventT compEvent,
                               ClBoolT eventCleanup)
{
    ClRcT rc = CL_OK;
    ClCpmEventPayLoadT cpmPayLoadData = {{0}};
    ClBufferHandleT payLoadMsg = 0;
    ClUint8T *payLoadBuffer = NULL;
    ClUint32T msgLength = 0;
    ClUint32T arrivalPattern = htonl(CL_CPM_COMP_ARRIVAL_PATTERN);
    ClUint32T deathPattern = htonl(CL_CPM_COMP_DEATH_PATTERN);
    ClUint32T departurePattern = htonl(CL_CPM_COMP_DEPART_PATTERN);
    ClEventPatternT compArrivalPattern[] = 
        { {0, (ClSizeT)sizeof(arrivalPattern), (ClUint8T*)&arrivalPattern } };
    ClEventPatternT compDeparturePattern[] = 
        { { 0, (ClSizeT)sizeof(departurePattern), (ClUint8T*)&departurePattern } };
    ClEventPatternT compDeathPattern[] = 
        { { 0, (ClSizeT)sizeof(deathPattern), (ClUint8T*)&deathPattern } };
    ClEventPatternArrayT compArrivalPatternArray = {
        0, sizeof(compArrivalPattern)/sizeof(compArrivalPattern[0]), compArrivalPattern
    };
    ClEventPatternArrayT compDeparturePatternArray = {
        0, sizeof(compDeparturePattern)/sizeof(compDeparturePattern[0]), compDeparturePattern 
    };
    ClEventPatternArrayT compDeathPatternArray = {
        0, sizeof(compDeathPattern)/sizeof(compDeathPattern[0]), compDeathPattern 
    };

    ClEventPatternArrayT *patternArray = &compDeathPatternArray;

    if (!gpClCpm->emUp || !gpClCpm->emInitDone)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                       "Event server is not %s. Hence not publishing the event",
                       !gpClCpm->emUp ? "up" : "initialized");
        return CL_OK;
    }

    if (compEvent == CL_CPM_COMP_DEATH)
    {
        if (comp->compEventPublished == CL_TRUE)
        {
            /*
             * If event already published, ignore publishing
             */
            clLogInfo(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                    "Skipping comp event already published for component eo port [%#x], eoID [%#llx]", comp->eoPort, comp->eoID);
            return CL_OK;
        }
    }
    else if (compEvent == CL_CPM_COMP_DEPARTURE)
    {
        patternArray = &compDeparturePatternArray;
    }
    else if(compEvent == CL_CPM_COMP_ARRIVAL)
    {
        patternArray = &compArrivalPatternArray;
    }
    else return CL_OK ; /*unknown event type*/
        
    strcpy(cpmPayLoadData.compName.value, comp->compConfig->compName);
    cpmPayLoadData.compName.length = strlen(cpmPayLoadData.compName.value);

    strcpy(cpmPayLoadData.nodeName.value, gpClCpm->pCpmConfig->nodeName);
    cpmPayLoadData.nodeName.length = strlen(cpmPayLoadData.nodeName.value);

    cpmPayLoadData.compId = comp->compId;
    cpmPayLoadData.eoId = comp->eoID;
    cpmPayLoadData.nodeIocAddress = clIocLocalAddressGet();
    cpmPayLoadData.eoIocPort = comp->eoPort;
    cpmPayLoadData.operation = compEvent;

    /* Inform the event server about the failed component,
     * so that it can cleanup any data structures related
     * to the component.
     */
    if(eventCleanup == CL_TRUE)
    {
        rc = clEventCpmCleanup(&cpmPayLoadData);
        if (CL_OK != rc)
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                       "Unable to inform event server about "
                       "the failed component [%s], error [%#x]",
                       comp->compConfig->compName,
                       rc);
        }
    }

    clLogMultiline(CL_LOG_SEV_INFO, CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                   "Publishing component %s event with the "
                   "following information : \n"
                   "Name : [%s]\n"
                   "ID : [%#x]\n"
                   "EO ID : [%#llx]\n"
                   "IOC port : [%#x]\n"
                   "IOC address of the containing node : [%#x]\n"
                   "Node name : [%s]",
                   (compEvent == CL_CPM_COMP_ARRIVAL ? "arrival":
                    compEvent == CL_CPM_COMP_DEPARTURE ? "departure":
                    compEvent == CL_CPM_COMP_DEATH ? "death": "junk"),
                   cpmPayLoadData.compName.value,
                   cpmPayLoadData.compId,
                   cpmPayLoadData.eoId,
                   cpmPayLoadData.eoIocPort,
                   cpmPayLoadData.nodeIocAddress,
                   cpmPayLoadData.nodeName.value);

    rc = clBufferCreate(&payLoadMsg);
    if (CL_OK != rc) 
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                   CL_CPM_LOG_1_BUF_CREATE_ERR, rc);
        return rc;
    }

    rc = VDECL_VER(clXdrMarshallClCpmEventPayLoadT, 4, 0, 0)(&cpmPayLoadData,
                                                             payLoadMsg,
                                                             0);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                   CL_LOG_MESSAGE_0_INVALID_BUFFER, rc);
        goto out_free;
    }

    rc = clBufferLengthGet(payLoadMsg, &msgLength);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                   CL_CPM_LOG_1_BUF_LENGTH_ERR, rc);
        goto out_free;
    }

    rc = clBufferFlatten(payLoadMsg, &payLoadBuffer);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                   CL_CPM_LOG_1_BUF_FLATTEN_ERR, rc);
        goto out_free;
    }
    
    rc = clEventAttributesSet(gpClCpm->cpmEventHandle, patternArray,
                              CL_EVENT_HIGHEST_PRIORITY, 0, &gpClCpm->name);
    if(rc != CL_OK)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT, "Attributes set returned [%#x]", rc);
        goto out_free;
    }

    rc = clEventPublish(gpClCpm->cpmEventHandle,
                        payLoadBuffer,
                        msgLength,
                        &gpClCpm->cpmEvtId);

    if (CL_OK != rc)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                   "Unable to publish event for component [%s], "
                   "error [%#x]",
                   comp->compConfig->compName,
                   rc);
    }
    else
    {
        if (compEvent == CL_CPM_COMP_DEATH)
        {
            comp->compEventPublished = CL_TRUE;
        }
    }

    out_free:
    if(payLoadBuffer)
        clHeapFree(payLoadBuffer);
    if(payLoadMsg)
        clBufferDelete(&payLoadMsg);

    return rc;
}

static ClRcT cpmComponentEventPublishDelay(ClPtrT invocation)
{
    ClRcT rc = CL_OK;
    ClEventPublishDataT *data = (ClEventPublishDataT *) invocation;
    ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 500};

    /* Quit this task since Event is already terminated */
    if (gCpmShuttingDown)
      goto error;

    while((!gpClCpm->emUp || !gpClCpm->emInitDone))
    {
        rc = clOsalTaskDelay(delay);
        if (rc != CL_OK)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                       "clOsalTaskDelay() error [%#x]", rc);
            goto error;
        }
    }

    _cpmComponentEventPublish(&data->comp, data->compEvent, data->eventCleanup);

error:
    clHeapFree(data->comp.compConfig);
    clHeapFree(data);
    return rc;
}

ClRcT cpmComponentEventPublish(ClCpmComponentT *comp,
                               ClCpmCompEventT compEvent,
                               ClBoolT eventCleanup)
{
    ClRcT rc = CL_OK;

    ClEventPublishDataT *job = NULL;

    /* Don't need push this task to job queue since Event is already terminated */
    if (gCpmShuttingDown && !gpClCpm->emUp && !gpClCpm->emInitDone)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,"Event server is already terminated. Hence not publishing the event");
        return CL_OK;
    }

    job = clHeapCalloc(1, sizeof(ClEventPublishDataT));
    CL_ASSERT(job != NULL);

    memcpy(&job->comp, comp, sizeof(ClCpmComponentT));

    job->comp.compConfig = clHeapCalloc(1, sizeof(ClCpmCompConfigT));
    CL_ASSERT(job->comp.compConfig != NULL);
    memcpy(job->comp.compConfig, comp->compConfig, sizeof(ClCpmCompConfigT));

    job->compEvent = compEvent;
    job->eventCleanup = eventCleanup;

    /*
     * Filter job for the same component to ignore in the future
     */
    clJobQueueWalk(&eventPublishQueue, eventPublishQueueCallback, job);
    rc = clJobQueuePush(&eventPublishQueue, cpmComponentEventPublishDelay, (ClPtrT)job);
    if(rc != CL_OK)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                   "Can not push event into job queue. Hence not publishing the event");
        clHeapFree(job->comp.compConfig);
        clHeapFree(job);
    }

    if (compEvent == CL_CPM_COMP_DEATH)
    {
        comp->compEventPublished = CL_TRUE;
    }
    return rc;
}

ClRcT VDECL(cpmIsCompRestarted)(ClEoDataT data,
                                ClBufferHandleT inMsgHandle,
                                ClBufferHandleT outMsgHandle)
{
	ClRcT rc = CL_OK;

	ClNameT compName = {0};
	ClCpmComponentT *comp = NULL;

	ClUint32T isRestarted = 0;

	rc = clXdrUnmarshallClNameT(inMsgHandle, (void *) &compName);
    	CL_CPM_CHECK_0(CL_DEBUG_ERROR, CL_LOG_MESSAGE_0_INVALID_BUFFER, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);

	rc = cpmCompFind(compName.value, gpClCpm->compTable, &comp);
    	CL_CPM_CHECK_3(CL_DEBUG_ERROR, CL_CPM_LOG_3_CNT_ENTITY_SEARCH_ERR,
		       "component", compName.value, rc, rc, CL_LOG_DEBUG,
		       CL_LOG_HANDLE_APP);
	if (comp->compRestartCount)
	{
		isRestarted = 1;
	}

    	rc = clXdrMarshallClUint32T((void *) &isRestarted, outMsgHandle, 0);
    	CL_CPM_CHECK_1(CL_DEBUG_ERROR, CL_CPM_LOG_1_BUF_WRITE_ERR, rc, rc,
                       CL_LOG_DEBUG, CL_LOG_HANDLE_APP);
	return CL_OK;

failure:
	return rc;
}

ClRcT VDECL(cpmAspCompLcmResHandle)(ClEoDataT data,
                                    ClBufferHandleT inMsgHandle,
                                    ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmLcmResponseT lcmResponse = {{0}};
    ClNameT compName = {0};

    rc = VDECL_VER(clXdrUnmarshallClCpmLcmResponseT, 4, 0, 0)(inMsgHandle, (void *)&lcmResponse);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_LCM,
                   "Failed to unmarshall buffer, error [%#x]",
                   rc);
        return rc;
    }

    strncpy(compName.value, lcmResponse.name, CL_MAX_NAME_LENGTH);
    compName.length = strlen(compName.value);
    
    if (CL_OK != lcmResponse.returnCode || cpmIsInfrastructureComponent(&compName) == CL_FALSE)
    {
        clLogCritical(CPM_LOG_AREA_CPM, CPM_LOG_AREA_CPM,
                      "LCM operation [%s] failed on ASP component [%s] !!, "
                      "error [%#x]. Shutting down the node...",
                      lcmResponse.requestType == CL_CPM_RESTART ? "Restart":
                      "Unknown",
                      lcmResponse.name,
                      lcmResponse.returnCode);
        
        clCpmNodeShutDown(clIocLocalAddressGet());
    }
    
    return CL_OK;
}

void cpmCompHeartBeatFailure(ClIocPortT iocPort)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT hNode = 0;
    ClUint32T compCount = 0;
    ClCpmComponentT *comp = NULL;
    ClCpmEOListNodeT *ptr = NULL;

    if ((!gpClCpm) || (!gpClCpm->polling)) return;

    clOsalMutexLock(gpClCpm->compTableMutex);
    rc = clCntFirstNodeGet(gpClCpm->compTable, &hNode);
    if(rc != CL_OK)
    {
        clOsalMutexUnlock(gpClCpm->compTableMutex);
        clLogError("HBT", "FAIL", "Container failure [%#x] on comp table access for port [%#x]",
                   rc, iocPort);
        goto failure;
    }

    /*
     * For every component 
     */
    compCount = gpClCpm->noOfComponent;
    while (compCount && rc == CL_OK)
    {
        rc = clCntNodeUserDataGet(gpClCpm->compTable, hNode,
                                  (ClCntDataHandleT *) &comp);
        if(rc != CL_OK)
        {
            clOsalMutexUnlock(gpClCpm->compTableMutex);
            clLogError("HBT", "FAIL", "Container data get failed with [%#x] "
                       "on comp table access for port [%#x]",
                       rc, iocPort);
            goto failure;
        }

        if (comp->compTerminated == CL_FALSE)
        {
            if ((rc = clOsalMutexLock(gpClCpm->eoListMutex)) != CL_OK)
            {
                clOsalMutexUnlock(gpClCpm->compTableMutex);
                goto failure;
            }
            ptr = comp->eoHandle;
            if(comp->eoPort == iocPort)
            {
                clOsalMutexUnlock(gpClCpm->compTableMutex);
                if(ptr)
                {
                    cpmEOHBFailure(ptr);
                }
                else
                {
                    cpmComponentEventPublish(comp, CL_CPM_COMP_DEATH, CL_TRUE);
                }
                clOsalMutexUnlock(gpClCpm->eoListMutex);
                goto done;
            }
            clOsalMutexUnlock(gpClCpm->eoListMutex);
        }

        compCount--;
        if (compCount)
        {
            rc = clCntNextNodeGet(gpClCpm->compTable, hNode, &hNode);
            if(rc != CL_OK)
            {
                clOsalMutexUnlock(gpClCpm->compTableMutex);
                clLogError("HBT", "FAIL", "Container next node fetch failed with [%#x] "
                           "on comp table access for port [%#x]", 
                           rc, iocPort);
                goto failure;
            }
        }
    }
    clOsalMutexUnlock(gpClCpm->compTableMutex);

    done:
    return;

    failure:
    return;
}

static ClRcT compHealthCheckAmfInvokedCB(ClPtrT arg)
{
    ClRcT rc = CL_OK;
    ClCpmComponentT *comp = (ClCpmComponentT *)arg;

    clOsalMutexLock(comp->compMutex);
    
    if (!comp->hbInvocationPending)
    {
        ClCpmClientCompTerminateT compHealthcheck = {0};
        ClIocPortT eoPort = 0;

        if ((comp->compConfig->compProperty ==
             CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE) ||
            (comp->compConfig->compProperty ==
             CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE))
        {
            if (!comp->proxyCompRef)
            {
                clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                           "Proxy for [%s] is not running",
                           comp->compConfig->compName);
                goto unlock;
            }
            eoPort = comp->proxyCompRef->eoPort;
        }
        else
        {
            eoPort = comp->eoPort;
        }

        if (!eoPort)
          {
            clLogWarning(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB, "Disable heartheat for component [%s]", comp->compConfig->compName);
            comp->hbInvocationPending = CL_NO;
            clTimerStop(comp->hbTimerHandle);
            clTimerDeleteAsync(&comp->hbTimerHandle);
            comp->hbTimerHandle = CL_HANDLE_INVALID_VALUE;
            goto unlock;
          }

        rc = cpmInvocationAdd(CL_CPM_HB_CALLBACK,
                              (void *) comp,
                              &compHealthcheck.invocation,
                              CL_CPM_INVOCATION_CPM | CL_CPM_INVOCATION_DATA_SHARED);
        if (CL_OK != rc)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                       "Unable to add heartbeat invocation, rc = [%#x]",
                       rc);
            goto unlock;
        }
        
        clNameSet(&compHealthcheck.compName, comp->compConfig->compName);

        rc = CL_CPM_CALL_RMD_ASYNC_NEW(clIocLocalAddressGet(),
                                       eoPort,
                                       CPM_HEALTH_CHECK_FN_ID,
                                       (ClUint8T *) &compHealthcheck,
                                       sizeof(ClCpmClientCompTerminateT),
                                       NULL,
                                       NULL,
                                       CL_RMD_CALL_ATMOST_ONCE,
                                       0,
                                       0,
                                       CL_IOC_HIGH_PRIORITY,
                                       NULL,
                                       NULL,
                                       MARSHALL_FN(ClCpmClientCompTerminateT, 4, 0, 0));

        if (CL_OK != rc)
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                       "Unable to send heartbeat request to the component, "
                       "rc = [%#x]",
                       rc);
            /*
             * Delete invocation entry.
             */
            cpmInvocationDeleteInvocation(compHealthcheck.invocation);
            goto unlock;
        }

        /*
         * Restart cpmHealthcheckTimerHandle in case request successful or report to CPM that healthCheck failure.
         * This would expect that component re-instantiated if previous instantiated failed.
         */
        cpmHealthCheckTimerStart(comp);

        comp->hbInvocationPending = CL_YES;
    }
    
    clOsalMutexUnlock(comp->compMutex);
    
    return CL_OK;

unlock:
    clOsalMutexUnlock(comp->compMutex);
    return rc;
}

static ClRcT compHealthCheckClientInvokedCB(ClPtrT arg)
{
    ClCpmComponentT *comp = (ClCpmComponentT *)arg;

    clOsalMutexLock(comp->compMutex);
    
    if (!comp->hcConfirmed)
    {
        ClNameT compName = {0};

        clNameSet(&compName, comp->compConfig->compName);
        comp->hbFailureDetected = CL_TRUE;
        clCpmCompPreCleanupInvoke(comp);
        clCpmComponentFailureReportWithCookie(0,
                                              &compName,
                                              comp->instantiateCookie,
                                              0,
                                              comp->compConfig->
                                              healthCheckConfig.recommendedRecovery,
                                              0);
    }
    else
    {
        comp->hcConfirmed = CL_NO;
    }

    clOsalMutexUnlock(comp->compMutex);
    
    return CL_OK;
}

static ClRcT cpmCompHealthcheckDispatch(const ClCpmCompHealthcheckT
                                        *cpmCompHealthcheck)
{
    ClRcT rc = CL_OK;
    ClCharT compName[CL_MAX_NAME_LENGTH] = {0};
    ClTimerTimeOutT timeOut;
    ClTimerCallBackT timerCallback = NULL;
    ClCpmComponentT *comp = NULL;

    memset(&timeOut, 0, sizeof(ClTimerTimeOutT));
    
    CL_ASSERT(cpmCompHealthcheck != NULL);

    strncpy(compName,
            cpmCompHealthcheck->compName.value,
            cpmCompHealthcheck->compName.length);
    rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
    if (CL_OK != rc)
    {
        /*
         * This could be a proxied on a non-asp aware node.
         * Learn for its presence from the master.
         */
        rc = cpmComponentAddDynamic(compName);
        if(rc == CL_OK)
        {
            rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
        }
        if(rc != CL_OK)
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                       "Unable to find component [%s], error [%#x]",
                       compName,
                       rc);
            goto failure;
        }
    }

    if (comp->compConfig->compProperty ==
        CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE)
    {
        ClCharT proxyCompName[CL_MAX_NAME_LENGTH] = {0};
        ClCpmComponentT *proxyComp = NULL;

        strncpy(proxyCompName,
                cpmCompHealthcheck->proxyCompName.value,
                cpmCompHealthcheck->proxyCompName.length);
        rc = cpmCompFind(proxyCompName, gpClCpm->compTable, &proxyComp);
        if (CL_OK != rc)
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                       "Unable to find component [%s], error [%#x]",
                       proxyCompName,
                       rc);
            goto failure;
        }

        comp->proxyCompRef = proxyComp;
        strncpy(comp->proxyCompName, proxyCompName, strlen(proxyCompName));
    }
    
    switch(cpmCompHealthcheck->invocationType)
    {
        case CL_AMS_COMP_HEALTHCHECK_AMF_INVOKED:
        {
            if (CL_HANDLE_INVALID_VALUE == comp->cpmHealthcheckTimerHandle)
            {
                rc = cpmHealthCheckTimerCreate(comp);
                if (CL_OK != rc) goto failure;
            }
            
            timerCallback = compHealthCheckAmfInvokedCB;

            break;
        }
        case CL_AMS_COMP_HEALTHCHECK_CLIENT_INVOKED:
        {
            timerCallback = compHealthCheckClientInvokedCB;

            break;
        }
        default:
        {
            CL_ASSERT(0);
            break;
        }
    }

    clOsalMutexLock(comp->compMutex);

    comp->compConfig->healthCheckConfig.recommendedRecovery =
    cpmCompHealthcheck->recommendedRecovery;

    if (CL_HANDLE_INVALID_VALUE == comp->hbTimerHandle)
    {
        timeOut.tsSec = 0;
        timeOut.tsMilliSec = comp->compConfig->healthCheckConfig.period;

        rc = clTimerCreateAndStart(timeOut,
                                   CL_TIMER_REPETITIVE,
                                   CL_TIMER_SEPARATE_CONTEXT,
                                   timerCallback,
                                   comp,
                                   &comp->hbTimerHandle);
        if (CL_OK != rc)
        {
            clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                       "Unable to create and start timer, error [%#x]",
                       rc);
            goto unlock;
        }
    }
    else
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                   "Health check start request already made. "
                   "To make a different health check request "
                   "with different parameters, call the health check stop "
                   "first and then call the health check start function.");
        rc = CL_CPM_RC(CL_ERR_ALREADY_EXIST);
        goto unlock;
    }

    clOsalMutexUnlock(comp->compMutex);

    return CL_OK;

unlock:
    clOsalMutexUnlock(comp->compMutex);
failure:
    return rc;
}

ClRcT VDECL(cpmComponentHealthcheckStart)(ClEoDataT data,
                                          ClBufferHandleT inMsgHandle,
                                          ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmCompHealthcheckT cpmCompHealthcheck = {{0}};
    
    rc = VDECL_VER(clXdrUnmarshallClCpmCompHealthcheckT, 4, 0, 0)(inMsgHandle,
                                              &cpmCompHealthcheck);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                   "Unable to unmarshall buffer, error [%#x]",
                   rc);
        goto failure;
    }

    switch(cpmCompHealthcheck.invocationType)
    {
        case CL_AMS_COMP_HEALTHCHECK_AMF_INVOKED:
        case CL_AMS_COMP_HEALTHCHECK_CLIENT_INVOKED:
        {
            rc = cpmCompHealthcheckDispatch(&cpmCompHealthcheck);
            if (CL_OK != rc) goto failure;
        }
        break;
        default:
        {
            clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                       "Invalid healthcheck invocation type [%#x]",
                       cpmCompHealthcheck.invocationType);
            rc = CL_CPM_RC(CL_ERR_INVALID_PARAMETER);
            goto failure;
        }
    }
        
    return CL_OK;

failure:
    return rc;
}

ClRcT cpmCompHealthcheckStop(ClNameT *pCompName)
{
    ClRcT rc = CL_OK;
    ClCharT compName[CL_MAX_NAME_LENGTH];
    ClCpmComponentT *comp = NULL;
    ClBoolT stopped = CL_FALSE;

    compName[0] = 0;
    strncat(compName, pCompName->value, pCompName->length);
    rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                   "Unable to find component [%s], error [%#x]",
                   compName,
                   rc);
        goto failure;
    }
    
    clOsalMutexLock(comp->compMutex);
    if (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)
    {
        stopped = CL_TRUE;
        clTimerStop(comp->hbTimerHandle);
        clTimerDeleteAsync(&comp->hbTimerHandle);
        comp->hbTimerHandle = CL_HANDLE_INVALID_VALUE;
    }
    if (CL_HANDLE_INVALID_VALUE != comp->cpmHealthcheckTimerHandle)
    {
        stopped = CL_TRUE;
        clTimerStop(comp->cpmHealthcheckTimerHandle);
        clTimerDeleteAsync(&comp->cpmHealthcheckTimerHandle);
        comp->cpmHealthcheckTimerHandle = CL_HANDLE_INVALID_VALUE;
    }
    clOsalMutexUnlock(comp->compMutex);
    
    if(stopped)
    {
        clLogNotice("HEALTH", "CHECK", "Health check stopped for component [%s]", compName);
    }

    return CL_OK;

    failure:
    return rc;
}

ClRcT VDECL(cpmComponentHealthcheckStop)(ClEoDataT data,
                                         ClBufferHandleT inMsgHandle,
                                         ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmCompHealthcheckT cpmCompHealthcheck = {{0}};
    
    rc = VDECL_VER(clXdrUnmarshallClCpmCompHealthcheckT, 4, 0, 0)(inMsgHandle,
                                              &cpmCompHealthcheck);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                   "Unable to unmarshall buffer, error [%#x]",
                   rc);
        goto failure;
    }

    rc = cpmCompHealthcheckStop(&cpmCompHealthcheck.compName);

    failure:
    return rc;
}

ClRcT VDECL(cpmComponentHealthcheckConfirm)(ClEoDataT data,
                                            ClBufferHandleT inMsgHandle,
                                            ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClCpmCompHealthcheckT cpmCompHealthcheck = {{0}};
    ClCharT compName[CL_MAX_NAME_LENGTH] = {0};
    ClCpmComponentT *comp = NULL;
    
    rc = VDECL_VER(clXdrUnmarshallClCpmCompHealthcheckT, 4, 0, 0)(inMsgHandle,
                                              &cpmCompHealthcheck);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                   "Unable to unmarshall buffer, error [%#x]",
                   rc);
        goto failure;
    }

    strncpy(compName,
            cpmCompHealthcheck.compName.value,
            cpmCompHealthcheck.compName.length);

    rc = cpmCompFind(compName, gpClCpm->compTable, &comp);
    if (CL_OK != rc)
    {
        clLogDebug(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                   "Unable to find component [%s], error [%#x]",
                   compName,
                   rc);
        goto failure;
    }

    clOsalMutexLock(comp->compMutex);
    if (!((CL_HANDLE_INVALID_VALUE == comp->cpmHealthcheckTimerHandle) &&
          (CL_HANDLE_INVALID_VALUE != comp->hbTimerHandle)))
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_HB,
                   "Attempt to call health check confirm without "
                   "starting the client invoked health check.");
        rc = CL_CPM_RC(CL_ERR_BAD_OPERATION);
        goto unlock;
    }
    
    comp->hcConfirmed = CL_YES;
    clOsalMutexUnlock(comp->compMutex);

    if (cpmCompHealthcheck.healthcheckResult != CL_OK)
    {
        comp->hbFailureDetected = CL_TRUE;
        clCpmCompPreCleanupInvoke(comp);
        clCpmComponentFailureReportWithCookie(0,
                                              &cpmCompHealthcheck.compName,
                                              comp->instantiateCookie,
                                              0,
                                              comp->compConfig->
                                              healthCheckConfig.recommendedRecovery,
                                              0);
    }
    
    return CL_OK;

unlock:
    clOsalMutexUnlock(comp->compMutex);
failure:
    return rc;
}

ClRcT VDECL(cpmComponentInfoGet)(ClEoDataT data,
                                 ClBufferHandleT inMsgHandle,
                                 ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClNameT compName;
    ClCharT name[CL_MAX_NAME_LENGTH];
    ClCpmComponentT *comp = NULL;
    ClCpmCompSpecInfoRecvT compInfoRecv;
    ClUint32T i = 0;
    ClUint32T numArgs = 0;

    memset(&compName, 0, sizeof(compName));
    memset(name, 0, sizeof(name));
    memset(&compInfoRecv, 0, sizeof(ClCpmCompSpecInfoRecvT));

    rc = clXdrUnmarshallClNameT(inMsgHandle, (void *) &compName);
    if (CL_OK != rc)
    {
        clLogError("CPM", "CPM", "Invalid buffer, errro [%#x]", rc);
        goto out;
    }

    strncpy(name, compName.value, compName.length);

    rc = cpmCompFind(name, gpClCpm->compTable, &comp);
    if (CL_OK != rc)
    {
        clLogError("CPM", "CPM", "Unable to find component %s, "
                   "error [%#x]", name, rc);
        goto out;
    }

    compInfoRecv.period = comp->compConfig->healthCheckConfig.period;
    compInfoRecv.maxDuration = comp->compConfig->healthCheckConfig.maxDuration;
    compInfoRecv.recovery = comp->compConfig->healthCheckConfig.recommendedRecovery;

    /* Don't count the first argument. */
    for (i = 1; comp->compConfig->argv[i]; ++i)
    {
        ++numArgs;
    }

    compInfoRecv.numArgs = numArgs;
    if (compInfoRecv.numArgs)
    {
        compInfoRecv.args = clHeapAllocate(compInfoRecv.numArgs * sizeof(ClStringT));
        if (!compInfoRecv.args)
        {
            rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
            goto out;
        }

        /* Don't copy the first argument. */
        for (i = 1; comp->compConfig->argv[i]; ++i)
        {
            ClStringT *s = (compInfoRecv.args)+(i-1);
            ClUint32T len = strlen(comp->compConfig->argv[i]);
        
            s->pValue = clHeapAllocate(len);
            if (!s->pValue)
            {
                rc = CL_CPM_RC(CL_ERR_NO_MEMORY);
                goto out;
            }
            
            strncpy(s->pValue, comp->compConfig->argv[i], len);
            s->length = len;
        }
    }

    rc = VDECL_VER(clXdrMarshallClCpmCompSpecInfoRecvT, 4, 0, 0)(&compInfoRecv,
                                                                 outMsgHandle,
                                                                 0);
    if (CL_OK != rc)
    {
        clLogError("CPM", "CPM", "Unable to marshall component data, "
                   "error [%#x]", rc);
        goto out;
    }

out:
    if (compInfoRecv.numArgs && compInfoRecv.args)
    {
        for (i = 0; i < numArgs; ++i)
        {
            ClStringT *s = (compInfoRecv.args)+i;
            if (s)
            {
                clHeapFree(s->pValue);
            }
        }
        clHeapFree(compInfoRecv.args);
    }
    return rc;
}

ClRcT cpmEventPublishQueueInit()
{
    ClRcT rc = CL_OK;

    if( (rc = clJobQueueInit(&eventPublishQueue, 0, 1) ) != CL_OK)
    {
        clLogError(CPM_LOG_AREA_CPM, CPM_LOG_CTX_CPM_EVT,
                   "Event publish job queue initialize returned [%#x]", rc);
    }

    return rc;
}
