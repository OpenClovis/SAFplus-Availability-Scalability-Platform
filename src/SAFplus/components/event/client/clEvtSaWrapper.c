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
 * ModuleName  : event                                                         
 * File        : clEvtSaWrapper.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module provides the SAF compliant Interface for 
 *          Clovis EM API.
 *****************************************************************************/

#include <string.h>
#include <clDebugApi.h>
#include "clCpmApi.h"
#include "clCksmApi.h"
#include "clTimerApi.h"
#include "clOsalApi.h"
#include "clEventApi.h"
#include "saAis.h"
#include "saEvt.h"
#include "clEventClientIpi.h"
#include "clEventSaWrapperIpi.h"
#include <ipi/clHandleIpi.h>
#include <ipi/clEoIpi.h>
/* Added to fix Bug 4544 */

#define isChannelFlagValid(X)  ((X) > 0 && (X) <= (SA_EVT_CHANNEL_PUBLISHER | SA_EVT_CHANNEL_SUBSCRIBER | SA_EVT_CHANNEL_CREATE)) 

#define isDispatchFlagValid(X) ((X) > 0 && (X) <= (SA_DISPATCH_ONE | SA_DISPATCH_ALL | SA_DISPATCH_BLOCKING))

ClUint32T gSaEvtInitCount = 0;
#define isInitDone() ((0 == gSaEvtInitCount)? CL_FALSE : CL_TRUE)
#define isLastFinalize() ((0 == gSaEvtInitCount)? CL_TRUE : CL_FALSE)
#define SA_EVT_INIT_COUNT_INC() gSaEvtInitCount++
#define SA_EVT_INIT_COUNT_DEC() gSaEvtInitCount--
#define SA_EVT_CHECK_INIT_COUNT()\
do{\
    if(0 == gSaEvtInitCount)\
    {\
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Event Initialization not done [0x%X]\n\r\n", CL_EVENT_ERR_INIT_NOT_DONE));\
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME, CL_LOG_MESSAGE_0_COMPONENT_UNINITIALIZED);\
            CL_FUNC_EXIT();\
            rc = CL_EVENTS_RC(CL_EVENT_ERR_INIT_NOT_DONE); \
            clEvtSafErrorMap(rc, &rc); \
            return rc;\
    }\
}while(0);

SaEvtHandleT *gpEvtHandle = NULL;

extern ClRcT clEvtInitValidate(ClEoExecutionObjT **pEoObj,
        ClEvtClientHeadT **ppEvtClientHead);

/* Database Keeping the actual callbacks */
ClHandleDatabaseHandleT gSaEvtHandleDatabase; 

ClRcT saEvtClientInit(void)
{  
    ClRcT rc = CL_OK;
    if(CL_FALSE == isInitDone())
    {
        rc = clHandleDatabaseCreate(NULL, &gSaEvtHandleDatabase);
        if(rc != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHandleDatabaseCreate Failed, rc[0x%X]", rc));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                    CL_LOG_MESSAGE_1_HANDLE_DB_CREATION_FAILED, rc);
            return SA_AIS_ERR_NO_RESOURCES;
        }
    }
    return CL_OK;
}

void clEvtSafErrorMap(ClRcT rc, ClRcT *pMappedError)
{
    ClRcT compId = CL_GET_CID(rc);
    ClRcT errorCode = CL_GET_ERROR_CODE(rc);

    if (CL_OK == rc)
    {
        *pMappedError = SA_AIS_OK;
        return;
    }

    if (CL_CID_EVENTS == compId)
    {
        switch (errorCode)
        {
            case CL_EVENT_ERR_INTERNAL:
                *pMappedError = SA_AIS_ERR_LIBRARY;
                return;

            case CL_EVENT_ERR_NULL_PTR: /* SAF expect Invalid Parameter */
            case CL_EVENT_ERR_INVALID_PARAM:
                *pMappedError = SA_AIS_ERR_INVALID_PARAM;
                return;

            case CL_EVENT_ERR_NO_MEM:
                *pMappedError = SA_AIS_ERR_NO_MEMORY;
                return;

            case CL_EVENT_ERR_INIT_NOT_DONE: /* Same as Bad Handle */
            case CL_EVENT_ERR_BAD_HANDLE:
                *pMappedError = SA_AIS_ERR_BAD_HANDLE;
                return;

            case CL_EVENT_ERR_BAD_FLAGS:
                *pMappedError = SA_AIS_ERR_BAD_FLAGS;
                return;

            case CL_EVENT_ERR_NOT_OPENED_FOR_SUBSCRIPTION:
            case CL_EVENT_ERR_NOT_OPENED_FOR_PUBLISH:
                *pMappedError = SA_AIS_ERR_ACCESS;
                return;

            case CL_EVENT_ERR_VERSION:
                *pMappedError = SA_AIS_ERR_VERSION;
                return;

            case CL_EVENT_ERR_NO_SPACE:
                *pMappedError = SA_AIS_ERR_NO_SPACE;
                return;
            
            case CL_EVENT_ERR_INIT:
                *pMappedError = SA_AIS_ERR_INIT;
                return;
        }
    }
    else
    {
        switch (errorCode)
        {
            case CL_ERR_TIMEOUT:
                *pMappedError = SA_AIS_ERR_TIMEOUT;
                return;
        }

    }
    *pMappedError = SA_AIS_ERR_LIBRARY; /* Defaults to this Error if no mapping 
                                         */
    return;

}

void clEventOpenAsyncCallbackTrap(ClInvocationT invocation,
        ClEventChannelHandleT channelHandle,
        ClRcT error)
{
    ClRcT rc = CL_OK;
    SaEvtHandleT evtHandle = 0; 
    SaInvocationT saInvocation = 0;
    SaEvtInitInfoT * pSaEvtInitInfo = NULL;
    SaEvtHdlDbDataT * pSaEvtHdlDbData = NULL;
    SaEvtChanInitInfoT * pSaEvtChanInitInfo = NULL;

    /*
     * Got the initialization handle from the invocation parameter, which 
     * was passed during channel Open Async call
     */

    SaEvtInvocationInfoT * pSaEvtInvInfo = (SaEvtInvocationInfoT *)(ClWordT)invocation; 
    evtHandle = pSaEvtInvInfo->evtHandle;
    saInvocation = (SaInvocationT) pSaEvtInvInfo->invocation;
    clHeapFree(pSaEvtInvInfo);

    /*
     * Now store the initialization handle for this channel Handle for the 
     * use of Event Delivery Callback, as this channel Handle is got asynch
     * ronously.
     */
    rc = clHandleCreateSpecifiedHandle(gSaEvtHandleDatabase, sizeof(SaEvtChanInitInfoT), channelHandle); 
    if(rc != CL_OK)
    {
        return ; 
    }
    rc = clHandleCheckout(gSaEvtHandleDatabase, channelHandle, (void**)&pSaEvtChanInitInfo);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHandleCheckout Failed, rc[0x%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        return ;
    }
    pSaEvtChanInitInfo->handleType = SA_EVT_CHANNEL_INFO_HANDLE;
    pSaEvtChanInitInfo->evtHandle = (ClEventInitHandleT) evtHandle;
    
    rc = clHandleCheckin(gSaEvtHandleDatabase, channelHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHandleCheckin Failed, rc[0X%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        return ;
    }

    /*
     * Checkout using the evtHandle obtained from the invocation 
     */
    rc = clHandleCheckout(gSaEvtHandleDatabase, evtHandle, (void **)&pSaEvtHdlDbData);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHandleCheckout Failed, rc[0x%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);

        return;
    }
    /*
     * Validate the handleType and go ahead if it matches.
     */
    if(pSaEvtHdlDbData->handleType == SA_EVT_INIT_INFO_HANDLE)
    {
        pSaEvtInitInfo =  (SaEvtInitInfoT *)pSaEvtHdlDbData;
    }
    else
    {
        rc = clHandleCheckin(gSaEvtHandleDatabase, evtHandle);
        return;
    }

    SaEvtChannelHandleT saChannelHandle = (SaEvtChannelHandleT) channelHandle;

    pSaEvtInitInfo->saEventCallbacks.saEvtChannelOpenCallback(saInvocation, saChannelHandle, (SaAisErrorT)error);

    rc = clHandleCheckin(gSaEvtHandleDatabase, evtHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHandleCheckin Failed, rc[0X%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        return;
    }
    return;
}


void clEventDeliverCallbackTrap(ClEventSubscriptionIdT subscriptionId,
                                ClEventHandleT eventHandle,
                                ClSizeT eventDataSize)
{
    ClRcT rc = CL_OK;
    SaEvtEventHandleT saEventHandle = 0; 
    SaEvtInitInfoT * pSaEvtInitInfo = NULL; 
    SaEvtHdlDbDataT * pSaEvtHdlDbData = NULL;
    
    SaEvtHandleT *pEvtHandle = NULL;
    /*
     * Obtain the evtHandle from the cookie passed during subscription
     * and checkout on that to obtain the callbacks stored in pSaEvtInit
     * Info
     */
    
    rc = clEventCookieGet(eventHandle, (void **)&pEvtHandle); // NTC
    if(rc != CL_OK)
    {  
       CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clEventCookieGet failed, rc[0x%X]\n\r", rc));
       return; 
    }
    saEventHandle = (SaEvtEventHandleT) eventHandle;
    /*
     * Checkout using the evtHandle obtained from the cookie
     */
  
    rc = clHandleCheckout(gSaEvtHandleDatabase, *pEvtHandle, (void **)&pSaEvtHdlDbData);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHandleCheckout Failed, rc[0x%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        
        goto failure;
    }
    /*
     * Validate the handleType and go ahead if it matches.
     */
     if(pSaEvtHdlDbData->handleType == SA_EVT_INIT_INFO_HANDLE)
     {
         pSaEvtInitInfo =  (SaEvtInitInfoT *)pSaEvtHdlDbData;
     }
     else
     {
        rc = clHandleCheckin(gSaEvtHandleDatabase, *pEvtHandle);
        goto failure;
     }
     /*
      * Call the Callback stored in pSaEvtInitInfo
      */
     pSaEvtInitInfo->saEventCallbacks.saEvtEventDeliverCallback(subscriptionId, saEventHandle, eventDataSize);
     
     rc = clHandleCheckin(gSaEvtHandleDatabase, *pEvtHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHandleCheckin Failed, rc[0X%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    }
   
// success:
failure:
// clHeapFree(pEvtHandle); This can't be freed here as it is allocated only once during subscribe. So moving to unsubscribe.

    return;
}

ClEventCallbacksT gEventCallBackTrap = {
    clEventOpenAsyncCallbackTrap,
    clEventDeliverCallbackTrap,
};


SaAisErrorT saEvtInitialize(SaEvtHandleT * pEvtHandle,
                            const SaEvtCallbacksT *pEvtCallbacks,
                            SaVersionT *pVersion)
{
    ClRcT rc = CL_OK;
    ClEventInitHandleT evtInitHandle = 0;
    SaEvtInitInfoT * pSaEvtCallbackInfo = NULL;/* structure containing callba                                                 -ks for this initialization                                               */ 
    ClEventCallbacksT clEventCallBackTrap = { NULL, NULL };

    rc = clASPInitialize();
    if(CL_OK != rc)
    {
        clLogCritical(CL_EVENT_LIB_NAME, "INI",
                      "ASP initialize failed, rc[0x%X]", rc);
        return SA_AIS_ERR_LIBRARY;
    }
    
    if (NULL == pEvtHandle)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Passed NULL for Init Handle, rc[0x%X]\n", SA_AIS_ERR_BAD_HANDLE));
        return SA_AIS_ERR_INVALID_PARAM;
    }
   
    /*
     * Pass the trap functions only when they are non-null: NULL 
     * callbacks will be caught in the clEventChannelOpenAsync and 
     * clEventSubscribe
     */
    
    if( NULL != pEvtCallbacks )
    {
        if( NULL != pEvtCallbacks->saEvtChannelOpenCallback )
        {
            clEventCallBackTrap.clEvtChannelOpenCallback = clEventOpenAsyncCallbackTrap;
        }
        if( NULL != pEvtCallbacks->saEvtEventDeliverCallback )
        {
            clEventCallBackTrap.clEvtEventDeliverCallback = clEventDeliverCallbackTrap;
        }
    }
    else
    {
        clLogWarning("EVT", "INI", "NULL callback structure passed");
    }

    /*
     * do the initialization of database etc. 
     */
    rc = saEvtClientInit(); 
    /*
     * Initialize with gEventCallBackTrap and maintain a mapping 
     * in the evtInitHandle handle for the actual callbacks passed
     * for the Initialization.
     */
    rc = clEventInitialize(&evtInitHandle, &clEventCallBackTrap,
            (ClVersionT *) pVersion);
    if(rc != CL_OK) 
    {
        clEvtSafErrorMap(rc, &rc);
        return rc;
    }
    
    rc = clHandleCreateSpecifiedHandle(gSaEvtHandleDatabase, sizeof(SaEvtInitInfoT), evtInitHandle);
    if(rc != CL_OK)
    {
        clEvtSafErrorMap(rc, &rc); 
        return rc; 
    }
    rc = clHandleCheckout(gSaEvtHandleDatabase, evtInitHandle, (void **)&pSaEvtCallbackInfo);                       
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHandleCheckout Failed, rc[0x%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        return SA_AIS_ERR_BAD_HANDLE;
    }
    /*
     * Keep the callback information for use in the callback traps
     * to execute the actual callbacks.
     */
    pSaEvtCallbackInfo->handleType = SA_EVT_INIT_INFO_HANDLE; 
    
    if( NULL != pEvtCallbacks )
    {
        pSaEvtCallbackInfo->saEventCallbacks.saEvtChannelOpenCallback = pEvtCallbacks->saEvtChannelOpenCallback;
        pSaEvtCallbackInfo->saEventCallbacks.saEvtEventDeliverCallback = pEvtCallbacks->saEvtEventDeliverCallback;
    }

    rc = clHandleCheckin(gSaEvtHandleDatabase, evtInitHandle); 
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHandleCheckin Failed, rc[0X%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        return SA_AIS_ERR_LIBRARY;
    }
    /*
     * Inc the Init Reference Count 
     */
    SA_EVT_INIT_COUNT_INC();

    *pEvtHandle = (SaEvtHandleT) evtInitHandle;

    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtSelectionObjectGet(SaEvtHandleT evtHandle,
                                    SaSelectionObjectT * selectionObject)
{
    ClRcT rc = CL_OK;

    rc = clEventSelectionObjectGet((ClEventInitHandleT) evtHandle,
                                   (ClSelectionObjectT *) selectionObject);

    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtDispatch(SaEvtHandleT evtHandle,
                          SaDispatchFlagsT dispatchFlags)
{
    ClRcT rc = CL_OK;

    if(!isDispatchFlagValid(dispatchFlags))
    {
        return SA_AIS_ERR_BAD_FLAGS;
    }
    
    rc = clEventDispatch((ClEventInitHandleT) evtHandle,
                         (ClDispatchFlagsT) dispatchFlags);

    clEvtSafErrorMap(rc, &rc);
    return rc;
}

ClRcT saEvtFinalizeWalkCallback(ClHandleDatabaseHandleT databaseHandle, ClHandleT handle, ClPtrT pCookie)
{
    ClRcT rc = CL_OK;
    SaEvtChanInitInfoT * pSaEvtChanInitInfo = NULL;
    SaEvtHdlDbDataT *pSaEvtHdlDbData = NULL;
    ClEventInitHandleT *pEvtInitHandle = NULL;
    
    pEvtInitHandle = (ClEventInitHandleT *)pCookie; 
    
    rc = clHandleCheckout(databaseHandle, handle, (void**)&pSaEvtHdlDbData);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHandleCheckout Failed, rc[0x%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        return CL_OK;
    }
    
    if(pSaEvtHdlDbData->handleType == SA_EVT_CHANNEL_INFO_HANDLE)
    {
        pSaEvtChanInitInfo = (SaEvtChanInitInfoT *)pSaEvtHdlDbData;
    }
    else
    {
        rc = clHandleCheckin(databaseHandle, handle);
        return CL_OK;
    }
    
    if(pSaEvtChanInitInfo->evtHandle == *pEvtInitHandle)
    {
        rc = clHandleCheckin(databaseHandle, handle);
        /*
         * Destroy the Handle for this Channel
         */
        rc = clHandleDestroy(databaseHandle, handle);
        return CL_OK;
    }
    rc = clHandleCheckin(databaseHandle, handle);
    return CL_OK;
}

SaAisErrorT saEvtFinalize(SaEvtHandleT evtHandle)
{

    ClRcT rc = CL_OK;

    rc = clEventFinalize((ClEventInitHandleT) evtHandle);
    /*
     * Walk the Sawrapper database and destroy the handles for the 
     * initialization handle, evtHandle, and destroy the channel 
     * information stored for channels open on this init handle
     */
    if(rc != CL_OK)
    {
        clEvtSafErrorMap(rc, &rc);
        return rc;
    }
    rc = clHandleWalk(gSaEvtHandleDatabase, saEvtFinalizeWalkCallback, &evtHandle);
    rc = clHandleDestroy(gSaEvtHandleDatabase, evtHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHandleDestroy Failed, rc[0x%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_DB_DESTROY_FAILED, rc);
        rc = CL_EVENT_ERR_INTERNAL;
        clEvtSafErrorMap(rc, &rc);
        return rc;
    }

    /* Decrement the Init count */
    SA_EVT_INIT_COUNT_DEC(); 
    
    /* 
     * Check if this is the last Finalize call for the library
     * if last finalize then destroy the Handle Database
     */
    if(CL_TRUE == isLastFinalize())
    {
        rc = clHandleDatabaseDestroy(gSaEvtHandleDatabase);
        if(CL_OK != rc)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHandleDestroy Failed, rc[0x%X]", rc));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                    CL_LOG_MESSAGE_1_HANDLE_DB_DESTROY_FAILED, rc);
            rc = CL_EVENT_ERR_INTERNAL;
            clEvtSafErrorMap(rc, &rc);
            return rc;
        }

    }
    
    if(CL_OK != clASPFinalize())
    {
        clLogInfo(CL_EVENT_LIB_NAME, "FIN",
                  "ASP finalize failed, rc[0x%X]", rc);
        return SA_AIS_ERR_LIBRARY;
    }

    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtChannelOpen(SaEvtHandleT evtHandle,
                             const SaNameT *pEvtChannelName,
                             SaEvtChannelOpenFlagsT evtChannelOpenFlag,
                             SaTimeT timeout,
                             SaEvtChannelHandleT *pChannelHandle)
{
    ClRcT rc = CL_OK;
    ClEventChannelOpenFlagsT flag = 0;
    ClEventChannelHandleT channelHandle;
    SaEvtChanInitInfoT * pSaEvtChanInitInfo = NULL;
   
    /*
     * Check if library is initialized or not
     */
    SA_EVT_CHECK_INIT_COUNT();

    if (NULL == pChannelHandle)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Passed NULL for Channel Handle, rc[0x%X]\n", SA_AIS_ERR_BAD_HANDLE));
        return SA_AIS_ERR_INVALID_PARAM;
    }

    if(!isChannelFlagValid(evtChannelOpenFlag))
    {
        return SA_AIS_ERR_BAD_FLAGS;
    }
    
    if (0 != (evtChannelOpenFlag & SA_EVT_CHANNEL_PUBLISHER))
    {
        flag |= CL_EVENT_CHANNEL_PUBLISHER;
    }

    if (0 != (evtChannelOpenFlag & SA_EVT_CHANNEL_SUBSCRIBER))
    {
        flag |= CL_EVENT_CHANNEL_SUBSCRIBER;
    }

    rc = clEventChannelOpen((ClEventInitHandleT) evtHandle,
                            (const ClNameT *) pEvtChannelName,
                            flag | CL_EVENT_GLOBAL_CHANNEL, timeout,
                            &channelHandle);
    if (CL_OK != rc)
    {
        clEvtSafErrorMap(rc, &rc);
        return rc;
    }
    rc = clHandleCreateSpecifiedHandle(gSaEvtHandleDatabase, sizeof(SaEvtChanInitInfoT), channelHandle); 
    if(rc != CL_OK)
    {
        clEvtSafErrorMap(rc, &rc); 
        return rc; 
    }
    rc = clHandleCheckout(gSaEvtHandleDatabase, channelHandle, (void**)&pSaEvtChanInitInfo);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHandleCheckout Failed, rc[0x%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        return SA_AIS_ERR_BAD_HANDLE;
    }
    pSaEvtChanInitInfo->handleType = SA_EVT_CHANNEL_INFO_HANDLE;
    pSaEvtChanInitInfo->evtHandle = (ClEventInitHandleT) evtHandle;
    
    rc = clHandleCheckin(gSaEvtHandleDatabase, channelHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHandleCheckin Failed, rc[0X%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        return SA_AIS_ERR_LIBRARY;
    }
    *pChannelHandle = (SaEvtChannelHandleT) channelHandle;

    clEvtSafErrorMap(rc, &rc);
    return rc;
}



SaAisErrorT saEvtChannelOpenAsync(SaEvtHandleT evtHandle,
                                  SaInvocationT invocation,
                                  const SaNameT *pEvtChannelName,
                                  SaEvtChannelOpenFlagsT channelOpenFlags)
{
    ClRcT rc = CL_OK;
    SaEvtInvocationInfoT * pSaEvtInvInfo;
    
    /*
     * Check if library is initialized or not
     */
    SA_EVT_CHECK_INIT_COUNT();
    
    /*
     * pass the evtHandle in a structure along with the invocation: This is      * used in the ChannelOpenAsynccallbackTrap for faster search of the 
     * callback
     */
    pSaEvtInvInfo =clHeapAllocate(sizeof(SaEvtInvocationInfoT)); 
    if(NULL == pSaEvtInvInfo)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate Memory \n\r"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        CL_FUNC_EXIT();
        rc = CL_EVENT_ERR_NO_MEM;
        clEvtSafErrorMap(rc, &rc);
        return rc;
 
    }
    pSaEvtInvInfo->evtHandle = evtHandle;
    pSaEvtInvInfo->invocation = invocation;
    /*
     * Convert the pointer to Handle type and then to integer type
     * for 32/64 bit compatibility
     */
    invocation = (ClInvocationT)(ClWordT)pSaEvtInvInfo;
    
    rc = clEventChannelOpenAsync((ClEventInitHandleT) evtHandle, invocation,
                                 (const ClNameT *) pEvtChannelName,
                                 channelOpenFlags | CL_EVENT_GLOBAL_CHANNEL);
    if(rc != CL_OK) 
    {
        clHeapFree(pSaEvtInvInfo); 
    }
    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtChannelClose(SaEvtChannelHandleT channelHandle)
{
    ClRcT rc = CL_OK;
    
    rc = clEventChannelClose((ClEventChannelHandleT) channelHandle);
    
    if(rc != CL_OK)
    {
        clEvtSafErrorMap(rc, &rc);
        return rc;
    }
    /*
     * Destroy the channel information stored in the saWrapper for 
     * this channel being closed
     */
    rc = clHandleDestroy(gSaEvtHandleDatabase, channelHandle);
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHandleDestroy Failed, rc[0x%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_DB_DESTROY_FAILED, rc);
        rc = CL_EVENT_ERR_INTERNAL;
        clEvtSafErrorMap(rc, &rc);
    }
    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtChannelUnlink(SaEvtHandleT evtHandle,
                               const SaNameT *pEvtChannelName)
{
    ClRcT rc = CL_OK;

    if( NULL == pEvtChannelName )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("pEvtChannelName passed is NULL\n"));
        return SA_AIS_ERR_INVALID_PARAM; 
    }        

    rc = clEventChannelUnlink((ClEventInitHandleT) evtHandle,
                              (const ClNameT *) pEvtChannelName);
    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtEventAllocate(SaEvtChannelHandleT channelHandle,
                               SaEvtEventHandleT *pEventHandle)
{

    ClRcT rc = CL_OK;
    ClEventHandleT eventHandle;

    if (NULL == pEventHandle)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Passed NULL for Event Handle, rc[0x%X]\n", SA_AIS_ERR_BAD_HANDLE));
        return SA_AIS_ERR_INVALID_PARAM;
    }

    rc = clEventAllocate((ClEventChannelHandleT) channelHandle, &eventHandle);
    clEvtSafErrorMap(rc, &rc);
    *pEventHandle = (SaEvtEventHandleT) eventHandle;
    return rc;
}

SaAisErrorT saEvtEventFree(SaEvtEventHandleT eventHandle)
{
    ClRcT rc = CL_OK;

    rc = clEventFree((ClEventHandleT) eventHandle);
    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtEventAttributesSet(SaEvtEventHandleT eventHandle,
                                    const SaEvtEventPatternArrayT
                                    *pPatternArray,
                                    SaEvtEventPriorityT priority,
                                    SaTimeT retentionTime,
                                    const SaNameT *pPublisherName)
{
    ClRcT rc = CL_OK;

    rc = clEventAttributesSet((ClEventHandleT) eventHandle,
                              (const ClEventPatternArrayT *) pPatternArray,
                              priority, retentionTime,
                              (const ClNameT *) pPublisherName);
    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtEventAttributesGet(SaEvtEventHandleT eventHandle,
                                    SaEvtEventPatternArrayT *pPatternArray,
                                    SaEvtEventPriorityT * pPriority,
                                    SaTimeT * pRetentionTime,
                                    SaNameT *pPublisherName,
                                    SaTimeT * pPublishTime,
                                    SaEvtEventIdT * pEventId)
{
    ClRcT rc = CL_OK;

    rc = clEventAttributesGet((ClEventHandleT) eventHandle,
                              (ClEventPatternArrayT *) pPatternArray, pPriority,
                              pRetentionTime, (ClNameT *) pPublisherName,
                              pPublishTime, (ClEventIdT *) pEventId);
    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtEventDataGet(SaEvtEventHandleT eventHandle, void *pEventData,
                              SaSizeT *pEventDataSize)
{
    ClRcT rc = CL_OK;

    rc = clEventDataGet((ClEventHandleT) eventHandle, pEventData,
                        pEventDataSize);
    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtEventPublish(SaEvtEventHandleT eventHandle,
                              const void *pEventData, SaSizeT eventDataSize,
                              SaEvtEventIdT * pEventId)
{
    ClRcT rc = CL_OK;

    rc = clEventPublish((ClEventHandleT) eventHandle, pEventData, eventDataSize,
                        (ClEventIdT *) pEventId);
    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtEventSubscribe(SaEvtChannelHandleT channelHandle,
                                const SaEvtEventFilterArrayT *pFilters,
                                SaEvtSubscriptionIdT subscriptionId)
{
    ClRcT rc = CL_OK;
    SaEvtHdlDbDataT * pSaEvtHdlDbData = NULL;
    SaEvtChanInitInfoT * pSaEvtChanInitInfo = NULL;
    SaEvtHandleT evtHandle = 0;
    SaEvtHandleT *pEvtHandle = NULL;
    
    SA_EVT_CHECK_INIT_COUNT();
   
    /*
     * Checkout the database on channelHandle to obtain initiali-
     * zation handle, pass that in the cookie to clEventSubscribe
     */
    rc = clHandleCheckout(gSaEvtHandleDatabase, channelHandle, (void**)&pSaEvtHdlDbData);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("clHandleCheckout Failed, rc[0x%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        return SA_AIS_ERR_BAD_HANDLE;
    }
    /*
     * Validate the data if its for Initialization Handle
     */
    if(pSaEvtHdlDbData->handleType == SA_EVT_CHANNEL_INFO_HANDLE)
    {
        pSaEvtChanInitInfo = (SaEvtChanInitInfoT *)pSaEvtHdlDbData; 
    }
    else
    {
        return SA_AIS_ERR_BAD_HANDLE;
    }
    evtHandle = pSaEvtChanInitInfo->evtHandle;
    
    rc = clHandleCheckin(gSaEvtHandleDatabase, channelHandle);
    if (CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clHandleCheckin Failed, rc[0X%X]", rc));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        return SA_AIS_ERR_LIBRARY;
    }
    
    /*
     * Allocate memory to hold the Handle as a cookie as it is 64 bits.
     */
    gpEvtHandle = pEvtHandle =clHeapAllocate(sizeof(*pEvtHandle)); 
    if(NULL == pEvtHandle)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Failed to allocate Memory \n\r"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, CL_EVENT_LIB_NAME,
                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        rc = CL_EVENT_ERR_NO_MEM;
        clEvtSafErrorMap(rc, &rc);
        return rc;
 
    }
    *pEvtHandle = evtHandle;

   rc = clEventSubscribe((ClEventChannelHandleT) channelHandle,
                          (const ClEventFilterArrayT *) pFilters,
                          subscriptionId, pEvtHandle);
    if(rc != CL_OK)
    {
        clHeapFree(pEvtHandle);
        clEvtSafErrorMap(rc, &rc);
        return rc;
    } 
    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtEventUnsubscribe(SaEvtChannelHandleT channelHandle,
                                  SaEvtSubscriptionIdT subscriptionId)
{
    ClRcT rc = CL_OK;

    rc = clEventUnsubscribe((ClEventChannelHandleT) channelHandle,
                            subscriptionId);
    clHeapFree(gpEvtHandle);
    clEvtSafErrorMap(rc, &rc);
    return rc;
}

SaAisErrorT saEvtEventRetentionTimeClear(SaEvtChannelHandleT channelHandle,
                                         const SaEvtEventIdT eventId)
{
    ClRcT rc = CL_OK;

    rc = clEventRetentionTimeClear((ClEventChannelHandleT) channelHandle,
                                   eventId);
    clEvtSafErrorMap(rc, &rc);
    return rc;
}
