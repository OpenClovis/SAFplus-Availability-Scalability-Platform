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
 * ModuleName  : event                                                         
 * File        : clEvtServerMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *          This module forms the EM server and contains all the
 *          functionality that EM API support.
 *****************************************************************************/
#ifdef SOLARIS_BUILD
#include <strings.h>
#endif 
#include <clCpmApi.h>
#include <clTimerApi.h>
#include <clOsalApi.h>
#include <clRmdApi.h>
#include <ipi/clRmdIpi.h>
#include <clEventServerIpi.h>
#include <clEventUtilsIpi.h>

#include "clEventCkptIpi.h"     /* For Check Pointing Service */

#include "xdrClEvtInitRequestT.h"
#include "xdrClEvtChannelOpenRequestT.h"
#include "xdrClEvtSubscribeEventRequestT.h"
#include "xdrClEvtUnsubscribeEventRequestT.h"

/* 
 * Added for getting local system time
 */
#include <sys/time.h>

#ifdef VERSION_READY

# include <clVersionApi.h>

static ClVersionT gEvtClientToServerVersionsSupported[] = { {'B', 0x1, 0x1},    /* Add 
                                                                                 * newer 
                                                                                 * version 
                                                                                 * here 
                                                                                 */  };
                  static ClVersionDatabaseT gEvtClientToServerVersionDb = {
                      sizeof(gEvtClientToServerVersionsSupported) / sizeof(ClVersionT),
                      gEvtClientToServerVersionsSupported
                  };

# define clEvtVerifyClientToServerVersion(pHeader, outMsgHandle) \
    do {\
        ClRcT retCode = CL_OK; \
        ClVersionT _version = { (pHeader)->releaseCode, (pHeader)->majorVersion, 0 }; \
        retCode = clVersionVerify(&gEvtClientToServerVersionDb, &_version); \
        if(CL_OK != retCode) \
        { \
            clBufferNBytesWrite(outMsgHandle, (ClUint8T *)&_version, \
                    sizeof(ClVersionT)); \
            clLogError(CL_EVENT_LOG_AREA_SRV, "VER", \
                    CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE, "Client-Server", \
                    (pHeader)->releaseCode, (pHeader)->majorVersion, \
                    (pHeader)->minorVersion, _version.releaseCode, \
                    _version.majorVersion, _version.minorVersion); \
            rc = CL_EVENT_ERR_VERSION; \
            goto inDataAllocated; \
        } \
    }while(0);


static ClVersionT gEvtServerToServerVersionsSupported[] = { {'B', 0x1, 0x1},    /* Add 
                                                                                 * newer 
                                                                                 * version 
                                                                                 * here 
                                                                                 */  };
                  static ClVersionDatabaseT gEvtServerToServerVersionDb = {
                      sizeof(gEvtServerToServerVersionsSupported) / sizeof(ClVersionT),
                      gEvtServerToServerVersionsSupported
                  };



# define clEvtServerToServerVersionSet(pHeader) (pHeader)->releaseCode = 'B', \
                                                                         (pHeader)->majorVersion = 0x1, \
(pHeader)->minorVersion = 0x1

#else
# define clEvtVerifyClientToServerVersion(pHeader, outMsgHandle)
#endif

ClIocPhysicalAddressT gEvtPrimaryMasterAddr;    /* Primary master IOC
                                                     * address */
ClIocPhysicalAddressT gEvtSecondaryMasterAddr;  /* Secondary master IOC
                                                     * address */

/*
 * Global/Local Channel Information stored in this container 
 */
ClEvtChannelInfoT *gpEvtGlobalECHDb;
ClEvtChannelInfoT *gpEvtLocalECHDb;

/*
 * Mutexs to protect ECH/G & ECH/L 
 */
ClOsalMutexIdT gEvtGlobalECHMutex;
ClOsalMutexIdT gEvtLocalECHMutex;

/*
 * Primary/Secondary Master information 
 */
ClCntHandleT gEvtMasterECHHandle;
/*
 * Mutexs to protect Master Information 
 */
ClOsalMutexIdT gEvtMasterECHMutex;

ClEvtHeadT gEvtHead;

ClRcT clEvtVerifyServerToServerVersion(ClEvtEventPrimaryHeaderT *pHeader)
{
    ClRcT rc = CL_OK;

    ClVersionT _version =
    { (pHeader)->releaseCode, (pHeader)->majorVersion, 0 };

    rc = clVersionVerify(&gEvtServerToServerVersionDb, &_version);
    if (CL_OK != rc)
    {
        ClIocAddressT srcAddr;
        ClRmdOptionsT rmdOptions = { 0 };

        ClBufferHandleT inMsgHandle = 0;

        ClEvtNackInfoT evtNackInfo = {
            _version.releaseCode,
            _version.majorVersion,
            _version.minorVersion,
            CL_EVT_NACK_PUBLISH,
        };

        clLogError(CL_EVENT_LOG_AREA_SRV, "VER",
                CL_EVENT_LOG_MSG_7_VERSION_INCOMPATIBLE, "Server-Server",
                (pHeader)->releaseCode, (pHeader)->majorVersion,
                (pHeader)->minorVersion, _version.releaseCode,
                _version.majorVersion, _version.minorVersion);

        rc = clBufferCreate(&inMsgHandle);
        rc = clBufferNBytesWrite(inMsgHandle, (ClUint8T *) &evtNackInfo,
                sizeof(ClEvtNackInfoT));

        /*
         * Invoke RMD call provided by EM/S for opening channel, with packed user
         * data. 
         */
        rmdOptions.priority = 0;
        rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
        rmdOptions.timeout = CL_EVT_RMD_TIME_OUT;

        rc = clRmdSourceAddressGet(&srcAddr.iocPhyAddress);

        rc = clRmdWithMsg(srcAddr, EO_CL_EVT_NACK_SEND, inMsgHandle, 0,
                CL_RMD_CALL_ASYNC, &rmdOptions, NULL);

        CL_FUNC_EXIT();
        return CL_EVENT_ERR_VERSION;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

#ifndef IDL_READY

# include <netinet/in.h>        /* For Byte Ordering */

/*
 ** Following API assume that Network ordering is Big Endian.
 */
ClUint64T clEvtHostUint64toNetUint64(ClUint64T hostUint64)
{
    CL_FUNC_ENTER();

    if (CL_TRUE == clEvtUtilsIsLittleEndian())
    {
        ClUnion64T *pUnion64 = (ClUnion64T *) & hostUint64;

        ClUint32T tmp32 = pUnion64->dWords[0];  /* Preserve the lower dword */

        pUnion64->dWords[0] = htonl(pUnion64->dWords[1]);   /* Convert the
                                                             * higher dword and 
                                                             * * assign to
                                                             * lower */
        pUnion64->dWords[1] = htonl(tmp32); /* Convert the preserved lower
                                             * dword and assign to higher */
    }

    CL_FUNC_EXIT();
    return hostUint64;
}

ClUint64T clEvtNetUint64toHostUint64(ClUint64T netUint64T)
{
    CL_FUNC_ENTER();

    if (CL_TRUE == clEvtUtilsIsLittleEndian())
    {
        ClUnion64T *pUnion64 = (ClUnion64T *) & netUint64T;

        ClUint32T tmp32 = pUnion64->dWords[0];  /* Preserve the lower dword */

        pUnion64->dWords[0] = ntohl(pUnion64->dWords[1]);   /* Convert the
                                                             * higher dword and 
                                                             * * assign to
                                                             * lower */
        pUnion64->dWords[1] = ntohl(tmp32); /* Convert the preserved lower
                                             * dword and assign to higher */
    }

    CL_FUNC_EXIT();
    return netUint64T;
}

#endif

/***************************************************************************************
  Functions that need to be exposed
 ***************************************************************************************/
extern ClRcT clEvtInitializeViaRequest(ClEvtInitRequestT *pEvtInitReq,
        ClUint32T type);
extern ClRcT clEvtChannelOpenViaRequest(ClEvtChannelOpenRequestT
        *pEvtChannelOpenRequest,
        ClUint32T type);
extern ClRcT clEvtEventSubscribeViaRequest(ClEvtSubscribeEventRequestT
        *pEvtSubsReq, ClUint32T type);
extern ClRcT clEvtEventCleanupViaRequest(ClEvtUnsubscribeEventRequestT
        *pEvtUnsubsReq, ClUint32T type);


/******************************************************************
  Handle Mangement
 ******************************************************************/

/*
 * The following are used in generation of event handles returned
 * to the user on the invocation of clEventInitialize.
 */

#include "clHandleApi.h"

/*
 * Use Specified Handle Api during recovery to populate the Handle Database. 
 */
#ifdef CKPT_ENABLED
# include <ipi/clHandleIpi.h>
#endif


/*
 ** Instance handle database.
 */
ClHandleDatabaseHandleT gEvtHandleDatabaseHdl;

/*
 ** Destructor to be called when deleting event handles.
 */
void clEvtHandleInstanceDestructor(void *notused)
{
    return ;
}

/*** EM Wrappers around Handle API to hide the Database Handle ***/

/*
 ** EM Wrapper around the Handle Create API calls underlying
 ** API with size of instance = 0.
 */
#define clEvtHandleCreate(pEvtHandle) clHandleCreate(gEvtHandleDatabaseHdl, sizeof(ClEvtUserIdT), pEvtHandle)
/*
 ** EM Wrapper around the Handle Destroy API.
 */
#define clEvtHandleDestroy(evtHandle) clHandleDestroy(gEvtHandleDatabaseHdl, evtHandle)
/*
 ** Handle Database Initialization API.
 */
ClRcT clEvtHandleDatabaseInit(void)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    /*
     ** Create the Handle Database for event handle generation during
     ** clEventInitialize.
     */
    rc = clHandleDatabaseCreate(clEvtHandleInstanceDestructor,
            &gEvtHandleDatabaseHdl);
    if (CL_OK != rc)
    {
        clLogCritical(CL_EVENT_LOG_AREA_SRV, "HDB",
                CL_LOG_MESSAGE_1_HANDLE_DB_CREATION_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

/*
 ** Handle Database Cleanup API.
 */
ClRcT clEvtHandleDatabaseExit(void)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    rc = clHandleDatabaseDestroy(gEvtHandleDatabaseHdl);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "HDB",
                CL_LOG_MESSAGE_1_HANDLE_DB_DESTROY_FAILED, rc);
        CL_FUNC_EXIT();
        return rc;
    }

    CL_FUNC_EXIT();
    return CL_OK;
}

/***************************************************************************************/

/*
 * This function will do key comparision for user data 
 */
ClInt32T clEvtUserKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClInt32T cmpResult = 0;

    ClEvtUserIdT *pUserId1 = (ClEvtUserIdT *) key1;
    ClEvtUserIdT *pUserId2 = (ClEvtUserIdT *) key2;

#ifdef EVT_DBG_ENABLE
    if (NULL == pUserId1 || NULL == pUserId2)
    {
        return CL_EVENT_ERR_NULL_PTR;
    }
#endif

    cmpResult = pUserId1->eoIocPort - pUserId2->eoIocPort;

    if (0 == cmpResult && 0 != pUserId2->evtHandle && 0 != pUserId1->evtHandle)
    {
        return (ClInt32T) (pUserId1->evtHandle - pUserId2->evtHandle); 
    }
    else
    {
        return cmpResult;
    }
}

/*
 * Wrapper for the clEvtUserKeyCompare 
 */

#define clEvtUserIDCompare(userId1, userId2)\
    clEvtUserKeyCompare((ClCntKeyHandleT)&(userId1), (ClCntKeyHandleT)&(userId2))

/*
 ** This function will deallocate the memroy which is allocated to store
 ** information about the EM user.
 */
void clEvtUserDataDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    ClRcT rc = CL_OK;
    ClEvtUserIdT *pUserID = (ClEvtUserIdT *) userKey;


    CL_FUNC_ENTER();

#ifdef EVT_DBG_ENABLE
    if (NULL == pUserID)
    {
        CL_FUNC_EXIT();
        return;
    }
#endif

    /*
     ** Destroy evtHandle returned during clEventInitialize.
     */
    rc = clEvtHandleDestroy(pUserID->evtHandle);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "CNT",
                CL_EVENT_LOG_MSG_1_INTERNAL_ERROR, rc);
        CL_FUNC_EXIT();
        return;
    }

    clHeapFree((void *) userKey);

    CL_FUNC_EXIT();
    return;
}


/*
 * This function will do key comparision for subscribtion 
 */
ClInt32T clEvtSubscribeKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClInt32T cmpResult = 0;

    ClEvtSubsKeyT *pKey1 = (ClEvtSubsKeyT *) key1;
    ClEvtSubsKeyT *pKey2 = (ClEvtSubsKeyT *) key2;

#ifdef EVT_DBG_ENABLE
    if (NULL == pKey1 || NULL == pKey2)
    {
        return CL_EVENT_ERR_NULL_PTR;
    }
#endif

    cmpResult = clEvtUserIDCompare(pKey1->userId, pKey2->userId);

    if (0 == cmpResult)
    {
        return (ClInt32T) (pKey1->subscriptionId - pKey2->subscriptionId);
    }
    else
    {
        return (ClInt32T) cmpResult;
    }
}

/*
 * This function will deallocate the memroy which is allocated to store
 * information about subscription information 1. Deallocate filter information
 * if any. 2. Deallocate subscription info 
 */
void clEvtSubscriberUserDelete(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData)
{
    clHeapFree((void *) userKey);
    return;
}


/*
 * This function will do key comparision for event channel Mask data 
 */
ClInt32T clEvtECHMaskKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}

/*
 * This function will deallocate the memroy which is allocated to store
 * information about event channel 
 */
void clEvtECHMaskUserDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    clHeapFree((void *) userData);
    return;
}


/*
 * This function will do key comparision for event channel 
 */
ClInt32T clEvtECHKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClEvtChannelKeyT *pEvtChannelKey1 = (ClEvtChannelKeyT *) key1;
    ClEvtChannelKeyT *pEvtChannelKey2 = (ClEvtChannelKeyT *) key2;

    if (pEvtChannelKey1->channelId == pEvtChannelKey2->channelId)
    {
        return clEvtUtilsNameCmp((&pEvtChannelKey1->channelName),
                (&pEvtChannelKey2->channelName));
    }
    return (pEvtChannelKey1->channelId - pEvtChannelKey2->channelId);
}


/*
 * This funciton will do hashing for event channel 
 */
ClUint32T clEvtECHHashFunction(ClCntKeyHandleT key)
{
    ClEvtChannelKeyT *pChannelKey = (ClEvtChannelKeyT *) key;

    return ((pChannelKey->channelId) % CL_EVT_MAX_ECH_BUCKET);
}

/*
 * This function will deallocate the memroy which is allocated to store
 * information about event channel 
 */
void clEvtECHInfoDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    ClRcT rc = CL_OK;

#ifdef CKPT_ENABLED
    ClEvtChannelDBT *pEchDb = (ClEvtChannelDBT *) userData;
    ClEvtChannelKeyT *pEchKey = (ClEvtChannelKeyT *) userKey;

    rc = clEvtCkptSubsDSDelete(&pEchKey->channelName, pEchDb->channelScope);
#endif

    rc = clEvtChannelDBDelete((ClEvtChannelDBT *) userData);
    clHeapFree((void *) userData);
    clHeapFree((void *) userKey);
    return;
}

/*
 * This function will do key comparision for event channel 
 */
ClInt32T clEvtTypeKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClEvtEventTypeKeyT *pKey1 = (ClEvtEventTypeKeyT *) key1;
    ClEvtEventTypeKeyT *pKey2 = (ClEvtEventTypeKeyT *) key2;

    ClRuleResultT result;

    if (pKey2->flag == CL_EVT_TYPE_RBE_KEY_TYPE)
    {
        result =
            clRuleDoubleExprEvaluate(pKey1->key.pRbeExpr, pKey2->key.pRbeExpr);
    }
    else
    {
        if (NULL == pKey2->key.pData)
            return 0;

        result =
            clRuleExprEvaluate(pKey1->key.pRbeExpr,
                    (ClUint32T *) pKey2->key.pData, pKey2->dataLen);
    }

    if (CL_RULE_TRUE == result)
    {
        return 0;
    }

    return -1;
}

/*
 ** This function will deallocate the memroy which is allocated to store
 ** information about event channel
 */
void clEvtTypeUserDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    ClEvtEventTypeKeyT *pKey = (ClEvtEventTypeKeyT *) userKey;
    ClEvtEventTypeInfoT *pEvtTypeInfo = (ClEvtEventTypeInfoT *) userData;

    clCntDelete(pEvtTypeInfo->subscriberInfo);
    if (pKey->key.pRbeExpr)
        clRuleExprDeallocate((ClRuleExprT *) pKey->key.pRbeExpr);

    clHeapFree(pKey);
    clHeapFree(pEvtTypeInfo);

    return;
}

/*
 * This function will do key comparision for user of event channel 
 */
ClInt32T clEvtChannelUserKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClInt32T cmpResult = 0;

    ClEvtUserIdT *pUserId1 = (ClEvtUserIdT *) key1;
    ClEvtUserIdT *pUserId2 = (ClEvtUserIdT *) key2;

#ifdef EVT_DBG_ENABLE
    if (NULL == pUserId1 || NULL == pUserId2)
    {
        return CL_EVENT_ERR_NULL_PTR;
    }
#endif

    cmpResult = pUserId1->eoIocPort - pUserId2->eoIocPort;

    if (0 == cmpResult && 0 != pUserId2->evtHandle && 0 != pUserId1->evtHandle)
    {
        return (ClInt32T) (pUserId1->evtHandle - pUserId2->evtHandle);  
    }
    else
    {
        return cmpResult;
    }
}

/*
 * This function will deallocate the memroy which is allocated to store
 * information about event channel 
 */
void clEvtChannelUserDelete(ClCntKeyHandleT userKey, ClCntDataHandleT userData)
{
    CL_FUNC_ENTER();

    clHeapFree((void *) userKey);

    CL_FUNC_EXIT();
    return;
}

ClUint32T gEvtInitDone = CL_EVT_FALSE;  /* This holds EM is initialized or not */

/*****************************************************************************/


ClRcT clEvtChannelDBInit()
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();

    gpEvtGlobalECHDb = clHeapAllocate(sizeof(*gpEvtGlobalECHDb));
    if (NULL == gpEvtGlobalECHDb)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        rc = CL_EVENT_ERR_NO_MEM;
        goto failure;
    }

    gpEvtLocalECHDb = clHeapAllocate(sizeof(*gpEvtLocalECHDb));
    if (NULL == gpEvtLocalECHDb)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        rc = CL_EVENT_ERR_NO_MEM;
        goto globalECHDBallocated;
    }

    /*
     * Create DB for ECH/G & ECH/L. This will store associated information for
     * given ECH 
     */

    rc = clCntHashtblCreate(CL_EVT_MAX_ECH_BUCKET, clEvtECHKeyCompare,
            clEvtECHHashFunction, clEvtECHInfoDelete, clEvtECHInfoDelete,
            CL_CNT_UNIQUE_KEY,
            &gpEvtGlobalECHDb->evtChannelContainer);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc);
        goto localECHDBallocated;
    }

    rc = clCntHashtblCreate(CL_EVT_MAX_ECH_BUCKET, clEvtECHKeyCompare,
            clEvtECHHashFunction, clEvtECHInfoDelete, clEvtECHInfoDelete,
            CL_CNT_UNIQUE_KEY,
            &gpEvtLocalECHDb->evtChannelContainer);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc);
        goto globalECHHashCreated;
    }

    /*
     * Create Mutex to protect ECH/G & ECH/L 
     */
    rc = clOsalMutexCreate(&gEvtGlobalECHMutex);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_OSAL_MUTEX_CREATION_FAILED, rc);
        goto localECHHashCreated;
    }

    rc = clOsalMutexCreate(&gEvtLocalECHMutex);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_OSAL_MUTEX_CREATION_FAILED, rc);
        goto globaECHMutexCreated;
    }

// success:
    clLogTrace(CL_EVENT_LOG_AREA_SRV, "ECH", "Event Channel Database Initialization Successful");

    CL_FUNC_EXIT();
    return rc;

globaECHMutexCreated:
    clOsalMutexDelete(gEvtGlobalECHMutex);

localECHHashCreated:
    clCntDelete(gpEvtLocalECHDb->evtChannelContainer);

globalECHHashCreated:
    clCntDelete(gpEvtGlobalECHDb->evtChannelContainer);

localECHDBallocated:
    clHeapFree(gpEvtLocalECHDb);

globalECHDBallocated:
    clHeapFree(gpEvtGlobalECHDb);

failure:

    CL_FUNC_EXIT();
    return rc;
}

/*****************************************************************************/

ClRcT clEvtChannelDBClean()
{
    CL_FUNC_ENTER();

    clOsalMutexDelete(gEvtLocalECHMutex);
    clOsalMutexDelete(gEvtGlobalECHMutex);
    clCntDelete(gpEvtLocalECHDb->evtChannelContainer);
    clCntDelete(gpEvtGlobalECHDb->evtChannelContainer);
    clHeapFree(gpEvtLocalECHDb);
    clHeapFree(gpEvtGlobalECHDb);

    clLogTrace(CL_EVENT_LOG_AREA_SRV, "ECH", "Event Channel Database Cleanup Successful");

    CL_FUNC_EXIT();
    return CL_OK;
}

ClRcT clEvtChannelDBCreate(ClEvtChannelDBT *pChannelInfoDb)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();


    rc = clCntLlistCreate(clEvtTypeKeyCompare, clEvtTypeUserDelete, clEvtTypeUserDelete,
            CL_CNT_UNIQUE_KEY, &pChannelInfoDb->eventTypeInfo);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc);
        goto failure;
    }

#ifdef RETENTION_ENABLE
    rc = clEvtRetentionListCreate(&pChannelInfoDb);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc);
        goto eventTypeListCreated;
    }
#endif

    rc = clOsalMutexCreate(&pChannelInfoDb->channelLevelMutex);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_OSAL_MUTEX_CREATION_FAILED, rc);
        goto eventTypeListCreated;
    }

    /*
     * List of the user, who opened the event channel 
     */
    rc = clCntLlistCreate(clEvtChannelUserKeyCompare, clEvtChannelUserDelete,
            clEvtChannelUserDelete, CL_CNT_UNIQUE_KEY,
            &pChannelInfoDb->evtChannelUserInfo);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc);
        goto channelLevelMutexCreated;
    }

    pChannelInfoDb->subscriberRefCount = 0;
    pChannelInfoDb->publisherRefCount = 0;

#ifdef CKPT_ENABLED
    /*
     * Added for CKPT support to hold the msg buffer handle for ckeck
     * pointing the subs info 
     */
    rc = clBufferCreate(&pChannelInfoDb->subsCkptMsgHdl);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_BUFFER_MSG_CREATION_FAILED, rc);
        goto userInfoListCreated;
    }
#endif

// success:
    clLogTrace(CL_EVENT_LOG_AREA_SRV, "ECH", "Event Channel Database Creation Successful");

    CL_FUNC_EXIT();
    return rc;

userInfoListCreated:
    clCntDelete(pChannelInfoDb->evtChannelUserInfo);

channelLevelMutexCreated:
    clOsalMutexDelete(pChannelInfoDb->channelLevelMutex);

#ifdef RETENTION_ENABLE
    clEvtRetentionListDelete(pChannelInfoDb);
#endif

eventTypeListCreated:
    clCntDelete(pChannelInfoDb->eventTypeInfo);

failure:
    CL_FUNC_EXIT();
    return rc;
}

/*****************************************************************************/

ClRcT clEvtChannelDBDelete(ClEvtChannelDBT *pChannelInfoDb)
{
    ClRcT rc = CL_OK;

    CL_FUNC_ENTER();


    /*
     * Lock the containter and delete the information then unlock and delete the mutex 
     */
    rc = clOsalMutexLock(pChannelInfoDb->channelLevelMutex);
#ifdef RETENTION_ENABLE
    clEvtRetentionListDelete(pChannelInfoDb);
#endif
    clCntDelete(pChannelInfoDb->eventTypeInfo);
    clCntDelete(pChannelInfoDb->evtChannelUserInfo);
    rc = clOsalMutexUnlock(pChannelInfoDb->channelLevelMutex);

#ifdef CKPT_ENABLED
    rc = clBufferDelete(&pChannelInfoDb->subsCkptMsgHdl);
#endif

    rc = clOsalMutexDelete(pChannelInfoDb->channelLevelMutex);

    clLogTrace(CL_EVENT_LOG_AREA_SRV, "ECH", "Event Channel Database Deletion Successful");

    CL_FUNC_EXIT();
    return CL_OK;
}

/*****************************************************************************/


ClRcT clEvtNewChannelAddAndOrUserAdd(ClEvtChannelInfoT *pEvtChannelInfo,
        ClEvtUserIdT *pUserOfECH,
        ClUint8T evtChannelUserType,
        ClEvtChannelKeyT *pEvtChannelKey,
        ClEvtChannelDBT *pChannelInfoDb,
        ClUint8T channelScope, ClUint32T flag)
{
    ClRcT rc = CL_OK;

    ClCntNodeHandleT nodeHandle;
    ClEvtUserIdT *pUserOfECHKey = NULL;
    ClEvtChannelKeyT *pEvtChannelKeyTemp = NULL;


    CL_FUNC_ENTER();

    switch (flag)
    {
        case CL_EVT_ADD_USER_AND_CHANNEL:
            pEvtChannelKeyTemp = clHeapAllocate(sizeof(*pEvtChannelKeyTemp));
            if (NULL == pEvtChannelKeyTemp)
            {
                clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
                goto failure;
            }

            pChannelInfoDb->channelScope = channelScope;
            // memcpy(pEvtChannelKeyTemp, pEvtChannelKey, sizeof(ClEvtChannelKeyT));
            *pEvtChannelKeyTemp = *pEvtChannelKey;

            rc = clCntNodeAddAndNodeGet(pEvtChannelInfo->evtChannelContainer,
                    (ClCntKeyHandleT) pEvtChannelKeyTemp,
                    (ClCntDataHandleT) pChannelInfoDb, NULL,
                    &nodeHandle);
            if (CL_OK != rc)
            {
                clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED, rc);
                goto channelKeyAllocated;
            }

            break;

        case CL_EVT_ADD_USER:
            break;
    }

    /*
     ** Create the key for User of ECH container using the userId.
     */
    pUserOfECHKey = clHeapAllocate(sizeof(*pUserOfECHKey));
    if (NULL == pUserOfECHKey)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        goto channelKeyAdded;
    }
    *pUserOfECHKey = *pUserOfECH;   /* Struct to struct copy */

    /*
     * Add the user of the event channel information 
     */
    rc = clCntNodeAdd(pChannelInfoDb->evtChannelUserInfo,
            (ClCntKeyHandleT) pUserOfECHKey,
            (ClCntDataHandleT)(ClWordT)evtChannelUserType, NULL);
    if (CL_ERR_DUPLICATE == CL_GET_ERROR_CODE(rc))
    {
        /*
         * If there is a duplicate it might be due to recovery (Bug: 3179).
         * Treat separately.
         */
        rc = CL_EVENT_ERR_EXIST;
        goto userKeyAllocated;
    }
    else if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH", CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED, rc);
        goto userKeyAllocated;
    }

    if (0 != (CL_EVENT_CHANNEL_PUBLISHER & evtChannelUserType))
    {
        pChannelInfoDb->publisherRefCount++;
    }
    if (0 != (CL_EVENT_CHANNEL_SUBSCRIBER & evtChannelUserType))
    {
        pChannelInfoDb->subscriberRefCount++;
    }

// success:
    clLogTrace(CL_EVENT_LOG_AREA_SRV, "ECH", "Event Channel Creation/User Addition Successful");

    CL_FUNC_EXIT();
    return CL_OK;

userKeyAllocated:
    clHeapFree(pUserOfECHKey);

channelKeyAdded:
    // NTC might come out of the switch

channelKeyAllocated:
    clHeapFree(pEvtChannelKeyTemp);

failure:

    CL_FUNC_EXIT();
    return rc;
}

void clEvtChannelInfoGet(ClEventChannelHandleT evtChannelHandle, ClOsalMutexIdT *pMutexId,
        ClUint16T *pEvtChannelID, ClUint8T *pEvtChannelScope,
        ClUint8T *pEvtChannelUserType,
        ClEvtChannelInfoT **ppEvtChannelInfo)
{
    CL_FUNC_ENTER();


    /*
     * Get the scope and user type of the event channel 
     */
    CL_EVT_CHANNEL_SCOPE_GET(evtChannelHandle, (*pEvtChannelScope));
    CL_EVT_CHANNEL_USER_TYPE_GET(evtChannelHandle, (*pEvtChannelUserType));

    /*
     * Get channel ID 
     */
    CL_EVT_CHANNEL_ID_GET(evtChannelHandle, (*pEvtChannelID));
    /*
     * Find out corresponding event channel is available 
     */
    if (CL_EVENT_GLOBAL_CHANNEL == *pEvtChannelScope)
    {
        *ppEvtChannelInfo = gpEvtGlobalECHDb;
        *pMutexId = gEvtGlobalECHMutex;
    }
    else
    {
        *ppEvtChannelInfo = gpEvtLocalECHDb;
        *pMutexId = gEvtLocalECHMutex;
    }

    CL_FUNC_EXIT();
    return;
}


/*
 * The Wrapper around the call to do the client initialize.
 */
ClRcT VDECL(clEvtInitializeLocal)(ClEoDataT cData, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClEvtInitRequestT evtInitReq = {0};
    ClHandleT evtHandle = 0;

    CL_FUNC_ENTER();

    if(!gEvtInitDone)
    {
        clLogWarning("EVT", "INI", "Event server is not yet initialized");
        return CL_EVENTS_RC(CL_ERR_TRY_AGAIN);
    }

    rc = VDECL_VER(clXdrUnmarshallClEvtInitRequestT, 4, 0, 0)(inMsgHandle,
                                                              &evtInitReq);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "INI",
                   "Failed to Unmarshall Buffer [%#X]", rc);
        goto failure;
    }
    
    /*
     * Verify if the Version is Compatible 
     */
    clEvtVerifyClientToServerVersion(&evtInitReq, outMsgHandle);

    rc = clEvtHandleCreate(&evtHandle);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "INI",
                CL_LOG_MESSAGE_1_HANDLE_CREATION_FAILED, rc);
        clLogError(CL_EVENT_LOG_AREA_SRV, "INI",
                "Failed to Obtain the Initialization Handle [%#X]", rc);
        goto inDataAllocated;
    }

    evtInitReq.userId.evtHandle = evtHandle;  /* Update the Init Request with 
                                                 * the evtHandle */

    rc = clEvtInitializeViaRequest(&evtInitReq, CL_EVT_NORMAL_REQUEST);
    if (CL_EVENT_ERR_ALREADY_INITIALIZED == rc)
    {
        rc = CL_EVENT_ERR_ALREADY_INITIALIZED;
        goto inDataAllocated;
    }
    else if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "INI",
                "Initialization Failed for EO{port[0x%llx], srvHdl[%#llX], cltHdl[%#llX]}, rc[0x%x]",
                evtInitReq.userId.eoIocPort, evtInitReq.userId.evtHandle,
                evtInitReq.userId.clientHandle, rc);
        goto inDataAllocated;
    }

    /*
     * Write the evtHandle to outMessage for the client 
     */
    rc = clBufferNBytesWrite(outMsgHandle, (ClUint8T *) &evtHandle,
            sizeof(evtHandle));
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "INI",
                "Writing evtHandle to outMessage Failed");
        clLogError(CL_EVENT_LOG_AREA_SRV, "INI",
                CL_LOG_MESSAGE_1_BUFFER_MSG_WRITE_FAILED, rc);
        goto inDataAllocated;
    }

#ifdef CKPT_ENABLED
    /*
     * Check Point only if Initialize was successful 
     */

    rc = clEvtCkptCheckPointInitialize(&evtInitReq);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "INI", "Check Pointing Initialize Failed...");
        goto inDataAllocated;
    }
#endif

// success:
    clLogTrace(CL_EVENT_LOG_AREA_SRV, "INI",
            "Initialization successful for EO{port[0x%llx], srvHdl[%#llX], cltHdl[%#llX]}, rc[0x%x]",
            evtInitReq.userId.eoIocPort, evtInitReq.userId.evtHandle,
            evtInitReq.userId.clientHandle, rc);

    CL_FUNC_EXIT();
    return rc;

inDataAllocated:
    rc = CL_EVENTS_RC(rc);
    clLogError(CL_EVENT_LOG_AREA_SRV, "INI",
            "Initialization Failed for EO{port[0x%llx], srvHdl[%#llX], cltHdl[%#llX]}, rc[0x%x]",
            evtInitReq.userId.eoIocPort, evtInitReq.userId.evtHandle,
            evtInitReq.userId.clientHandle, rc);

failure:

    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEvtInitializeViaRequest(ClEvtInitRequestT *pEvtInitReq, ClUint32T type)
{
    ClRcT rc = CL_OK;

    ClEvtUserIdT *pInitKey = NULL;  /* Create the key using the userId for Init 
                                     * Container */


    CL_FUNC_ENTER();

#ifdef CKPT_ENABLED
    if (CL_EVT_CKPT_REQUEST == type)
    {
        rc = clHandleCreateSpecifiedHandle(gEvtHandleDatabaseHdl, sizeof(*pInitKey),
                pEvtInitReq->userId.evtHandle);
        if (CL_OK != rc && CL_ERR_ALREADY_EXIST != CL_GET_ERROR_CODE(rc))
        {
            /*
             * Ignore if Already exist since it is recovery Bug - 3179 
             */
            clLogWarning(CL_EVENT_LOG_AREA_SRV, "INI",
                    "clHandleCreateSpecifiedHandle Failed, rc[%#X]\n\r", rc);
        }
    }
#endif
    rc = clHandleCheckout(gEvtHandleDatabaseHdl, pEvtInitReq->userId.evtHandle, (void **)&pInitKey);
    if(CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "INI",
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        goto failure;
    }

    *pInitKey = pEvtInitReq->userId;    /* Struct to Struct copy */

    rc = clHandleCheckin(gEvtHandleDatabaseHdl, pEvtInitReq->userId.evtHandle);
    if(CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "INI",
                CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
        goto failure;
    } 

// success:

failure:
    CL_FUNC_EXIT();
    return rc;
}

/*
 * This function will open the ECH and return ECH handle 
 */
/*
 * 1. Find out what is the scope of the ECH. 2. Get the appropriate handle of
 * ECH. 3. Find out this ECH is already opened. 3.1 If it is already opened by
 * the same EO then return error. 3.2 or else add the user information into
 * user DB. 3.3 Get the proper ECH place holder and increment the referance
 * count. 4. If ECH is not opened 4.1 If the opening request is for ECH/G 4.1.1 
 * Send registration request to Master 4.1.2 If the registration request passed 
 * then allocate place holder for ECH 4.1.3 Add into appropriate containter.
 * 4.2 If the opening request is for ECH/L. 4.2.1 Allocate palce holder for
 * ECH. 4.2.2 Add into appropriate containter. 5. If it is for first time for
 * subscription then register with corresponding group.
 * 
 */
ClRcT VDECL(clEvtChannelOpenLocal)(ClEoDataT cData, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;

    ClEvtChannelOpenRequestT evtChannelOpenRequest = {0};

    CL_FUNC_ENTER();

    if(!gEvtInitDone)
    {
        clLogWarning("EVT", "OPEN", "Event server is not yet initialized");
        return CL_EVENTS_RC(CL_ERR_TRY_AGAIN);
    }

    rc = VDECL_VER(clXdrUnmarshallClEvtChannelOpenRequestT, 4, 0, 0)(inMsgHandle,
                                                                     &evtChannelOpenRequest);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "INI",
                   "Failed to Unmarshall Buffer [%#X]", rc);
        goto failure;
    }

    /*
     * Verify if the Version is Compatible 
     */
    clEvtVerifyClientToServerVersion(&evtChannelOpenRequest, outMsgHandle);

    /*
     * Make the Call to the ViaRequest Api 
     */
    rc = clEvtChannelOpenViaRequest(&evtChannelOpenRequest,
            CL_EVT_NORMAL_REQUEST);
    if (CL_EVENT_ERR_EXIST == rc)
    {
        /*
         * Must be due to recovery so issue a warning 
         */
        clLogWarning(CL_EVENT_LOG_AREA_SRV, "ECH",
                "Channel Already Opened, rc[%#X]\n\r", rc);
        goto inDataAllocated;
    }
    else if (CL_OK != rc)
    {
        goto inDataAllocated;
    }

#ifdef CKPT_ENABLED
    /*
     * Check Point only if Open was successful 
     */

    rc = clEvtCkptCheckPointChannelOpen(&evtChannelOpenRequest);
    if (CL_OK != rc)
    {
        goto inDataAllocated;
    }
#endif

// success:
    clLogTrace(CL_EVENT_LOG_AREA_SRV, "ECH",
            "Channel Open successful for EO{port[0x%llx], srvHdl[%#llX], cltHdl[%#llX]}"
            "on Channel {[name [%.*s] handle[%#llX]}, rc=[%#X]",
            evtChannelOpenRequest.userId.eoIocPort,
            evtChannelOpenRequest.userId.evtHandle,
            evtChannelOpenRequest.userId.clientHandle,
            evtChannelOpenRequest.evtChannelName.length,
            evtChannelOpenRequest.evtChannelName.value,
            evtChannelOpenRequest.evtChannelHandle, rc);

    CL_FUNC_EXIT();
    return rc;

inDataAllocated:
    rc = CL_EVENTS_RC(rc);
    clLogError(CL_EVENT_LOG_AREA_SRV, "ECH",
            "Channel Open failed for EO{port[0x%llx], srvHdl[%#llX], cltHdl[%#llX]}"
            "on Channel {[name [%.*s] handle[%#llX]}, rc=[%#X]",
            evtChannelOpenRequest.userId.eoIocPort,
            evtChannelOpenRequest.userId.evtHandle,
            evtChannelOpenRequest.userId.clientHandle,
            evtChannelOpenRequest.evtChannelName.length,
            evtChannelOpenRequest.evtChannelName.value,
            evtChannelOpenRequest.evtChannelHandle, rc);

failure:
    CL_FUNC_EXIT();
    return rc;
}


ClRcT clEvtChannelOpenViaRequest(ClEvtChannelOpenRequestT *pEvtChanOpenReq,
        ClUint32T type)
{
    ClRcT rc = CL_OK;

    ClUint8T channelScope = 0;
    ClUint8T evtChannelUserType;
    ClUint16T channelId;
    ClEvtChannelInfoT *pEvtChannelInfo = NULL;
    ClEvtChannelDBT *pEvtChannelDB = NULL;
    ClEvtChannelKeyT evtChannelKey;
    ClOsalMutexIdT mutexId;


    CL_FUNC_ENTER();

    /*
     * Get the channel associated information 
     */
    clEvtChannelInfoGet(pEvtChanOpenReq->evtChannelHandle, &mutexId, &channelId,
            &channelScope, &evtChannelUserType, &pEvtChannelInfo);

    clEvtUtilsChannelKeyConstruct(pEvtChanOpenReq->evtChannelHandle,
            &pEvtChanOpenReq->evtChannelName,
            &evtChannelKey);

    clOsalMutexLock(mutexId);

    rc = clCntDataForKeyGet(pEvtChannelInfo->evtChannelContainer,
            (ClCntKeyHandleT) &evtChannelKey,
            (ClCntDataHandleT *) &pEvtChannelDB);
    if (CL_OK == rc)
    {
        /*
         * If the ECH already opened by some other application then, just add
         * the opening user information 
         */
        rc = clEvtNewChannelAddAndOrUserAdd(pEvtChannelInfo,
                &pEvtChanOpenReq->userId,
                evtChannelUserType, &evtChannelKey,
                pEvtChannelDB, channelScope,
                CL_EVT_ADD_USER);
        if (CL_EVT_CKPT_REQUEST == type && CL_EVENT_ERR_EXIST == rc)
        {
            /*
             * If due to recovery (Bug:3179) ignore 
             */
            rc = CL_OK;
        }
        else if (CL_OK != rc)
        {
            clLogWarning(CL_EVENT_LOG_AREA_SRV, "ECH",
                    "clEvtNewChannelAddAndOrUserAdd Failed, rc[%#X]", rc); // NTC
            rc = CL_OK;
            goto success;
        }
    }
    else if (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
    {
        /*
         * Entry is not found in the ECH, then send request to ECH/PM and then
         * add it to the data base 
         */

        pEvtChannelDB =
            (ClEvtChannelDBT *) clHeapAllocate(sizeof(ClEvtChannelDBT));
        if (NULL == pEvtChannelDB)
        {
            clLogError(CL_EVENT_LOG_AREA_SRV, "ECH",
                    "Allocation of memory for channel database failed");
            clLogError(CL_EVENT_LOG_AREA_SRV, "ECH",
                    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            rc = CL_EVENT_ERR_NO_MEM;
            goto mutexLocked;
        }

        /*
         * Create ECH DB 
         */
        rc = clEvtChannelDBCreate(pEvtChannelDB);
        if (CL_OK != rc)
        {
            clLogError(CL_EVENT_LOG_AREA_SRV, "ECH",
                    "Channel database creation failed");
            goto channelDbAllocated;
        }

        /*
         * Add the new channel and add the user of the channel into this 
         */
        rc = clEvtNewChannelAddAndOrUserAdd(pEvtChannelInfo,
                &pEvtChanOpenReq->userId,
                evtChannelUserType, &evtChannelKey,
                pEvtChannelDB, channelScope,
                CL_EVT_ADD_USER_AND_CHANNEL);
        if (CL_EVT_CKPT_REQUEST == type && CL_EVENT_ERR_EXIST == rc)
        {
            /*
             * If due to recovery (Bug:3179) ignore 
             */
            rc = CL_OK;
        }
        else if (CL_OK != rc)
        {
            goto channelDbCreated;
        }

#ifdef CKPT_ENABLED
        /*
         * Added for CKPT support - create the check point for this channel
         * only if opened for subscription as no information is maintained
         * for publisher. What if both wat's the type? Delete selective
         * if(CL_EVENT_CHANNEL_SUBSCRIBER == evtChannelUserType) 
         */
        rc = clEvtCkptSubsDSCreate(&pEvtChanOpenReq->evtChannelName,
                channelScope);
        if (CL_OK != rc)
        {
            goto channelDbCreated;
        }
#endif
        /*
         * Adding new ECH is succesfully done 
         */
    }
    else
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH",
                "Fetching channel database info failed");
        clLogError(CL_EVENT_LOG_AREA_SRV, "ECH",
                CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);
        goto mutexLocked;
    }

    /*
     * All done successfully. Return from here after releasing the mutex. 
     */
success:
    clOsalMutexUnlock(mutexId);

    clLogTrace(CL_EVENT_LOG_AREA_SRV, "ECH",
            "[%s] Event Channel[%.*s] Open succeeded, rc[%#X]",
            (channelScope == CL_EVENT_GLOBAL_CHANNEL ? "Global" : "Local"),
            pEvtChanOpenReq->evtChannelName.length,
            pEvtChanOpenReq->evtChannelName.value,
            rc);
    CL_FUNC_EXIT();
    return rc;

channelDbCreated:
    clEvtChannelDBDelete(pEvtChannelDB);

channelDbAllocated:
    clHeapFree(pEvtChannelDB);

mutexLocked:
    clOsalMutexUnlock(mutexId);

// failure:
    rc = CL_EVENTS_RC(rc);
    clLogError(CL_EVENT_LOG_AREA_SRV, "ECH",
            "[%s] Event Channel[%.*s] Open failed, rc[%#X]",
            (channelScope == CL_EVENT_GLOBAL_CHANNEL ? "Global" : "Local"),
            pEvtChanOpenReq->evtChannelName.length,
            pEvtChanOpenReq->evtChannelName.value,
            rc);

    CL_FUNC_EXIT();
    return rc;
}

/*****************************************************************************/

/*
 ** The following walk was introduced to fix bug 3177. It checks if the
 ** EO has used the same subscription id on the given channel.
 */

ClRcT clEvtSubsIdCheckWalk(ClCntKeyHandleT userKey, ClCntDataHandleT userData,
        ClCntArgHandleT userArg, ClUint32T dataLength)
{
    ClRcT rc = CL_OK;

    ClEvtSubscribeEventRequestT *pEvtSubsReq =
        (ClEvtSubscribeEventRequestT *) userArg;
    ClEvtEventTypeInfoT *pEvtTypeInfo = (ClEvtEventTypeInfoT *) userData;

    ClCntNodeHandleT nodeHandle;
    ClEvtSubsKeyT subsKey = {0};

    CL_FUNC_ENTER();

    subsKey.userId = pEvtSubsReq->userId;
    subsKey.subscriptionId = pEvtSubsReq->subscriptionId;
    subsKey.pCookie = pEvtSubsReq->pCookie; /* This is unecessary as it's not
                                             * checked */

    rc = clCntNodeFind(pEvtTypeInfo->subscriberInfo, (ClCntKeyHandleT) &subsKey,
            (ClCntNodeHandleT *) &nodeHandle);
    if (CL_OK == rc)
    {
        /*
         ** If found it implies the ID is duplicate so throw error.
         */
        rc = CL_EVENT_ERR_EXIST;
        goto failure;
    }
    else if (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc)) 
    {
        /*
         * This is good. There is nobody subscribed with the specified ID.
         */
        rc = CL_OK;
    }
    else if(CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "Search for the Duplicate SubsID failed [%#X]", rc);
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);
        goto failure;
    }
    /*
     ** If the Subscription is not found (CL_ERR_NOT_EXIST) return OK and
     ** continue to search in the next Event Type.
     */

// success:
    CL_FUNC_EXIT();
    return rc;

failure:
    CL_FUNC_EXIT();
    return rc;
}

/*
 * 1. Get the corresponding ECH info. 2. Get the Event type. 3. Find out is
 * some else is alredy done subsctiption on that Event Type. 3.1 If it is not
 * already subscribed then add the Event Type information into containter. 4.
 * Unpack the filter information if any. 5. Fill in subscriber information and
 * add into corresponding containter list. 6. If there is any retention event
 * for that Event Type then given that info to subscriber. 
 */
ClRcT VDECL(clEvtEventSubscribeLocal)(ClEoDataT cData,
        ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClEvtSubscribeEventRequestT evtSubsReq = {0};

    CL_FUNC_ENTER();

    if(!gEvtInitDone)
    {
        clLogWarning("EVT", "SUBS", "Event server is not yet initialized");
        return CL_EVENTS_RC(CL_ERR_TRY_AGAIN);
    }
    
    rc = VDECL_VER(clXdrUnmarshallClEvtSubscribeEventRequestT, 4, 0, 0)(inMsgHandle,
                                                                        &evtSubsReq);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                   "Failed to Unmarshall Buffer [%#X]", rc);
        goto failure;
    }

    /*
     * Verify if the Version is Compatible 
     */
    clEvtVerifyClientToServerVersion(&evtSubsReq, outMsgHandle);

    rc = clEvtEventSubscribeViaRequest(&evtSubsReq, CL_EVT_NORMAL_REQUEST);
    if (CL_EVENT_ERR_EXIST == rc)
    {
        /*
         ** Fix for Bug 3177 -
         ** The Subscription ID specified has already been used so throw Error.
         */
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "The EO is already subscribed using this ID, rc[%#X]", rc);
        goto inDataAllocated;
    }
    else if (CL_OK != rc)
    {
        goto inDataAllocated;
    }

#ifdef RETENTION_ENABLE
    /*
     * Notify about subscription if all went well. Get the length of packedRbe
     * from total length and invoke the Subscription broadcast/unicast - 
     */
    len -= sizeof(ClEvtSubscribeEventRequestT);

    clEvtRetentionQuerySend(&evtSubsReq, len);
#endif

// success:
    clLogTrace(CL_EVENT_LOG_AREA_SRV, "SUB",
            "Subscribe successful for EO {port[0x%llx], evtHandle[%#llX], clientHdl[%#llX]} "
            "on Channel{name [%.*s] handle[%#llX]}, subscription ID [%#X], rc=[%#X]",
            evtSubsReq.userId.eoIocPort,
            evtSubsReq.userId.evtHandle,
            evtSubsReq.userId.clientHandle,
            evtSubsReq.evtChannelName.length,
            evtSubsReq.evtChannelName.value,
            evtSubsReq.evtChannelHandle, 
            evtSubsReq.subscriptionId, rc);

    clHeapFree(evtSubsReq.packedRbe);
    
    CL_FUNC_EXIT();
    return rc;

inDataAllocated:
    rc = CL_EVENTS_RC(rc);
    clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
            "Subscribe failed for EO {port[0x%llx], evtHandle[%#llX], clientHdl[%#llX]} "
            "on Channel{name [%.*s] handle[%#llX]}, subscription ID [%#X], rc=[%#X]",
            evtSubsReq.userId.eoIocPort,
            evtSubsReq.userId.evtHandle,
            evtSubsReq.userId.clientHandle,
            evtSubsReq.evtChannelName.length,
            evtSubsReq.evtChannelName.value,
            evtSubsReq.evtChannelHandle, 
            evtSubsReq.subscriptionId, rc);

    clHeapFree(evtSubsReq.packedRbe);

failure:
    CL_FUNC_EXIT();
    return rc;
}


ClRcT clEvtEventSubscribeViaRequest(ClEvtSubscribeEventRequestT *pEvtSubsReq,
        ClUint32T type)
{
    ClRcT rc = CL_OK;

    ClOsalMutexIdT mutexId = 0;
    ClOsalMutexIdT channelLevelMutex = 0;
    ClUint8T channelScope = 0;
    ClUint8T evtChannelUserType = 0;
    ClUint16T channelId = 0;
    ClEvtChannelInfoT *pEvtChannelInfo = NULL;
    ClEvtChannelDBT *pEvtChannelDB = NULL;
    ClEvtEventTypeInfoT *pEvtTypeInfo = NULL;
    ClEvtChannelKeyT evtChannelKey = { 0 };
    ClEvtSubsKeyT *pSubsKey = NULL;
    ClRuleExprT *pEventType = NULL; /* Event handler function ID */
    ClEvtEventTypeKeyT *pEvtEventTypeKey = NULL;


    CL_FUNC_ENTER();


    /*
     * Get the channel associated information 
     */
    clEvtChannelInfoGet(pEvtSubsReq->evtChannelHandle, &mutexId, &channelId,
            &channelScope, &evtChannelUserType, &pEvtChannelInfo);

    clEvtUtilsChannelKeyConstruct(pEvtSubsReq->evtChannelHandle,
            &pEvtSubsReq->evtChannelName, &evtChannelKey);

    /*
     * Get ECH related information 
     */
    clOsalMutexLock(mutexId);
    rc = clCntDataForKeyGet(pEvtChannelInfo->evtChannelContainer,
            (ClCntKeyHandleT) &evtChannelKey,
            (ClCntDataHandleT *) &pEvtChannelDB);
    if (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "Channel not opened, [%#X]", rc);
        rc = CL_EVENT_ERR_CHANNEL_NOT_OPENED;
        goto mutexLocked;
    }
    else if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "Could not get ECH info [%#X]", rc);
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);
        goto mutexLocked;
    }

    channelLevelMutex = pEvtChannelDB->channelLevelMutex;

    /*
     * Check if the Subscription ID has already been used (Bug 3177) 
     */

    if (CL_EVT_NORMAL_REQUEST == type)
    {
        /*
         * Checking only if not recovery (Bug: 3179) 
         */
        clOsalMutexLock(channelLevelMutex);
        rc = clCntWalk(pEvtChannelDB->eventTypeInfo, clEvtSubsIdCheckWalk,
                (ClCntArgHandleT) pEvtSubsReq, 0);
        clOsalMutexUnlock(channelLevelMutex);
        if (CL_OK != rc)
        {
            goto mutexLocked;
        }
    }

    /*
     * Find out event type information 
     */

    if (pEvtSubsReq->packedRbeLen)
    {
        rc = clRuleExprUnpack(pEvtSubsReq->packedRbe,
                              pEvtSubsReq->packedRbeLen,
                              &pEventType);
        if (rc != CL_OK) goto mutexLocked;
    }

    pEvtEventTypeKey = clHeapAllocate(sizeof(ClEvtEventTypeKeyT));
    if (NULL == pEvtEventTypeKey)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB", "Event Type allocation failed \n\r");
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        clOsalMutexLock(channelLevelMutex);
        goto rbeUnpacked;
    }

    pEvtEventTypeKey->flag = CL_EVT_TYPE_RBE_KEY_TYPE;
    pEvtEventTypeKey->key.pRbeExpr = pEventType;

    clOsalMutexLock(channelLevelMutex);
    rc = clCntDataForKeyGet(pEvtChannelDB->eventTypeInfo,
            (ClCntKeyHandleT) pEvtEventTypeKey,
            (ClCntDataHandleT *) &pEvtTypeInfo);

    if (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
    {
        /*
         * If it is not subscribed by any one else. Then add allocate and add
         * the information into appopriate container 
         */
        pEvtTypeInfo =
            (ClEvtEventTypeInfoT *) clHeapAllocate(sizeof(ClEvtEventTypeInfoT));
        if (NULL == pEvtTypeInfo)
        {
            clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                    "Failed to allocate memory for event type info");
            clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                    CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
            rc = CL_EVENT_ERR_NO_MEM;
            goto eventTypeKeyAllocated;
        }
        pEvtTypeInfo->refCount = 0;

        /*
         * Create place holder for subscriber information 
         */
        rc = clCntLlistCreate(clEvtSubscribeKeyCompare,
                clEvtSubscriberUserDelete, clEvtSubscriberUserDelete,
                CL_CNT_UNIQUE_KEY, &pEvtTypeInfo->subscriberInfo);
        if (CL_OK != rc)
        {
            clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                    "Failed to create continer for subscription info, rc[%#X]", rc);
            clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                    CL_LOG_MESSAGE_1_CNT_CREATE_FAILED, rc);
            goto eventTypeInfoAllocated;
        }

        rc = clCntNodeAdd(pEvtChannelDB->eventTypeInfo,
                (ClCntKeyHandleT) pEvtEventTypeKey,
                (ClCntDataHandleT) pEvtTypeInfo, NULL);
        if (CL_OK != rc)
        {
            clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                    "Failed to add event info into containter");
            clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                    CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED, rc);
            goto eventTypeInfoCntCreated;
        }
    }
    else if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "Failed to lookup event info in the containter");
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);
        goto eventTypeKeyAllocated;
    }
    else
    {
        /*
         * The Info already exist so free the allocated resources 
         */
        if (pEventType)
            clRuleExprDeallocate(pEventType);
        clHeapFree(pEvtEventTypeKey);
    }

    /*
     * Alocate memory and fill in the inforamtion and add subscriber info into
     * containter 
     */

    pSubsKey = clHeapAllocate(sizeof(ClEvtSubsKeyT));
    if (NULL == pSubsKey)
    {
        rc = CL_EVENT_ERR_NO_MEM;
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "Failed to memory for Subscription Key");
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        goto chanelLevelMutexLocked;
    }

    /*
     * Construct subscriber key 
     */
    pSubsKey->userId = pEvtSubsReq->userId;
    pSubsKey->subscriptionId = pEvtSubsReq->subscriptionId;
    pSubsKey->pCookie = pEvtSubsReq->pCookie;

    rc = clCntNodeAdd(pEvtTypeInfo->subscriberInfo, (ClCntKeyHandleT) pSubsKey,
            (ClCntDataHandleT)(ClWordT) pEvtSubsReq->subscriberCommPort, NULL);
    if (CL_OK != rc && CL_ERR_DUPLICATE != CL_GET_ERROR_CODE(rc))
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "Failed to add event type into container");
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                CL_LOG_MESSAGE_1_CNT_DATA_ADD_FAILED, rc);

        clHeapFree(pSubsKey);
        goto chanelLevelMutexLocked;
    }
    else if (CL_ERR_DUPLICATE == CL_GET_ERROR_CODE(rc))
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "Duplicate subscription");
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                CL_LOG_MESSAGE_0_DUPLICATE_ENTRY);
        rc = CL_EVENT_ERR_EXIST;

        clHeapFree(pSubsKey);
        goto chanelLevelMutexLocked;
    }

    pEvtTypeInfo->refCount++;
    clOsalMutexUnlock(channelLevelMutex);
    clOsalMutexUnlock(mutexId);

#ifdef CKPT_ENABLED
    /*
     ** Check Point only if Subscription was successful.
     ** Firstly check if the call was a normal call or
     ** a call from the re-construction Api.
     */
    if (CL_EVT_NORMAL_REQUEST == type)  /* Needn't Check Point if from
                                         * Reconstruct */
    {
        rc = clEvtCkptCheckPointSubscribe(pEvtSubsReq, pEvtChannelDB);
        if (CL_OK != rc)
        {
            clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                    "Check pointing subscribe failed");
            clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                    CL_EVENT_LOG_MSG_7_CKPT_SUBS_FAIL,
                    pEvtSubsReq->subscriptionId,
                    pEvtSubsReq->evtChannelName.length,
                    pEvtSubsReq->evtChannelName.value,
                    pEvtSubsReq->userId.eoIocPort,
                    pEvtSubsReq->userId.evtHandle, rc);
            goto failure;
        }
    }
#endif

// success:
    clLogTrace(CL_EVENT_LOG_AREA_SRV, "SUB",
            "Subscribe successful for EO {port[0x%llx], evtHandle[%#llX], clientHdl[%#llX]} "
            "on Channel{name [%.*s] handle[%#llX]}, subscription ID [%#X], rc=[%#X]",
            pEvtSubsReq->userId.eoIocPort,
            pEvtSubsReq->userId.evtHandle,
            pEvtSubsReq->userId.clientHandle,
            pEvtSubsReq->evtChannelName.length,
            pEvtSubsReq->evtChannelName.value,
            pEvtSubsReq->evtChannelHandle, 
            pEvtSubsReq->subscriptionId, rc);

    CL_FUNC_EXIT();
    return rc;

eventTypeInfoCntCreated:
    clCntDelete(pEvtChannelDB->eventTypeInfo);

eventTypeInfoAllocated:
    clHeapFree(pEvtTypeInfo);

eventTypeKeyAllocated:
    clHeapFree(pEvtEventTypeKey);

rbeUnpacked:
    if (pEventType)
        clRuleExprDeallocate(pEventType);

chanelLevelMutexLocked:
    clOsalMutexUnlock(channelLevelMutex);

mutexLocked:
    clOsalMutexUnlock(mutexId);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
            "Subscribe failed for EO {port[0x%llx], evtHandle[%#llX], clientHdl[%#llX]} "
            "on Channel{name [%.*s] handle[%#llX]}, subscription ID [%#X], rc=[%#X]",
            pEvtSubsReq->userId.eoIocPort,
            pEvtSubsReq->userId.evtHandle,
            pEvtSubsReq->userId.clientHandle,
            pEvtSubsReq->evtChannelName.length,
            pEvtSubsReq->evtChannelName.value,
            pEvtSubsReq->evtChannelHandle, 
            pEvtSubsReq->subscriptionId, rc);

    CL_FUNC_EXIT();
    return rc;
}


ClRcT clEvtSubscribeWalkForPublish(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData,
        ClCntArgHandleT userArg,
        ClUint32T dataLength)
{
    ClRcT rc = CL_OK;

    ClEvtEventPrimaryHeaderT *pEvtPrimaryHeader =
        (ClEvtEventPrimaryHeaderT *) userArg;
    ClEvtEventSecondaryHeaderT evtSecHeader = {0};
    ClEvtEventSecondaryHeaderT *pEvtSecHeader = NULL;

    ClEvtSubsKeyT *pEvtSubsKey = (ClEvtSubsKeyT *) userKey;
    ClIocNodeAddressT iocLocalAddress = 0;
    ClRmdOptionsT rmdOptions = { 0 };
    ClIocAddressT remoteObjAddr = {{0}};
    ClIocPortT subsCommPort = (ClIocPortT)(ClWordT) userData;
    ClBufferHandleT inMsgHandle = 0;


    CL_FUNC_ENTER();

    if(!pEvtSubsKey || !subsCommPort)
    {
        clLogWarning("SUBS", "PUBLISH", "Skipping subscriber walk as the subscriber entry "
                     "in the event database is 0");
        return CL_OK;
    }

    pEvtSecHeader =
        (ClEvtEventSecondaryHeaderT *) ((char *) (pEvtPrimaryHeader + 1) +
                pEvtPrimaryHeader->channelNameLen +
                pEvtPrimaryHeader->publisherNameLen +
                pEvtPrimaryHeader->patternSectionLen +
                pEvtPrimaryHeader->eventDataSize);

#ifdef SOLARIS_BUILD
    bcopy(pEvtSecHeader,&evtSecHeader, sizeof(evtSecHeader));
#else
    memcpy(&evtSecHeader, pEvtSecHeader, sizeof(evtSecHeader));
#endif

#ifdef RETENTION_ENABLE
    if ((pEvtPrimaryHeader->flag & CL_EVT_RETAINED_EVENT) &&
            evtSecHeader.evtHandle == pEvtSubsKey->userId.evtHandle &&
            evtSecHeader.subscriptionId == pEvtSubsKey->subscriptionId)
    {

        /*
         * The Event is not published if it is a Retained Event and the current 
         * Subscriber isn't the one who requested for this event 
         */
        goto success; // Need to continue for other event types - Bug 6934
    }
#endif

    iocLocalAddress = clIocLocalAddressGet();
    evtSecHeader.subscriptionId = pEvtSubsKey->subscriptionId;
    evtSecHeader.pCookie = pEvtSubsKey->pCookie;

    /*
     ** Need to mention the evtHandle to identify the Callback at client end.
     ** Change introduced to support multiple initializations.
     */
    evtSecHeader.evtHandle = pEvtSubsKey->userId.evtHandle;
#ifdef SOLARIS_BUILD
    bcopy(&evtSecHeader, pEvtSecHeader, sizeof(evtSecHeader));
#else
    memcpy(pEvtSecHeader, &evtSecHeader, sizeof(evtSecHeader));
#endif

    rc = clBufferCreate(&inMsgHandle);
    rc = clBufferNBytesWrite(inMsgHandle, (ClUint8T *) pEvtPrimaryHeader,
            dataLength);
    /*
     * Invoke RMD call provided by EM/S for opening channel, with packed user
     * data. 
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
    rmdOptions.timeout = CL_EVT_RMD_TIME_OUT;


    remoteObjAddr.iocPhyAddress.nodeAddress = iocLocalAddress;
    remoteObjAddr.iocPhyAddress.portId = subsCommPort;
    rc = clRmdWithMsg(remoteObjAddr, CL_EVT_EVENT_DELIVERY_FN_ID, inMsgHandle,
            0, CL_RMD_CALL_ASYNC | CL_RMD_CALL_ORDERED, &rmdOptions, NULL);
    (void) clBufferDelete(&inMsgHandle);
    if (CL_OK != rc)
    {
        /*
         * Even if rmd fails for single subscriber(most probably
         * NOT_REACHABLE), server should continue to publish to other
         * subscribers. Merely issuing a warning...
         */
        clLogWarning(CL_EVENT_LOG_AREA_SRV, "PUB", 
                "Event delivery failed to"
                "subscriber{node[%#x]:port[%#x]:evtHandle[%#llX]}"
                "error code [%#x]. continuing...", 
                iocLocalAddress, subsCommPort, evtSecHeader.evtHandle, rc);
        rc = CL_OK;
    }
    else
    {
        clLogTrace(CL_EVENT_LOG_AREA_SRV, "PUB", "Event delivered to subscriber{node[%#x]:port[%#x]:evtHandle[%#llX]}", iocLocalAddress, subsCommPort, evtSecHeader.evtHandle);
    }
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEvtEventTypeWalkForPublish(ClCntKeyHandleT userKey,
        ClCntDataHandleT userData,
        ClCntArgHandleT userArg,
        ClUint32T dataLength)
{
    ClRcT rc = CL_OK;

    ClEvtPublishInfoT *pPublishInfo = (ClEvtPublishInfoT *) userArg;
    ClEvtEventTypeKeyT *pPublishKey = pPublishInfo->pEventTypeKey;
    ClEvtEventPrimaryHeaderT *pEvtPrimaryHeader =
        pPublishInfo->pEvtPrimaryHeader;

    ClEvtEventTypeInfoT *pEvtTypeInfo = (ClEvtEventTypeInfoT *) userData;
    ClEvtEventTypeKeyT *pEvtEventTypeKey = (ClEvtEventTypeKeyT *) userKey;

    CL_FUNC_ENTER();

    /*
     * If not the same Event Type simply ignore and continue to 
     * match the other event types. This should be done even in 
     * case of errros issuing a warning - Bug 6934.
     */
    if (0 !=
            clEvtTypeKeyCompare((ClCntKeyHandleT) pEvtEventTypeKey,
                (ClCntKeyHandleT) pPublishKey))
    {
        goto success;
    }

#ifdef RETENTION_ENABLE
    if (pEvtPrimaryHeader->flag & CL_EVT_RETAINED_EVENT)
    {
        /*
         * If we need to publish the retained event we need to find
         * the subscriber who made the retention query based on the
         * userId and subscriptionId.
         */

        rc = clCntDataForKeyGet(pEvtTypeInfo->subscriberInfo,
                (ClCntKeyHandleT) &KEY,
                ClCntDataHandleT *pUserData) rc =
            clCntNodeFind(pEvtTypeInfo->subscriberInfo,
                    (ClCntKeyHandleT) &subsKey,
                    (ClCntNodeHandleT *) &nodeHandle);
        rc = clCntNodeUserKeyGet(containerHandle, nodeHandle, &userKey); 
        rc = clEvtPublishRetainedEvent(pEvtPrimaryHeader, pEvtTypeInfo);

        goto success;
    }
#endif

    /*
     * If the Event type matches walk throught subscriber list and Publish.
     * Note that the dataLength is the length of the Event Header(Secondary
     * Header inclusive) 
     */
    rc = clCntWalk(pEvtTypeInfo->subscriberInfo, clEvtSubscribeWalkForPublish,
            (ClCntArgHandleT) pEvtPrimaryHeader, dataLength);
    if (CL_OK != rc)
    {
        /*
         * Even if the above walk fails for a certain event type, the server
         * should continue to the other event types and publish to subscribers
         * on that list. This call could only fail due to some error in the
         * walk itself. Merely issuing a warning here... Bug 6934
         */
        clLogWarning(CL_EVENT_LOG_AREA_SRV, "PUB", 
                CL_LOG_MESSAGE_1_CNT_WALK_FAILED, rc);
        rc = CL_OK;
        goto failure;
    }

success:
failure:
    CL_FUNC_EXIT();
    return rc;
}

/*
 * 1. Find the ECH info. 2. Get the Event Type info. 3. Go through the Mask
 * information. 3.1 If the mask information matched with published event then
 * discard the publish. 4. Get subscription Information and publish to
 * subscriber. 4.1 If there is filter set by the given subscriber. 4.2 Evaluate 
 * the filter information. 4.3 If it is matched then notify or else don't
 * notify to that subscriber. 
 */
ClRcT VDECL(clEvtEventPublishLocal)(ClEoDataT cData,
        ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClOsalMutexIdT mutexId = 0;
    ClUint8T channelScope = 0;
    ClEvtChannelInfoT *pEvtChannelInfo = NULL;
    ClEvtChannelDBT *pEvtChannelDB = NULL;
    ClEvtChannelKeyT evtChannelKey = {0};
    ClEvtEventPrimaryHeaderT *pEvtPrimaryHeader = NULL;
    void *pPatternSection = NULL;

    ClRmdOptionsT rmdOptions = {0};
    ClIocAddressT remoteObjAddr = {{0}};
    ClUint32T inLen = 0;
    ClUint8T *pInData = NULL;

    ClSizeT *pPatternSize = NULL;
    ClSizeT patternSize = 0;
    ClSizeT tmpPatternSize = 0;
    ClUint32T noOfPatterns = 0;

#ifdef RETENTION_ENABLE
    ClEvtRetentionKeyT *pRetentionKey = NULL;
#endif

    CL_FUNC_ENTER();

    if(!gEvtInitDone)
    {
        clLogWarning("EVT", "PUB-LOCAL", "Event server is not yet initialized");
        return CL_EVENTS_RC(CL_ERR_TRY_AGAIN);
    }

    rc = clBufferLengthGet(inMsgHandle, &inLen);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                "Failed to flatten buffer [%#X]", rc);
        goto failure;
    }

    rc = clBufferFlatten(inMsgHandle, &pInData);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                "Failed to Flatten Buffer [%#X]", rc);
        goto failure;
    }

    pEvtPrimaryHeader = (ClEvtEventPrimaryHeaderT *) pInData;


    /*
     * Verify if the Version is Compatible 
     */
    clEvtVerifyClientToServerVersion(pEvtPrimaryHeader, outMsgHandle);

    /*
     * Replace with Server-Server Version 
     */
    clEvtServerToServerVersionSet(pEvtPrimaryHeader);

    channelScope = pEvtPrimaryHeader->channelScope;

    /*
     * Fill in channel Key 
     */
    evtChannelKey.channelId = pEvtPrimaryHeader->channelId;
    evtChannelKey.channelName.length = pEvtPrimaryHeader->channelNameLen;
    strncpy(evtChannelKey.channelName.value,
            (char *) (pInData + sizeof(ClEvtEventPrimaryHeaderT)),
            pEvtPrimaryHeader->channelNameLen);

    /*
     * Find out corresponding event channel is available 
     */
    if (CL_EVENT_GLOBAL_CHANNEL == channelScope)
    {
        pEvtChannelInfo = gpEvtGlobalECHDb;
        mutexId = gEvtGlobalECHMutex;
    }
    else
    {
        pEvtChannelInfo = gpEvtLocalECHDb;
        mutexId = gEvtLocalECHMutex;
    }


    /*
     * Get ECH related information 
     */
    clOsalMutexLock(mutexId);
    rc = clCntDataForKeyGet(pEvtChannelInfo->evtChannelContainer,
            (ClCntKeyHandleT) &evtChannelKey,
            (ClCntDataHandleT *) &pEvtChannelDB);
    if (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                "ChannelScope [%d]\n\r", channelScope); // NTC
        rc = CL_EVENT_ERR_CHANNEL_NOT_OPENED;
        goto mutexLocked;
    }
    else if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                "Could not get ECH info [%#X]", rc); // NTC
        clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                   CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);
        goto mutexLocked;
    }
    clOsalMutexUnlock(mutexId);

    pPatternSection =
        (pInData +
         (sizeof(ClEvtEventPrimaryHeaderT) + pEvtPrimaryHeader->channelNameLen +
          pEvtPrimaryHeader->publisherNameLen));


#ifdef RETENTION_ENABLE
    /*
     * Get the EventID to identify the event. This is unique only if
     * Retention is needed else it's merely the Node Address left 
     * shifted by 32 bits ANDed with 0xFFFFFFFF.
     */
    CL_EVT_EVENT_ID_GENERATE(pEvtPrimaryHeader->eventId);

    if (0 != pEvtPrimaryHeader->retentionTime)
    {
        rc = clEvtRetentionKeyGet(pEvtPrimaryHeader, &pRetentionKey);
        if (CL_OK != rc)
        {

        }
    }
#endif

#ifndef IDL_READY
    /*
     ** Following code was added to provide network byte ordering on Little
     ** Endian Machines.
     */
    if (CL_TRUE == clEvtUtilsIsLittleEndian())
    {
        /*
         ** Provide Byte Ordering for PatternSize in the Pattern Section.
         */
        pPatternSize = pPatternSection;
        noOfPatterns = pEvtPrimaryHeader->noOfPatterns;

        while (noOfPatterns--)
        {
            memcpy(&patternSize, pPatternSize, sizeof (*pPatternSize));

            /* Preserve since this value is lost - place in temporary location */
            tmpPatternSize = patternSize; 

            patternSize = clEvtHostUint64toNetUint64(patternSize);

            memcpy(pPatternSize, &patternSize, sizeof (*pPatternSize) );

            pPatternSize =
                (ClSizeT *) ((ClUint8T *) (pPatternSize + 1) + tmpPatternSize);
        }

        /*
         ** The following code was added to enable proper display in ethereal.
         ** This needs to change accordingly for mixed endian support.
         ** The changes need to be applied to 64 bit structures as well.
         */

        pEvtPrimaryHeader->channelId = htons(pEvtPrimaryHeader->channelId);
        pEvtPrimaryHeader->channelNameLen =
            htons(pEvtPrimaryHeader->channelNameLen);

        pEvtPrimaryHeader->patternSectionLen =
            htonl(pEvtPrimaryHeader->patternSectionLen);
        pEvtPrimaryHeader->noOfPatterns =
            htonl(pEvtPrimaryHeader->noOfPatterns);

        /*
         ** Converting the 64 Bit fields using internal API.
         ** This is prone to change once IDL is ready.
         */

        pEvtPrimaryHeader->retentionTime =
            clEvtHostUint64toNetUint64(pEvtPrimaryHeader->retentionTime);
        pEvtPrimaryHeader->publishTime =
            clEvtHostUint64toNetUint64(pEvtPrimaryHeader->publishTime);

        pEvtPrimaryHeader->eventId =
            clEvtHostUint64toNetUint64(pEvtPrimaryHeader->eventId);
        pEvtPrimaryHeader->eventDataSize =
            clEvtHostUint64toNetUint64(pEvtPrimaryHeader->eventDataSize);

        pEvtPrimaryHeader->publisherNameLen =
            htons(pEvtPrimaryHeader->publisherNameLen);


        /*
         ** Overwrite the header section of inMsgHandle with this altered placeholder.
         */
        rc = clBufferWriteOffsetSet(inMsgHandle, 0, CL_BUFFER_SEEK_SET);
        if (CL_OK != rc)
        {
            clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                    "Set Buffer Message Write Offset Failed");
            clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                    CL_EVENT_LOG_MSG_1_INTERNAL_ERROR, rc);
            goto inDataAllocated;
        }

        rc = clBufferNBytesWrite(inMsgHandle,
                (ClUint8T *) pEvtPrimaryHeader, inLen);
        /*
         * sizeof(*pEvtPrimaryHeader)); 
         */
        if (CL_OK != rc)
        {
            clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                    "Buffer Message Write Failed");
            clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                    CL_LOG_MESSAGE_1_BUFFER_MSG_WRITE_FAILED, rc);
            goto inDataAllocated;
        }

    }   /* End of Byte Ordering for Little Endian M/Cs */
#endif

#ifdef RETENTION_ENABLE
    rc = clEvtRetentionNodeAdd(pEvtChannelDB, pRetentionKey, inMsgHandle);
    /*
     * clCntNodeAddAndNodeGet to store handle for Timer if appropriate
     * rather obtain handle from handle library and put associated
     * information as the "handle obtained above" obviating rand or
     * inMsgHandle directly but deleting container fails so... 2. Re-think
     * about Data. Is message handle safe? Do we need all the info since
     * most are incomplete 
     */
#endif

    /*
     ** Get the IOC address depending on channel Scope.
     ** Fix for bug 3398.
     */
    if (CL_EVENT_GLOBAL_CHANNEL == channelScope)
    {
        remoteObjAddr.iocPhyAddress.nodeAddress = CL_IOC_BROADCAST_ADDRESS;
    }
    else
    {
        remoteObjAddr.iocPhyAddress.nodeAddress = clIocLocalAddressGet();
    }

    /*
     * Invoke RMD call provided by EM/S for opening channel, with packed user
     * data. 
     */
    rmdOptions.priority = 0;
    rmdOptions.retries = CL_EVT_RMD_MAX_RETRIES;
    rmdOptions.timeout = CL_EVT_RMD_TIME_OUT;

    remoteObjAddr.iocPhyAddress.portId = CL_EVT_COMM_PORT;
    rc = clRmdWithMsg(remoteObjAddr, EO_CL_EVT_PUBLISH_PROXY, inMsgHandle, 0,
            CL_RMD_CALL_ASYNC | CL_RMD_CALL_ORDERED, &rmdOptions, NULL); // ordering not needed since single thread in ES but for future
    if(CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "PUB", 
                "RMD to Publish Proxy Failed, rc[%#X]", rc);
        goto failure;
    }

// success:
    clLogTrace(CL_EVENT_LOG_AREA_SRV, "PUB", "Publish to Proxy successful");

    clHeapFree(pInData);        /* Free the memory allocated */
    // clHeapFree(pInData); // Free the buffer from clBufferFlatten()

    CL_FUNC_EXIT();
    return rc;

mutexLocked:
    clOsalMutexUnlock(mutexId);

inDataAllocated:  // NTC also more info in the log
    clHeapFree(pInData);

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError(CL_EVENT_LOG_AREA_SRV, "PUB", "Publish to Proxy Failed, rc[%#x]", rc);

    CL_FUNC_EXIT();
    return rc;
}

ClRcT VDECL(clEvtEventPublishProxy)(ClEoDataT cData,
        ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClOsalMutexIdT mutexId = 0;
    ClOsalMutexIdT channelLevelMutex = 0;
    ClUint8T channelScope = 0;
    ClEvtChannelInfoT *pEvtChannelInfo = NULL;
    ClEvtChannelDBT *pEvtChannelDB = NULL;
    ClEvtChannelKeyT evtChannelKey = { 0 };
    ClEvtEventPrimaryHeaderT *pEvtPrimaryHeader = NULL;
    void *pFlatBuff = NULL;

    ClUint32T inLen = 0;
    ClUint8T *pInData = NULL;
    ClEvtEventTypeKeyT evtEventTypeKey = { 0 };
    ClEvtPublishInfoT eventPublishInfo = { 0 };

    ClSizeT *pPatternSize = NULL;
    ClSizeT patternSize = 0;
    ClUint32T noOfPatterns = 0;

    ClUint32T headerLen = 0;

    CL_FUNC_ENTER();

    if(!gEvtInitDone)
    {
        clLogWarning("EVT", "PUB-PROXY", "Event server is not yet initialized");
        return CL_EVENTS_RC(CL_ERR_TRY_AGAIN);
    }

    rc = clBufferLengthGet(inMsgHandle, &inLen);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "Failed to get the length of buffer, rc[%#X]", rc);
        goto failure;
    }

    /*
     * Allocate extra space for the Secondary Header which is populated in
     * clEvtSubscribeWalkForPublish() to avoid multiple allocation during the
     * walk. 
     */
    headerLen = inLen + sizeof(ClEvtEventSecondaryHeaderT);

    pInData = clHeapAllocate(headerLen);
    if (NULL == pInData)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                CL_LOG_MESSAGE_0_MEMORY_ALLOCATION_FAILED);
        rc = CL_EVENT_ERR_NO_MEM;
        goto failure;
    }
    memset(pInData, 0, headerLen);

    rc = clBufferNBytesRead(inMsgHandle, pInData, &inLen);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                "Failed to read buffer, rc[%#X]", rc);
        goto failure;
    }

    pEvtPrimaryHeader = (ClEvtEventPrimaryHeaderT *) pInData;

    /*
     * Verify if the Version is Compatible 
     */
    rc = clEvtVerifyServerToServerVersion(pEvtPrimaryHeader);
    if (CL_OK != rc)
    {
        goto inDataAllocated;
    }

    /*
     * Byter Ordering applied only on little endian machines as big endian
     * machines are already in Network Order.
     * The changes need to be applied to 64 bit structures as well.
     */
    if (CL_TRUE == clEvtUtilsIsLittleEndian())
    {
        pEvtPrimaryHeader->channelId = ntohs(pEvtPrimaryHeader->channelId);
        pEvtPrimaryHeader->channelNameLen =
            ntohs(pEvtPrimaryHeader->channelNameLen);

        pEvtPrimaryHeader->patternSectionLen =
            ntohl(pEvtPrimaryHeader->patternSectionLen);
        pEvtPrimaryHeader->noOfPatterns =
            ntohl(pEvtPrimaryHeader->noOfPatterns);

        /*
         * Converting the 64 Bit fields using internal API.
         * This is prone to change once IDL is ready.
         */

        pEvtPrimaryHeader->retentionTime =
            clEvtNetUint64toHostUint64(pEvtPrimaryHeader->retentionTime);
        pEvtPrimaryHeader->publishTime =
            clEvtNetUint64toHostUint64(pEvtPrimaryHeader->publishTime);
        pEvtPrimaryHeader->eventId =
            clEvtNetUint64toHostUint64(pEvtPrimaryHeader->eventId);
        pEvtPrimaryHeader->eventDataSize =
            clEvtNetUint64toHostUint64(pEvtPrimaryHeader->eventDataSize);

        pEvtPrimaryHeader->publisherNameLen =
            ntohs(pEvtPrimaryHeader->publisherNameLen);
    }


    /*
     ** After recovering the host format the Event Header can be used as before.
     */

    channelScope = pEvtPrimaryHeader->channelScope;

    /*
     * Fill in channel Key 
     */
    evtChannelKey.channelId = pEvtPrimaryHeader->channelId;
    evtChannelKey.channelName.length = pEvtPrimaryHeader->channelNameLen;
    strncpy(evtChannelKey.channelName.value,
            (char *) (pInData + sizeof(ClEvtEventPrimaryHeaderT)),
            pEvtPrimaryHeader->channelNameLen);

    /*
     * Find out corresponding event channel is available 
     */
    if (CL_EVENT_GLOBAL_CHANNEL == channelScope)
    {
        pEvtChannelInfo = gpEvtGlobalECHDb;
        mutexId = gEvtGlobalECHMutex;
    }
    else
    {
        pEvtChannelInfo = gpEvtLocalECHDb;
        mutexId = gEvtLocalECHMutex;
    }

    /*
     * Get ECH related information 
     */
    clOsalMutexLock(mutexId);
    rc = clCntDataForKeyGet(pEvtChannelInfo->evtChannelContainer,
            (ClCntKeyHandleT) &evtChannelKey,
            (ClCntDataHandleT *) &pEvtChannelDB);
    if (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
    {
        /*
         * This is OK since there may be no subscribers on the specified
         * channel on this node. Hence, return CL_OK.
         */
        // rc = CL_EVENT_ERR_CHANNEL_NOT_OPENED;
        rc = CL_OK;
        goto mutexLocked;
    }
    else if(CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                "Failed to get ECH info");
        clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                CL_LOG_MESSAGE_1_CNT_DATA_GET_FAILED, rc);
        goto mutexLocked;
    }

    channelLevelMutex = pEvtChannelDB->channelLevelMutex;

    pFlatBuff =
        (pInData +
         (sizeof(ClEvtEventPrimaryHeaderT) + pEvtPrimaryHeader->channelNameLen +
          pEvtPrimaryHeader->publisherNameLen));

    if (CL_TRUE == clEvtUtilsIsLittleEndian())
    {
        /*
         ** Recover the PatternSize for each Pattern in the Pattern Section.
         */
        pPatternSize = pFlatBuff;
        noOfPatterns = pEvtPrimaryHeader->noOfPatterns;

        while (noOfPatterns--)
        {
            memcpy( &patternSize, pPatternSize, sizeof(*pPatternSize));

            patternSize = clEvtNetUint64toHostUint64(patternSize);

            memcpy( pPatternSize, &patternSize, sizeof(*pPatternSize));

            pPatternSize =
                (ClSizeT *) ((ClUint8T *) (pPatternSize + 1) + patternSize);
        }
    }

#ifdef RETENTION_ENABLE
    if (pEvtPrimaryHeader->flag & CL_EVT_RETAINED_EVENT)
    {
        /*
         * If the Event is a retained event then directly invoke
         * the Publish for the specific subscriber.
         */
        rc = clEvtPublishRetainedEvent(pEvtPrimaryHeader);
        if ()
        {
        }
        clOsalMutexUnlock(mutexId);
        CL_FUNC_EXIT();
        return CL_OK;
    }
#endif

    rc = clEvtUtilsFlatPattern2FlatBuffer(pFlatBuff,
            pEvtPrimaryHeader->noOfPatterns,
            &evtEventTypeKey.key.pData,
            &evtEventTypeKey.dataLen);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                CL_LOG_MESSAGE_1_CNT_WALK_FAILED, rc);
        clOsalMutexUnlock(mutexId);
        goto inDataAllocated;
    }

    evtEventTypeKey.flag = CL_EVT_TYPE_DATA_KEY_TYPE;

    /*
     * Walk through the event types and Publish for all the types that match
     * the specified pattern 
     */
    clOsalMutexLock(channelLevelMutex);

    eventPublishInfo.pEventTypeKey = &evtEventTypeKey;
    eventPublishInfo.pEvtPrimaryHeader = pEvtPrimaryHeader;

    /*
     * Note that headerLen is being passed and not the length of Argument 
     */
    rc = clCntWalk(pEvtChannelDB->eventTypeInfo, clEvtEventTypeWalkForPublish,
            (ClCntArgHandleT) &eventPublishInfo, headerLen);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "clEvtEventTypeWalkForPublish failed");
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                CL_LOG_MESSAGE_1_CNT_WALK_FAILED, rc);
        goto chanelLevelMutexLocked;
    }

    clOsalMutexUnlock(channelLevelMutex);
    clOsalMutexUnlock(mutexId);

// success:
    clLogMultiline(CL_LOG_TRACE, CL_EVENT_LOG_AREA_SRV, "PUB", 
            "Event{\n"
            "\tno of patterns[%d]\n"
            "\teventId[%#llX]\n"
            "\teventDataSize[%lld]\n"
            "\tpublishTime[%lld]\n"
            "\tretentionTime[%lld]\n"
            "}\n"
            "was sent to [%d] subscriber(s) on\n"
            "[%s] Channel{\n"
            "\tname[%.*s]\n"
            "\tID[%#X]\n"
            "}",
            pEvtPrimaryHeader->noOfPatterns,
            pEvtPrimaryHeader->eventId,
            pEvtPrimaryHeader->eventDataSize,
            pEvtPrimaryHeader->publishTime,
            pEvtPrimaryHeader->retentionTime,
            pEvtChannelDB->subscriberRefCount,
            (pEvtPrimaryHeader->channelScope == CL_EVENT_GLOBAL_CHANNEL ? 
             "Global" : "Local"),
            evtChannelKey.channelName.length,
            evtChannelKey.channelName.value,
            pEvtPrimaryHeader->channelId);

    /*
     * This Memory is allocated in clEvtUtilsFlatPattern2FlatBuffer() API 
     */
    clHeapFree(evtEventTypeKey.key.pData);

    /*
     * Release the memory since flatten is no longer used 
     */
    clHeapFree(pInData);        /* Free the memory allocated */

    CL_FUNC_EXIT();
    return rc;

chanelLevelMutexLocked:
    clOsalMutexUnlock(channelLevelMutex);

// eventTypeKeyObtained:
    clHeapFree(evtEventTypeKey.key.pData);

mutexLocked:
    clOsalMutexUnlock(mutexId);

inDataAllocated:
    if(CL_OK == rc) goto silent;

    rc = CL_EVENTS_RC(rc);
    clLogError(CL_EVENT_LOG_AREA_SRV, "PUB", "Event delivery failed, rc[%#x]", rc);
    clLogMultiline(CL_LOG_ERROR, CL_EVENT_LOG_AREA_SRV, "PUB", 
            "Event{\n"
            "\tno of patterns[%d]\n"
            "\teventId[%#llX]\n"
            "\teventDataSize[%lld]\n"
            "\tpublishTime[%lld]\n"
            "\tretentionTime[%lld]\n"
            "}\n"
            "was being sent to subscriber(s) on\n"
            "[%s] Channel{\n"
            "\tname[%.*s]\n"
            "\tID[%#X]\n"
            "}",
            pEvtPrimaryHeader->noOfPatterns,
            pEvtPrimaryHeader->eventId,
            pEvtPrimaryHeader->eventDataSize,
            pEvtPrimaryHeader->publishTime,
            pEvtPrimaryHeader->retentionTime,
            (pEvtPrimaryHeader->channelScope == CL_EVENT_GLOBAL_CHANNEL ? 
             "Global" : "Local"),
            evtChannelKey.channelName.length,
            evtChannelKey.channelName.value,
            pEvtPrimaryHeader->channelId);

silent:
    clHeapFree(pInData);

failure:

    CL_FUNC_EXIT();
    return CL_EVENTS_RC(rc);
}

ClRcT VDECL(clEvtNackSend)(ClEoDataT cData, ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClUint8T *pInData = NULL;
    ClIocAddressT srcAddr = {{ 0 }};

    ClEvtNackInfoT *pEvtNackInfo = NULL;

    CL_FUNC_ENTER();

    if(!gEvtInitDone)
    {
        clLogWarning("EVT", "NACK", "Event server is not yet initialized");
        return CL_EVENTS_RC(CL_ERR_TRY_AGAIN);
    }

    rc = clBufferFlatten(inMsgHandle, &pInData);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                "Failed to flatten buffer [%#X]", rc);
        goto failure;
    }

    pEvtNackInfo = (ClEvtNackInfoT *) pInData;


    switch (pEvtNackInfo->nackType)
    {
        case CL_EVT_NACK_PUBLISH:

            rc = clRmdSourceAddressGet(&srcAddr.iocPhyAddress);
            if (CL_OK != rc)
            {
                clLogError(CL_EVENT_LOG_AREA_SRV, "SUB",
                        "Source Address Get Failed, rc=[0x%x]\n", rc);
                goto inDataAllocated;
            }

            clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                    CL_EVENT_LOG_MSG_6_VERSION_NACK, "Event Publish",
                    srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId, pEvtNackInfo->releaseCode,
                    pEvtNackInfo->majorVersion, pEvtNackInfo->minorVersion);

            break;

        default:
            clLogError(CL_EVENT_LOG_AREA_SRV, "PUB",
                    "Invalid Nack Type [0x%x]\n",
                    pEvtNackInfo->nackType);
            goto inDataAllocated;
    }
// success: fall through for now

inDataAllocated:
    clHeapFree(pInData);

failure:
    CL_FUNC_EXIT();
    return rc;
}

ClRcT clEvtUnSubsWalk(ClCntHandleT eventTypeHandle,
        ClEvtEventTypeInfoT *pEvtTypeInfo,
        ClEvtUnsubscribeEventRequestT *pEvtUnsubsReq)
{
    ClRcT rc = CL_OK;
    ClRcT ec = CL_OK;
    ClCntNodeHandleT nodeHandle = CL_HANDLE_INVALID_VALUE;
    ClCntNodeHandleT nodeHandle1 = CL_HANDLE_INVALID_VALUE;
    ClEvtSubsKeyT *pEvtSubsKey = NULL;
    ClBoolT found = CL_FALSE; // NTC

    rc = clCntFirstNodeGet(eventTypeHandle, &nodeHandle1);
    if (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
    {
        rc = CL_OK;
        goto success;
    }
    else if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "CLN", 
                "Failed to get event type hanlde, rc[%#x]", rc);
        goto failure;
    }

    while (CL_OK == ec)
    {
        nodeHandle = nodeHandle1;
        rc = clCntNodeUserKeyGet(eventTypeHandle, nodeHandle,
                (ClCntKeyHandleT *) &pEvtSubsKey);
        if (CL_OK != rc)
        {
            clLogError(CL_EVENT_LOG_AREA_SRV, "CLN", 
                    "Failed to get subscription key, rc[%#x]", rc);
            goto failure;
        }

        /*
         * Moved here to fix FMR 
         */
        ec = clCntNextNodeGet(eventTypeHandle, nodeHandle, &nodeHandle1);

        switch (pEvtUnsubsReq->reqFlag)
        {
            /*
             * If it is only for unsubscription 
             */
            case CL_EVT_UNSUBSCRIBE:
                if (pEvtUnsubsReq->subscriptionId == pEvtSubsKey->subscriptionId
                        && 0 == clEvtUserIDCompare(pEvtUnsubsReq->userId,
                            pEvtSubsKey->userId))
                {
                    rc = clCntNodeDelete(eventTypeHandle, nodeHandle);
                    /*
                     * Decrease the referance count 
                     */
                    pEvtTypeInfo->refCount--;
                    found = CL_TRUE;
                    rc = CL_EVENT_ERR_STOP_WALK;
                    goto success;
                }
                break;

            case CL_EVT_CHANNEL_CLOSE:
            case CL_EVT_FINALIZE:
                if (0 ==
                        clEvtUserIDCompare(pEvtUnsubsReq->userId,
                            pEvtSubsKey->userId))
                {
                    rc = clCntNodeDelete(eventTypeHandle, nodeHandle);
                    /*
                     * Decrease the referance count 
                     */
                    pEvtTypeInfo->refCount--;
                    break;
                }
        }
    }

success: // fall through for now

failure:
    return rc;
}

ClRcT clEvtUnSubsEventTypeWalk(ClCntHandleT eventTypeHandle,
        ClEvtUnsubscribeEventRequestT *pEvtUnsubsReq)
{
    ClRcT rc = CL_OK;
    ClRcT ec = CL_OK;
    ClRcT unSubsRc = CL_OK;
    ClCntNodeHandleT nodeHandle = NULL;
    ClCntNodeHandleT nodeHandle1 = NULL;
    ClEvtEventTypeInfoT *pEvtTypeInfo = NULL;

    rc = clCntFirstNodeGet(eventTypeHandle, &nodeHandle1);
    if (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
    {
        rc = CL_OK;
        goto success;
    }
    else if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "CLN", 
                "Failed to get event type hanlde, rc[%#x]", rc);
        goto failure;
    }

    while (CL_OK == ec)
    {
        nodeHandle = nodeHandle1;
        rc = clCntNodeUserDataGet(eventTypeHandle, nodeHandle,
                (ClCntDataHandleT *) &pEvtTypeInfo);
        if (CL_OK != rc)
        {
            clLogError(CL_EVENT_LOG_AREA_SRV, "CLN", 
                    "Failed to get event type info, rc[%#x]", rc);
            goto failure;
        }

        unSubsRc =
            clEvtUnSubsWalk(pEvtTypeInfo->subscriberInfo, pEvtTypeInfo,
                    pEvtUnsubsReq);

        ec = clCntNextNodeGet(eventTypeHandle, nodeHandle, &nodeHandle1);

        if (0 == pEvtTypeInfo->refCount)
        {
            rc = clCntNodeDelete(eventTypeHandle, nodeHandle);
        }

        if (CL_EVENT_ERR_STOP_WALK == unSubsRc)
        {
            rc = unSubsRc;
            goto success;
        }
        else if (CL_OK != ec && CL_EVENT_ERR_INVALID_PARAM == unSubsRc) // NTC
        {
            /*
             * If the subscription ID is does not match.
             */
            rc = CL_EVENT_ERR_INVALID_PARAM;
            goto failure;
        }
    }

success: // fall through for now

failure:
    return rc;
}

ClRcT
clEventCpmCleanupWalk(ClHandleDatabaseHandleT databaseHandle,ClHandleT handle, void *pCookie)
{
    ClRcT rc = CL_OK;	

    ClEvtUnsubscribeEventRequestT *pEvtUnsubsReq = NULL;
    ClEvtUserIdT *pInitKey = NULL;   
    pEvtUnsubsReq = (ClEvtUnsubscribeEventRequestT *)pCookie;

    rc = clHandleCheckout(databaseHandle, handle, (void **)&pInitKey); 
    if(CL_OK != rc)
    {
        rc = CL_OK; // NTC
        clLogError(CL_EVENT_LOG_AREA_SRV, "CLN",
                CL_LOG_MESSAGE_1_HANDLE_CHECKOUT_FAILED, rc);
        goto failure;
    } 

    if(pInitKey->eoIocPort == pEvtUnsubsReq->userId.eoIocPort) 
    {
        rc = clHandleCheckin(databaseHandle, handle);
        if(CL_OK != rc)
        {
            rc = CL_OK; // NTC
            clLogError(CL_EVENT_LOG_AREA_SRV, "CLN",
                    CL_LOG_MESSAGE_1_HANDLE_CHECKIN_FAILED, rc);
            goto failure;
        }
        rc = clHandleDestroy(databaseHandle, handle);
        if(CL_OK != rc)
        {
            rc = CL_OK; // NTC
            clLogError(CL_EVENT_LOG_AREA_SRV, "CLN",
                    CL_LOG_MESSAGE_1_HANDLE_DB_DESTROY_FAILED, rc);
            goto failure;
        }
    }

// success: fall through for now

failure:
    return rc;
}

extern ClRcT clEvtCkptCheckPointAll(void); // NTC 

ClRcT VDECL(clEvtEventUnsubscribeAllLocal)(ClEoDataT cData,
        ClBufferHandleT inMsgHandle,
        ClBufferHandleT outMsgHandle)
{
    ClRcT rc = CL_OK;
    ClEvtUnsubscribeEventRequestT evtUnsubsReq = {0};

    CL_FUNC_ENTER();

    if(!gEvtInitDone)
    {
        clLogWarning("EVT", "UNSUB-LOCAL", "Event server is not yet initialized");
        return CL_EVENTS_RC(CL_ERR_TRY_AGAIN);
    }

    rc = VDECL_VER(clXdrUnmarshallClEvtUnsubscribeEventRequestT, 4, 0, 0)(inMsgHandle,
                                                                          &evtUnsubsReq);
    if (CL_OK != rc)
    {
        clLogError(CL_EVENT_LOG_AREA_SRV, "CLN",
                   "Failed to Unmarshall Buffer, error [%#X]", rc);
        goto failure;
    }

    /*
     * Verify if the Version is Compatible 
     */
    clEvtVerifyClientToServerVersion(&evtUnsubsReq, outMsgHandle);

    if (CL_EVT_FINALIZE == evtUnsubsReq.reqFlag)
    {
        /*
         * this is for GLOBAL scope 
         */
        CL_EVT_SET_GLOBAL_SCOPE(evtUnsubsReq.evtChannelHandle);

        rc = clEvtEventCleanupViaRequest(&evtUnsubsReq, CL_EVT_NORMAL_REQUEST);
        if (CL_OK != rc)
        {
            goto inDataAllocated;
        }

        /*
         * this is for local scope 
         */
        CL_EVT_SET_LOCAL_SCOPE(evtUnsubsReq.evtChannelHandle);

        rc = clEvtEventCleanupViaRequest(&evtUnsubsReq, CL_EVT_NORMAL_REQUEST);
        if (CL_OK != rc)
        {
            goto inDataAllocated;
        }

        if(0 == evtUnsubsReq.userId.evtHandle)
        {
            /* This may be the call clEventCpmCleanup which passes evtHandle as 0 and just provides the 
             * eoIocPort for the application going down. Do a walk on the database for the node with 
             * the eoIocPort = the eoIocPort passed by the call. 
             */

            rc = clHandleWalk(gEvtHandleDatabaseHdl, clEventCpmCleanupWalk,(void *)&evtUnsubsReq);
            if(CL_OK != rc)
            {
                clLogError(CL_EVENT_LOG_AREA_SRV, "CLN",
                        "clHandleWalk failed, rc[%#X]", rc);
            } 
        }
        else
        {	
            rc = clHandleDestroy( gEvtHandleDatabaseHdl, (ClHandleT)evtUnsubsReq.userId.evtHandle);
            if(CL_OK != rc)
            {
                clLogError(CL_EVENT_LOG_AREA_SRV, "CLN",
                        "clHandleDestroy failed, rc[%#X]", rc);
                goto inDataAllocated;
            }
        }
    }
    else
    {
        rc = clEvtEventCleanupViaRequest(&evtUnsubsReq, CL_EVT_NORMAL_REQUEST);
        if (CL_OK != rc)
        {
            goto inDataAllocated;
        }
    }

    rc = clEvtCkptCheckPointAll();
    if (CL_OK != rc)
    {
        goto inDataAllocated;
    }

// success:
    clLogTrace(CL_EVENT_LOG_AREA_SRV, "CLN", "Unsubscribe All successful"); // NTC

    CL_FUNC_EXIT();
    return rc;

inDataAllocated:  // NTC also more info in the log

failure:
    rc = CL_EVENTS_RC(rc);
    clLogError(CL_EVENT_LOG_AREA_SRV, "CLN", "Unbsubscribe All failed, rc[%#X]", rc); // NTC

    CL_FUNC_EXIT();
    return rc;
}


/*
 * This Cleanup function can be invoked from the server with
 * CL_EVT_NORMAL_REQUEST 
 */

ClRcT clEvtEventCleanupViaRequest(ClEvtUnsubscribeEventRequestT *pEvtUnsubsReq,
        ClUint32T type)
{
    ClRcT rc = CL_OK;
    ClRcT ec = CL_OK;
    ClRcT unSubsRc = CL_OK;
    ClEvtChannelInfoT *pEvtChannelInfo = NULL;
    ClEvtChannelDBT *pEvtChannelDB = NULL;
    ClEvtChannelKeyT *pEvtChannelKey = NULL;
    ClUint16T channelScope = 0;
    ClUint16T channelId = 0;
    ClCntNodeHandleT evtChannelNodeHandle = NULL;
    ClCntNodeHandleT evtChannelNodeHandle1 = NULL;
    ClWordT userType = 0;
    ClCntNodeHandleT userOfEchNodeHandle = CL_HANDLE_INVALID_VALUE;

    ClOsalMutexIdT mutexId = 0;


    CL_FUNC_ENTER();


    CL_EVT_CHANNEL_SCOPE_GET(pEvtUnsubsReq->evtChannelHandle, channelScope);

    if (CL_EVENT_GLOBAL_CHANNEL == channelScope)
    {
        mutexId = gEvtGlobalECHMutex;
        pEvtChannelInfo = gpEvtGlobalECHDb;
    }
    else
    {
        mutexId = gEvtLocalECHMutex;
        pEvtChannelInfo = gpEvtLocalECHDb;
    }

    /*
     * Channel Information 
     */
    clOsalMutexLock(mutexId);
    rc = clCntFirstNodeGet(pEvtChannelInfo->evtChannelContainer,
            &evtChannelNodeHandle1);
    if (CL_ERR_NOT_EXIST == CL_GET_ERROR_CODE(rc))
    {
        /*
         * No Channel opened which is OK 
         */
        rc = CL_OK;
        goto success;
    }
    else if (CL_OK != rc)
    {
        goto mutexLocked;
    }

    /*
     * NOTE: Need to take care for decrement the referance count and if reaches
     * zero then delete the node 
     */
    while (CL_OK == ec)
    {
        evtChannelNodeHandle = evtChannelNodeHandle1;
        rc = clCntNodeUserDataGet(pEvtChannelInfo->evtChannelContainer,
                evtChannelNodeHandle,
                (ClCntDataHandleT *) &pEvtChannelDB);
        if (CL_OK != rc && CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(rc)) // NTC
        {
            goto mutexLocked;
        }

        rc = clCntNodeUserKeyGet(pEvtChannelInfo->evtChannelContainer,
                evtChannelNodeHandle,
                (ClCntDataHandleT *) &pEvtChannelKey);
        if (CL_OK != rc && CL_ERR_NOT_EXIST != CL_GET_ERROR_CODE(rc)) // NTC
        {
            goto mutexLocked;
        }

        CL_EVT_CHANNEL_ID_GET(pEvtUnsubsReq->evtChannelHandle, channelId);
        CL_EVT_CHANNEL_USER_TYPE_GET(pEvtUnsubsReq->evtChannelHandle, userType);

        ec =
            clCntNextNodeGet(pEvtChannelInfo->evtChannelContainer,
                    evtChannelNodeHandle, &evtChannelNodeHandle1);

        switch (pEvtUnsubsReq->reqFlag)
        {
            case CL_EVT_CHANNEL_CLOSE:
                if (channelId == pEvtChannelKey->channelId &&
                        0 == clEvtUtilsNameCmp(&pEvtChannelKey->channelName,
                            &pEvtUnsubsReq->evtChannelName))
                {

                    unSubsRc =
                        clEvtUnSubsEventTypeWalk(pEvtChannelDB->eventTypeInfo,
                                pEvtUnsubsReq);
                    /*
                     * Find and delete the user information 
                     */
                    rc = clCntNodeFind(pEvtChannelDB->evtChannelUserInfo,
                            (ClCntKeyHandleT) &pEvtUnsubsReq->userId,
                            &userOfEchNodeHandle);
                    if (CL_OK == rc)
                    {
                        rc = clCntNodeDelete(pEvtChannelDB->evtChannelUserInfo,
                                userOfEchNodeHandle);
                    }


                    if ( rc == CL_OK && (0 != (userType & CL_EVENT_CHANNEL_SUBSCRIBER)
                                         && (pEvtChannelDB->subscriberRefCount > 0)))
                    {
                        pEvtChannelDB->subscriberRefCount--;
                    }
                    if ( rc == CL_OK && ( 0 != (userType & CL_EVENT_CHANNEL_PUBLISHER) 
                                          && (pEvtChannelDB->publisherRefCount > 0)) )
                    {
                        pEvtChannelDB->publisherRefCount--;
                    }

                    if (0 ==
                            pEvtChannelDB->publisherRefCount +
                            pEvtChannelDB->subscriberRefCount)
                    {
                        rc = clCntNodeDelete(pEvtChannelInfo->
                                evtChannelContainer,
                                evtChannelNodeHandle);
                    }

                    // clOsalMutexUnlock(mutexId); // NTC why not include check point under this?
#ifndef CKPT_ENABLED
                    if (CL_EVT_NORMAL_REQUEST == type)
                    {
                        rc = clEvtCkptCheckPointChannelClose(pEvtUnsubsReq);
                        if (CL_OK != rc)
                        {
                            clLogError(CL_EVENT_LOG_AREA_SRV, "CLN",
                                    CL_EVENT_LOG_MSG_5_CKPT_CLOSE_FAIL,
                                    pEvtChannelKey->channelName.length,
                                    pEvtChannelKey->channelName.value,
                                    pEvtUnsubsReq->userId.eoIocPort,
                                    pEvtUnsubsReq->userId.evtHandle, rc);
                            goto failure; // NTC if mutexLocked
                        }
                    }
#endif
                    goto success;
                }
                break;

            case CL_EVT_FINALIZE:
                /*
                 * Find out if the given user opened this event channel 
                 */
                rc = clCntNodeFind(pEvtChannelDB->evtChannelUserInfo,
                        (ClCntKeyHandleT) &pEvtUnsubsReq->userId,
                        &userOfEchNodeHandle);
                if (CL_OK == rc)
                {
                    rc = clCntNodeUserDataGet(pEvtChannelDB->evtChannelUserInfo,
                            userOfEchNodeHandle,
                            (ClCntDataHandleT *) &userType);

                    unSubsRc =
                        clEvtUnSubsEventTypeWalk(pEvtChannelDB->eventTypeInfo,
                                pEvtUnsubsReq);

                    /*
                     * Decrement corresponding referance count 
                     */
                    if ( (0 != (userType & CL_EVENT_CHANNEL_SUBSCRIBER)
                                && (pEvtChannelDB->subscriberRefCount > 0)) )
                    {
                        pEvtChannelDB->subscriberRefCount--;
                    }
                    if ( (0 != (userType & CL_EVENT_CHANNEL_PUBLISHER)
                                && (pEvtChannelDB->publisherRefCount > 0)) )
                    {
                        pEvtChannelDB->publisherRefCount--;
                    }

                    /*
                     * Delete the user information EM 
                     */
                    rc = clCntNodeDelete(pEvtChannelDB->evtChannelUserInfo,
                            userOfEchNodeHandle);

                    /*
                     * If both the referance count become zero then delete the
                     * event channel 
                     */
                    if (0 ==
                            (pEvtChannelDB->subscriberRefCount +
                             pEvtChannelDB->publisherRefCount))
                    {
                        rc = clCntNodeDelete(pEvtChannelInfo->
                                evtChannelContainer,
                                evtChannelNodeHandle);
                    }

                }

#ifndef CKPT_ENABLED
                if (CL_EVT_NORMAL_REQUEST == type)
                {
                    rc = clEvtCkptCheckPointFinalize(pEvtUnsubsReq);
                    if (CL_OK != rc)
                    {
                        clLogError(CL_EVENT_LOG_AREA_SRV, "CLN",
                                CL_EVENT_LOG_MSG_3_CKPT_FIN_FAIL,
                                pEvtUnsubsReq->userId.eoIocPort,
                                pEvtUnsubsReq->userId.evtHandle, rc);
                        goto failure; // NTC if mutexLocked
                    }
                }
#endif

                break;


            case CL_EVT_UNSUBSCRIBE:
                if (channelId == pEvtChannelKey->channelId &&
                        0 == clEvtUtilsNameCmp(&pEvtChannelKey->channelName,
                            &pEvtUnsubsReq->evtChannelName))
                {
                    unSubsRc =
                        clEvtUnSubsEventTypeWalk(pEvtChannelDB->eventTypeInfo,
                                pEvtUnsubsReq);
                    if (CL_EVENT_ERR_STOP_WALK != unSubsRc && unSubsRc != CL_OK)    /* If 
                                                                                 * the 
                                                                                 * subscription 
                                                                                 * ID 
                                                                                 * is 
                                                                                 * not 
                                                                                 * matched 
                                                                                 */
                    {
                        goto mutexLocked;
                    }
                }

#ifndef CKPT_ENABLED
                if (CL_EVT_NORMAL_REQUEST == type)
                {
                    rc = clEvtCkptCheckPointUnsubscribe(pEvtUnsubsReq,
                            pEvtChannelDB);
                    if (CL_OK != rc)
                    {
                        clLogError(CL_EVENT_LOG_AREA_SRV, "CLN",
                                CL_EVENT_LOG_MSG_6_CKPT_UNSUB_FAIL,
                                pEvtUnsubsReq->subscriptionId,
                                pEvtChannelKey->channelName.length,
                                pEvtChannelKey->channelName.value,
                                pEvtUnsubsReq->userId.eoIocPort,
                                pEvtUnsubsReq->userId.evtHandle, rc);
                        goto failure;
                    }
                }
#endif
                break;
        }

    }

success:
    clOsalMutexUnlock(mutexId);

    clLogTrace(CL_EVENT_LOG_AREA_SRV, "CLN", "Cleanup Via Request successful");

    CL_FUNC_EXIT();
    return CL_OK;

mutexLocked:
    clOsalMutexUnlock(mutexId);

// failure:
    rc = CL_EVENTS_RC(rc);
    clLogError(CL_EVENT_LOG_AREA_SRV, "CLN", "Cleanup Via Request failed, rc[%#x]", rc);

    CL_FUNC_EXIT();
    return rc;
}
