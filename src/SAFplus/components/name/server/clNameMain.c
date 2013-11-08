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
 * ModuleName  : name
File        : clNameMain.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *  This file contains Name Service Server Side functionality.
 *
 *
 *****************************************************************************/

#define __SERVER__
#include "stdio.h"
#include "string.h"
#include "clCommon.h"
#include "clEoApi.h"
#include "clNameCommon.h"
#include "clNameErrors.h"
#include "clCommonErrors.h"
#include "clNameIpi.h"
#include "clTimerApi.h"
#include "clRmdApi.h"
#include <../../include/ipi/clRmdIpi.h>
#include "clDebugApi.h"
#include "clNameConfigApi.h"
#include "clCpmApi.h"
#include "clCksmApi.h"
#include "clNameCkptIpi.h"
#include "clNameEventIpi.h"
#include "clEventApi.h"
#include "clIocErrors.h"
#include "clLogApi.h"
#include "clNameLog.h"
#include <clIdlApi.h> 
#include "xdrClNameSvcAttrEntryIDLT.h"
#include "xdrClNameSvcBindingDetailsIDLT.h"
#include "xdrClNameSvcBindingIDLT.h"
#include "xdrClNameSvcCompListIDLT.h"
#include "xdrClNameSvcContextInfoIDLT.h"
#include "xdrClNameSvcPriorityIDLT.h"
#include "xdrClNameSvcAttrEntryWithSizeIDLT.h"
#include "xdrClNameSvcInfoIDLT.h"
#include "xdrClNameVersionT.h"
#include <ipi/clSAClientSelect.h>
#include <clIocApiExt.h>
#include <netinet/in.h>
#include <clIocLogicalAddresses.h>

#undef __CLIENT__
#include "clNameServerFuncTable.h"

#define CL_NS_CALL_RMD_ASYNC(addr, fcnId, pInBuf, pOpBuf, rc)\
do  \
{ \
  ClRmdOptionsT      rmdOptions = CL_RMD_DEFAULT_OPTIONS;\
  ClIocAddressT      address;\
  address.iocPhyAddress.nodeAddress = addr; \
  address.iocPhyAddress.portId      = CL_IOC_NAME_PORT; \
  rmdOptions.timeout = CL_NS_DFLT_TIMEOUT; \
  rmdOptions.retries = CL_NS_DFLT_RETRIES; \
  rmdOptions.priority = 2; \
  rc = clRmdWithMsg(address, \
                   fcnId, (ClBufferHandleT) pInBuf,\
                   (ClBufferHandleT) pOpBuf, CL_RMD_CALL_ASYNC,\
                    &rmdOptions, NULL);\
} \
while(0)


# define CL_NS_SERVER_SERVER_VERSION_SET(pVersion) (pVersion)->releaseCode = 'B', \
                                                    (pVersion)->majorVersion = 0x1, \
                                                    (pVersion)->minorVersion = 0x1

static ClOsalMutexIdT    gSem;

/* LOCALS */
static ClInt32T          sNSInitDone      = 0;
static ClNameServiceT    sNameService     = {0};
static ClUint32T         sNoUserGlobalCxt = 0;
static ClUint32T         sNoUserLocalCxt  = 0;
static ClUint64T         sObjCount        = CL_IOC_DYNAMIC_LOGICAL_ADDRESS_START;
ClCpmHandleT    	     cpmHandle        = 0;

ClCntHandleT           gNSDefaultGlobalHashTable    = 0;
ClCntHandleT           gNSDefaultNLHashTable        = 0;
ClCntHandleT           gNSHashTable                 = 0;
ClNameSvcContextInfoT* pgDeftGlobalHashTableStat    = NULL; 
ClNameSvcContextInfoT* pgDeftNLHashTableStat        = NULL; 
ClVersionDatabaseT     gNSClientToServerVersionInfo = {0};
ClVersionDatabaseT     gNSServerToServerVersionInfo = {0};
static ClVersionT      gNSClientToServerVersionSupported[]={
{'B',0x01 , 0x01}
};
static ClVersionT      gNSServerToServerVersionSupported[]={
{'B',0x01 , 0x01}
};


extern ClCkptSvcHdlT   gNsCkptSvcHdl ;
static ClNameSvcConfigT gpConfig = {0};

ClCntKeyHandleT     gUserKey            = 0;
ClCntKeyHandleT     gNameEntryUserKey   = 0;
ClUint32T           gContextIndex       = 0;
ClUint32T           gEntryIndex         = 0;
ClUint32T           gDelete             = 0;
ClUint32T           gFound              = 0;
ClUint32T           gNameEntryDelete    = 0;
ClUint32T           gHashTableIndex     = 0;
ClIocNodeAddressT   gMasterAddress      = 0;
ClCntHandleT        gHashTableHdl       = 0;
ClCntHandleT        gNameEntryHashTable = 0;

#define CL_NAME_MAX_ENTRY_LEN   1024
#define CL_NS_CONTEXT_NOT_FOUND 0xFFFFFFFF 

static ClUint32T* gpContextIdArray  = NULL;

extern ClCharT  *clNameLogMsg[];

extern ClRcT clIocLocalAddressGet();
extern ClEventHandleT        gEventHdl;
extern ClRcT clNameCompCfg(void);
ClRcT _nameSvcContextDelete(ClNameSvcInfoIDLT* nsInfo, ClUint32T flag,
                            ClNameVersionT *pVersion);
ClRcT _nameSvcContextLevelWalkForFinalize(ClCntKeyHandleT    userKey,
                                          ClCntDataHandleT   hashTable,
                                          ClCntArgHandleT    userArg,
                                          ClUint32T          dataLength);

ClRcT _nameSvcEntryDeleteCallback(ClCntKeyHandleT    userKey,
                                  ClCntDataHandleT   nsInfo,
                                  ClCntArgHandleT    arg,
                                  ClUint32T          dataLength);

ClRcT _nameSvcContextLevelWalkForDelete(ClCntKeyHandleT    userKey,
                                        ClCntDataHandleT   hashTable,
                                        ClCntArgHandleT    userArg,
                                        ClUint32T          dataLength);

static ClRcT
clNSLookupKeyForm(ClNameT               *pName,
                  ClNameSvcNameLookupT  *pLookupKey)
{
    ClRcT  rc = CL_OK;

    memset(pLookupKey, '\0', sizeof(ClNameSvcNameLookupT));
    clNameCopy(&pLookupKey->name, pName);
    rc = clCksm32bitCompute((ClUint8T *) pLookupKey->name.value, pLookupKey->name.length, &pLookupKey->cksum);
    if( CL_OK != rc )
    {
        return rc;
    }
    return rc;
}
                                                                                                                             


/*************************************************************************/
/******************** Functions needed by EO infrastructure **************/
/*************************************************************************/

ClRcT   nameSvcFinalize(ClInvocationT invocation,
    		const ClNameT  *compName)
{
    ClRcT             rc      = CL_OK;
    ClEoExecutionObjT *pEOObj = NULL;

    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Couldnt get Lock successfully--\n"));
    }
    
    /* Do NS related cleanup */

    /* Delete all the entries in all the contexts */
    /* Delete all the per context has tables */
    clCntWalk(gNSHashTable, _nameSvcContextLevelWalkForFinalize, NULL, 0);
    clCntDelete(gNSHashTable);
    gNSHashTable = 0;
    /* Finalize ckpt lib */    
    clNameSvcCkptFinalize();
    clHeapFree(gpContextIdArray);
    clHeapFree(gNSClientToServerVersionInfo.versionsSupported);
    clHeapFree(gNSServerToServerVersionInfo.versionsSupported);

    /* Release the semaphore */
    clOsalMutexUnlock(gSem);
    
    /* Uninstall the native table */
    (void)clEoMyEoObjectGet(&pEOObj);
    (void)clEoClientUninstallTables(pEOObj, CL_EO_SERVER_SYM_MOD(gAspFuncTable, NAM));
    /* Finalize event lib */
    (void)nameSvcEventFinalize();

    /* DeRegister with debug infra */
    (void)nameDebugDeregister(pEOObj);

    rc = clCpmComponentUnregister(cpmHandle, compName, NULL);
    rc = clCpmClientFinalize(cpmHandle);

    clLogNotice("SVR", "SHU", "Name service has been finalized successfully");
    clCpmResponse(cpmHandle, invocation, CL_OK);

    return CL_OK;
}


ClRcT   nameSvcInitialize(ClUint32T argc, ClCharT *argv[])
{
    ClNameT           appName   = {0};
    ClCpmCallbacksT   callbacks = {0};
    ClVersionT	      version   = {0};
    ClIocPortT	      iocPort   = 0;
    ClRcT             rc        = CL_OK;
    ClSvcHandleT      svcHandle = {0};
    ClEoExecutionObjT *eoObj    = NULL ;
    //ClOsalTaskIdT     taskId    = 0;

    /* NS Initialize */
    clNameCompCfg();

   /*  Do the CPM client init/Register */
    version.releaseCode = 'B';
    version.majorVersion = 0x01;
    version.minorVersion = 0x01;
    
    callbacks.appHealthCheck = NULL;
    callbacks.appTerminate = nameSvcFinalize;
    callbacks.appCSISet = NULL;
    callbacks.appCSIRmv = NULL;
    callbacks.appProtectionGroupTrack = NULL;
    callbacks.appProxiedComponentInstantiate = NULL;
    callbacks.appProxiedComponentCleanup = NULL;

   
    (void)clEoMyEoObjectGet(&eoObj);
    (void)clEoMyEoIocPortGet(&iocPort);
    rc = clCpmClientInitialize(&cpmHandle, &callbacks, &version);
    if( CL_OK != rc ) return rc;
    svcHandle.cpmHandle = &cpmHandle;
#if 0
    svcHandle.evtHandle = &evtHandle;
    svcHandle.cpsHandle = &cpsHandle;
    svcHandle.gmsHandle = &gmsHandle;
    svcHandle.dlsHandle = &dlsHandle;
    rc = clDispatchThreadCreate(eoObj, &taskId, svcHandle);
#endif    
    rc = clCpmComponentNameGet(cpmHandle, &appName);
    rc = clCpmComponentRegister(cpmHandle, &appName, NULL);
    /*clDebugCli("NAME-CLI");*/

    int cnt = 0;
    do
    {
        cnt++;
        rc = clCpmMasterAddressGet(&gMasterAddress);
        if (rc != CL_OK)
        {
            if ((cnt&15)==0) CL_DEBUG_PRINT(CL_DEBUG_INFO,("Waiting for active system controller.  rc [0x%x]",rc));
            sleep(1);
        }
    } while(rc != CL_OK);
    
    
    
    return CL_OK;
}


ClRcT   nameSvcStateChange(ClEoStateT eoState)
{
    return CL_OK;
}


ClRcT   nameSvcHealthCheck(ClEoSchedFeedBackT* schFeedback)
{
    return CL_OK;
}


/*************************************************************************/
/************************* NS utility functions***************************/
/*************************************************************************/


static ClUint32T _clNameIsLittleEndian(void)
{
    ClInt32T i = 1;
                                                                                                                             
    return (*((char *) &i) ? CL_TRUE : CL_FALSE);
}


/*
 ** Following API assume that Network ordering is Big Endian.
 */
static ClUint64T _clNameHostUint64toNetUint64(ClUint64T hostUint64)
{
    CL_FUNC_ENTER();
                                                                                                                             
    if (CL_TRUE == _clNameIsLittleEndian())
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

                                                                                                                             

static ClUint32T nameSvcPerContextEntryHashFunction(ClCntKeyHandleT key)
{
    ClNameSvcNameLookupT  *pNSInfo = (ClNameSvcNameLookupT*)key;
    ClUint32T              hashValue = 0;
    hashValue = (pNSInfo->cksum % gpConfig.nsMaxNoEntries);
    return hashValue;
}

static ClUint32T nameSvcContextHashFunction(ClCntKeyHandleT key)
{
    return((ClWordT)key%CL_NS_MAX_NO_CONTEXTS);
}

static ClUint32T nameSvcEntryDetailsHashFunction(ClCntKeyHandleT key)
{
    ClNameSvcBindingDetailsT   *pNSInfo1 = (ClNameSvcBindingDetailsT*)key;
    return((pNSInfo1->cksum)%(CL_NS_MAX_NO_OBJECT_REFERENCES));
}

static ClInt32T nameSvcHashKeyCmp(ClCntKeyHandleT key1, 
                                   ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}
    
static ClInt32T nameSvcHashEntryKeyCmp(ClCntKeyHandleT key1, 
                                   ClCntKeyHandleT key2)
{
    ClNameSvcBindingT    *pNSInfo1 = (ClNameSvcBindingT*)key1;
    ClNameSvcNameLookupT *pNSInfo2 = (ClNameSvcNameLookupT*)key2;

    clLogDebug("SVR", "CMP", "StoredEntry: [%.*s] PassedEntry [%.*s] cksum [%d] cksum [%d]", 
            pNSInfo1->name.length, pNSInfo1->name.value, pNSInfo2->name.length, 
            pNSInfo2->name.value, pNSInfo1->cksum, pNSInfo2->cksum);
    if((pNSInfo1->cksum == pNSInfo2->cksum) &&
       (pNSInfo1->name.length == pNSInfo2->name.length) && 
       (!strncmp(pNSInfo1->name.value, pNSInfo2->name.value, pNSInfo2->name.length)) )
    {
        return 0;
    }
    else
    {
        return 1; 
    }
}

static ClInt32T nameSvcEntryDetailsHashKeyCmp(ClCntKeyHandleT key1, 
                                              ClCntKeyHandleT key2)
{
    ClNameSvcBindingDetailsT   *pNSInfo1 = (ClNameSvcBindingDetailsT*)key1;
    ClNameSvcBindingDetailsT   *pNSInfo2 = (ClNameSvcBindingDetailsT*)key2;
#if 0
    ClUint64T                  objRef;
    ClUint16T                  addrType;
    ClUint16T                  objType;
                                                                                                                             
    addrType = pNSInfo2->objReference>>56;
    objType  = pNSInfo2->objReference>>32 & 0xFFFF;

    objRef = CL_NS_RESERVED_OBJREF;
    if((pNSInfo1->cksum == pNSInfo2->cksum) && 
       !memcmp(pNSInfo1->attr, pNSInfo2->attr,
              pNSInfo1->attrCount * sizeof(ClNameSvcAttrEntryT)))
    {
        if((addrType == CL_IOC_LOGICAL_ADDRESS_TYPE) && (objType  != 0))
            return 0;
        if(memcmp(&(pNSInfo2->objReference), &objRef,
                       sizeof(ClUint64T)))
        {
            if(!memcmp(&(pNSInfo1->objReference), &(pNSInfo2->objReference),
                       sizeof(ClUint64T)))
            {
                return 0;
            }
        }   
        else
        {
            return 0;
        }
    }
    return 1;
#endif
    return (pNSInfo1->cksum -  pNSInfo2->cksum);
}

static void nameSvcHashContextDeleteCallback(ClCntKeyHandleT userKey,
            ClCntDataHandleT userData)
{
    ClNameSvcContextInfoPtrT *ptr = (ClNameSvcContextInfoPtrT*)userData;
    if(ptr)
    clHeapFree(ptr);
    return;
}

static void nameSvcHashEntryDeleteCallback(ClCntKeyHandleT userKey,
            ClCntDataHandleT userData)
{
    ClNameSvcBindingT  *pNSInfo = (ClNameSvcBindingT*)userData;
    if(pNSInfo)
    {
        clLogDebug("SVR", "DEL", "Service Entry has been [%.*s] has been removed", 
                pNSInfo->name.length, pNSInfo->name.value);
        clCntDelete(pNSInfo->hashId);
        clHeapFree(pNSInfo);
    }
    return;
}

static void nameSvcHashEntryDetailsDeleteCallback(ClCntKeyHandleT userKey,
            ClCntDataHandleT userData)
{
    ClNameSvcBindingDetailsT *pNSInfo = (ClNameSvcBindingDetailsT*)userData;
    if(pNSInfo)
        clHeapFree(pNSInfo);
    return;
}


/**
 *  Name: _nameSvcContextGetCallback 
 *
 *  Walk funtion for getting context given contextMapCookie 
 *
 *  @param  userKey: hashid of the entry
 *          hashTable: context info
 *          userArg: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT _nameSvcContextGetCallback(ClCntKeyHandleT    userKey,
                                 ClCntDataHandleT   hashTable,
                                 ClCntArgHandleT    userArg,
                                 ClUint32T          dataLength)
{
    ClRcT                     ret       = CL_OK;
    ClNameSvcContextInfoPtrT  pStatInfo = (ClNameSvcContextInfoPtrT) hashTable;
    ClNameSvcCookieInfoT*     pData     = (ClNameSvcCookieInfoT*) userArg;

    gHashTableIndex =  (ClWordT)userKey;

    CL_FUNC_ENTER();

    if(pStatInfo->contextMapCookie == pData->cookie)
    {
        pData->contextId =  (ClWordT)userKey;
    } 

    CL_FUNC_EXIT();
    return ret;
}



/**
 *  Name: nameScvContextFromCookieGet 
 *
 *  Funtion for getting context given contextMapCookie 
 *
 *  @param  contextMapCookie: context map cookie 
 *          pContextId: will contain the obtained context id
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 *    CL_NS_ERR_CONTEXT_NOT_CREATED - context not found
 */

ClRcT nameSvcContextFromCookieGet(ClUint32T contextMapCookie, 
                                  ClUint32T* pContextId)
{
    ClRcT                ret        = CL_OK;
    ClNameSvcCookieInfoT cookieInfo = {0};
    CL_FUNC_ENTER();

    memset(&cookieInfo, '\0', sizeof(ClNameSvcCookieInfoT));
    cookieInfo.cookie = contextMapCookie;
    cookieInfo.contextId = CL_NS_CONTEXT_NOT_FOUND;
    
    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Couldnt get Lock successfully--\n"));
    }
    
    ret = clCntWalk(gNSHashTable, _nameSvcContextGetCallback, 
                    (ClCntArgHandleT) &cookieInfo, 
                    sizeof(ClNameSvcCookieInfoT));
    /* Release the semaphore */
    clOsalMutexUnlock(gSem);
    
    if(cookieInfo.contextId == CL_NS_CONTEXT_NOT_FOUND)
    {
        clLogError("SVR", "LOOKUP", "ContextMapCookie [%#X] to context find failed rc [0x %x]", cookieInfo.cookie, ret);
        CL_FUNC_EXIT();
        return CL_NS_RC(CL_NS_ERR_CONTEXT_NOT_CREATED);
   } 

   *pContextId = cookieInfo.contextId;
    CL_FUNC_EXIT();
    return ret;

}

ClRcT nameSvcContextGet(ClUint32T contextId,
                        ClNameSvcContextInfoPtrT *pCtxData)
{
    ClRcT rc = CL_OK;
    ClCntNodeHandleT pNodeHandle = 0;

    if(contextId == CL_NS_DEFT_GLOBAL_CONTEXT)
        contextId = CL_NS_DEFAULT_GLOBAL_CONTEXT;

    else if(contextId == CL_NS_DEFT_LOCAL_CONTEXT)
        contextId = CL_NS_DEFAULT_NODELOCAL_CONTEXT;
    
    rc = clCntNodeFind(gNSHashTable, (ClPtrT)(ClWordT)contextId, &pNodeHandle);
    if(rc != CL_OK)
        return rc;

    rc = clCntNodeUserDataGet(gNSHashTable, pNodeHandle, (ClCntDataHandleT*)pCtxData);
    
    return rc;
}


/**
 *  Name: nameSvcContextIdGet 
 *
 *  Funtion for getting next free context slot
 *
 *  @param  contextType: CL_NS_USER_GLOBAL/CL_NS_USER_NODELOCAL 
 *          pContextId : Context Id  
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *    CL_ERR_INVALID_PARAMETER - invalid context type
 *    CL_NS_ERR_LIMIT_EXCEEDED - no slot free [Ideally this should
 *                               never be returned]  
 */

ClRcT nameSvcContextIdGet(ClUint32T contextType, ClUint32T *pContextId)
{
    ClUint32T startLimit = 0;
    ClUint32T stopLimit  = 0;
    ClUint32T index      = 0
    CL_FUNC_ENTER();

    switch(contextType)
    {
        case CL_NS_USER_GLOBAL:
            startLimit = CL_NS_BASE_GLOBAL_CONTEXT; 
            stopLimit  = CL_NS_BASE_GLOBAL_CONTEXT +
                         nameSvcMaxNoGlobalContextGet();
            break;
        case CL_NS_USER_NODELOCAL:
            startLimit = CL_NS_BASE_NODELOCAL_CONTEXT;
            stopLimit  = CL_NS_BASE_NODELOCAL_CONTEXT +
                         nameSvcMaxNoLocalContextGet();
            break;
        default:
            CL_FUNC_EXIT();
            return CL_ERR_INVALID_PARAMETER;
    }
  
    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Couldnt get Lock successfully--\n"));
    }
    /* Find the free slot */
    for(index=startLimit; index<=stopLimit; index++)
    {
        if(gpContextIdArray[index] == CL_NS_SLOT_FREE)
        {
            *pContextId = index;
            gpContextIdArray[index] = CL_NS_SLOT_ALLOCATED; 
            /* Release the semaphore */
            clOsalMutexUnlock(gSem);
            CL_FUNC_EXIT();
            return CL_OK;
        }
        else
            continue;
    }   

    /* Release the semaphore */
    clOsalMutexUnlock(gSem);
    CL_FUNC_EXIT();
    return CL_NS_ERR_LIMIT_EXCEEDED;
} 
    


/**
 *  Name: _nameSvcEntryDisplayCallback 
 *
 *  This function is called for every entry in NS database during 
 *  binding listing related query. Prints information about all 
 *  object references associated with the given name
 *
 *  @param 
 *                       no. of references, no. of attributes associated)
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT _nameSvcEntryDisplayCallback(ClCntKeyHandleT   userKey,
                                   ClCntDataHandleT  nsInfo,
                                   ClCntArgHandleT   userArg,
                                   ClUint32T         dataLength)
{
    ClCharT                   nameCliStr[CL_NAME_MAX_ENTRY_LEN]="\0";
    ClNameSvcDisplayInfoT*    pWalkInfo = (ClNameSvcDisplayInfoT*)userArg;
    ClNameSvcBindingDetailsT* pNSInfo   = (ClNameSvcBindingDetailsT*)nsInfo;
    ClNameSvcCompListT*       pCompId   = &pNSInfo->compId;
    ClNameSvcAttrEntryT*      pAttr     = pNSInfo->attr;
    ClUint32T                 iCnt      = 0, jCnt = 0;
    ClUint32T                 addr[2]   = {0};
    

    addr[1] = pNSInfo->objReference>>32;
    addr[0] = pNSInfo->objReference & 0xFFFFFFFF;

    sprintf(nameCliStr, "\n Name: %10s  ObjReference: %06d:%06d  CompId: %04d"
                     "  EoID: %04lld  CompAddr: %04d:%04d Priority: %d", 
                     pWalkInfo->name.value,
                      addr[1],  addr[0],
                     pCompId->compId, pCompId->eoID, pCompId->nodeAddress,
                     pCompId->clientIocPort, pCompId->priority);

    
    if(pNSInfo->attrCount > 0)
    {
        for(iCnt=0; iCnt<pNSInfo->attrCount; iCnt++)
        {
            sprintf(nameCliStr+strlen(nameCliStr), "\t %10s:%10s", 
                            pAttr->type,  pAttr->value);
            pAttr++;
        }
    }
    for(iCnt=0; iCnt<pNSInfo->refCount; iCnt++)
    {
        pAttr = pNSInfo->attr;
        pCompId = pCompId->pNext;
        sprintf(nameCliStr+strlen(nameCliStr), "\n Name: %10s"
                    "  ObjReference: %06d:%06d  CompId: %04d  "
                    "EoID: %04lld  CompAddr: %04d:%04d Priority: %d ", 
                      pWalkInfo->name.value,
                      (ClUint32T) addr[1], (ClUint32T) addr[0],
                     pCompId->compId, pCompId->eoID, pCompId->nodeAddress,
                     pCompId->clientIocPort, pCompId->priority);
                                                                                                                             
        if(pNSInfo->attrCount > 0)
        {
            for(jCnt=0; jCnt<pNSInfo->attrCount; jCnt++)
            {
                sprintf(nameCliStr+strlen(nameCliStr), "\t %10s:%10s",
                            pAttr->type,  pAttr->value);
                pAttr++;
            }
        }
    }
    clBufferNBytesWrite (pWalkInfo->outMsgHdl, (ClUint8T*)&nameCliStr,
                                strlen(nameCliStr));      

    return CL_OK;
}



/**
 *  Name: _nameSvcEntryLevelWalkForDisplay 
 *
 *  This function is called for every entry in NS database during names 
 *  and binding listing related query.
 *
 *  @param 
 *                       no. of references, no. of attributes associated)
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT _nameSvcEntryLevelWalkForDisplay(ClCntKeyHandleT    userKey,
                                       ClCntDataHandleT  nsInfo,
                                       ClCntArgHandleT   userArg,
                                       ClUint32T         dataLength)
{
    ClNameSvcBindingT*     pNSInfo = (ClNameSvcBindingT*)nsInfo;
    ClNameSvcDisplayInfoT* pWalkInfo = (ClNameSvcDisplayInfoT*)userArg;
      
    switch(pWalkInfo->operation)
    {
#if 0
        case CL_NS_LIST_NAMES:
    ClCharT                nameCliStr[CL_NAME_MAX_ENTRY_LEN]="\0";
            sprintf(nameCliStr, "\n %s ",  pNSInfo->name.value);
            clBufferNBytesWrite (pWalkInfo->outMsgHdl, 
                                (ClUint8T*)&nameCliStr,
                                strlen(nameCliStr));      
            break;
#endif
        case CL_NS_LIST_BINDINGS:
            /* Copy the name & end with NULL character */
            clNameCopy(&pWalkInfo->name, &pNSInfo->name);
            clCntWalk((ClCntHandleT) pNSInfo->hashId,
                     _nameSvcEntryDisplayCallback,
                     (ClCntArgHandleT) pWalkInfo, dataLength);
            break;
        default:
            break;
    }
    
    return CL_OK;
}



ClRcT _nameSvcObjRefGenerate(ClUint64T *pLogicalAddr)
{
    sObjCount++;
    *pLogicalAddr = sObjCount;
    return CL_OK;
}


/*************************************************************************/
/*********************** NS core functionality ***************************/
/*************************************************************************/

/**
 *  Name: nameSvcNack
 *
 *  Function called if nace of version mismatch
 *
 *  @param  data: rmd data
 *          inMsgHandle: contains registration related information
 *          outMsgHandle: Not needed (rmd compliance)
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 */
                                                                                                                             
ClRcT VDECL(nameSvcNack)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                  ClBufferHandleT  outMsgHandle)
{
    ClRcT           ret      = CL_OK;
    ClNameSvcNackT  nackType = 0;
    ClNameVersionT  version  = {0};
    ClIocAddressT   srcAddr  = {{0}};
                                                                                                                             
    /* Extract version information */
    VDECL_VER(clXdrUnmarshallClNameVersionT, 4, 0, 0)(inMsgHandle, (void *)&version);
                                                                                                                             
    /* Extract nack type */
    clXdrUnmarshallClUint32T(inMsgHandle, (void *)&nackType);
                                                                                                                             
    clRmdSourceAddressGet(&srcAddr.iocPhyAddress);
    switch(nackType)
    {
        case CL_NS_NACK_REGISTER:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Registration failed at %x\n",
                           srcAddr.iocPhyAddress.nodeAddress));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                       CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
            break;
        case CL_NS_NACK_COMPONENT_DEREGISTER:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Component deregistration failed at %x\n",
                           srcAddr.iocPhyAddress.nodeAddress));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                       CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
            break;
        case CL_NS_NACK_SERVICE_DEREGISTER:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Service deregistration failed at %x\n",
                           srcAddr.iocPhyAddress.nodeAddress));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                       CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
            break;
        case CL_NS_NACK_CONTEXT_CREATE:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context create failed at %x\n",
                           srcAddr.iocPhyAddress.nodeAddress));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                       CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
            break;
        case CL_NS_NACK_CONTEXT_DELETE:
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context delete failed at %x\n",
                           srcAddr.iocPhyAddress.nodeAddress));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                       CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
            break;
        default:
            break;
    }
                                                                                                                             
    return ret;
}



/**
 *  Name: nameSvcRegister
 *
 *  Function for registering with NS
 *
 *  @param  data: rmd data
 *          inMsgHandle: contains registration related information
 *          outMsgHandle: Not needed (rmd compliance)   
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *    CL_ERR_INVALID_BUFFER    - when unable to retrieve info from inMsgHandle
 *    CL_ERR_NO_MEMORY         - malloc failure
 *    CL_NS_ERR_CONTEXT_NOT_CREATED - trying to register to nonexisting context
 *    CL_NS_ERR_LIMIT_EXCEEDED - max no. of entries limit reached
 */
 
ClRcT VDECL(nameSvcRegister)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                      ClBufferHandleT  outMsgHandle)
{
    ClRcT                    ret               = CL_OK;
    ClNameSvcBindingDetailsT *pNSEntry         = NULL;
    ClNameSvcBindingT        *pNSBinding       = NULL;
    ClUint32T                cksum             = 0;
    ClCntNodeHandleT         pNodeHandle       = 0;
    ClCntHandleT             pStoredInfo       = 0;
    ClNameSvcCompListT       *pTemp            = NULL;
    ClNameSvcBindingT        *pStoredNSBinding = NULL;
    ClNameSvcBindingDetailsT *pStoredNSEntry   = NULL;
    ClIocNodeAddressT        sdAddr            = 0;
    ClNameSvcCompListPtrT    pCompList         = NULL;
    ClNameSvcContextInfoPtrT pStatInfo         = NULL;
    ClUint32T                noEntries         = 0, iCnt = 0;
    ClIocNodeAddressT        *pAddrList        = NULL;
    ClNameSvcInfoIDLT        *nsInfo           = NULL;
    ClUint32T                inLen             = 0;
    ClBufferHandleT          inMsgHdl          = 0;
    ClNameSvcNameLookupT     lookupData        = {0};
    ClUint64T                generatedObjRef   = 0;
    ClNameVersionT           version           = {0};
    ClNameSvcDeregisInfoT    walkData          = {0};

    CL_FUNC_ENTER();
    /* Extract version information */
    VDECL_VER(clXdrUnmarshallClNameVersionT, 4, 0, 0)(inMsgHandle, (void *)&version);

    clXdrUnmarshallClUint32T(inMsgHandle, (void *)&inLen);

    nsInfo = (ClNameSvcInfoIDLT *)clHeapAllocate(inLen);

    if(nsInfo == NULL)
    {
        ret = CL_NS_RC(CL_ERR_NO_MEMORY);
        clLogError("SVR", "REG", "Service registration failed due to memory allocation rc [0x %x]",
                ret); 
        CL_FUNC_EXIT();
        return ret;
    }
    clBufferCreate (&inMsgHdl);
    VDECL_VER(clXdrUnmarshallClNameSvcInfoIDLT, 4, 0, 0)(inMsgHandle, (void *)nsInfo); 

    /* Version Verification */
    if(nsInfo->source != CL_NS_MASTER)
    {
        ret = clVersionVerify(&gNSClientToServerVersionInfo,(ClVersionT *)&version);
    }
    else
    {
        ret = clVersionVerify(&gNSServerToServerVersionInfo,(ClVersionT *)&version);
    }
    if(ret != CL_OK)
    {
        clBufferClear(inMsgHdl);
        clLogError("SVR", "REG", "Name service register failed due to invalid version rc[0x %x]", ret);

        if(nsInfo->source == CL_NS_MASTER)
        {
            ClNameSvcNackT           nackType = CL_NS_NACK_REGISTER;
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, inMsgHdl, 0);
            clXdrMarshallClUint32T(&nackType, inMsgHdl, 0);
            ret = clCpmMasterAddressGet(&gMasterAddress);
            if (ret != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",ret));
                return ret;
            }
            CL_NS_CALL_RMD_ASYNC(gMasterAddress, CL_NS_NACK, inMsgHdl,
                                 outMsgHandle, ret);
        }
        else
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, outMsgHandle, 0);

        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        clBufferDelete(&inMsgHdl);
        CL_FUNC_EXIT();
        return ret;
    }
    /* Find the context to add to */
    if(nsInfo->contextId == CL_NS_DEFT_GLOBAL_CONTEXT)
        nsInfo->contextId = CL_NS_DEFAULT_GLOBAL_CONTEXT;
    else if(nsInfo->contextId == CL_NS_DEFT_LOCAL_CONTEXT)
        nsInfo->contextId = CL_NS_DEFAULT_NODELOCAL_CONTEXT;

    clLogDebug("SVR", "REG", "Service Register [%.*s] contextId [%d]", 
             nsInfo->name.length, nsInfo->name.value, nsInfo->contextId);
    sdAddr = clIocLocalAddressGet();
    ret = clCpmMasterAddressGet(&gMasterAddress);
    if (ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",ret));
        return ret;
    }

    if((sdAddr != gMasterAddress) && 
       (nsInfo->contextId < CL_NS_BASE_NODELOCAL_CONTEXT) &&
       (nsInfo->source != CL_NS_MASTER))
    {
        clLogDebug("SVR", "REG", "Service [%.*s] register forwarding to master [%d]", 
                  nsInfo->name.length, nsInfo->name.value, gMasterAddress);
        nsInfo->source = CL_NS_LOCAL;
    
        /* Fill the message to be sent to the peers */    
        VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl, 0);
        clXdrMarshallClUint32T((void *)&inLen, inMsgHdl, 0);
        VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHdl, 0);
 
        CL_NS_CALL_RMD(gMasterAddress, CL_NS_REGISTER, inMsgHdl, outMsgHandle, ret);
        if(ret != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Registration failed \n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                       CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
        }

        /* delete the message created for updating NS/M */
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr != NULL)
            clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return ret;
    }
    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                  ("\n NS: Could not get Lock successfully------\n"));
    }

    /* Check whether the context is created or not */
    ret = clCntNodeFind(gNSHashTable, (ClPtrT)(ClWordT)nsInfo->contextId, &pNodeHandle);
    if(ret != CL_OK)
    {
        ret = CL_NS_RC(CL_NS_ERR_CONTEXT_NOT_CREATED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context %d not found"\
                             " in NS \n",  nsInfo->contextId));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret); 
        /* delete the message created for updating peers */
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr != NULL)
            clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        CL_FUNC_EXIT();
        return ret;
    }
                                                                                                                             
    ret = clCntNodeUserDataGet(gNSHashTable,pNodeHandle,
                                (ClCntDataHandleT*)&pStatInfo);
    pStoredInfo = pStatInfo->hashId;
    if(pStatInfo->entryCount >= gpConfig.nsMaxNoEntries)
    {
        ret = CL_NS_RC(CL_NS_ERR_LIMIT_EXCEEDED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Max no. of entries per "\
                             "context limit reached \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
        /* delete the message created for updating peers */
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr != NULL)
            clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        CL_FUNC_EXIT();
        return ret;
    }
    clLogDebug("SVR", "REG", "Service [%.*s] register, 2nd level hashId [%p]", 
               nsInfo->name.length, nsInfo->name.value, (void *)pStoredInfo);
 
    /* Check whether the entry for the service already exists or not */
    clNSLookupKeyForm(&nsInfo->name, &lookupData);
    /* Copy the name & end with NULL character */
    clLogDebug("SVR", "REG", "Service [%.*s] register, cksum [%u]",
                lookupData.name.length, lookupData.name.value, lookupData.cksum);

    ret = clCntNodeFind((ClCntHandleT)pStoredInfo, 
                        (ClCntKeyHandleT)&lookupData, &pNodeHandle);
    if(ret != CL_OK)
    { 
        /* Create the entry for the service */
        pNSBinding = (ClNameSvcBindingT *) clHeapAllocate
                             (sizeof(ClNameSvcBindingT));
        if(pNSBinding == NULL)
        {
            ret = CL_NS_RC(CL_ERR_NO_MEMORY);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
            /* delete the message created for updating peers */
            clBufferDelete(&inMsgHdl);
            if(nsInfo->attr != NULL)
                clHeapFree(nsInfo->attr);
            clHeapFree(nsInfo);
            /* Release the semaphore */
            clOsalMutexUnlock(gSem);
            CL_FUNC_EXIT();
            return ret;
        }
        /* Copy the name and append null character */
        clNameCopy(&pNSBinding->name, &nsInfo->name);
        pNSBinding->refCount = 0;
        pNSBinding->cksum    = lookupData.cksum;

        /* Create the entry details hash table. Remember there can be multiple
           obj references associated with a service */ 
        ret = clCntHashtblCreate(CL_NS_MAX_NO_OBJECT_REFERENCES, 
                          nameSvcEntryDetailsHashKeyCmp,
                          nameSvcEntryDetailsHashFunction,
                          nameSvcHashEntryDetailsDeleteCallback, 
                          nameSvcHashEntryDetailsDeleteCallback,
                          CL_CNT_UNIQUE_KEY, &pNSBinding->hashId);
        pStoredNSBinding     = pNSBinding;
        pNSBinding->priority = CL_NS_TEMPORARY_PRIORITY; 

        /* Add the entry */
        ret = clCntNodeAdd((ClCntHandleT)pStoredInfo, 
                      (ClCntKeyHandleT)pNSBinding, 
                      (ClCntDataHandleT)pNSBinding, NULL);
        /*
         * Create the dataSet and Write into it. 
         * dsIdInfo = CL_NAME_SVC_ENTRY. ramesh
         */
        if( (nsInfo->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT) 
            || ( sdAddr == gMasterAddress ) )
        {
            ret = clNameSvcBindingDataWrite(nsInfo->contextId, 
                    pStatInfo, pNSBinding);
            if( CL_OK != ret )
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("clLogNameSvcBindingDataCheckpoint(): rc[0x %x]", ret));
            }    
            ret = clNameSvcPerCtxInfoWrite(nsInfo->contextId, pStatInfo);
            if( CL_OK != ret )
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("clNameSvcPerCtxInfoWrite(): rc[0x %x]", ret));
            }    
        }    
        clLogDebug("SVR", "REG", "Added service entry [%.*s] to the table", 
                pStoredNSBinding->name.length, pStoredNSBinding->name.value);
    }
    else
    {
        /* Found the entry */
        
        ret = clCntNodeUserDataGet(pStoredInfo,pNodeHandle,
                                (ClCntDataHandleT*)&pStoredNSBinding);
    
    }    

    /* Delete the previous entries if the eoID of the entry is different
     * from the one that we got in the register request. This is to avoid
     * duplicate entries in case of component death and restarts before
     * comp death event is received and processed.
     */
    gNameEntryDelete   = 0;
    gDelete            = 0;
    walkData.compId    = nsInfo->compId;
    walkData.eoID      = nsInfo->eoID;
    walkData.operation = CL_NS_EO_DEREGISTER_OP;
    walkData.contextId = nsInfo->contextId;
 
    ret = clCntWalk(gNSHashTable, _nameSvcContextLevelWalkForDelete, 
                    (ClCntArgHandleT) &walkData, sizeof(ClUint32T));

    /* Allocate and fill entry details */
    pNSEntry = (ClNameSvcBindingDetailsT *) clHeapAllocate 
                      (sizeof(ClNameSvcBindingDetailsT)+
                       nsInfo->attrCount * sizeof(ClNameSvcAttrEntryT));
    if(pNSEntry == NULL)
    {
        ret = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
        /* delete the message created for updating peers */
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr != NULL)
            clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        clHeapFree(pNSBinding);
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        CL_FUNC_EXIT();
        return ret;
    }

    if(nsInfo->objReference == CL_NS_GET_OBJ_REF)
    {
        _nameSvcObjRefGenerate(&generatedObjRef);
        pNSEntry->objReference = generatedObjRef;
    }
    else
    {
        sObjCount++;
        pNSEntry->objReference    = nsInfo->objReference;
        generatedObjRef           = nsInfo->objReference;
    }

    pNSEntry->refCount               = 0;
    pNSEntry->compId.priority        = nsInfo->priority;
    pNSEntry->compId.compId          = nsInfo->compId;
    pNSEntry->compId.eoID            = nsInfo->eoID;
    pNSEntry->compId.nodeAddress     = nsInfo->nodeAddress;
    pNSEntry->compId.clientIocPort   = nsInfo->clientIocPort;
    pNSEntry->compId.pNext           = NULL;
    pNSEntry->attrCount              = nsInfo->attrCount;

    pNSEntry->attrLen                = nsInfo->attrCount * 
                                         sizeof(ClNameSvcAttrEntryT);

    if(nsInfo->attrCount > 0)
    {
        memcpy(pNSEntry->attr, nsInfo->attr, nsInfo->attrCount * 
               sizeof(ClNameSvcAttrEntryT));
        clCksm32bitCompute ((ClUint8T *)nsInfo->attr, 
               nsInfo->attrCount * sizeof(ClNameSvcAttrEntryT), 
               &cksum);
         pNSEntry->cksum =  cksum;
    }
    else
         pNSEntry->cksum = 0;

    /* Add entry details to the entry details hash table */
    ret=clCntNodeAdd((ClCntHandleT)pStoredNSBinding->hashId, 
                     (ClCntKeyHandleT )  pNSEntry, 
                     (ClCntDataHandleT ) pNSEntry, NULL);
    if(ret != CL_OK)
    {
        if(ret == CL_RC(CL_CID_CNT, CL_ERR_DUPLICATE))
        {
            /* Details already present in the entry details hash table */
            ret = clCntNodeFind((ClCntHandleT)pStoredNSBinding->hashId, 
                                (ClCntKeyHandleT )pNSEntry,
                                 &pNodeHandle);
            ret = clCntNodeUserDataGet(pStoredNSBinding->hashId,pNodeHandle,
                                (ClCntDataHandleT*)&pStoredNSEntry);
            /*
             * No need to check return types as we are sure that 
             * node does exist
             */
            pTemp = &(pStoredNSEntry->compId);
            while(pTemp != NULL)
            {
                if(pTemp->compId == pNSEntry->compId.compId) 
                {
                    /* Duplicate entry case. Silently ignore the message */
                    /* Release the semaphore */
                    clOsalMutexUnlock(gSem);
                    clHeapFree(pNSEntry);
                    if(nsInfo->attr != NULL)
                        clHeapFree(nsInfo->attr);
                    clHeapFree(nsInfo);
                    /* delete the message created for updating peers */
                    clBufferDelete(&inMsgHdl);
                    clXdrMarshallClUint64T((void *)&pStoredNSEntry->objReference, 
                                            outMsgHandle, 0); 
                    CL_FUNC_EXIT();
                    return CL_OK;
                }
                pTemp = pTemp->pNext;
            }

            /* This is a reference to already existing entry */         
            pStoredNSEntry->refCount++;
            pCompList = (ClNameSvcCompListPtrT) 
                        clHeapAllocate(sizeof(ClNameSvcCompListT));
            if(pCompList == NULL)
            {
                ret = CL_NS_RC(CL_ERR_NO_MEMORY);
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
                /* delete the message created for updating peers */
                clBufferDelete(&inMsgHdl);
                if(nsInfo->attr != NULL)
                    clHeapFree(nsInfo->attr);
                clHeapFree(nsInfo);
                /* Release the semaphore */
                clOsalMutexUnlock(gSem);
                clHeapFree(pNSEntry);
                clHeapFree(pNSBinding);
                CL_FUNC_EXIT();
                return ret;
            }

            pCompList->pNext    = pStoredNSEntry->compId.pNext;
            pCompList->priority = nsInfo->priority; 
            if(pCompList->priority > pStoredNSBinding->priority)
                pStoredNSBinding->priority = pCompList->priority;
            pStoredNSEntry->compId.pNext = pCompList;
            pCompList->compId = pNSEntry->compId.compId;
            pCompList->eoID = pNSEntry->compId.eoID;
            pCompList->nodeAddress = pNSEntry->compId.nodeAddress;
            pCompList->clientIocPort = pNSEntry->compId.clientIocPort;
            generatedObjRef = pStoredNSEntry-> objReference;
            /*
             * Create the dataset for compInfo.
             * dsIdInfo = compInfo.
             */
            if( (nsInfo->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT) 
            || ( sdAddr == gMasterAddress ) )
            {    
                ret = clNameSvcCompInfoWrite(nsInfo->contextId, pStatInfo,
                        pStoredNSBinding, pNSEntry);
                if( CL_OK != ret )
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("clNameSvcCompInfoCheckpoint(): rc[0x %x]", ret));
                }    
                pCompList->dsId = pNSEntry->compId.dsId;
                ret = clNameSvcPerCtxInfoWrite(nsInfo->contextId, pStatInfo);
                if( CL_OK != ret )
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("clNameSvcPerCtxInfoWrite(): rc[0x %x]", ret));
                }    
            }
            clHeapFree(pNSEntry);
        }
        else
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Registration request failed \n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_NS_REGISTRATION_FAILED, ret);
            /* delete the message created for updating peers */
            clBufferDelete(&inMsgHdl);
            if(nsInfo->attr != NULL)
                clHeapFree(nsInfo->attr);
            clHeapFree(nsInfo);
            /* Release the semaphore */
            clOsalMutexUnlock(gSem);
            clHeapFree(pNSEntry);
            clHeapFree(pNSBinding);
            CL_FUNC_EXIT();
            return ret;
        }
    }
    else 
    {
        /* Cache the details of component with highest priority for the
           given service. This will be useful when query for component 
           with highest priority foe a given service is made */ 
        if((pNSEntry->compId.priority > pStoredNSBinding->priority) ||
           (pStoredNSBinding->priority == CL_NS_TEMPORARY_PRIORITY))
        {
            (void)clCntNodeFind((ClCntHandleT)pStoredNSBinding->hashId,
                             (ClCntKeyHandleT )pNSEntry,
                              &pStoredNSBinding->nodeHdl);
        
            pStoredNSBinding->priority = pNSEntry->compId.priority; 
        }
        pStatInfo->entryCount++;
        pStoredNSBinding->refCount++;

        clLogDebug("SVR", "REG", "Adding attr entry [%.*s] to the table, refCount [%d] serviceCnt [%d]", 
                 pStoredNSBinding->name.length, pStoredNSBinding->name.value, 
                 pStatInfo->entryCount, pStoredNSBinding->refCount);
        /*
         * checkpoint the bindingDetails info here.
         * dsIdInfo - bindingDetailsInfo. ramesh.
         */
            if( (nsInfo->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT) 
                || (sdAddr == gMasterAddress) )
            {
                ret = clNameSvcBindingDetailsWrite(nsInfo->contextId, pStatInfo,
                        pStoredNSBinding, pNSEntry);
                if( CL_OK != ret )
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("clNameSvcBindingDetaCheckpoint(): rc[0x %x]", ret));
                }    
                ret = clNameSvcPerCtxInfoWrite(nsInfo->contextId, pStatInfo);
                if( CL_OK != ret )
                {
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                            ("clNameSvcPerCtxInfoWrite(): rc[0x %x]", ret));
                }    
            }
    }
    /* Update slaves */ 
    if((nsInfo->source != CL_NS_MASTER) && 
       (nsInfo->contextId < CL_NS_BASE_NODELOCAL_CONTEXT))
    { 
        nsInfo->source = CL_NS_MASTER;
        ret = clIocTotalNeighborEntryGet(&noEntries);
        if(ret == CL_OK)
        {
            pAddrList = (ClIocNodeAddressT *)
                        clHeapAllocate(sizeof(ClIocNodeAddressT)*noEntries);
            clIocNeighborListGet(&noEntries,pAddrList);
            for(iCnt=0; iCnt<noEntries; iCnt++)
            {
                ClIocNodeAddressT addr = *(pAddrList+iCnt);
                if(addr == clIocLocalAddressGet())
                {
                    continue;
                }
                nsInfo->objReference = generatedObjRef;
                CL_NS_SERVER_SERVER_VERSION_SET(&version);
                VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl, 0);
                clXdrMarshallClUint32T((void *)&inLen, inMsgHdl, 0);
                VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHdl, 0);
                clBufferNBytesWrite (inMsgHdl, (ClUint8T*)nsInfo,
                                            inLen);
                CL_NS_CALL_RMD_ASYNC(addr, CL_NS_REGISTER, inMsgHdl, NULL, ret);
            }
            clHeapFree(pAddrList);
        }
        else
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Couldnt update the peers \n"));
    
        clXdrMarshallClUint64T((void *)&generatedObjRef, outMsgHandle, 0);
    }
    else
        clXdrMarshallClUint64T((void *)&nsInfo->objReference, outMsgHandle, 0);
    clLogInfo("SVR", "INI", "Name service [%.*s] has been registered object reference [%#llX]", 
               nsInfo->name.length, nsInfo->name.value, generatedObjRef);

    /* delete the message created for updating peers */
    clBufferDelete(&inMsgHdl);
    if(nsInfo->attr != NULL)
        clHeapFree(nsInfo->attr);
    clHeapFree(nsInfo);
    /* Release the semaphore */
    clOsalMutexUnlock(gSem);
    CL_FUNC_EXIT();
    return CL_OK;
}


/**
 *  Name: _nameSvcNextTopPriorityGetCallback 
 *
 *  Gets the key details of the component with highest priority
 *  for a given service. Gets called after service/component
 *  deregistration.   
 *
 *  @param  userKey: hashid of the entry
 *          nsInfo: entry info
 *          arg: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT _nameSvcNextTopPriorityGetCallback(ClCntKeyHandleT    userKey,
                                         ClCntDataHandleT   nsInfo,
                                         ClCntArgHandleT    arg,
                                         ClUint32T          dataLength)
{
    ClRcT                     ret        = CL_OK;
    ClNameSvcBindingDetailsT  *pNSInfo   = (ClNameSvcBindingDetailsT*)nsInfo;
    ClNameSvcBindingT         *pWalkInfo = (ClNameSvcBindingT*)arg;        
    ClUint32T                 priority   =  0;
    ClNameSvcCompListT        *pTemp     = &pNSInfo->compId;   
    
    if(userKey == gUserKey)
        return CL_OK;

    priority = pNSInfo->compId.priority;

    /* Find the highest priority component among all the references*/
    while(pTemp->pNext != NULL)
    {
        pTemp = pTemp->pNext;
        if(pTemp->priority > priority)
            priority = pTemp->priority;
    } 

    /* Update the key details if the given node's priority is hihger */
    if(priority >= pWalkInfo->priority)
    {
        pWalkInfo->priority = priority;
        ret = clCntNodeFind(pWalkInfo->hashId, userKey, &pWalkInfo->nodeHdl);
        if(ret != CL_OK)
            return ret;
    }
    
    return CL_OK;
}


/**
 *  Name: _nameSvcMatchedEntryDeleteCallback 
 *
 *  Callback function for deleting the matched entry in 
 *  a given context
 *
 *  @param  userKey: hashid of the entry
 *          hashTable: entry info
 *          userArg: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT _nameSvcMatchedEntryDeleteCallback(ClCntKeyHandleT    userKey,
                                         ClCntDataHandleT   nsInfo,
                                         ClCntArgHandleT    arg,
                                         ClUint32T          dataLength)
{
    ClRcT                     ret       = CL_OK;
    ClNameSvcBindingDetailsT  *pNSInfo  = (ClNameSvcBindingDetailsT*)nsInfo;
    ClNameSvcCompListT        *pTemp    = NULL;
    ClNameSvcCompListT        *pTemp1   = &pNSInfo->compId;
    ClUint32T                 index     = 0;
    ClNameSvcEventInfoT       eventInfo; 
    ClEventIdT                eventId;
    ClNameSvcDeregisInfoT*    userArg   = (ClNameSvcDeregisInfoT*)arg;
    ClUint32T                 op        = userArg->operation;
    ClCntNodeHandleT          pNodeHandle, pTempHandle;
    ClNameSvcBindingT         walkData;
    ClUint32T                 dataLen   = sizeof(ClNameSvcBindingT);

    if((gDelete == 1))
    {
        ClNameSvcBindingDetailsT *prevBinding = NULL;
        /* Delete the previously marked entry */
        ret = clCntNodeFind(gHashTableHdl, gUserKey, &pNodeHandle);
        if(ret != CL_OK)
            return ret;

        clCntNodeUserDataGet(gHashTableHdl, pNodeHandle,
                             (ClCntDataHandleT*)&prevBinding);
        if(prevBinding
           &&
           (
            userArg->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT
            ||
            clCpmIsMaster()))
        {
            clNameSvcDataSetDelete(userArg->contextId, prevBinding->dsId);
            clNameSvcPerCtxDataSetIdPut(userArg->contextId, prevBinding->dsId);
        }

        ret = clCntNodeDelete(gHashTableHdl, pNodeHandle);
        if(ret != CL_OK)
            return ret;
        /*
         * Binding Details Entry is getting removed.
         *FIXME
         */
        gDelete = 0;
    }
   
    while(pTemp1 != NULL)
    {
        if (((op == CL_NS_NODE_DEREGISTER_OP) && (pTemp1->nodeAddress == userArg->nodeAddress)) ||
                ((op != CL_NS_NODE_DEREGISTER_OP) && (pTemp1->compId == userArg->compId)))
        {
            if(((op == CL_NS_EO_DEREGISTER_OP) && (pTemp1->eoID == userArg->eoID)) ||
               ((op == CL_NS_COMP_DEATH_DEREGISTER_OP) && (pTemp1->eoID != userArg->eoID)))
            {
                pTemp = pTemp1;
                pTemp1 = pTemp1->pNext;
                index ++;
                continue;
            }

            if(op == CL_NS_SERVICE_DEREGISTER_OP)
                gFound = 1;
   
            if(pNSInfo->refCount >= 1)
            {
                /* More than one references to a given service */
                pNSInfo->refCount--;
                if(index == 0)
                {
                    pTemp1->compId = pTemp1->pNext->compId;
                    pTemp = pTemp1->pNext;
                    pTemp1->pNext = pTemp->pNext;
                    if(userArg->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT
                       ||
                       clCpmIsMaster())
                    {
                        clNameSvcDataSetDelete(userArg->contextId, pTemp->dsId);
                    }
                    clNameSvcPerCtxDataSetIdPut(userArg->contextId, pTemp->dsId);
                    clHeapFree(pTemp);
                    return ret;
                }
                pTemp->pNext = pTemp1->pNext;
                /*
                 * Component deregister. delete the dsId from ckpt/
                 */
                if( (userArg->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT) 
                    || 
                    clCpmIsMaster())
                {    
                    ret = clNameSvcDataSetDelete(userArg->contextId, pTemp1->dsId);
                    if( CL_OK != ret )
                    {
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                                ("clNameSvcDataSetDelete() :datsSet [%d] rc[0x %x]", pTemp1->dsId, ret));
                    }    
                    clNameSvcPerCtxDataSetIdPut(userArg->contextId, pTemp1->dsId);
                }
                clHeapFree(pTemp1);
                 
                return ret;
            }
            else
            {
                gUserKey = userKey;
                gDelete = 1;
                gHashTableHdl = userArg->nameEntry->hashId;
                ret = clCntNodeFind(gHashTableHdl, gUserKey, &pTempHandle);
                if(userArg->nameEntry->nodeHdl == pTempHandle)
                {
                    /* Cache the next highest priority service provider */
                    memset(&walkData, 0, dataLen);
                    walkData.hashId = userArg->nameEntry->hashId;
                    
                    ret = clCntWalk((ClCntHandleT) userArg->nameEntry->hashId,
                                    _nameSvcNextTopPriorityGetCallback,
                                    (ClCntArgHandleT) &walkData, dataLen);
                    if(ret != CL_OK)
                        return ret;
                     
                    userArg->nameEntry->nodeHdl  = walkData.nodeHdl;
                    userArg->nameEntry->priority = walkData.priority;
                    
                }
                userArg->nameEntry->refCount--;
                if(userArg->nameEntry->refCount == 0)
                {
                   userArg->hashTable->entryCount--;
                   if (op != CL_NS_EO_DEREGISTER_OP)
                       gNameEntryDelete        = 1;
                   gNameEntryUserKey       = userArg->nameEntryUserKey;
                   gNameEntryHashTable     = userArg->hashTable->hashId;
                   /*
                    * Delete the service entry.
                    */ 
                }
                /* Inform the NS users */
                eventInfo.contextMapCookie = htonl(userArg->hashTable->
                                             contextMapCookie);
                eventInfo.operation        = htonl(userArg->operation);
                eventInfo.objReference     = 
                        _clNameHostUint64toNetUint64(pNSInfo->objReference);
                clNameCopy(&eventInfo.name, &userArg->nameEntry->name);
                eventInfo.name.length = htons(eventInfo.name.length);
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Publishing an event for"
                                     " service unavailability \n"));
                clEventPublish(gEventHdl, (const void*)&eventInfo, 
                         sizeof(ClNameSvcEventInfoT), &eventId);

            }
        } 
        pTemp = pTemp1;
        pTemp1 = pTemp1->pNext;
        index ++;
    }
   
        
    return ret;
}


/**
 *  Name: _nameSvcEntryLevelWalkForDelete 
 *
 *  For walking on all entries registered in
 *  entry details hash table
 * 
 *  @param  userKey: hashid of the context
 *          nsBinding: binding details info
 *          arg: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT _nameSvcEntryLevelWalkForDelete(ClCntKeyHandleT    userKey,
                                      ClCntDataHandleT   nsBinding,
                                      ClCntArgHandleT    arg,
                                      ClUint32T          dataLength)
{
    ClRcT                    ret         = CL_OK;
    ClNameSvcBindingT        *pNSBinding = (ClNameSvcBindingT*)nsBinding;
    ClNameSvcDeregisInfoT    *pWalkData  = (ClNameSvcDeregisInfoT*) arg;
    ClCntNodeHandleT         pNodeHandle = 0 ;

    if(gNameEntryDelete == 1)
    {
        /* Delete the previously marked entry */
        ClNameSvcBindingT *prevNSBinding = NULL;
        ret = clCntNodeFind(gNameEntryHashTable, gNameEntryUserKey, &pNodeHandle);
        if(ret != CL_OK)
            return ret;
        clCntNodeUserDataGet(gNameEntryHashTable, pNodeHandle, 
                             (ClCntDataHandleT*)&prevNSBinding);
        if(prevNSBinding 
           &&
           (
            pWalkData->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT
            ||
            clCpmIsMaster()))
        {
            clNameSvcDataSetDelete(pWalkData->contextId, prevNSBinding->dsId);
            clNameSvcPerCtxDataSetIdPut(pWalkData->contextId, prevNSBinding->dsId);
        }
           
        ret = clCntNodeDelete(gNameEntryHashTable, pNodeHandle);
        if(ret != CL_OK)
            return ret;

        gNameEntryDelete        = 0;
        gNameEntryHashTable     = 0;
        gNameEntryUserKey       = 0;
    }
    pWalkData->nameEntry        = pNSBinding; 
    pWalkData->nameEntryUserKey = userKey; 
  
    if(pWalkData->operation == CL_NS_CONTEXT_DELETE)
    {
        ret = clCntWalk((ClCntHandleT) pNSBinding->hashId,
                        _nameSvcEntryDeleteCallback,
                        (ClCntArgHandleT) pWalkData, dataLength);
    } else {
        ret = clCntWalk((ClCntHandleT) pNSBinding->hashId, 
                        _nameSvcMatchedEntryDeleteCallback, 
                        (ClCntArgHandleT) pWalkData, dataLength);
    }
    if(gDelete == 1)
    {
        /* Delete the previously marked entry */
        ClNameSvcBindingDetailsT *prevBinding = NULL; 
        ret = clCntNodeFind(gHashTableHdl, gUserKey, &pNodeHandle);
        if(ret != CL_OK)
            return ret;
        clCntNodeUserDataGet(gHashTableHdl, pNodeHandle,
                             (ClCntDataHandleT*)&prevBinding);
        if(prevBinding
           &&
           (
            pWalkData->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT
            ||
            clCpmIsMaster()))
        {
            clNameSvcDataSetDelete(pWalkData->contextId, prevBinding->dsId);
            clNameSvcPerCtxDataSetIdPut(pWalkData->contextId, prevBinding->dsId);
        }
        ret = clCntNodeDelete(gHashTableHdl, pNodeHandle);
        if(ret != CL_OK)
            return ret;

        gHashTableHdl   = 0;
        gDelete         = 0;
        gHashTableIndex = 0;
    }

    return ret;
}



/**
 *  Name: _nameSvcContextLevelWalkForDelete 
 *
 *  For walking on all the contexts for the purpose of entry 
 *  deregistration/deletion
 * 
 *  @param  userKey: hashid of the context
 *          hashTable: context info
 *          userArg: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT _nameSvcContextLevelWalkForDelete(ClCntKeyHandleT    userKey,
                                        ClCntDataHandleT   hashTable,
                                        ClCntArgHandleT    userArg,
                                        ClUint32T          dataLength)
{
    ClRcT ret = CL_OK;
    ClNameSvcContextInfoPtrT pStatInfo   = (ClNameSvcContextInfoPtrT) hashTable;
    ClCntNodeHandleT         pNodeHandle = 0;
    ClNameSvcDeregisInfoT*   pWalkData   = (ClNameSvcDeregisInfoT*) userArg;

    if(!pStatInfo || !pWalkData) return CL_OK;
    
    pWalkData->hashTable = pStatInfo;
    gHashTableIndex      = (ClUint32T)(ClWordT) userKey;
    pWalkData->contextId = (ClUint32T)(ClWordT)userKey; 

    /* Walk the entry details hash table */
    ret = clCntWalk((ClCntHandleT) pStatInfo->hashId, 
                     _nameSvcEntryLevelWalkForDelete, 
                     (ClCntArgHandleT) pWalkData, dataLength);

    if(gNameEntryDelete == 1)
    {
        ClNameSvcBindingT *prevNSBinding = NULL;

        /* Delete the previously marked entry */
        ret = clCntNodeFind(pStatInfo->hashId, gNameEntryUserKey, &pNodeHandle);
        if(ret != CL_OK)
            return ret;
        
        clCntNodeUserDataGet(gNameEntryHashTable, pNodeHandle, 
                                   (ClCntDataHandleT*)&prevNSBinding);
        if(prevNSBinding 
           &&
           (
            pWalkData->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT
            ||
            clCpmIsMaster()))
        {
            clNameSvcDataSetDelete(pWalkData->contextId, prevNSBinding->dsId);
            clNameSvcPerCtxDataSetIdPut(pWalkData->contextId, prevNSBinding->dsId);
        }

        ret = clCntNodeDelete(pStatInfo->hashId, pNodeHandle);
        if(ret != CL_OK)
            return ret;
        gNameEntryDelete    = 0;
        gNameEntryHashTable = 0;
        gNameEntryUserKey   = 0;
    }

    return ret;
}



/**
 *  Name: nameSvcComponentDeregister 
 *
 *  Function for deregistering all the services registered by a given
 *  component.
 *
 *  @param  data: rmd data
 *          inMsgHandle: contains deregistration related information
 *          outMsgHandle: Not needed (rmd compliance)   
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *    CL_ERR_INVALID_BUFFER    - when unable to retrieve info from inMsgHandle
 *    CL_ERR_NO_MEMORY         - malloc failure
 */
ClRcT VDECL(nameSvcComponentDeregister)(ClEoDataT data,  
                                 ClBufferHandleT  inMsgHandle,
                                 ClBufferHandleT  outMsgHandle)
{
    ClRcT                   ret       = CL_OK;
    ClIocNodeAddressT       sdAddr    = 0;
    ClUint32T               noEntries = 0, iCnt = 0;
    ClIocNodeAddressT*      pAddrList = NULL;
    ClNameSvcInfoIDLT*      nsInfo    = NULL;
    ClBufferHandleT         inMsgHdl  = 0;
    ClNameSvcDeregisInfoT   walkData  = {0};
    ClNameVersionT          version   = {0};

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside nameSvcComponentDeregister \n"));

    /* Extract Version information */
    VDECL_VER(clXdrUnmarshallClNameVersionT, 4, 0, 0)(inMsgHandle, (void *)&version);

    nsInfo = (ClNameSvcInfoIDLT *)clHeapAllocate(sizeof(ClNameSvcInfoIDLT));
    if(nsInfo == NULL)
    {
        ret = CL_NS_RC(CL_ERR_NO_MEMORY);    
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Malloc failed \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_COMPONENT_DEREGIS_FAILED, ret); 
        CL_FUNC_EXIT();
        return ret;
    }

    clBufferCreate (&inMsgHdl);
    VDECL_VER(clXdrUnmarshallClNameSvcInfoIDLT, 4, 0, 0)(inMsgHandle, (void *)nsInfo); 
    /* Version Verification */
    if(nsInfo->source != CL_NS_MASTER)
    {
        ret = clVersionVerify(&gNSClientToServerVersionInfo,(ClVersionT *)&version);
    }
    else
    {
        ret = clVersionVerify(&gNSServerToServerVersionInfo,(ClVersionT *)&version);
    }
    if(ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Version not suppoterd \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_COMPONENT_DEREGIS_FAILED, ret); 

        clBufferClear(inMsgHdl);

        if(nsInfo->source == CL_NS_MASTER)
        {
            ClNameSvcNackT           nackType = CL_NS_NACK_COMPONENT_DEREGISTER;
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, inMsgHdl, 0);
            clXdrMarshallClUint32T(&nackType, inMsgHdl, 0);
            ret = clCpmMasterAddressGet(&gMasterAddress);
            if (ret != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",ret));
                return ret;
            }
            CL_NS_CALL_RMD_ASYNC(gMasterAddress, CL_NS_NACK, inMsgHdl,
                                 outMsgHandle, ret);
        }
        else
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, outMsgHandle, 0);

        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return ret;
    }

    /* Sending request to master */
    sdAddr = clIocLocalAddressGet();
    ret = clCpmMasterAddressGet(&gMasterAddress);
    if (ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",ret));
        return ret;
    }
    if((sdAddr != gMasterAddress) && 
       (nsInfo->source != CL_NS_MASTER))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n request has come to slave."
                             " Forwarding it to master... \n"));
        VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl, 0);
        nsInfo->source = CL_NS_LOCAL;
        VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHdl, 0);        
        CL_NS_CALL_RMD(gMasterAddress, CL_NS_COMPONENT_DEREGISTER, inMsgHdl,\
                    NULL, ret);
        if(ret != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                       CL_NS_LOG_1_COMPONENT_DEREGIS_FAILED, ret); 
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: NS deregister failed \n"));
        }
        /* delete the message created for updating NS/M */
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return ret;
    }

    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Couldnt get Lock successfully------\n"));
    }
    
    gNameEntryDelete   = 0;
    gDelete            = 0;
    walkData.compId    = nsInfo->compId;
    walkData.operation = CL_NS_COMPONENT_DEREGISTER_OP;
    walkData.contextId = nsInfo->contextId;
 
    ret = clCntWalk(gNSHashTable, _nameSvcContextLevelWalkForDelete, 
                    (ClCntArgHandleT) &walkData, sizeof(ClUint32T));

    /* Update the slaves */
    if(nsInfo->source != CL_NS_MASTER) 
    {
        nsInfo->source = CL_NS_MASTER;
        ret = clIocTotalNeighborEntryGet(&noEntries);
        if(ret == CL_OK)
        {
            pAddrList = (ClIocNodeAddressT *)clHeapAllocate
                                (sizeof(ClIocNodeAddressT)*noEntries);
            clIocNeighborListGet(&noEntries,pAddrList);
            for(iCnt=0; iCnt<noEntries; iCnt++)
            {
                ClIocNodeAddressT addr = *(pAddrList+iCnt);
                if(addr == clIocLocalAddressGet())
                {
                    continue;
                }
                CL_NS_SERVER_SERVER_VERSION_SET(&version);
                VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl, 0);
                VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHdl, 0);        
                CL_NS_CALL_RMD_ASYNC(addr, CL_NS_COMPONENT_DEREGISTER, 
                                  inMsgHdl, NULL, ret);
            }
            clHeapFree(pAddrList);
        }
        else
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Couldnt update the peers \n"));

    }

    /* Release the semaphore */
    clOsalMutexUnlock(gSem);

    /* delete the message created for updating peers */
    clBufferDelete(&inMsgHdl);
    if(nsInfo->attr) clHeapFree(nsInfo->attr);
    clHeapFree(nsInfo);
    CL_FUNC_EXIT();
    return CL_OK;
}


/**
 *  Name: nameSvcServiceDeregister 
 *
 *  Function for deregistering a given services registered by a given
 *  component in a given context
 *
 *  @param  data: rmd data
 *          inMsgHandle: contains deregistration related information
 *          outMsgHandle: Not needed (rmd compliance)   
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *    CL_ERR_INVALID_BUFFER    - when unable to retrieve info from inMsgHandle
 *    CL_ERR_NO_MEMORY         - malloc failure
 *    CL_NS_ERR_SERVICE_NOT_REGISTERED - trying to deregister a nonregistered 
 *                                       enrty
 *    CL_NS_ERR_CONTEXT_NOT_CREATED   - trying to deregister from nonexisting 
 *                                      context
 */
ClRcT VDECL(nameSvcServiceDeregister)(ClEoDataT data,  
                               ClBufferHandleT  inMsgHandle,
                               ClBufferHandleT  outMsgHandle)
{
    ClRcT                    ret               = CL_OK;
    ClIocNodeAddressT        sdAddr            = 0;
    ClCntHandleT             pStoredInfo       = 0;
    ClNameSvcContextInfoPtrT pStatInfo         = NULL;
    ClNameSvcBindingT        *pStoredNSBinding = NULL;
    ClCntNodeHandleT         pNodeHandle       = 0;
    ClUint32T                noEntries         = 0, iCnt = 0;
    ClIocNodeAddressT        *pAddrList        = NULL;
    ClNameSvcInfoIDLT        *nsInfo           = NULL;
    ClBufferHandleT          inMsgHdl          = 0;
    ClNameSvcDeregisInfoT    walkData          = {0};
    ClNameSvcNameLookupT     lookupData        = {0};
    ClNameVersionT           version           = {0};
    ClNameSvcBindingDetailsT *pBindingDetail   = NULL;
    ClUint32T                dsId              = 0;
    ClCntDataHandleT         pDataHdl          = 0;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside nameSvcServiceDeregister \n"));

    /* Extract Version Information */
    VDECL_VER(clXdrUnmarshallClNameVersionT, 4, 0, 0)(inMsgHandle, (void *)&version);

    nsInfo = (ClNameSvcInfoIDLT *)clHeapAllocate(sizeof(ClNameSvcInfoIDLT));
    if(nsInfo == NULL)
    {
        ret = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Malloc failed \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_SERVICE_DEREGIS_FAILED, ret);
        CL_FUNC_EXIT();
        return ret;
    }

    clBufferCreate (&inMsgHdl);
    VDECL_VER(clXdrUnmarshallClNameSvcInfoIDLT, 4, 0, 0)(inMsgHandle, (void *)nsInfo);

    /* Version Verification */
    if(nsInfo->source != CL_NS_MASTER)
    {
        ret = clVersionVerify(&gNSClientToServerVersionInfo,(ClVersionT *)&version);
    }
    else
    {
        ret = clVersionVerify(&gNSServerToServerVersionInfo,(ClVersionT *)&version);
    }
    if(ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Version not suppoterd \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_SERVICE_DEREGIS_FAILED, ret);

        clBufferClear(inMsgHdl);
        if(nsInfo->source == CL_NS_MASTER)
        {
            ClNameSvcNackT           nackType = CL_NS_NACK_SERVICE_DEREGISTER;
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl, 0);
            clXdrMarshallClUint32T(&nackType, inMsgHdl, 0);
            ret = clCpmMasterAddressGet(&gMasterAddress);
            if (ret != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",ret));
                return ret;
            }
            CL_NS_CALL_RMD(gMasterAddress, CL_NS_NACK, inMsgHdl,
                           outMsgHandle, ret);
        }
        else
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, outMsgHandle, 0);
                                                                                                                             
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return ret;
    }

    /* Find the appripriate context to look into */
    if(nsInfo->contextId == CL_NS_DEFT_GLOBAL_CONTEXT)
        nsInfo->contextId = CL_NS_DEFAULT_GLOBAL_CONTEXT;
    else if(nsInfo->contextId == CL_NS_DEFT_LOCAL_CONTEXT)
        nsInfo->contextId = CL_NS_DEFAULT_NODELOCAL_CONTEXT;


    /* Sending request to master */
    sdAddr = clIocLocalAddressGet();
    ret = clCpmMasterAddressGet(&gMasterAddress);
    if (ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",ret));
        return ret;
    }
    if((sdAddr != gMasterAddress) && 
       (nsInfo->contextId < CL_NS_BASE_NODELOCAL_CONTEXT) && 
       (nsInfo->source != CL_NS_MASTER))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n request has come to slave."
                             " Forwarding it to master .. \n"));
        VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl, 0);
        nsInfo->source = CL_NS_LOCAL;
        VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHdl, 0);        
        CL_NS_CALL_RMD(gMasterAddress, CL_NS_SERVICE_DEREGISTER, inMsgHdl, 
                    NULL, ret);
        if(ret != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                       CL_NS_LOG_1_SERVICE_DEREGIS_FAILED, ret);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: NS deregister failed \n"));
        }
        /* delete the message created for updating NS/M */
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return ret;
    }

    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Could not get Lock successfully----\n"));
    }
 
    ret = clCntNodeFind(gNSHashTable, (ClPtrT)(ClWordT)nsInfo->contextId, &pNodeHandle);
    if(ret != CL_OK)
    {
        ret = CL_NS_RC(CL_NS_ERR_CONTEXT_NOT_CREATED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context %d not found in NS \n", 
                             nsInfo->contextId));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_SERVICE_DEREGIS_FAILED, ret);
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        /* delete the message created for updating peers */
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return ret;
    }

    ret = clCntNodeUserDataGet(gNSHashTable,pNodeHandle,
                                (ClCntDataHandleT*)&pStatInfo);
    pStoredInfo = pStatInfo->hashId;
                                                                                                                             
    /* Find the node */
    clNSLookupKeyForm(&nsInfo->name, &lookupData);
    gNameEntryDelete   = 0;
    gDelete            = 0;
    ret = clCntNodeFind((ClCntHandleT)pStoredInfo,
                       (ClCntKeyHandleT)&lookupData, &pNodeHandle);
    if(ret != CL_OK)
    {
        ret = CL_NS_RC(CL_NS_ERR_SERVICE_NOT_REGISTERED);
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Service not registered" \
                             "by this component \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_SERVICE_DEREGIS_FAILED, ret);
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        /* delete the message created for updating peers */
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return ret;
    }                                                                                                                        
    ret = clCntNodeUserDataGet((ClCntHandleT)pStoredInfo,pNodeHandle,
                        (ClCntDataHandleT*)&pStoredNSBinding);

    walkData.nameEntry = pStoredNSBinding;
    walkData.compId    = nsInfo->compId;
    walkData.hashTable = pStatInfo;
    walkData.operation = CL_NS_SERVICE_DEREGISTER_OP;
    walkData.contextId = nsInfo->contextId;
                                                                                                                             
    /* Walk the entries */
    ret = clCntWalk((ClCntHandleT) pStoredNSBinding->hashId,
                     _nameSvcMatchedEntryDeleteCallback,
                     (ClCntArgHandleT) &walkData, sizeof(walkData));

    /* Delete the previously marked entries */
    if(gDelete == 1)
    {
        ret = clCntDataForKeyGet(gHashTableHdl, gUserKey, 
                                &pDataHdl);
        if(ret != CL_OK)
        {
            /* Release the semaphore */
            clOsalMutexUnlock(gSem);
            return ret;
        }
        pBindingDetail = (ClNameSvcBindingDetailsT *)pDataHdl;
        dsId = pBindingDetail->dsId;
        ret = clCntAllNodesForKeyDelete(gHashTableHdl, gUserKey);
        if(ret != CL_OK)
        {
            /* Release the semaphore */
            clOsalMutexUnlock(gSem);
            return ret;
        }
        gHashTableHdl   = 0;
        gDelete         = 0;
        gHashTableIndex = 0;
        /*
         * Binding details entry removal from Bind Data.
         */
        if( (nsInfo->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT) 
            || (sdAddr == gMasterAddress) )
        {    
            if( CL_OK != (ret = clNameSvcDataSetDelete(nsInfo->contextId, dsId)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("clNameSvcDataSetDelete(): rc[0x %x]", ret));
            }    
            clNameSvcPerCtxDataSetIdPut(nsInfo->contextId, dsId);
        }
     }
    if(gNameEntryDelete == 1)
    {
        dsId = pStoredNSBinding->dsId;
        ret = clCntNodeDelete(pStoredInfo, pNodeHandle);
        if(ret != CL_OK)
        {
            /* Release the semaphore */
            clOsalMutexUnlock(gSem);
            return ret;
        }
        gNameEntryDelete    = 0;
        gNameEntryHashTable = 0;
        gNameEntryUserKey   = 0;
        /*
         * Name service Entry is removed from Context.
         */ 
        if( (nsInfo->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT) 
            || (sdAddr == gMasterAddress) )
        {    
            if( CL_OK != (ret = clNameSvcDataSetDelete(nsInfo->contextId, dsId)))
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("clNameSvcDataSetDelete(): rc[0x %x]", ret));
            }    
            clNameSvcPerCtxDataSetIdPut(nsInfo->contextId, dsId);
        }
    }

    if(gFound == 0)
    {
        ret = CL_NS_RC(CL_NS_ERR_SERVICE_NOT_REGISTERED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Service not registered"\
                             " by this component \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_SERVICE_DEREGIS_FAILED, ret);
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        /* delete the message created for updating peers */
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return ret;
    }                                                                                                                        
    else
        gFound = 0;
     
    /* Update the slaves */
    if((nsInfo->source != CL_NS_MASTER)  && 
       (nsInfo->contextId < CL_NS_BASE_NODELOCAL_CONTEXT))
    {
        nsInfo->source = CL_NS_MASTER;
        ret = clIocTotalNeighborEntryGet(&noEntries);
        if(ret == CL_OK)
        {
            pAddrList = (ClIocNodeAddressT *)clHeapAllocate
                          (sizeof(ClIocNodeAddressT)*noEntries);
            clIocNeighborListGet(&noEntries,pAddrList);
            for(iCnt=0; iCnt<noEntries; iCnt++)
            {
                ClIocNodeAddressT addr = *(pAddrList+iCnt);
                if(addr == clIocLocalAddressGet())
                {
                    continue;
                }
                CL_NS_SERVER_SERVER_VERSION_SET(&version);
                VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl, 0);
                VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHdl, 0);        
                CL_NS_CALL_RMD_ASYNC(addr, CL_NS_SERVICE_DEREGISTER, 
                                  inMsgHdl, NULL, ret);
            }
            clHeapFree(pAddrList);
        }
        else
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Couldnt update the peers \n"));
    }
    clLogInfo("SVR", "DEREG", "Service [%.*s] has been deregistered at address [%d]", 
              nsInfo->name.length, nsInfo->name.value, clIocLocalAddressGet());
              

    /* Release the semaphore */
    clOsalMutexUnlock(gSem);
    /* delete the message created for updating peers */
    clBufferDelete(&inMsgHdl);
    if(nsInfo->attr) clHeapFree(nsInfo->attr);
    clHeapFree(nsInfo);
    CL_FUNC_EXIT();
    return CL_OK;
}



/**
 *  Name: nameSvcContextCreate 
 *
 *  Function for creating a context
 *
 *  @param  data: rmd data
 *          inMsgHandle: contains context creation related information
 *          outMsgHandle: will carry back the id of the created context
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *    CL_ERR_INVALID_BUFFER    - when unable to retrieve info from inMsgHandle
 *    CL_ERR_NO_MEMORY         - malloc failure
 *    CL_NS_ERR_CONTEXT_ALREADY_CREATED - if the context already exists
 *    CL_NS_ERR_LIMIT_EXCEEDED - if limit for max no of context exceeds
 *    CL_ERR_INVALID_PARAMETER - if improper contetx type specified
 */
ClRcT VDECL(nameSvcContextCreate)(ClEoDataT data, ClBufferHandleT  inMsgHandle,
                           ClBufferHandleT  outMsgHandle)
{
    ClRcT                    rc        = CL_OK;
    ClCntHandleT             *pTable   = NULL;
    ClUint32T                contxtId  = 0;
    ClNameSvcContextInfoPtrT pContStat = NULL;
    ClUint32T                noEntries = 0, iCnt = 0;
    ClIocNodeAddressT        sdAddr    = 0,  recvdAddr = 0;
    ClIocNodeAddressT*       pAddrList = NULL;
    ClUint32T                isPeer    = 1, sentToMaster = 0;
    ClNameSvcInfoIDLT        *nsInfo   = NULL;
    ClBufferHandleT          inMsgHdl  = 0;
    ClUint32T                recvdContextId = 0, contextPresent = 0;
    ClNameVersionT           version   = {0};
    
    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside nameSvcContextCreate \n"));

    /* Extract Version information */
    VDECL_VER(clXdrUnmarshallClNameVersionT, 4, 0, 0)(inMsgHandle, (void *)&version);

    nsInfo = (ClNameSvcInfoIDLT *)clHeapAllocate(sizeof(ClNameSvcInfoIDLT));
    if(nsInfo == NULL)
    {
        rc = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Malloc failed \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc); 
        CL_FUNC_EXIT();
        return rc; 
    }

    VDECL_VER(clXdrUnmarshallClNameSvcInfoIDLT, 4, 0, 0)(inMsgHandle, (void *)nsInfo); 

    /* Version Verification */
    if(nsInfo->source != CL_NS_MASTER)
    {
        rc = clVersionVerify(&gNSClientToServerVersionInfo,(ClVersionT *)&version);
    }
    else
    {
        rc = clVersionVerify(&gNSServerToServerVersionInfo,(ClVersionT *)&version);
    }
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Version not suppoterd \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc); 
                                                                                                                             
        if(nsInfo->source == CL_NS_MASTER)
        {
            ClBufferHandleT   inMsgHdl;
            ClNameSvcNackT           nackType = CL_NS_NACK_CONTEXT_CREATE;
                                                                                                                             
            clBufferCreate(&inMsgHdl);
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl, 0);
            clXdrMarshallClUint32T(&nackType, inMsgHdl, 0);
            rc = clCpmMasterAddressGet(&gMasterAddress);
            if (rc != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",rc));
                return rc;
            }
                                                                                                                             
            CL_NS_CALL_RMD_ASYNC(gMasterAddress, CL_NS_NACK, inMsgHdl,
                                 outMsgHandle, rc);
            clBufferDelete(&inMsgHdl);
        }
        else
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, outMsgHandle, 0);

        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return rc;
    }

    rc = nameSvcContextFromCookieGet(nsInfo->contextMapCookie, &recvdContextId);
    if(rc == CL_OK)
    {
        rc = CL_NS_RC(CL_NS_ERR_CONTEXT_ALREADY_CREATED);
        clXdrMarshallClUint32T((void*)&recvdContextId, outMsgHandle, 0);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc);          
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context already created \n"));
        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return rc;
    }
    
    recvdContextId = 0;     
    clBufferCreate (&inMsgHdl);
    sdAddr = clIocLocalAddressGet();
    rc = clCpmMasterAddressGet(&gMasterAddress);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",rc));
        return rc;
    }
    if((sdAddr != gMasterAddress) && 
       (nsInfo->contextType == (ClInt32T)CL_NS_USER_GLOBAL) &&
       (nsInfo->source != (ClInt32T)CL_NS_MASTER))
    {
        CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n request has come to slave for entry with" \
               " global wide. Forwarding it to master ..... \n"));
        nsInfo->source = CL_NS_LOCAL;
        sentToMaster = 1;
        /* Set the context Id as local address, so that NS/M can check this 
           address and choose not to send duplicate context create msg */
        VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl, 0);
        VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHdl, 0);
        clXdrMarshallClUint32T((void *)&sdAddr, inMsgHdl, 0);
        CL_NS_CALL_RMD(gMasterAddress, CL_NS_CONTEXT_CREATE, inMsgHdl, 
                    outMsgHandle, rc);
        if(rc != CL_OK)
        {
            if(CL_GET_ERROR_CODE(rc) == CL_NS_ERR_CONTEXT_ALREADY_CREATED)
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Context already "\
                                     "created in NS/M \n"));
                contextPresent = 1;
            }
            else
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context Creation failed \n"));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                           CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc);          
                /* delete the message created for updating NS/M */
                clBufferDelete(&inMsgHdl);
                if(nsInfo->attr) clHeapFree(nsInfo->attr);
                clHeapFree(nsInfo);
                CL_FUNC_EXIT();
                return rc;
            }
        }
        clXdrUnmarshallClUint32T(outMsgHandle, (void*)&recvdContextId);
        /* set source to CL_NS_MASTER as RMD has been processed at 
           NS/M and then come here */
        nsInfo->source = CL_NS_MASTER;
    }
    
    switch(nsInfo->contextType)
    {
    case CL_NS_USER_NODELOCAL:
        if(sNoUserLocalCxt >= gpConfig.nsMaxNoLocalContexts)
        {
            rc = CL_NS_RC(CL_NS_ERR_LIMIT_EXCEEDED);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Limit on Max no. of"\
                                 " NODE LOCAL CONTEXTS already reached \n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                       CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc); 
            /* delete the message */
            clBufferDelete(&inMsgHdl);
            if(nsInfo->attr) clHeapFree(nsInfo->attr);
            clHeapFree(nsInfo);
            CL_FUNC_EXIT();
            return rc;
        }
        pTable = (ClCntHandleT *) clHeapAllocate(sizeof(ClCntHandleT));
        if(pTable == NULL)
        {
            rc = CL_NS_RC(CL_ERR_NO_MEMORY);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                       CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc); 
            /* delete the message */
            clBufferDelete(&inMsgHdl);
            if(nsInfo->attr) clHeapFree(nsInfo->attr);
            clHeapFree(nsInfo);
            CL_FUNC_EXIT();
            return rc;
        }
        pContStat = (ClNameSvcContextInfoPtrT) 
                    clHeapAllocate(sizeof(ClNameSvcContextInfoT));
        if(pContStat == NULL)
        {
            rc = CL_NS_RC(CL_ERR_NO_MEMORY);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                       CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc); 
            /* delete the message */
            clBufferDelete(&inMsgHdl);
            if(nsInfo->attr) clHeapFree(nsInfo->attr);
            clHeapFree(nsInfo);
            clHeapFree(pTable);
            CL_FUNC_EXIT();
            return rc;
        }

        if(contextPresent == 1)
            contxtId = recvdContextId; 
        else
            nameSvcContextIdGet(CL_NS_USER_NODELOCAL, &contxtId);
            
        rc = clCntHashtblCreate(gpConfig.nsMaxNoEntries, 
                          nameSvcHashEntryKeyCmp,
                          nameSvcPerContextEntryHashFunction,
                          nameSvcHashEntryDeleteCallback, 
                          nameSvcHashEntryDeleteCallback,
                          CL_CNT_UNIQUE_KEY, pTable);
        pContStat->hashId            = *pTable;
        pContStat->entryCount        = 0;
        pContStat->dsIdCnt           = 1;
        pContStat->contextMapCookie  = nsInfo->contextMapCookie;
 
        /* take the semaphore */
        if(clOsalMutexLock(gSem)  != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Couldnt get Lock successfully--\n"));
        }
        rc = clCntNodeAdd(gNSHashTable, (ClCntKeyHandleT)(ClWordT) contxtId,
                           (ClCntDataHandleT ) pContStat, NULL);
        sNoUserLocalCxt++;
        clNameCkptCtxInfoWrite();
        clNameSvcPerCtxCkptCreate(contxtId, 0);
        clNameSvcPerCtxInfoWrite(contxtId, pContStat);
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        isPeer = 0;

        break;
    case CL_NS_USER_GLOBAL:
        if(sNoUserGlobalCxt >= gpConfig.nsMaxNoGlobalContexts)
        {
            rc = CL_NS_RC(CL_NS_ERR_LIMIT_EXCEEDED);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Limit on Max no. of "\
                                 "GLOBAL CONTEXTS already reached \n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                       CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc); 
            /* delete the message */
            clBufferDelete(&inMsgHdl);
            if(nsInfo->attr) clHeapFree(nsInfo->attr);
            clHeapFree(nsInfo);
            CL_FUNC_EXIT();
            return rc;
        }
        pTable = (ClCntHandleT *) clHeapAllocate(sizeof(ClCntHandleT));
        if(pTable == NULL)
        {
            rc = CL_NS_RC(CL_ERR_NO_MEMORY); 
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                       CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc); 
            /* delete the message */
            clBufferDelete(&inMsgHdl);
            if(nsInfo->attr) clHeapFree(nsInfo->attr);
            clHeapFree(nsInfo);
            CL_FUNC_EXIT();
            return rc;
        }
        pContStat = (ClNameSvcContextInfoPtrT) 
                    clHeapAllocate(sizeof(ClNameSvcContextInfoT));
        if(pContStat == NULL)
        {
            rc = CL_NS_RC(CL_ERR_NO_MEMORY); 
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                       CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc); 
            /* delete the message */
            clBufferDelete(&inMsgHdl);
            if(nsInfo->attr) clHeapFree(nsInfo->attr);
            clHeapFree(nsInfo);
            clHeapFree(pTable);
            CL_FUNC_EXIT();
            return rc;
        }


        if(contextPresent == 1)
        {
            contxtId = recvdContextId; 
        }
        else
            nameSvcContextIdGet(CL_NS_USER_GLOBAL, &contxtId);

        rc = clCntHashtblCreate(gpConfig.nsMaxNoEntries, 
                          nameSvcHashEntryKeyCmp,
                          nameSvcPerContextEntryHashFunction,
                          nameSvcHashEntryDeleteCallback, 
                          nameSvcHashEntryDeleteCallback,
                          CL_CNT_UNIQUE_KEY, pTable);

        pContStat->hashId            = *pTable;
        pContStat->entryCount        = 0;
        pContStat->dsIdCnt           = 1;
        pContStat->contextMapCookie  = nsInfo->contextMapCookie;

        /* take the semaphore */
        if(clOsalMutexLock(gSem)  != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Couldnt get Lock successfully--\n"));
        }
        rc = clCntNodeAdd(gNSHashTable, (ClCntKeyHandleT)(ClWordT) contxtId,
                           (ClCntDataHandleT ) pContStat, NULL);
        sNoUserGlobalCxt++;
        if( sdAddr == gMasterAddress )
        {    
            clNameCkptCtxInfoWrite();
            clNameSvcPerCtxCkptCreate(contxtId, 0);
            clNameSvcPerCtxInfoWrite(contxtId, pContStat);
        }
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        break;
    default: 
        rc = CL_NS_RC(CL_ERR_INVALID_PARAMETER); 
        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        /* delete the message */
        clBufferDelete(&inMsgHdl);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Improper CONTEXT TYPE \n")); 
        CL_FUNC_EXIT();
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc); 
        return rc;
    }
    
    if(pTable)
        clHeapFree(pTable);

    if(contextPresent == 1)
    {
        CL_FUNC_EXIT();
        /* delete the message */
        clBufferDelete(&inMsgHdl);
        if(nsInfo->attr) clHeapFree(nsInfo->attr);
        clHeapFree(nsInfo);
        return rc;
    }
    /* Update the slaves */
    if((nsInfo->source != (ClInt32T)CL_NS_MASTER)  &&
       (nsInfo->contextType == (ClInt32T)CL_NS_USER_GLOBAL))
    {
        isPeer = 0;
        rc = clIocTotalNeighborEntryGet(&noEntries);
        if(rc == CL_OK)
        {
            pAddrList = (ClIocNodeAddressT *)clHeapAllocate
                          (sizeof(ClIocNodeAddressT)*noEntries);
            clIocNeighborListGet(&noEntries,pAddrList);
            if(nsInfo->source == CL_NS_LOCAL)
            {
                clXdrUnmarshallClUint32T(inMsgHandle, (void*)&recvdAddr);
            }
            else
                 recvdAddr = clIocLocalAddressGet();
           for(iCnt=0; iCnt<noEntries; iCnt++)
            {
                ClIocNodeAddressT addr = *(pAddrList+iCnt);
                /* Dont send request to self and to the original NS/L */
                if(addr == clIocLocalAddressGet() || 
                   addr == recvdAddr)
                {
                    continue;
                }
                nsInfo->contextId = contxtId;
                clXdrMarshallClUint32T((void *)&contxtId, outMsgHandle, 0);
                nsInfo->source = CL_NS_MASTER;
                CL_NS_SERVER_SERVER_VERSION_SET(&version);
                VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl,0);
                VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHdl, 0);
                CL_NS_CALL_RMD_ASYNC(addr, CL_NS_CONTEXT_CREATE, 
                                     inMsgHdl, NULL, rc);
            }
            clHeapFree(pAddrList);
        }
        else
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Couldnt update the peers \n"));
    }
    /* Check if the context id gerenrated in NS/M and self is same or not */
    if(isPeer == 1)
    {
        if(sentToMaster == 0)
        {
            if(nsInfo->contextId != contxtId)
            {
                rc = CL_NS_RC(CL_NS_ERR_CONTEXT_CREATION_FAILED);
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, 
                           NULL, 
                           CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc);
                clBufferDelete(&inMsgHdl);
                if(nsInfo->attr) clHeapFree(nsInfo->attr);                
                clHeapFree(nsInfo);
                CL_FUNC_EXIT();
                return rc;
            }
   
        }
        else
        {            
            if(recvdContextId != contxtId)
            {
                rc = CL_NS_RC(CL_NS_ERR_CONTEXT_CREATION_FAILED);
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, 
                           NULL, 
                           CL_NS_LOG_1_CONTEXT_CREATION_FAILED, rc);
                clBufferDelete(&inMsgHdl);
                if(nsInfo->attr) clHeapFree(nsInfo->attr);
                clHeapFree(nsInfo);
                CL_FUNC_EXIT();
                return rc;
            }
        }
    }
    clXdrMarshallClUint32T((void*)&contxtId, outMsgHandle, 0);
    if(nsInfo->attr) clHeapFree(nsInfo->attr);
    clHeapFree(nsInfo);
    /* delete the message */
    clBufferDelete(&inMsgHdl);
    CL_FUNC_EXIT();
    return rc;
}



/**
 *  Name: _nameSvcEntryDeleteCallback 
 *
 *  Callback function for deleting the entries in 
 *  a given context
 * 
 *  @param  userKey: hashid of the entry
 *          hashTable: entry info
 *          arg: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT _nameSvcEntryDeleteCallback(ClCntKeyHandleT    userKey,
                                  ClCntDataHandleT   nsInfo,
                                  ClCntArgHandleT    arg,
                                  ClUint32T          dataLength)
{
    ClRcT                       ret         = CL_OK;
    ClNameSvcBindingDetailsT    *pNSInfo    = (ClNameSvcBindingDetailsT*)nsInfo;
    ClNameSvcCompListT          *pTemp      = NULL;
    ClNameSvcCompListT          *pTemp1     = &pNSInfo->compId;
    ClCntNodeHandleT            pNodeHandle = 0;
    ClNameSvcDeregisInfoT*      userArg     = (ClNameSvcDeregisInfoT*)arg;


    if(gDelete == 1)
    {
        /* Delete the previously marked entry */
        ret = clCntNodeFind(gHashTableHdl, gUserKey, &pNodeHandle);
        if(ret != CL_OK)
            return ret;
        ret = clCntNodeDelete(gHashTableHdl, pNodeHandle);
        if(ret != CL_OK)
            return ret;
        gDelete = 0;

    }
   
    do
    {
       /* first delete all the references */
        while(pNSInfo->refCount >= 1)
        {
            pNSInfo->refCount--;
            pTemp1->compId = pTemp1->pNext->compId;
            pTemp = pTemp1->pNext;
            pTemp1->pNext = pTemp->pNext;
            clHeapFree(pTemp);
        }
        gUserKey = userKey;
        gDelete = 1;
        gHashTableHdl = userArg->nameEntry->hashId;
        userArg->nameEntry->refCount--;
        if(userArg->nameEntry->refCount == 0)
        {
            userArg->hashTable->entryCount--;
            gNameEntryDelete    = 1;
            gNameEntryUserKey   = userArg->nameEntryUserKey;
            gNameEntryHashTable = userArg->hashTable->hashId;
        }
        pTemp = pTemp1;
        pTemp1 = pTemp1->pNext;
    } while(pTemp1 != NULL);
   
        
    CL_FUNC_EXIT();
    return ret;
}



/**
 *  Name: nameSvcContextDelete 
 *
 *  Function for deleting a context
 *
 *  @param  data: rmd data
 *          inMsgHandle: contains context creation related information
 *          outMsgHandle: will carry back the id of the created context
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *    CL_ERR_INVALID_BUFFER    - when unable to retrieve info from inMsgHandle
 *    CL_ERR_NO_MEMORY         - malloc failure
 *    CL_NS_ERR_CONTEXT_NOT_CREATED - trying to delete a non existing context
 *    CL_NS_ERR_OPERATION_NOT_PERMITTED - trying to delete default contexts
 */

ClRcT VDECL(nameSvcContextDelete)(ClEoDataT data, ClBufferHandleT inMsgHandle,
                           ClBufferHandleT  outMsgHandle)
{
    ClRcT                    ret       = CL_OK;
    ClNameSvcInfoIDLT        *nsInfo   = NULL;
    ClNameVersionT           version   = {0};


    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside nameSvcContextDelete \n"));

    /* Extract Version information */
    VDECL_VER(clXdrUnmarshallClNameVersionT, 4, 0, 0)(inMsgHandle, (void *)&version);
    ret = clVersionVerify(&gNSClientToServerVersionInfo,(ClVersionT *)&version);
    if(ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Version not suppoterd \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret); 
        CL_FUNC_EXIT();
        return ret;
    }

    nsInfo = (ClNameSvcInfoIDLT *)clHeapAllocate(sizeof(ClNameSvcInfoIDLT));
    if(nsInfo == NULL)
    {
        ret = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Malloc failed \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret); 
        CL_FUNC_EXIT();
        return ret;
    }

    VDECL_VER(clXdrUnmarshallClNameSvcInfoIDLT, 4, 0, 0)(inMsgHandle, (void *)nsInfo); 

    /* Version Verification */
    if(nsInfo->source != CL_NS_MASTER)
    {
        ret = clVersionVerify(&gNSClientToServerVersionInfo,(ClVersionT *)&version);
    }
    else
    {
        ret = clVersionVerify(&gNSServerToServerVersionInfo,(ClVersionT *)&version);
    }
    if(ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Version not suppoterd \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret); 

        if(nsInfo->source == CL_NS_MASTER)
        {
            ClBufferHandleT   inMsgHdl;
            ClNameSvcNackT           nackType = CL_NS_NACK_CONTEXT_DELETE;
                                                                                                                             
            clBufferCreate(&inMsgHdl);
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl, 0);
            clXdrMarshallClUint32T(&nackType, inMsgHdl, 0);
            ret = clCpmMasterAddressGet(&gMasterAddress);
            if (ret != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",ret));
                return ret;
            }
                                                                                                                             
            CL_NS_CALL_RMD_ASYNC(gMasterAddress, CL_NS_NACK, inMsgHdl,
                                 outMsgHandle, ret);
            clBufferDelete(&inMsgHdl);
        }
        else
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, outMsgHandle, 0);

        if(nsInfo->attr) clHeapFree(nsInfo->attr);        
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return ret;
    }

    if(nsInfo->contextId == CL_NS_DEFT_GLOBAL_CONTEXT)
        nsInfo->contextId = CL_NS_DEFAULT_GLOBAL_CONTEXT;
    else if(nsInfo->contextId == CL_NS_DEFT_LOCAL_CONTEXT)
        nsInfo->contextId = CL_NS_DEFAULT_NODELOCAL_CONTEXT;


    /* One cannot delete default global and nodelocal contexts */
    if((nsInfo->contextId == CL_NS_DEFAULT_GLOBAL_CONTEXT) ||
       (nsInfo->contextId == CL_NS_DEFAULT_NODELOCAL_CONTEXT))
   {
        ret = CL_NS_RC(CL_NS_ERR_OPERATION_NOT_PERMITTED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Trying to delete default contexts \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret);
        if(nsInfo->attr) clHeapFree(nsInfo->attr);        
        clHeapFree(nsInfo);
        CL_FUNC_EXIT();
        return ret;
    }

    ret  = _nameSvcContextDelete(nsInfo, 0, &version);
    if(ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\nNS: Context deletion failed, ret=%x \n", ret));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret);
    }
    if(nsInfo->attr) clHeapFree(nsInfo->attr);        
    clHeapFree(nsInfo);
    CL_FUNC_EXIT();
    return ret;
}


ClRcT _nameSvcContextDeleteLocked(ClNameSvcInfoIDLT* nsInfo, ClUint32T  flag,
                            ClNameVersionT *pVersion)
{
    ClBufferHandleT   inMsgHdl;
    ClRcT                    ret         = CL_OK;
    ClIocNodeAddressT        sdAddr      = 0;
    ClUint32T                noEntries   = 0, iCnt = 0;
    ClIocNodeAddressT        *pAddrList  = NULL;
    ClCntNodeHandleT         pNodeHandle = 0;
    ClCntNodeHandleT         pTempHandle = 0;
    ClNameSvcContextInfoPtrT pStatInfo   = NULL;
    ClCntHandleT             pStoredInfo = 0;
    ClNameSvcDeregisInfoT    walkData    = {0};
    ClCntKeyHandleT          userKey     = 0;
    ClNameVersionT           version     = {0};
    ClUint32T                dsIdCnt     = 0;
    ClUint8T                 *freeDsIdMap = NULL;
    ClUint32T                freeMapSize = 0;
    if(nsInfo == NULL)
    {
        ret = CL_NS_RC(CL_ERR_NULL_POINTER);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: NULL input parameter \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret); 
        CL_FUNC_EXIT();
        return ret;
    }
    clBufferCreate (&inMsgHdl);
    sdAddr = clIocLocalAddressGet();
    if(flag == 0)
    {
        ret = clCpmMasterAddressGet(&gMasterAddress);
        if (ret != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",ret));
            return ret;
        }
    }
    else
    {
        ClTimerTimeOutT delay = {.tsSec = 0, .tsMilliSec = 50 };
        ret = clCpmMasterAddressGetExtended(&gMasterAddress, 3, &delay);
        if (ret != CL_OK)
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",ret));
            return ret;
        }
    }

    /*Sending request to master */
    if((sdAddr != gMasterAddress) && 
       (nsInfo->contextId < CL_NS_BASE_NODELOCAL_CONTEXT) &&
       (nsInfo->source != CL_NS_MASTER))
    {
        if(flag == 0)
        {
            if( nsInfo->contextId == CL_NS_BASE_GLOBAL_CONTEXT )
            {
                CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                        ("Just ignoring, coz global default context\n"));
                clBufferDelete(&inMsgHdl);
                CL_FUNC_EXIT();
                return ret;
            }    
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n request has come to slave for deleting" \
                        "global scope context. Forwarding it to master ..... \n"));
            nsInfo->source = CL_NS_LOCAL;
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(pVersion,inMsgHdl,0);
            VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHdl, 0);
            CL_NS_CALL_RMD(gMasterAddress, CL_NS_CONTEXT_DELETE, inMsgHdl, NULL, ret);
            if(ret != CL_OK)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context Deletion failed \n"));
                clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                        CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret); 
            }
            /* delete the message created for updating NS/M */
            clBufferDelete(&inMsgHdl);
            CL_FUNC_EXIT();
            return ret;
        }
    }

    ret = clCntNodeFind(gNSHashTable, (ClPtrT)(ClWordT)nsInfo->contextId, &pNodeHandle);
    if(ret != CL_OK)
    {
        ret = CL_NS_RC(CL_NS_ERR_CONTEXT_NOT_CREATED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context %d not found in NS \n",
                             nsInfo->contextId));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret);
        /* delete the message created for updating peers */
        clBufferDelete(&inMsgHdl);
        CL_FUNC_EXIT();
        return ret;
    }
    ret = clCntNodeUserDataGet(gNSHashTable,pNodeHandle,
                               (ClCntDataHandleT*)&pStatInfo);
    pStoredInfo = pStatInfo->hashId;
    dsIdCnt     = pStatInfo->dsIdCnt;
    freeMapSize = (ClUint32T) sizeof(pStatInfo->freeDsIdMap);
    freeDsIdMap = clHeapCalloc(freeMapSize,  1);
    CL_ASSERT(freeDsIdMap != NULL);
    walkData.hashTable = pStatInfo;
    walkData.operation = CL_NS_CONTEXT_DELETE;
    walkData.contextId = nsInfo->contextId;
    ret = clCntNodeUserKeyGet(gNSHashTable, pNodeHandle,
                             &userKey);

    gHashTableIndex    = (ClUint32T)(ClWordT) userKey;
                                                                                                                             
    /* Walk the entry details hash table */
    ret = clCntWalk((ClCntHandleT) pStatInfo->hashId,
                     _nameSvcEntryLevelWalkForDelete,
                     (ClCntArgHandleT)&walkData, sizeof(walkData));

    if(gNameEntryDelete == 1)
    {
        /* Delete the previously marked entry */
        ret = clCntNodeFind(pStatInfo->hashId, gNameEntryUserKey, &pTempHandle);
        if(ret != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                    CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret);
            /* delete the message created for updating peers */
            clBufferDelete(&inMsgHdl);
            return ret;
        }    
        ret = clCntNodeDelete(pStatInfo->hashId, pTempHandle);
        if(ret != CL_OK)
        {
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                    CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret);
            /* delete the message created for updating peers */
            clBufferDelete(&inMsgHdl);
            return ret;
        }    
        gNameEntryHashTable = 0;
        gNameEntryDelete    = 0;
        gNameEntryUserKey   = 0;
    }

    ret = clCntDelete(pStoredInfo);
    if(ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCntDelete(): rc [0x %x]", ret));
    }
    pStatInfo->entryCount = 0;    
    memcpy(freeDsIdMap, pStatInfo->freeDsIdMap, freeMapSize);
    if (flag == 0)
        clCntNodeDelete(gNSHashTable, pNodeHandle);
    /* Decrememt the count of already created contexts */
    if(nsInfo->contextId < CL_NS_BASE_NODELOCAL_CONTEXT)
        sNoUserGlobalCxt--;
    else
        sNoUserLocalCxt--;

    /* Update the ContextIdArray */
    gpContextIdArray[nsInfo->contextId] = CL_NS_SLOT_FREE;

    /* Update the slaves */
    if((nsInfo->source != CL_NS_MASTER)  && 
       (nsInfo->contextId < CL_NS_BASE_NODELOCAL_CONTEXT))
    {
        nsInfo->source = CL_NS_MASTER;
        if( nsInfo->contextId == CL_NS_BASE_GLOBAL_CONTEXT)
        {
            CL_DEBUG_PRINT(CL_DEBUG_TRACE,
                    ("Just ignoring, default global context\n"));
        }    
        else
        {    
            ret = clIocTotalNeighborEntryGet(&noEntries);
            if(ret == CL_OK)
            {
                pAddrList = (ClIocNodeAddressT *)clHeapAllocate
                    (sizeof(ClIocNodeAddressT)*noEntries);
                clIocNeighborListGet(&noEntries,pAddrList);
                for(iCnt=0; iCnt<noEntries; iCnt++)
                {
                    ClIocNodeAddressT addr = *(pAddrList+iCnt);
                    if(addr == clIocLocalAddressGet())
                    {
                        continue;
                    }
                    CL_NS_SERVER_SERVER_VERSION_SET(&version);
                    VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version,inMsgHdl,0);
                    VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHdl, 0);
                    CL_NS_CALL_RMD_ASYNC(addr, CL_NS_CONTEXT_DELETE, inMsgHdl,
                            NULL, ret);
                }
                clHeapFree(pAddrList);
            }
            else
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Couldnt update the peers \n"));
        }
    }
    if( (nsInfo->contextId >= CL_NS_DEFAULT_NODELOCAL_CONTEXT) 
            || (sdAddr == gMasterAddress) )
    {
        if( CL_OK != (ret = clNameCkptCtxAllDSDelete(nsInfo->contextId, dsIdCnt, freeDsIdMap, freeMapSize)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clNameCkptCtxAllDSDelete(): rc[0x %x]", ret));
        }    
    }

    clHeapFree(freeDsIdMap);

    /* delete the message created for updating peers */
    clBufferDelete(&inMsgHdl);
    CL_FUNC_EXIT();
   if(CL_GET_ERROR_CODE(ret) == CL_IOC_ERR_COMP_UNREACHABLE || CL_GET_ERROR_CODE(ret) == CL_IOC_ERR_HOST_UNREACHABLE)
      ret = CL_OK;
   return ret;
}

ClRcT _nameSvcContextDelete(ClNameSvcInfoIDLT* nsInfo, ClUint32T  flag,
                            ClNameVersionT *pVersion)
{
    ClRcT rc = CL_OK;
    
    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Could not get Lock successfully---\n"));
    }
    
    rc = _nameSvcContextDeleteLocked(nsInfo, flag, pVersion);
            
    /* Release the semaphore */
    clOsalMutexUnlock(gSem);    
    
    return rc;    
}

    
/*************************************************************************/
/*********************** Query related functions *************************/
/*************************************************************************/


ClRcT _nameSvcAttrDisplayCallback(ClCntKeyHandleT   userKey,
                                   ClCntDataHandleT  nsInfo,
                                   ClCntArgHandleT   userArg,
                                   ClUint32T         dataLength)
{
    ClRcT ret = CL_OK;
    ClCharT  nameCliStr[CL_NAME_MAX_ENTRY_LEN]="\0";

    ClUint32T attrCount=0, passedAttrCount=0, result=0;
    ClNameSvcAttrLevelQueryT  *pData     = (ClNameSvcAttrLevelQueryT*) userArg;
    ClNameSvcAttrSearchT      *pAttr     = pData->pAttrList;
    ClNameT                   *pName     = pData->pName;
    ClNameSvcBindingDetailsT  *pNSInfo   = (ClNameSvcBindingDetailsT*)nsInfo;
    ClNameSvcAttrEntryPtrT  pStoredAttr  =  pNSInfo->attr;
    ClNameSvcAttrEntryPtrT  pTempAttr    =  pNSInfo->attr;
    ClNameSvcAttrEntryPtrT  pPassedAttrEntry = NULL;
    ClUint32T found[NS_MAX_NO_ATTR]={0};
    ClUint32T iCnt,jCnt,kCnt;
    ClUint32T len = 0, nameLen = 0;
    ClUint32T                 addr[2];
                                                                                                                             
    attrCount = pNSInfo->attrCount;
    passedAttrCount = pAttr->attrCount;

    addr[1] = pNSInfo->objReference>>32;
    addr[0] = pNSInfo->objReference & 0xFFFFFFFF;

    if(passedAttrCount == 0)
    {
        if(attrCount  == 0)
        {
            sprintf(nameCliStr, 
                    "\n Name: %10s  ObjReference: %06d:%06d  CompId: %04d"
                     "  Priority: %d",
                     pName->value,
                     (ClUint32T) addr[1], (ClUint32T) addr[0],
                     pNSInfo->compId.compId, pNSInfo->compId.priority);
                                                                                                                             
        }
        clBufferNBytesWrite(pData->outMsgHandle, 
                                   (ClUint8T*)&nameCliStr,
                                   strlen(nameCliStr));
        return ret;
    }
    for(kCnt=0;kCnt<passedAttrCount;kCnt++)
    {
        found[kCnt]=0;
        pPassedAttrEntry = &pAttr->attrList[kCnt];
        pStoredAttr =  pNSInfo->attr;
                                                                                                                             
        /* needed for supporting wildcard pattern search on names only */
        if((memcmp(pPassedAttrEntry->type, "name", strlen("name")) == 0)
           && (attrCount == 0))
           attrCount = 1;
                                                                                                                             
        for(iCnt=0;iCnt<attrCount;iCnt++)
        {
            if(memcmp(pPassedAttrEntry->type, "name", strlen("name")) == 0)
            {
                len = strlen((ClCharT*)pPassedAttrEntry->value);
                nameLen = strlen(pName->value);
                                                                                                                             
                if(((pPassedAttrEntry->value[0] == '*') && (len==1))  ||
                   ((memcmp(pName->value, pPassedAttrEntry->value,
                     nameLen) == 0)  &&  strlen(pName->value) ==
                   strlen((ClCharT*)pPassedAttrEntry->value))  ||
                   ((len>1) && (pPassedAttrEntry->value[len-1] == '*') &&
                   (memcmp(pName->value, pPassedAttrEntry->value,
                           len-1) == 0)) ||
                   ((pPassedAttrEntry->value[0] == '*') && (len>1) &&
                   (memcmp(&pName->value[nameLen-(len-1)],
                           &pPassedAttrEntry->value[1], len-1) == 0)))
                {
                    found[kCnt] = 1;
                    break;
                }
            }
            else
            if(strlen((ClCharT*)pStoredAttr->type) ==
                     strlen((ClCharT*)pPassedAttrEntry->type) &&
              (memcmp(pStoredAttr->type, pPassedAttrEntry->type,
                       strlen((ClCharT*)pStoredAttr->type)) == 0) &&
              strlen((ClCharT*)pStoredAttr->value) ==
                     strlen((ClCharT*)pPassedAttrEntry->value) &&
               (memcmp(pStoredAttr->value, pPassedAttrEntry->value,
                       strlen((ClCharT*)pStoredAttr->value)) == 0))
                  
            {
                found[kCnt] = 1;
                break;
            }
            else
                pStoredAttr++;
        } /* end of for(i=0;i<attrCount;i++) */
                                                                                                                             
        if(found[kCnt] == 1)
        {
            if(passedAttrCount == 1)
            {
                sprintf(nameCliStr, 
                     "\n Name: %10s  ObjReference: %06d:%06d"
                     "  CompId: %04d  Priority: %d",
                      pName->value,
                     (ClUint32T) addr[1], (ClUint32T) addr[0],
                     pNSInfo->compId.compId,  pNSInfo->compId.priority);
                if(attrCount > 0)
                {
                    for(jCnt=0; jCnt<attrCount; jCnt++)
                    {
                        sprintf(nameCliStr+strlen(nameCliStr),	
                                "\t %10s:%10s", pTempAttr->type,  
                                pTempAttr->value);
                        pTempAttr++;
                    }
                }
            }
        } /* end of if(found[k] == 1) */
    } /* end of for(k=0;k<passedAttrCount;k++) */
                                                                                                                             
    for(iCnt=0; iCnt<passedAttrCount-1;iCnt++)
    {
        if(iCnt==0)
        {
            if(pAttr->opCode[iCnt] == 0)
                result = found[iCnt] || found[iCnt+1];
            else
                result = found[iCnt] && found[iCnt+1];
        }
        else
        {
            if(pAttr->opCode[iCnt] == 0)
                result = result || found[iCnt+1];
            else
                result = result && found[iCnt+1];
        }
    }
    if(result == 1)
    {
        pTempAttr =  pNSInfo->attr;
        sprintf(nameCliStr+strlen(nameCliStr),	
                "\n Name: %10s  ObjReference: %06d:%06d"
                 "  CompId: %04d  Priority: %d",
                 pName->value,
                 (ClUint32T) addr[1], (ClUint32T) addr[0],
                 pNSInfo->compId.compId, pNSInfo->compId.priority);
                                                                                                                             
        if(attrCount > 0)
        {
            for(jCnt=0; jCnt<attrCount; jCnt++)
            {
                sprintf(nameCliStr+strlen(nameCliStr),	
                "\t %10s:%10s", pTempAttr->type,
                pTempAttr->value);
                pTempAttr++;
            }
        }
    }
                                                                                                                             
    clBufferNBytesWrite(pData->outMsgHandle, 
                               (ClUint8T*)&nameCliStr,
                               strlen(nameCliStr));
    return ret;
}
                                                                                                                            


/**
 *  Name: _nameSvcEntryLevelWalkForAttrQuery 
 *
 *  Walk funtion for going through all the entries in a given context and 
 *  finding entries that match the queried attributes
 * 
 *  @param  userKey: hashid of the entry
 *          hashTable: entry info
 *          userArg: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT _nameSvcEntryLevelWalkForAttrQuery(ClCntKeyHandleT    userKey,
                                         ClCntDataHandleT   nsInfo,
                                         ClCntArgHandleT    userArg,
                                         ClUint32T          dataLength)
{
    ClNameSvcBindingT *pNSInfo = (ClNameSvcBindingT*)nsInfo;
    ClNameSvcAttrLevelQueryT* pData      = (ClNameSvcAttrLevelQueryT*) userArg;
    
    pData->pName = &pNSInfo->name;
    clCntWalk((ClCntHandleT) pNSInfo->hashId,
                _nameSvcAttrDisplayCallback,
                (ClCntArgHandleT) pData, dataLength);
    return CL_OK;
}
    


/**
 *  Name: nameSvcAttributeQuery 
 *
 *  Function for carrying out attribute level query
 *
 *  @param  nsInfo: contains the query related info
 *          outMsgHandle: will carry resultant entries(if needed)
 *
 *  @returns
 *    CL_OK                    - everything is ok <br>
 *    CL_NS_ERR_CONTEXT_NOT_CREATED - trying to delete a non existing context
 */

ClRcT _nameSvcAttributeQuery(ClUint32T contextMapCookie, 
                             ClNameSvcAttrSearchT *pAttr,
                             ClBufferHandleT outMsgHandle)

{
    ClRcT                    ret         = CL_OK;
    ClIocNodeAddressT        sdAddr      = 0;
    ClCntNodeHandleT         pNodeHandle = 0;
    ClNameSvcContextInfoPtrT pStatInfo   = NULL;
    ClCntHandleT             pStdInfo    = 0;
    ClUint32T                contextId   = 0;
    ClBufferHandleT   inMsgHandle;
    ClNameSvcAttrLevelQueryT walkData;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside nameSvcAttributeQuery\n"));

    sdAddr = clIocLocalAddressGet();

    if(contextMapCookie == CL_NS_DEFT_GLOBAL_MAP_COOKIE)
        contextMapCookie = CL_NS_DEFAULT_GLOBAL_MAP_COOKIE;
    else if(contextMapCookie == CL_NS_DEFT_LOCAL_MAP_COOKIE)
        contextMapCookie = CL_NS_DEFAULT_LOCAL_MAP_COOKIE;

    ret = nameSvcContextFromCookieGet(contextMapCookie, &contextId);
    if(ret != CL_OK)
    {
        ret = CL_NS_RC(CL_NS_ERR_CONTEXT_NOT_CREATED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context not found in NS \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_NS_QUERY_FAILED, ret);
        CL_FUNC_EXIT();
        return ret;
    }

    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Could not get Lock successfully---\n"));
    }
    
    ret = clCntNodeFind(gNSHashTable, (ClPtrT)(ClWordT)contextId, &pNodeHandle);
    if(ret != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_NS_QUERY_FAILED, ret);
        CL_FUNC_EXIT();
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        /* delete the message created for querying NS/M */
        clBufferDelete(&inMsgHandle);
        return ret;
    }
    ret = clCntNodeUserDataGet(gNSHashTable,pNodeHandle,
                                (ClCntDataHandleT*)&pStatInfo);
    pStdInfo = pStatInfo->hashId;

    memset(&walkData, 0, sizeof(walkData));
    walkData.pAttrList    = pAttr ;
    walkData.outMsgHandle = outMsgHandle;
    ret = clCntWalk(pStdInfo, _nameSvcEntryLevelWalkForAttrQuery, 
                    (ClCntArgHandleT) &walkData, 
                    sizeof(walkData));

    /* Release the semaphore */
    clOsalMutexUnlock(gSem);
    CL_FUNC_EXIT();
    return CL_OK;
}



/**
 *  Name: nameSvcBindingIntoMessageCopy 
 *
 *  Copies the details of all the bindings into ClBufferHandleT
 *
 *  @param  pStoredNSEntry: contains the binding details
 *          pName: passed as name is not a part of binding details structure
 *          outMsgHandle : Will contain the output
 *         
 *  @returns
 *    CL_OK               - everything is ok <br>
 */
                                                                                                                             
ClRcT nameSvcBindingIntoMessageCopy(ClNameSvcBindingDetailsT *pStoredNSEntry,
                                    ClNameT *pName,
                                    ClBufferHandleT outMsgHandle,
                                    ClNameSvcOpsT op) 
{
    ClNameSvcEntryT     *pNSData         = 0;
    ClUint32T           size      = 0, iCnt=0; 
    ClNameSvcCompListT  *pCompId          = NULL;
    ClNameSvcAttrEntryWithSizeIDLT attrib = {0};
    ClUint32T           attrSize          = 0;

    size = sizeof(ClNameSvcEntryT) + ((pStoredNSEntry->attrCount)*
           sizeof(ClNameSvcAttrEntryT));

    if(op == CL_NS_QUERY_MAPPING)
    {
        attrib.attr = NULL;
        attrSize = (pStoredNSEntry->attrCount)*sizeof(ClNameSvcAttrEntryT);
        attrib.attrLen = pStoredNSEntry->attrLen;
        attrib.attrCount = pStoredNSEntry->attrCount;
        if(attrSize>0)
        {
            attrib.attr  = (ClNameSvcAttrEntryIDLT*)
                clHeapCalloc(1, attrib.attrLen);
            memcpy(attrib.attr, pStoredNSEntry->attr, attrib.attrLen);
        }
        clXdrMarshallClUint32T((void *)&size, outMsgHandle, 0);
        clXdrMarshallClNameT((void *)pName, outMsgHandle, 0);
        clXdrMarshallClUint64T((void *)&pStoredNSEntry->objReference, 
                           outMsgHandle, 0);
        clXdrMarshallClUint32T((void *)&pStoredNSEntry->refCount,
                           outMsgHandle, 0);
        clXdrMarshallClUint32T((void *)&pStoredNSEntry->compId.compId,
                           outMsgHandle, 0);
        clXdrMarshallClUint32T((void *)&pStoredNSEntry->compId.priority,
                           outMsgHandle, 0);
        clXdrMarshallClUint32T((void *)&pStoredNSEntry->attrCount,
                           outMsgHandle, 0);
        VDECL_VER(clXdrMarshallClNameSvcAttrEntryWithSizeIDLT, 4, 0, 0)(
                            (void *)&attrib, outMsgHandle, 0);
    }
    else
    {
        pNSData = (ClNameSvcEntryT*) clHeapAllocate(sizeof(ClNameSvcEntryT));
        clNameCopy(&pNSData->name, pName);
        memcpy(&pNSData->objReference, &pStoredNSEntry->objReference,
           sizeof(ClUint64T));
        pNSData->refCount = pStoredNSEntry->refCount;
        memcpy(&pNSData->compId, &pStoredNSEntry->compId,
           sizeof(ClNameSvcCompListT));
        pNSData->attrCount = pStoredNSEntry->attrCount;
        if( pNSData->attrCount > 0 )
        {
            memcpy(&pNSData->attr, &pStoredNSEntry->attr,
               ((pStoredNSEntry->attrCount)*
               sizeof(ClNameSvcAttrEntryT)));
        }
       clBufferNBytesWrite(outMsgHandle,
                              (ClUint8T*)pNSData, size);
    }

    pCompId = &pStoredNSEntry->compId;
    for(iCnt=0; iCnt<pStoredNSEntry->refCount; iCnt++)
    {
        pCompId = pCompId->pNext;
        if(op == CL_NS_QUERY_MAPPING)
        {
            clXdrMarshallClUint32T((void *)&pCompId->compId,
                               outMsgHandle, 0);
            clXdrMarshallClUint32T((void *)&pCompId->priority,
                               outMsgHandle, 0);
        }
        else
        {

            clBufferNBytesWrite(outMsgHandle,
                        (ClUint8T*)&pCompId->compId, sizeof(ClUint32T));
            clBufferNBytesWrite(outMsgHandle,
                        (ClUint8T*)&pCompId->priority, sizeof(ClUint32T));
        }
    }

    if(op == CL_NS_QUERY_MAPPING)
    {
        if(attrSize>0)
            clHeapFree(attrib.attr); 
    }
    else
        clHeapFree(pNSData);
    return CL_OK;
}


/**
 *  Name: _nameSvcAllBindingsGetCallback 
 *
 *  Gets the details of all the bindings associated with the 
 *  queried name.
 *
 *  @param  userKey: hashid of the entry
 *          hashTable: entry info
 *          userArg: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */
                                                                                                                             
ClRcT _nameSvcAllBindingsGetCallback(ClCntKeyHandleT    userKey,
                               ClCntDataHandleT   nsData,
                               ClCntArgHandleT    arg,
                               ClUint32T          dataLength)
{
    ClRcT                     ret       = CL_OK;
    ClNameSvcBindingDetailsT  *pNSInfo  = (ClNameSvcBindingDetailsT*)nsData;
    ClNameSvcAllBindingsGetT* userArg   = (ClNameSvcAllBindingsGetT*)arg;
    ClUint32T                 op        = userArg->nsInfo->op;
    ClNameSvcInfoT*           nsInfo    = userArg->nsInfo;
    if(pNSInfo->attrCount == nsInfo->attrCount)
    {
        if(!memcmp(pNSInfo->attr, nsInfo->attr, nsInfo->attrCount *
               sizeof(ClNameSvcAttrEntryT))) 
        {
             switch(op)
             {
                 case CL_NS_QUERY_ALL_MAPPINGS:
                     nameSvcBindingIntoMessageCopy(pNSInfo, &nsInfo->name, 
                                                   userArg->outMsgHandle,
                                                   op);
                 break;
                 default:
                 break;
            }
        }
    }
    return ret;
}             
                 

/**
 *  Name: nameSvcLAQuery
 *
 *  Function for carrying out object reference related query
 *
 *  @param  nsInfo: contains the query related info
 *          outMsgHandle: will carry resultant entries(if needed)
 *
 *  @returns
 *    CL_OK                         - everything is ok <br>
 *    CL_NS_ERR_CONTEXT_NOT_CREATED - trying to qury on a non existing context
 */
ClRcT nameSvcLAQuery(ClNameSvcInfoIDLT *nsInfo, 
                     ClNameVersionT *pVersion,
                     ClUint32T inLen,
                     ClBufferHandleT outMsgHandle)
{
    ClRcT                    ret             = CL_OK;
    ClUint32T                cksum           = 0;
    ClCntNodeHandleT         pNodeHandle     = 0;
    ClNameSvcBindingT        *pStoredInfo    = NULL;
    ClCntHandleT             pStdInfo        = 0;
    ClNameSvcBindingDetailsT *pNSEntry       = NULL;
    ClNameSvcBindingDetailsT *pStoredNSEntry = NULL;
    ClIocNodeAddressT        sdAddr          = 0;
    ClNameSvcContextInfoPtrT pStatInfo       = NULL;
    ClUint32T                contextId       = 0, size=0;
    ClBufferHandleT          inMsgHandle     = 0;
    ClUint32T                noEntries       = 0;
    ClNameSvcAllBindingsGetT walkData        = {0};
    ClNameSvcNameLookupT     lookupData      = {0};

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside nameSvcQuery \n"));

    sdAddr = clIocLocalAddressGet();
    ret = clCpmMasterAddressGet(&gMasterAddress);
    if (ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",ret));
        return ret;
    }

    clLogDebug("SVR", "LOOKUP", "Looking up service [%.*s] at contextCookie [%#x] at addr [%d]",
            nsInfo->name.length, nsInfo->name.value, nsInfo->contextMapCookie, clIocLocalAddressGet());
    if(nsInfo->contextMapCookie == CL_NS_DEFT_GLOBAL_MAP_COOKIE)
        nsInfo->contextMapCookie = CL_NS_DEFAULT_GLOBAL_MAP_COOKIE;
    else if(nsInfo->contextMapCookie == CL_NS_DEFT_LOCAL_MAP_COOKIE)
        nsInfo->contextMapCookie = CL_NS_DEFAULT_LOCAL_MAP_COOKIE;

    ret = nameSvcContextFromCookieGet(nsInfo->contextMapCookie, &contextId);
    if(ret != CL_OK)
    {
        ret = CL_NS_RC(CL_NS_ERR_CONTEXT_NOT_CREATED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context not found in NS \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_NS_QUERY_FAILED, ret);
        CL_FUNC_EXIT();
        return ret;
    }
    clLogDebug("SVR", "LOOKUP", "Searching service [%.*s] at context [%d]", nsInfo->name.length,
            nsInfo->name.value, contextId);
    
    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Could not get Lock successfully---\n"));
    }
    
    ret = clCntNodeFind(gNSHashTable, (ClPtrT)(ClWordT)contextId, &pNodeHandle);
    if(ret != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_NS_QUERY_FAILED, ret);
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        CL_FUNC_EXIT();
        return ret;
    }
                                                                                                                             
    ret = clCntNodeUserDataGet(gNSHashTable,pNodeHandle,
                                (ClCntDataHandleT*)&pStatInfo);
    pStdInfo = pStatInfo->hashId;

    clNSLookupKeyForm(&nsInfo->name, &lookupData);
    clLogDebug("SVR", "LOOKUP", "Finding [%.*s] at hashId [%p] with cksum [%u]", 
              lookupData.name.length, lookupData.name.value, (void *)pStdInfo, 
              lookupData.cksum);
    ret = clCntNodeFind(pStdInfo, (ClCntKeyHandleT)(ClWordT)&lookupData, &pNodeHandle);
    if(ret != CL_OK)
    {
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        if((sdAddr == gMasterAddress) ||
           (contextId >= CL_NS_BASE_NODELOCAL_CONTEXT))
        {
            ret = CL_NS_RC(CL_NS_ERR_ENTRY_NOT_FOUND);
            clLogError("SVR", "LOOKUP", "Service [%.*s] is not available at context [%d] with rc[0x %x]", 
                    lookupData.name.length, lookupData.name.value, contextId, ret);
            /* Release the semaphore */
            clOsalMutexUnlock(gSem);
            return ret;
        }
        else
        {
            clLogError("SVR", "LOOKUP", "Entry is not found at local address[%d], forwarding to master [%d]", 
                   clIocLocalAddressGet(), gMasterAddress);
            clBufferCreate (&inMsgHandle);        
            VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(pVersion, inMsgHandle, 0);
            clXdrMarshallClUint32T((void *)&inLen, inMsgHandle, 0);
            VDECL_VER(clXdrMarshallClNameSvcInfoIDLT, 4, 0, 0)((void *)nsInfo, inMsgHandle, 0);

            CL_NS_CALL_RMD(gMasterAddress, CL_NS_QUERY, inMsgHandle, 
                        outMsgHandle, ret);
            /* Release the semaphore */
            clOsalMutexUnlock(gSem);
            /* delete the message created for querying NS/M */
            clBufferDelete(&inMsgHandle);
            CL_FUNC_EXIT();
            return ret;
        }
    }
    
    ret = clCntNodeUserDataGet(pStdInfo,pNodeHandle,
                               (ClCntDataHandleT*)&pStoredInfo);
    size = sizeof(ClNameSvcBindingDetailsT);

    if(nsInfo->attrCount == CL_NS_DEFT_ATTR_LIST)
    {
        ret = clCntNodeUserDataGet((ClCntHandleT)pStoredInfo->hashId, 
                               pStoredInfo->nodeHdl,
                               (ClCntDataHandleT*)&pStoredNSEntry);

        if (ret == CL_OK)
        {
            if(nsInfo->op == (ClInt32T)CL_NS_QUERY_OBJREF)
            {
                clXdrMarshallClUint64T((void *)&pStoredNSEntry->objReference,
                                    outMsgHandle, 0);            
            }
            else
            {
                nameSvcBindingIntoMessageCopy(pStoredNSEntry, &pStoredInfo->name,
                                              outMsgHandle, nsInfo->op);
            } 
        }
        else
        { 
            ret = CL_NS_RC(CL_NS_ERR_ENTRY_NOT_FOUND);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Entry not found in NS \n"));
            CL_FUNC_EXIT();
            /* Release the semaphore */
            clOsalMutexUnlock(gSem);
            clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                       CL_NS_LOG_1_NS_QUERY_FAILED, ret);
            return ret;
        }

        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        CL_FUNC_EXIT();
        return ret;
        
    }

    if(nsInfo->attrCount > 0)
    {
        size = size + (nsInfo->attrCount * sizeof(ClNameSvcAttrEntryT));
    }

    pNSEntry = (ClNameSvcBindingDetailsT *) clHeapAllocate(size);
    memset(pNSEntry, 0, size);

    if(nsInfo->attrCount > 0)
    {
        memcpy(pNSEntry->attr, nsInfo->attr, nsInfo->attrCount * 
               sizeof(ClNameSvcAttrEntryT));
        clCksm32bitCompute ((ClUint8T *)nsInfo->attr, 
               nsInfo->attrCount * sizeof(ClNameSvcAttrEntryT), 
               &cksum);
        pNSEntry->cksum     =  cksum;
        pNSEntry->attrCount = nsInfo->attrCount;
    }
    else
    {
        pNSEntry->cksum     = 0;
        pNSEntry->attrCount = 0;
    }

    ret = clCntNodeFind((ClCntHandleT)pStoredInfo->hashId, 
                        (ClCntKeyHandleT )pNSEntry,
                         &pNodeHandle);
    if(ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Entry not found in NS \n"));
        CL_FUNC_EXIT();
        clHeapFree(pNSEntry);
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_NS_QUERY_FAILED, ret);
        return CL_NS_RC(CL_NS_ERR_ENTRY_NOT_FOUND);
    }
    ret = clCntNodeUserDataGet(pStoredInfo->hashId, pNodeHandle,
                               (ClCntDataHandleT*)&pStoredNSEntry);
    switch(nsInfo->op)
    {
        case CL_NS_QUERY_OBJREF:
            clXdrMarshallClUint64T((void *)&pStoredNSEntry->objReference,
                                    outMsgHandle, 0);            
        break;
        case CL_NS_QUERY_ALL_MAPPINGS:
            memset(&walkData, 0, sizeof(ClNameSvcAllBindingsGetT));
            walkData.outMsgHandle = outMsgHandle;
            walkData.nsInfo       = (ClNameSvcInfoT*)nsInfo;
            ret = clCntWalk(pStoredInfo->hashId, _nameSvcAllBindingsGetCallback,
                    (ClCntArgHandleT) &walkData,
                    sizeof(ClNameSvcAllBindingsGetT));
        break;
        case CL_NS_QUERY_MAPPING:
            noEntries = 1;
        
            nameSvcBindingIntoMessageCopy(pStoredNSEntry, &pStoredInfo->name,
                                          outMsgHandle, nsInfo->op);
        break;
        default:
        break;
    }

    clHeapFree(pNSEntry);
    /* Release the semaphore */
    clOsalMutexUnlock(gSem);
    CL_FUNC_EXIT();
    return ret;
}



/**
 *  Name: nameSvcObjRefQuery
 *
 *  This function calls the appropriate query function
 *
 *  @param  data: rmd data
 *          inMsgHandle: contains the query related info
 *          outMsgHandle: will carry resultant entries(if needed)
 *
 *  @returns
 *    CL_OK                         - everything is ok <br>
 *    CL_ERR_INVALID_BUFFER    - when unable to retrieve info from inMsgHandle
 *    CL_ERR_NO_MEMORY         - malloc failure
 */
ClRcT VDECL(nameSvcQuery)(ClEoDataT data,  ClBufferHandleT  inMsgHandle,
                   ClBufferHandleT  outMsgHandle)
{
    ClRcT             ret     = CL_OK;
    ClNameSvcInfoIDLT *nsInfo = NULL;
    ClUint32T         inLen   = 0;
    ClNameVersionT    version = {0};
    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside nameSvcQuery \n"));

    /* Extract Version information */
    VDECL_VER(clXdrUnmarshallClNameVersionT, 4, 0, 0)(inMsgHandle, (void *)&version);

    clXdrUnmarshallClUint32T(inMsgHandle, (void *)&inLen);
    if(inLen >=  sizeof(ClNameSvcInfoIDLT))
    {
        nsInfo = (ClNameSvcInfoIDLT *)clHeapCalloc(1,inLen);
    }
    else
    {
        ret = CL_NS_RC(CL_ERR_INVALID_BUFFER);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Invalid Buffer Passed \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_NS_QUERY_FAILED, ret)
        CL_FUNC_EXIT();
        return ret;
    }
    if(nsInfo == NULL)
    {
        ret = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Malloc failed \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_NS_QUERY_FAILED, ret)
        CL_FUNC_EXIT();
        return ret;
    }

    VDECL_VER(clXdrUnmarshallClNameSvcInfoIDLT, 4, 0, 0)(inMsgHandle, (void *)nsInfo); 
    /* Version Verification */
    if(nsInfo->source != CL_NS_MASTER)
    {
        ret = clVersionVerify(&gNSClientToServerVersionInfo,(ClVersionT *)&version);
    }
    else
    {
        ret = clVersionVerify(&gNSServerToServerVersionInfo,(ClVersionT *)&version);
    }
    if(ret != CL_OK)
    { 
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Version not suppoterd \n"));
        if(nsInfo->attr) clHeapFree(nsInfo->attr);        
        clHeapFree(nsInfo);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_NS_QUERY_FAILED, ret);
        VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, outMsgHandle, 0);
        CL_FUNC_EXIT();
        return ret;
    }

    switch(nsInfo->op)
    {
        case CL_NS_QUERY_OBJREF:
        case CL_NS_QUERY_MAPPING:
            ret = nameSvcLAQuery((ClNameSvcInfoIDLT*)nsInfo, &version, 
                                  inLen, outMsgHandle);
        break;
        default:
        break;
    }
    
    if(nsInfo->attr != NULL)
        clHeapFree(nsInfo->attr);
    clHeapFree(nsInfo);
    CL_FUNC_EXIT();
    return ret;
}    



/**
 *  Name: _nameSvcPerContextInfo
 *
 *  This function id for listing names/bindings of all services registered
 *  in the given context
 *
 *  @param  data: rmd data
 *          inMsgHandle: contains the query related info
 *          outMsgHandle: not needed (for rmd compliance)
 *
 *  @returns
 *    CL_OK                         - everything is ok <br>
 *    CL_ERR_INVALID_BUFFER    - when unable to retrieve info from inMsgHandle
 *    CL_ERR_NO_MEMORY         - malloc failure
 *    CL_NS_ERR_CONTEXT_NOT_CREATED - trying to qury on a non existing context
 */
ClRcT _nameSvcPerContextInfo(ClUint32T contextMapCookie,
                            ClBufferHandleT  outMsgHandle)
{
    ClRcT                    ret         = CL_OK;
    ClCntNodeHandleT         pNodeHandle = 0;
    ClCntHandleT             pStdInfo    = 0;
    ClNameSvcContextInfoPtrT pStatInfo   = NULL;
    ClUint32T                contextId   = 0;
    ClNameSvcDisplayInfoT    walkInfo    = {{0}};

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside nameSvcPerContextInfo \n"));

    /* Find the "context" to look into */
    if(contextMapCookie == CL_NS_DEFT_GLOBAL_MAP_COOKIE)
        contextMapCookie = CL_NS_DEFAULT_GLOBAL_MAP_COOKIE;
    else if(contextMapCookie == CL_NS_DEFT_LOCAL_MAP_COOKIE)
        contextMapCookie = CL_NS_DEFAULT_LOCAL_MAP_COOKIE;

    ret = nameSvcContextFromCookieGet(contextMapCookie, &contextId);
    if(ret != CL_OK)
    {
        ret = CL_NS_RC(CL_NS_ERR_CONTEXT_NOT_CREATED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context not found in NS 1\n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                CL_NS_LOG_1_NS_DISPLAY_FAILED, ret);
        CL_FUNC_EXIT();
        return ret;
    }

    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Could not get Lock successfully---\n"));
    }
    
    ret = clCntNodeFind(gNSHashTable, (ClPtrT)(ClWordT)contextId, &pNodeHandle);
    if(ret != CL_OK)
    {
        ret = CL_NS_RC(CL_NS_ERR_CONTEXT_NOT_CREATED);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context not found in NS \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                CL_NS_LOG_1_NS_DISPLAY_FAILED, ret);
        /* Release the semaphore */
        clOsalMutexUnlock(gSem);
        CL_FUNC_EXIT();
        return ret;
    }

    ret = clCntNodeUserDataGet(gNSHashTable,pNodeHandle,
            (ClCntDataHandleT*)&pStatInfo);
    pStdInfo = pStatInfo->hashId;
    walkInfo.operation = CL_NS_LIST_BINDINGS;
    walkInfo.outMsgHdl = outMsgHandle;
    ret = clCntWalk(pStdInfo, _nameSvcEntryLevelWalkForDisplay, 
            (ClCntArgHandleT) &walkInfo, sizeof(walkInfo));
    /* Release the semaphore */
    clOsalMutexUnlock(gSem);
    CL_FUNC_EXIT();
    return ret;
}


/*************************************************************************/
/********************** Syncup related functions *************************/
/*************************************************************************/

ClRcT _nameSvcEntryPackCallback(ClCntKeyHandleT    userKey,
                                ClCntDataHandleT   nsInfo,
                                ClCntArgHandleT    info,
                                ClUint32T          dataLength)
{
    ClRcT                       ret        = CL_OK;
    ClUint32T                   count      = 0;
    ClNameSvcBindingDetailsIDLT nsIDLInfo  = {0};
    ClNameSvcCompListPtrT       pCompList  = NULL;
    ClNameSvcCompListIDLT       compList   = {0};
    ClNameSvcBindingDetailsT    *pNSEntry  = (ClNameSvcBindingDetailsT *)nsInfo;
    ClUint32T                    tag        = 3;

    ClNameSvcDBSyncupWalkInfoPtrT pWalkInfo = 
                 (ClNameSvcDBSyncupWalkInfoPtrT) info;
    ClBufferHandleT  data = pWalkInfo->outMsgHandle;
    

    ret = clXdrMarshallClUint32T(&tag, data, 0);
    if(CL_OK != ret)
    {
       CL_FUNC_EXIT();
       return(ret);
    }
    /*  
     * Copy the fields inline since the 2 structures could have holes resulting
     * in incorrect copy.
     */
    memset(&nsIDLInfo, 0, sizeof(nsIDLInfo));
    nsIDLInfo.cksum = pNSEntry->cksum;
    nsIDLInfo.objReference = pNSEntry->objReference;
    nsIDLInfo.refCount = pNSEntry->refCount;
    nsIDLInfo.dsId = pNSEntry->dsId;
    nsIDLInfo.attrCount = pNSEntry->attrCount;
    nsIDLInfo.attrLen = pNSEntry->attrLen;
    /*CL_ASSERT(sizeof(nsIDLInfo.compId) == sizeof(pNSEntry->compId));*/
    memcpy(&nsIDLInfo.compId, &pNSEntry->compId, sizeof(nsIDLInfo.compId));
    nsIDLInfo.attr = (ClNameSvcAttrEntryIDLT*) clHeapAllocate(nsIDLInfo.attrLen);
    if(!nsIDLInfo.attr)
    {
        clLogError("SVC", "PACK", "NS attribute allocation failed");
        return CL_NS_RC(CL_ERR_NO_MEMORY);
    }
    memcpy(nsIDLInfo.attr, pNSEntry->attr, nsIDLInfo.attrLen); 
    clXdrMarshallClUint32T(&nsIDLInfo.attrLen,data,0);
    nsIDLInfo.compId.pNext = 0;
    VDECL_VER(clXdrMarshallClNameSvcBindingDetailsIDLT, 4, 0, 0)(&nsIDLInfo, data, 0);
    clHeapFree(nsIDLInfo.attr);
    count = pNSEntry->refCount;
   
    pCompList = pNSEntry->compId.pNext;
    while(count > 0)
    {
        tag = 4;
        ret = clXdrMarshallClUint32T(&tag, data, 0);
        if(CL_OK != ret)
        {
           CL_FUNC_EXIT();
           return(ret);
        }
        memcpy(&compList, pCompList, sizeof(ClNameSvcCompListIDLT));
        compList.pNext = 0;
        VDECL_VER(clXdrMarshallClNameSvcCompListIDLT, 4, 0, 0)((ClNameSvcCompListIDLT*)&compList, data, 0);
        pCompList = pCompList->pNext;
        count --;
    }
    return ret;
}


/**
 *  Name: _nameSvcEntryLevelWalkForSyncup 
 *
 *  This function packs all the entires in the conetext being walked
 *
 *  @param  userKey: hashid of the entry
 *          nsInfo: entry details
 *          info: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK                 - everything is ok <br>
 */

ClRcT _nameSvcEntryLevelWalkForSyncup(ClCntKeyHandleT    userKey,
                                      ClCntDataHandleT   nsInfo,
                                      ClCntArgHandleT    info,
                                      ClUint32T          dataLength)
{
    ClRcT       ret   = CL_OK;
    ClUint32T    tag   = 2;

    ClNameSvcDBSyncupWalkInfoPtrT pWalkInfo = 
                 (ClNameSvcDBSyncupWalkInfoPtrT) info;
    ClBufferHandleT       data = pWalkInfo->outMsgHandle;
    
    ret = clXdrMarshallClUint32T(&tag, data, 0);
    if(CL_OK != ret)
    {
       CL_FUNC_EXIT();
       return(ret);
    }

    VDECL_VER(clXdrMarshallClNameSvcBindingIDLT, 4, 0, 0)((ClNameSvcBindingIDLT*)nsInfo, data, 0);
    ret = clCntWalk((ClCntHandleT) ((ClNameSvcBindingT*)nsInfo)->hashId,
                    _nameSvcEntryPackCallback,
                    (ClCntArgHandleT) info, dataLength);

    return ret;
}


/**
 *  Name: _nameSvcContextLevelWalkForSyncup 
 *
 *  This function walks through all the global contexts
 *  for packing the the entries 
 *
 *  @param  userKey: hashid of the context
 *          hashTable: context info
 *          info: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK                 - everything is ok <br>
 */

ClRcT _nameSvcContextLevelWalkForSyncup(ClCntKeyHandleT    userKey,
                                        ClCntDataHandleT   hashTable,
                                        ClCntArgHandleT    info,
                                        ClUint32T          dataLength)
{
    ClRcT     ret      = CL_OK;
    ClUint32T    contextId = (ClUint32T)(ClWordT)userKey;
    ClUint32T proceed  = 0;
    ClUint32T tag       = 1;
    ClNameSvcContextInfoPtrT      pStatInfo = (ClNameSvcContextInfoPtrT) hashTable;
    ClNameSvcDBSyncupWalkInfoPtrT pWalkInfo = (ClNameSvcDBSyncupWalkInfoPtrT) info;
    ClBufferHandleT data = pWalkInfo->outMsgHandle;
    ClNameSvcContextInfoIDLT contextInfo;

    memset(&contextInfo, 0, sizeof(ClNameSvcContextInfoIDLT));
    
    if(pWalkInfo->source == CL_NS_CKPT_LOCAL)
    {
        if(contextId >= CL_NS_BASE_NODELOCAL_CONTEXT)
            proceed = 1;
    }
    else
    {
        if(contextId <  CL_NS_BASE_NODELOCAL_CONTEXT)
            proceed = 1; 
    }
  
    if(proceed == 1)
    {
        if(pWalkInfo->operation == CL_NS_ENTRIES_PACK)
        {
            ret = clXdrMarshallClUint32T(&tag, data, 0);
            if(CL_OK != ret)
            {
               CL_FUNC_EXIT();
               return(ret);
            }

            contextInfo.hashId           = contextId;
            contextInfo.contextMapCookie = pStatInfo->contextMapCookie;
            VDECL_VER(clXdrMarshallClNameSvcContextInfoIDLT, 4, 0, 0)(&contextInfo, data, 0);
            ret = clCntWalk((ClCntHandleT) pStatInfo->hashId, 
                            _nameSvcEntryLevelWalkForSyncup,
                            (ClCntArgHandleT) info, dataLength);
        }
            
    }
    return ret;
}


ClRcT _nameSvcDBEntriesPack(ClBufferHandleT  outMsgHandle, ClUint32T flag)
{
    ClRcT                      ret      = CL_OK;
    ClNameSvcDBSyncupWalkInfoT walkInfo = {0};
 
    walkInfo.operation     = CL_NS_ENTRIES_PACK;
    walkInfo.outMsgHandle  = outMsgHandle;
    walkInfo.source        = flag;
    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Could not get Lock successfully---\n"));
    }

    gContextIndex = 0;
    gEntryIndex   = 0;
    ret = clCntWalk(gNSHashTable, _nameSvcContextLevelWalkForSyncup, (ClCntArgHandleT) &walkInfo, 0);

    outMsgHandle = walkInfo.outMsgHandle;
    /* Release the semaphore */
    clOsalMutexUnlock(gSem);
    CL_FUNC_EXIT();
    return ret;
}



/**
 *  Name: nameSvcDBEntriesPack 
 *
 *  This function packs the global scope DB entries at NS/M.
 *  This function is invoked whenever NS/L comes up and contacts NS/M
 *  for DB syncup
 *
 *  @param  data: rmd data
 *          inMsgHandle: Input buffer (contains version related info)
 *          outMsgHandle: will carry back the  packed information   
 *
 *  @returns
 *    CL_OK                 - everything is ok <br>
 *    CL_ERR_INVALID_BUFFER - when unable to retrieve info from inMsgHandle
 *    CL_ERR_NO_MEMORY      - malloc failure
 */

ClRcT VDECL(nameSvcDBEntriesPack)(ClEoDataT data, ClBufferHandleT inMsgHandle,
                           ClBufferHandleT  outMsgHandle)
{

    ClRcT           ret     = CL_OK;
    ClNameVersionT  version = {0};
    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside nameSvcDBEntriesPack \n"));

    /* Version Verification */
    VDECL_VER(clXdrUnmarshallClNameVersionT, 4, 0, 0)(inMsgHandle, (void *)&version);
    ret = clVersionVerify(&gNSServerToServerVersionInfo,(ClVersionT *)&version);
    if(ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Version not suppoterd \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                   CL_NS_LOG_1_SYNCUP_FAILED, ret); 
        CL_FUNC_EXIT();
        return ret;
    }

    ret = _nameSvcDBEntriesPack(outMsgHandle, CL_NS_FROM_SERVER);
    if(ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: DB Packing failed, ret = %x \n", ret));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_SYNCUP_FAILED, ret); 
    } 
    CL_FUNC_EXIT();
    return ret;
}


/**
 *  Name: nameSvcDBEntriesUnpack 
 *
 *  Unpack routine used during syncup at NS/L. NS DB is built at
 *  NS/L according to the unpacked information
 *
 *  @param  msgHdl: contains the packed information 
 *
 *  @returns
 *    CL_OK                 - everything is ok <br>
 *    CL_ERR_NO_MEMORY      - malloc failure
 */

ClRcT nameSvcDBEntriesUnpack(ClBufferHandleT msgHdl)
{
    ClNameSvcBindingDetailsT *pEntry     = NULL;
    ClRcT                    ret         = CL_OK;
    ClInt32T                 context     = 0;
    ClUint32T                priority    = 0;
    ClCntHandleT             *pTable     = NULL;
    ClCntNodeHandleT         pNodeHandle = 0;
    ClCntHandleT             pStoredInfo = 0;
    ClNameSvcContextInfoPtrT pContStat   = NULL;
    ClNameSvcContextInfoPtrT pStatInfo   = NULL;
    ClUint32T                count       = 0, attrCount = 0;
    ClNameSvcCompListPtrT    pContList   = NULL;
    ClNameSvcCompListT       tempComp    = {0};
    ClUint32T                contextMapCookie = 0, refCount = 0;
    ClUint32T                attrSize      = 0;
    ClNameT                  name          = {0}; 
    ClUint32T                bindingSize   = sizeof(ClNameSvcBindingT);
    ClUint32T                cksum         = 0;
    ClNameSvcBindingT        *pBindingInfo = NULL;
    ClUint32T                updatePriorityHash = 0;
    ClNameSvcNameLookupT     lookupData    = {0};
    ClUint32T                 tag           = 0;

    ClNameSvcContextInfoIDLT    contextInfo    = {0};
    ClNameSvcBindingIDLT        bindingInfo    = {0};
    ClNameSvcBindingDetailsIDLT bindingDetails = {0};
    ClNameSvcBindingDetailsT    *pBindData     = NULL;
    ClNameSvcCompListIDLT       compList       = {0};
    ClUint32T                   infoToBeAdded  = 0;
    ClUint32T                   attribLen      = 0;
    ClUint32T                     bufSize;
    ClUint32T                     curLoc;
    
    CL_FUNC_ENTER();

    clLogDebug("SVR", "INI", "Unpacking the entries after sync up from master");
    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("\n NS: Could not get Lock successfully---\n"));
    }

    clBufferLengthGet(msgHdl, &bufSize);    
    clBufferReadOffsetGet(msgHdl,&curLoc);
    
    while(1)
    {
        /* Buffer completely parsed is the termination condition */
        clBufferReadOffsetGet(msgHdl,&curLoc);
        if (curLoc == bufSize) break;

        /* Error probably corrupt buffer termination */
        if (CL_OK != clXdrUnmarshallClUint32T(msgHdl,&tag)) break;
        
        if((infoToBeAdded == 1) && (tag !=4))
        {
                if(attrCount > 0)
                {
                    clCksm32bitCompute ((ClUint8T *)pEntry->attr, attrCount * sizeof(ClNameSvcAttrEntryT), &cksum);
                    pEntry->cksum =  cksum;
                }
                else
                    pEntry->cksum = 0;
                                                                                                                             
                if( NULL != pBindingInfo )
                {
                    ret=clCntNodeAdd((ClCntHandleT)pBindingInfo->hashId,
                            (ClCntKeyHandleT )pEntry,
                            (ClCntDataHandleT )pEntry, NULL);
                    if( CL_OK == ret )
                    {
                        sObjCount++;
                    }
                }
                                                                                                                             
                if(updatePriorityHash == 1)
                {
                    if( NULL != pBindingInfo )
                    {
                        (void)clCntNodeFind((ClCntHandleT)pBindingInfo->hashId,
                                (ClCntKeyHandleT )pEntry,
                                &pBindingInfo->nodeHdl);
                    }
                    updatePriorityHash = 0;
                }
               infoToBeAdded = 0;
        }

        switch(tag)
        {
            case 1:
               infoToBeAdded = 0;
               VDECL_VER(clXdrUnmarshallClNameSvcContextInfoIDLT, 4, 0, 0)(msgHdl, &contextInfo); 
               context = contextInfo.hashId;
               contextMapCookie = contextInfo.contextMapCookie; 
 
               gpContextIdArray[context] = CL_NS_SLOT_ALLOCATED;
               /* check whether context is present or not */
               ret = clCntNodeFind(gNSHashTable, (ClPtrT)(ClWordT)context, &pNodeHandle);
               if(ret == CL_OK)
               {
                   /* Context is present */
                   clLogDebug("SVR", "INI", "Context [%d] is already created", context);
               }
               else
               {
                    /* context needs to be created */
                    pTable = (ClCntHandleT *) clHeapAllocate(sizeof(ClCntHandleT));
                    if(pTable == NULL)
                    {
                        ret = CL_NS_RC(CL_ERR_NO_MEMORY);
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
                        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                                   CL_NS_LOG_1_SYNCUP_FAILED, ret);
                        /* Release the semaphore */
                        clOsalMutexUnlock(gSem);
                        CL_FUNC_EXIT();
                        return ret;
                    }
                    ret = clCntHashtblCreate(gpConfig.nsMaxNoEntries,
                          nameSvcHashEntryKeyCmp,
                          nameSvcPerContextEntryHashFunction,
                          nameSvcHashEntryDeleteCallback,
                          nameSvcHashEntryDeleteCallback,
                          CL_CNT_UNIQUE_KEY, pTable);
                                                                                                                             
                   pContStat = (ClNameSvcContextInfoPtrT)
                                clHeapAllocate(sizeof(ClNameSvcContextInfoT));
                   if(pContStat == NULL)
                   {
                       ret = CL_NS_RC(CL_ERR_NO_MEMORY);
                       CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
                       clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                               CL_NS_LOG_1_SYNCUP_FAILED, ret);
                       /* Release the semaphore */
                       clOsalMutexUnlock(gSem);
                       CL_FUNC_EXIT();
                       return ret;
                   }
                                                                                                                             
                   pContStat->hashId            = *pTable;
                   pContStat->entryCount        = 0;
                   pContStat->dsIdCnt           = 1;
                   pContStat->contextMapCookie  = contextMapCookie;
                                                                                                                             
                   if(pTable)
                       clHeapFree(pTable);
                   ret = clCntNodeAdd(gNSHashTable, (ClCntKeyHandleT)(ClWordT) context,
                           (ClCntDataHandleT ) pContStat, NULL);
                                                                                                                             
                   /* Increment the count of created contexts */
                   if((context > CL_NS_DEFAULT_GLOBAL_CONTEXT) &&
                      (context < CL_NS_BASE_NODELOCAL_CONTEXT))
                   {
                       sNoUserGlobalCxt++;
                   }
                   if(context > CL_NS_BASE_NODELOCAL_CONTEXT)
                        sNoUserLocalCxt++;
                                                                                                                             
               }
               break;
            case 2:
               VDECL_VER(clXdrUnmarshallClNameSvcBindingIDLT, 4, 0, 0)(msgHdl, &bindingInfo);
               pBindingInfo = (ClNameSvcBindingT*) clHeapAllocate(bindingSize);
               clNameCopy(&name, &bindingInfo.name);
               refCount = bindingInfo.refCount;
               priority = bindingInfo.priority;
               clNameCopy(&pBindingInfo->name, &name);
               pBindingInfo->refCount = refCount;
               pBindingInfo->priority = priority;

               clLogDebug("SVR", "SYN", "Received service entry [%.*s]", name.length, name.value);
               clCntHashtblCreate(CL_NS_MAX_NO_OBJECT_REFERENCES,
                          nameSvcEntryDetailsHashKeyCmp,
                          nameSvcEntryDetailsHashFunction,
                          nameSvcHashEntryDetailsDeleteCallback,
                          nameSvcHashEntryDetailsDeleteCallback,
                          CL_CNT_UNIQUE_KEY, &pBindingInfo->hashId);
               clCksm32bitCompute ((ClUint8T *)&(name.value), name.length, &cksum);
               pBindingInfo->cksum = cksum;

               memset(&lookupData, 0, sizeof(lookupData));
               lookupData.cksum = cksum;
               clNameCopy(&lookupData.name, &name);
               /* Find the appropriate context */
               ret = clCntNodeFind(gNSHashTable, (ClPtrT)(ClWordT)context, &pNodeHandle);
               ret = clCntNodeUserDataGet(gNSHashTable,pNodeHandle,
                                (ClCntDataHandleT*)&pStatInfo);
               pStoredInfo = pStatInfo->hashId;
               /* Add the entry */
                                                                                                                             
               ret=clCntNodeAdd((ClCntHandleT)pStoredInfo,
                             (ClCntKeyHandleT)pBindingInfo,
                             (ClCntDataHandleT)pBindingInfo, NULL);
               if(ret != CL_OK)
               {
                   clOsalMutexUnlock(gSem);
                   return ret;
               }
               pStatInfo->entryCount++;

               break;
            case 3:
               infoToBeAdded = 1;
               clXdrUnmarshallClUint32T(msgHdl,&attribLen); 
               pBindData = (ClNameSvcBindingDetailsT *)clHeapCalloc(1, sizeof(*pBindData) + attribLen);
               if(pBindData == NULL) 
               {
                    ret = CL_NS_RC(CL_ERR_NO_MEMORY);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                               CL_NS_LOG_1_SYNCUP_FAILED, ret);
                    /* Release the semaphore */
                    clOsalMutexUnlock(gSem);
                    clHeapFree(pContStat);
                    CL_FUNC_EXIT();
                    return ret;
               }
               VDECL_VER(clXdrUnmarshallClNameSvcBindingDetailsIDLT, 4, 0, 0)(msgHdl, &bindingDetails);
               attrCount = bindingDetails.attrCount;
               attrSize = attrCount*sizeof(ClNameSvcAttrEntryT);
               CL_ASSERT(attrSize == attribLen);

               /*
                * copy inline as structures could have holes.
                */
               pBindData->cksum = bindingDetails.cksum;
               pBindData->objReference = bindingDetails.objReference;
               pBindData->refCount = bindingDetails.refCount;
               pBindData->dsId = bindingDetails.dsId;
               pBindData->attrCount = bindingDetails.attrCount;
               pBindData->attrLen = bindingDetails.attrLen;
               memcpy(&pBindData->compId, &bindingDetails.compId, sizeof(pBindData->compId));

               if(bindingDetails.attr != NULL)
               {
                   memcpy(pBindData->attr, bindingDetails.attr,attribLen);
                   clHeapFree(bindingDetails.attr);
               } 

               pEntry = (ClNameSvcBindingDetailsT*) clHeapAllocate(
                    sizeof(ClNameSvcBindingDetailsT) + attrSize);
               if(pEntry == NULL)
               {
                    ret = CL_NS_RC(CL_ERR_NO_MEMORY);
                    CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
                    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                               CL_NS_LOG_1_SYNCUP_FAILED, ret);
                    /* Release the semaphore */
                    clOsalMutexUnlock(gSem);
                    clHeapFree(pContStat);
                    CL_FUNC_EXIT();
                    return ret;
               }
               memcpy(pEntry, pBindData,  (sizeof(ClNameSvcBindingDetailsT) +
                      attrSize));
               pEntry->compId.pNext= NULL;
               if(priority == pEntry->compId.priority)
                   updatePriorityHash = 1;
                                                                                                                             
               count = pEntry->refCount;
               clHeapFree(pBindData);
               break;
            case 4:
               VDECL_VER(clXdrUnmarshallClNameSvcCompListIDLT, 4, 0, 0)(msgHdl, &compList);
               pContList = (ClNameSvcCompListPtrT)
                            clHeapAllocate(sizeof(ClNameSvcCompListT));
               if(pContList == NULL)
               {
                   ret = CL_NS_RC(CL_ERR_NO_MEMORY);
                   CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
                   clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                              CL_NS_LOG_1_SYNCUP_FAILED, ret);
                   /* Release the semaphore */
                   clOsalMutexUnlock(gSem);
                   clHeapFree(pContStat);
                   clHeapFree(pEntry);
                   CL_FUNC_EXIT();
                   return ret;
               }
               compList.pNext = 0;
               memcpy(pContList, &compList, sizeof(ClNameSvcCompListT));   
               if(priority == tempComp.priority)
                   updatePriorityHash = 1;
               if( NULL != pEntry )
               {
                   pContList->pNext = pEntry->compId.pNext;
                   pEntry->compId.pNext = pContList;
               }
               break;
            default:
               clLogError("DB","SYN","Invalid value of tag [%d] found. Unable to add entries",tag);
               goto errorExit;
        }
    }

    // There was some info to be added in the last iteration
    if(infoToBeAdded == 1)
    {
        if(attrCount > 0)
        {
            clCksm32bitCompute ((ClUint8T *)pEntry->attr,
                    attrCount * sizeof(ClNameSvcAttrEntryT),
                    &cksum);
            pEntry->cksum =  cksum;
        }
        else
            pEntry->cksum = 0;

        if( NULL != pBindingInfo )
        {
            ret=clCntNodeAdd((ClCntHandleT)pBindingInfo->hashId,
                    (ClCntKeyHandleT )pEntry,
                    (ClCntDataHandleT )pEntry, NULL);
            if( CL_OK == ret )
            {
                sObjCount++;
            }
        }

        if(updatePriorityHash == 1)
        {
            if( NULL != pBindingInfo )
            {
                (void)clCntNodeFind((ClCntHandleT)pBindingInfo->hashId,
                        (ClCntKeyHandleT )pEntry,
                        &pBindingInfo->nodeHdl);
            }
            updatePriorityHash = 0;
        }
        infoToBeAdded = 0;
    }

errorExit:
    /* Release the semaphore */
    clOsalMutexUnlock(gSem);

    CL_FUNC_EXIT();
    return ret;        
}


/**
 *  Name: nameSvcDBEntriesGet 
 *
 *  Makes rmd to NS/M for syncup purpose
 *
 *  @param  pOutHdl : will contain the packed syncup information
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT nameSvcDBEntriesGet(ClBufferHandleT pOutHdl)
{
    ClRcT            rc      = CL_OK;
    ClNameVersionT   version = {0};
    ClBufferHandleT  pInHdl  = 0;

    CL_FUNC_ENTER();
    CL_DEBUG_PRINT(CL_DEBUG_TRACE,("\n NS: Inside nameSvcDBEntriesGet \n"));
    rc = clBufferCreate(&pInHdl);
    if(CL_OK != rc) {
        return(rc);
    }

    CL_NS_SERVER_SERVER_VERSION_SET(&version);
    VDECL_VER(clXdrMarshallClNameVersionT, 4, 0, 0)(&version, pInHdl, 0);
    rc = clCpmMasterAddressGet(&gMasterAddress);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",rc));
        return rc;
    }

    CL_NS_CALL_RMD(gMasterAddress, CL_NS_DB_ENTRIES_PACK, pInHdl, pOutHdl, rc);
                                                                                                                      
    if(rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("nameSvcDBEntriesGet  Failed \n rc =%x",rc));
    }
                                                                                                                             
    /* delete the message */
    clBufferDelete(&pInHdl);
    CL_FUNC_EXIT();
    return rc;
}



/**
 *  Name: nameSvcDatabaseSyncup 
 *
 *  This function is called whenever NS/L comes up. It initiates
 *  the NS DB syncup process 
 *
 *  @param  none
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 */

ClRcT nameSvcDatabaseSyncup()
{
    ClRcT           ret     = CL_OK;
    ClUint32T       size    = 0;
    ClBufferHandleT pOutHdl = 0;
    
    ret = clBufferCreate(&pOutHdl);
    if(CL_OK != ret) {
        return(ret);
    }

    nameSvcDBEntriesGet(pOutHdl);

    clBufferLengthGet(pOutHdl, &size);
    nameSvcDBEntriesUnpack(pOutHdl);

    /* delete the message */
    clBufferDelete(&pOutHdl);

    return ret;
    
}


/*************************************************************************/
/********************* Lifecycle related functions ***********************/
/*************************************************************************/


/**
 *  Name: clNameInitialize 
 *
 *  This function is initialises NS EO, NS database. In case the instance is 
 *  of NS/L, the DB syncup with NS/M is initiated
 *
 *  @param  pConfig: Configuration information
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 *    CL_ERR_DUPLICATE    - trying to run another instance of NS on same node
 *    CL_ERR_NULL_POINTER - Null pointer passed

 */

ClRcT clNameInitialize(ClNameSvcConfigT* pConfig)
{
    ClRcT               rc           = CL_OK;
    ClIocNodeAddressT   sdAddr       = 0;
    ClEoExecutionObjT*  pEOObj       = NULL;
    ClVersionT*         pTempVersion = NULL;
    ClUint32T           count        = 0;
    CL_FUNC_ENTER();
                                                                                                                             
    if(sNSInitDone)
    {
        rc = CL_NS_RC(CL_ERR_DUPLICATE);
        clLogWarning("SVR", "INI", "Name service has been already initialized rc[0x %x]", rc);
        return rc;
    }

    if(pConfig == NULL)
    {
        rc = CL_NS_RC(CL_ERR_NULL_POINTER);     
        clLogWarning("SVR", "INI", "Name bootup failed due to config param was null rc[0x %x]", rc);
        return rc;
    }
    gpConfig.nsMaxNoEntries        = pConfig->nsMaxNoEntries;
    gpConfig.nsMaxNoGlobalContexts = pConfig->nsMaxNoGlobalContexts;
    gpConfig.nsMaxNoLocalContexts  = pConfig->nsMaxNoLocalContexts;

    gpContextIdArray = (ClUint32T*)clHeapAllocate
                          (CL_NS_MAX_NO_CONTEXTS * sizeof(ClUint32T));
    if(gpContextIdArray == NULL)
    {
        clLogCritical("SVR", "INI", "Context create failed due to memory alloc failure");
        rc = CL_NS_RC(CL_ERR_NO_MEMORY);
        return rc;
    }
    memset(gpContextIdArray, 0, 
           (CL_NS_MAX_NO_CONTEXTS * sizeof(ClUint32T)));
 
    /* Update version info */
    /* Client to server versioning */
    gNSClientToServerVersionInfo.versionCount = 
               sizeof(gNSClientToServerVersionSupported)/sizeof(ClVersionT);
    gNSClientToServerVersionInfo.versionsSupported = 
       (ClVersionT *)clHeapAllocate(
           gNSClientToServerVersionInfo.versionCount * sizeof(ClVersionT));
    memset(gNSClientToServerVersionInfo.versionsSupported,'\0',
           gNSClientToServerVersionInfo.versionCount * sizeof(ClVersionT));
    pTempVersion = gNSClientToServerVersionInfo.versionsSupported;
    for(count = 0;count <  gNSClientToServerVersionInfo.versionCount ;
        count++)
    {
      memcpy(gNSClientToServerVersionInfo.versionsSupported,
             &gNSClientToServerVersionSupported[count],sizeof(ClVersionT));
      gNSClientToServerVersionInfo.versionsSupported++;
    }
    gNSClientToServerVersionInfo.versionsSupported = pTempVersion;

    /* Server to server versioning */
    gNSServerToServerVersionInfo.versionCount = 
               sizeof(gNSServerToServerVersionSupported)/sizeof(ClVersionT);
    gNSServerToServerVersionInfo.versionsSupported = 
        (ClVersionT *)clHeapAllocate(
           gNSServerToServerVersionInfo.versionCount * sizeof(ClVersionT));
    memset(gNSServerToServerVersionInfo.versionsSupported,'\0',
           gNSServerToServerVersionInfo.versionCount * sizeof(ClVersionT));
    pTempVersion = gNSServerToServerVersionInfo.versionsSupported;
    for(count = 0;count <  gNSServerToServerVersionInfo.versionCount ;
        count++)
    {
      memcpy(gNSServerToServerVersionInfo.versionsSupported,
             &gNSServerToServerVersionSupported[count],sizeof(ClVersionT));
      gNSServerToServerVersionInfo.versionsSupported++;
    }
    gNSServerToServerVersionInfo.versionsSupported = pTempVersion;

    /* Initialize the NS database */
    clOsalMutexCreate(&gSem);
    rc = clCntHashtblCreate(CL_NS_MAX_NO_CONTEXTS, nameSvcHashKeyCmp,
                            nameSvcContextHashFunction, 
                            nameSvcHashContextDeleteCallback,
                            nameSvcHashContextDeleteCallback,
                            CL_CNT_UNIQUE_KEY, &gNSHashTable);
    rc = clCntHashtblCreate(gpConfig.nsMaxNoEntries, nameSvcHashEntryKeyCmp,
                            nameSvcPerContextEntryHashFunction, 
                            nameSvcHashEntryDeleteCallback, 
                            nameSvcHashEntryDeleteCallback,
                            CL_CNT_UNIQUE_KEY, &gNSDefaultGlobalHashTable);

    pgDeftGlobalHashTableStat = (ClNameSvcContextInfoPtrT) 
                                 clHeapAllocate(sizeof(ClNameSvcContextInfoT));
    if(pgDeftGlobalHashTableStat == NULL)
    {
        rc = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_NS_INIT_FAILED, rc);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, NULL, 
                   CL_LOG_MESSAGE_2_SERVICE_START_FAILED, "Name Service", rc);
        clHeapFree(gpContextIdArray);
        clHeapFree(gNSClientToServerVersionInfo.versionsSupported);
        clHeapFree(gNSServerToServerVersionInfo.versionsSupported);
        CL_FUNC_EXIT();
        return rc;
    }
    pgDeftGlobalHashTableStat->hashId            = gNSDefaultGlobalHashTable;
    pgDeftGlobalHashTableStat->entryCount        = 0;
    pgDeftGlobalHashTableStat->dsIdCnt           = 1;
    pgDeftGlobalHashTableStat->contextMapCookie  = 
                                              CL_NS_DEFAULT_GLOBAL_MAP_COOKIE;
    rc = clCntNodeAdd(gNSHashTable, 
                      (ClCntKeyHandleT) CL_NS_BASE_GLOBAL_CONTEXT,
                      (ClCntDataHandleT ) pgDeftGlobalHashTableStat, NULL);

    gpContextIdArray[CL_NS_BASE_GLOBAL_CONTEXT] = CL_NS_SLOT_ALLOCATED;

    rc = clCntHashtblCreate(gpConfig.nsMaxNoEntries, nameSvcHashEntryKeyCmp, 
                            nameSvcPerContextEntryHashFunction, 
                            nameSvcHashEntryDeleteCallback, 
                            nameSvcHashEntryDeleteCallback,
                            CL_CNT_UNIQUE_KEY, &gNSDefaultNLHashTable);

    pgDeftNLHashTableStat = (ClNameSvcContextInfoPtrT) 
                              clHeapAllocate(sizeof(ClNameSvcContextInfoT));
    if(pgDeftNLHashTableStat == NULL)
    {
        rc = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_NS_INIT_FAILED, rc);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, NULL, 
                   CL_LOG_MESSAGE_2_SERVICE_START_FAILED, "Name Service", rc);
        CL_FUNC_EXIT();
        clHeapFree(pgDeftGlobalHashTableStat);
        clHeapFree(gpContextIdArray);
        clHeapFree(gNSClientToServerVersionInfo.versionsSupported);
        clHeapFree(gNSServerToServerVersionInfo.versionsSupported);
        return rc;
    }
    pgDeftNLHashTableStat->hashId            = gNSDefaultNLHashTable;
    pgDeftNLHashTableStat->entryCount        = 0;
    pgDeftNLHashTableStat->dsIdCnt           = 1;
    pgDeftNLHashTableStat->contextMapCookie  = CL_NS_DEFAULT_LOCAL_MAP_COOKIE;

    rc = clCntNodeAdd(gNSHashTable,
                      (ClPtrT)(ClWordT)CL_NS_BASE_NODELOCAL_CONTEXT,
                      (ClCntDataHandleT ) pgDeftNLHashTableStat, NULL);

    gpContextIdArray[CL_NS_BASE_NODELOCAL_CONTEXT] = CL_NS_SLOT_ALLOCATED;

    /* Install the native function table with its EO */
    (void)clEoMyEoObjectGet(&pEOObj);

    rc = clEoClientInstallTables(pEOObj, CL_EO_SERVER_SYM_MOD(gAspFuncTable, NAM));
    if(CL_OK != rc)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Native clEoClientInstall failed \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_NS_INIT_FAILED, rc);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_CRITICAL, NULL, 
                   CL_LOG_MESSAGE_2_SERVICE_START_FAILED, "Name Service", rc);
        /* Do all necessary cleanups */
        (void)clIocCommPortDelete((sNameService.EOId)->commObj);
        clHeapFree((sNameService.EOId)->pClient);
        clHeapFree(sNameService.EOId);
        clHeapFree(pgDeftNLHashTableStat);
        clHeapFree(pgDeftGlobalHashTableStat);
        clHeapFree(gpContextIdArray);
        clHeapFree(gNSClientToServerVersionInfo.versionsSupported);
        clHeapFree(gNSServerToServerVersionInfo.versionsSupported);
        CL_FUNC_EXIT();
        return rc;
    }

    int retries = 5;
    do
    {
  	rc = clNameSvcCkptInit();
        clLogInfo("SVR", "INI", "clNameSvcCkptInit return  [0x %x] ",rc);
        if( CL_OK != rc )
        {
            retries--;
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clNameSvcCkptInit() rc[0x %x],try again", rc));
            if (sleep(1) != 0)
            {
                CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                        ("Failure in sleep system call: %s",strerror(errno)));
            }            
        }
        else
        {
            break;
        }
    }while(retries);
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_WARNING, NULL,
                   CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED, "ckpt", rc);
        return rc;
    }
    sNSInitDone = 1;
    sdAddr = clIocLocalAddressGet();
    rc = clCpmMasterAddressGet(&gMasterAddress);
    if (rc != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("clCpmMasterAddressGet failed with rc 0x%x",rc));
        return rc;
    }

    if( sdAddr != gMasterAddress )
    {
        clLogInfo("SVR", "INI", "Syncup the database from master [%d] at [%d]", 
                gMasterAddress, sdAddr);
        nameSvcDatabaseSyncup();
    }    
    /* Initializing  event lib  */
    rc = nameSvcEventInitialize();
    if(rc != CL_OK)
    {
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_WARNING, NULL, 
                CL_LOG_MESSAGE_2_LIBRARY_INIT_FAILED, "event", rc);
    }                                                                                                                        

    /* Register with debug infra */
    nameDebugRegister(pEOObj);
    
    clLogNotice("SVR", "INI", "Name server is fully up");

    if( CL_OK != (rc = clNameCkptCtxInfoWrite()) )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("Context info write failed\n"));
    }    
    if( sdAddr == gMasterAddress )
    {        
        if( CL_OK != (rc = clNameSvcPerCtxCkptCreate(
                        CL_NS_BASE_GLOBAL_CONTEXT, 0)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("PerCtx Info Create  failed\n"));
        }    
        if( CL_OK != (rc = clNameSvcPerCtxInfoWrite(CL_NS_BASE_GLOBAL_CONTEXT,
                        pgDeftGlobalHashTableStat)))
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("PerCtx Info Write failed\n"));
        }    
    }
    if( CL_OK != (rc = clNameSvcPerCtxCkptCreate(
                        CL_NS_BASE_NODELOCAL_CONTEXT, 0)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("PerCtx Ckpt Create failed\n"));
    }    
    if( CL_OK != (rc = clNameSvcPerCtxInfoWrite(CL_NS_BASE_NODELOCAL_CONTEXT,
                             pgDeftNLHashTableStat)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("PerCtx DataSetWrite failed"));
    }    
 
    CL_FUNC_EXIT();
    return rc;
}



/**
 *  Name: _nameSvcContextLevelWalkForFinalize
 *
 *  Deletes the contexts 
 *
 *  @param  userKey: hashid of the entry
 *          nsInfo:  entry details
 *          userArg: user passed arg
 *          dataLength: length of user passed arg
 *
 *  @returns
 *    CL_OK                 - everything is ok <br>
 */

ClRcT _nameSvcContextLevelWalkForFinalize(ClCntKeyHandleT    userKey,
                                          ClCntDataHandleT   hashTable,
                                          ClCntArgHandleT    userArg,
                                          ClUint32T          dataLength)
{
    ClNameSvcInfoIDLT      nsInfo      = {0};
    ClRcT               ret         = CL_OK;
    ClIocNodeAddressT   sAddr       = 0;
    ClBufferHandleT     inMsgHandle = 0 ;

    sAddr = clIocLocalAddressGet();
    ret = clBufferCreate (&inMsgHandle);

    memset(&nsInfo, 0, sizeof(nsInfo));
    nsInfo.version      = CL_NS_VERSION_NO;
    nsInfo.contextId    = (ClUint32T)(ClWordT)userKey;
    nsInfo.source       = CL_NS_LOCAL;
    ret = _nameSvcContextDeleteLocked(&nsInfo, 1, NULL);
    if(ret != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: Context deletion failed :%x \n",
                    ret));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                   CL_NS_LOG_1_CONTEXT_DELETION_FAILED, ret);
    }
      
    /* delete the message */
    clBufferDelete(&inMsgHandle);
     
    return ret;
}

ClRcT
clNameSvcCtxRecreate(ClUint32T   key)
{
    ClRcT                  rc        = CL_OK;
    ClNameSvcContextInfoT  *pCtxData = NULL;

    clLogDebug("SVR", "RESTART", "Context [%d] is to be created", 
            key);
    rc = clCntDataForKeyGet(gNSHashTable, (ClCntKeyHandleT)(ClWordT)key,
                            (ClCntDataHandleT *)&pCtxData);
    if( CL_OK != rc )
    {
        pCtxData = clHeapCalloc(1, sizeof(ClNameSvcContextInfoT));
        if( NULL == pCtxData )
        {
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clHeapCalloc()"));
            return CL_NS_RC(CL_ERR_NO_MEMORY);
        }    
        rc = clCntHashtblCreate(gpConfig.nsMaxNoEntries,
                nameSvcHashEntryKeyCmp,
                nameSvcPerContextEntryHashFunction,
                nameSvcHashEntryDeleteCallback,
                nameSvcHashEntryDeleteCallback,
                CL_CNT_UNIQUE_KEY, &pCtxData->hashId);
        if( CL_OK != rc )
        {
            clHeapFree(pCtxData);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clCntHashtblCreate(): rc[0x %x]", rc));
            return rc;
        }    
        rc = clCntNodeAdd(gNSHashTable, (ClCntKeyHandleT)(ClWordT) key,
                         (ClCntDataHandleT) pCtxData, NULL);
        if( CL_OK != rc )
        {
            clCntDelete(pCtxData->hashId);
            clHeapFree(pCtxData);
            CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clCntNodeAdd(): rc[0x %x]", rc));
            return rc;
        }    
        if((key > CL_NS_DEFAULT_GLOBAL_CONTEXT) &&
           (key < CL_NS_BASE_NODELOCAL_CONTEXT))
        {
            sNoUserGlobalCxt++;
        }
        if(key > CL_NS_BASE_NODELOCAL_CONTEXT)
            sNoUserLocalCxt++;
        gpContextIdArray[key] = CL_NS_SLOT_ALLOCATED;
    }    
    
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

static ClRcT nameSvcEntryRecreate(ClUint32T key, ClUint32T dsId, 
                                  ClNameT *nsCkptName, ClNameSvcContextInfoT *pCtxData)
{
    ClRcT rc = CL_OK;
    ClNsEntryPackT nsEntryInfo  = {0};

    rc = clNameSvcPerCtxDataSetCreate(key, dsId);

    if( CL_OK != rc )
    {
        return rc;
    }

    rc = clCkptLibraryCkptDataSetRead(gNsCkptSvcHdl, nsCkptName,
                                      dsId, &nsEntryInfo);
    if( CL_OK != rc )
    {
        return rc;
    }    

    rc = clNameSvcBindingEntryRecreate(pCtxData, nsEntryInfo);
    if( CL_OK != rc )
    {
        return rc;
    }    

    return rc;
}

ClRcT
clNameSvcEntryRecreate(ClUint32T              key,
                       ClNameSvcContextInfoT  *pCtxData)
{
    ClRcT          rc           = CL_OK;
    ClNameT        nsCkptName   = {0};
    ClUint32T      count        = 0;
    ClNameSvcContextInfoT contextInfo = {0};
    
    CL_NAME_DEBUG_TRACE(("Enter: key %d", key));
    clNamePrintf(nsCkptName, "clNSCkpt%d", key);

    rc = clNameSvcPerCtxCkptCreate(key, 0);
    if(CL_OK != rc)
    {
        return rc;
    }

    rc = clCkptLibraryCkptDataSetRead(gNsCkptSvcHdl, &nsCkptName,
                                      CL_NS_PER_CTX_DSID,
                                      &contextInfo);

    if( CL_OK != rc )
    {
        return rc;
    }    
    pCtxData->dsIdCnt = contextInfo.dsIdCnt;
    pCtxData->contextMapCookie = contextInfo.contextMapCookie;

    for(count = CL_NS_RESERVED_DSID; count < sizeof(pCtxData->freeDsIdMap); ++count)
    {
        register ClInt32T j;
        if(!pCtxData->freeDsIdMap[count]) continue;
        for(j = 0; j < 8; ++j)
        {
            ClUint32T dsId = 0;
            if(!(pCtxData->freeDsIdMap[count] & ( 1 << j))) continue;
            dsId = ( count << 3) + j;
            rc = nameSvcEntryRecreate(key, dsId, &nsCkptName, pCtxData);
            if(rc != CL_OK)
                return rc;
        }    
    }
    
    for(count = (ClUint32T)sizeof(pCtxData->freeDsIdMap) << 3;
        count <= pCtxData->dsIdCnt; ++count)
    {
        nameSvcEntryRecreate(key, count, &nsCkptName, pCtxData);
    }

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    

ClRcT
clNameSvcPerCtxInfoUpdate(ClNameSvcContextInfoT  *pCtxData,
                          ClNameSvcInfoIDLT      *pData)
{
    ClRcT  rc = CL_OK;

    pCtxData->dsIdCnt          = pData->dsIdCnt;
    pCtxData->contextMapCookie = pData->contextMapCookie;
    clLogDebug("SVR", "RESTART", "Received dsId [%d] mapCookie [%#X]", 
                                pCtxData->dsIdCnt, pCtxData->contextMapCookie); 
    return rc;
}    

ClRcT
clNameSvcCompInfoAdd(ClNameSvcContextInfoT *pCtxData,
                     ClNameSvcInfoIDLT     *pData)
{
    ClRcT                     rc           = CL_OK;
    ClNameSvcNameLookupT      lookupData   = {0};
    ClNameSvcBindingT         *pBindData   = NULL;
    ClNameSvcBindingDetailsT  *pNSEntry    = NULL;
    ClUint32T                 cksum        = 0;
    ClNameSvcBindingDetailsT  *pStoredData = NULL;
    ClNameSvcCompListT        *pTemp       = NULL;
    ClNameSvcCompListT        *pCompList   = NULL;
    ClCntDataHandleT           dataHdl     = 0;

    CL_NAME_DEBUG_TRACE(("Enter"));

    clNSLookupKeyForm(&pData->name, &lookupData);
    rc = clCntDataForKeyGet(pCtxData->hashId,
                            (ClCntKeyHandleT)&lookupData,
                            &dataHdl);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCntDataForKeyGet(); rc[0x %x]", rc));
        return rc;
    }        
    pBindData = (ClNameSvcBindingT *)dataHdl;
    pNSEntry = clHeapCalloc(sizeof(ClNameSvcBindingDetailsT) + 
                            pData->attrCount * sizeof(ClNameSvcAttrEntryT), 
                            sizeof(ClCharT));
    if( NULL == pNSEntry )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHeapCalloc()"));
        return rc;
    }
    pNSEntry->objReference    = pData->objReference;
    pNSEntry->compId.priority = pData->priority;
    pNSEntry->compId.compId   = pData->compId;
    pNSEntry->compId.eoID   = pData->eoID;
    pNSEntry->compId.nodeAddress   = pData->nodeAddress;
    pNSEntry->compId.clientIocPort   = pData->clientIocPort;
    pNSEntry->compId.pNext    = NULL;
    pNSEntry->attrCount       = pData->attrCount;
    pNSEntry->attrLen         = pData->attrCount *
                                sizeof(ClNameSvcAttrEntryT);
    if( pData->attrCount > 0 )
    {
        memcpy(pNSEntry->attr, pData->attr, pData->attrCount * 
               sizeof(ClNameSvcAttrEntryT));
        clCksm32bitCompute ((ClUint8T *)pData->attr, 
               pData->attrCount * sizeof(ClNameSvcAttrEntryT), 
               &cksum);
         pNSEntry->cksum =  cksum;
    }
    else
         pNSEntry->cksum = 0;

    /* Add entry detailsto the entry details hash table */
    rc = clCntDataForKeyGet((ClCntHandleT)pBindData->hashId,
                            (ClCntKeyHandleT)pNSEntry,
                            &dataHdl);
    if( CL_OK != rc )
    {
        clHeapFree(pNSEntry);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCntNodeAdd(): rc[0x %x]", rc));
        return rc;
    }        
    pStoredData = (ClNameSvcBindingDetailsT *)dataHdl;
    pTemp = &(pStoredData->compId);
    while( pTemp != NULL )
    {
        if( pTemp->compId == pNSEntry->compId.compId ) 
        {
            clHeapFree(pNSEntry);
            return CL_OK;
        }
        pTemp = pTemp->pNext;
    }
    pStoredData->refCount++;
    pCompList = (ClNameSvcCompListPtrT) 
        clHeapAllocate(sizeof(ClNameSvcCompListT));
    if(pCompList == NULL)
    {
        clHeapFree(pNSEntry);
        rc = CL_NS_RC(CL_ERR_NO_MEMORY);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("\n NS: MALLOC FAILED \n"));
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL, 
                CL_NS_LOG_1_NS_REGISTRATION_FAILED, rc);
        return rc;
    }
    pCompList->pNext    = pStoredData->compId.pNext;
    pCompList->priority = pData->priority; 
    pCompList->dsId     = pData->dsId;
    if(pCompList->priority > pBindData->priority)
    {    
        pBindData->priority   = pCompList->priority;
        (void)clCntNodeFind(pBindData->hashId,
                      (ClCntKeyHandleT)pNSEntry,
                      (ClCntNodeHandleT *)&(pBindData->nodeHdl));
    }    
    pStoredData->compId.pNext = pCompList;
    pCompList->compId         = pNSEntry->compId.compId;

    clHeapFree(pNSEntry);
    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}            

ClRcT
clNameSvcBindingDetailEntryCreate(ClNameSvcContextInfoT *pCtxData,
                                  ClNameSvcInfoIDLT     *pData)
{
    ClRcT                     rc         = CL_OK;
    ClNameSvcNameLookupT      lookupData = {0};
    ClNameSvcBindingT         *pBindData = NULL;
    ClNameSvcBindingDetailsT  *pNSEntry  = NULL;
    ClUint32T                  cksum     = 0;

    clLogDebug("SVR", "RESTART", "Binding entry for name: %.*s", 
            pData->name.length, pData->name.value);
    rc = clNSLookupKeyForm(&pData->name, &lookupData);
    if( CL_OK != rc )
    {
        clLogError("SVR", CL_LOG_CONTEXT_UNSPECIFIED, 
                   "Lookup key form failed rc[0x %x]", rc);
        return rc;
    }
    rc = clCntDataForKeyGet(pCtxData->hashId,
                            (ClCntKeyHandleT)&lookupData,
                            (ClCntDataHandleT *)&pBindData);
    if( CL_OK != rc )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCntDataForKeyGet(); rc[0x %x]", rc));
        return rc;
    }        
    pNSEntry = clHeapCalloc(sizeof(ClNameSvcBindingDetailsT) + 
                            pData->attrCount * sizeof(ClNameSvcAttrEntryT), 
                            sizeof(ClCharT));
    if( NULL == pNSEntry )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHeapCalloc()"));
        return rc;
    }
    pNSEntry->objReference = pData->objReference;
    pNSEntry->compId.priority = pData->priority;
    pNSEntry->compId.compId   = pData->compId;
    pNSEntry->compId.eoID   = pData->eoID;
    pNSEntry->compId.nodeAddress   = pData->nodeAddress;
    pNSEntry->compId.clientIocPort   = pData->clientIocPort;
    pNSEntry->compId.pNext    = NULL;
    pNSEntry->attrCount       = pData->attrCount;
    pNSEntry->attrLen         = pData->attrCount *
                                sizeof(ClNameSvcAttrEntryT);
    pNSEntry->dsId            = pData->dsId;
    if( pData->attrCount > 0 )
    {
        memcpy(pNSEntry->attr, pData->attr, pData->attrCount * 
               sizeof(ClNameSvcAttrEntryT));
        clCksm32bitCompute ((ClUint8T *)pData->attr, 
               pData->attrCount * sizeof(ClNameSvcAttrEntryT), 
               &cksum);
         pNSEntry->cksum =  cksum;
    }
    else
         pNSEntry->cksum = 0;

    /* Add entry detailsto the entry details hash table */
    rc = clCntNodeAdd((ClCntHandleT)pBindData->hashId, 
                      (ClCntKeyHandleT )pNSEntry, 
                      (ClCntDataHandleT )pNSEntry, NULL);
    if( CL_OK != rc )
    {
        clHeapFree(pNSEntry);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCntNodeAdd(): rc[0x %x]", rc));
        return rc;
    }        
    if( CL_OK != (rc = clCntNodeFind((ClCntHandleT)pBindData->hashId,
                                (ClCntKeyHandleT)pNSEntry,
                                (ClCntNodeHandleT *)&pBindData->nodeHdl)))
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                    ("clCntNodeFind(): rc[0x %x]", rc));
    }
    pBindData->priority = pData->priority;
    pBindData->refCount++;

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}

ClRcT
clNameSvcBindingEntryCreate(ClNameSvcContextInfoT  *pCtxData,
                            ClNameSvcInfoIDLT      *pData)
{
    ClRcT              rc         = CL_OK;
    ClNameSvcBindingT  *pBindData = NULL;
    ClUint32T           cksum      = 0;

    clLogDebug("SVR", "RESTART", "Service name: %.*s", 
            pData->name.length, pData->name.value);
    pBindData = (ClNameSvcBindingT*) clHeapCalloc(1,
                        sizeof(ClNameSvcBindingT));
    if( NULL == pBindData )
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clHeapCalloc(): rc[0x %x]", rc));
        return rc;
    }
    clNameCopy(&pBindData->name, &pData->name);
    (pBindData)->priority = pData->priority;
    rc = clCntHashtblCreate(CL_NS_MAX_NO_OBJECT_REFERENCES,
            nameSvcEntryDetailsHashKeyCmp,
            nameSvcEntryDetailsHashFunction,
            nameSvcHashEntryDetailsDeleteCallback,
            nameSvcHashEntryDetailsDeleteCallback,
            CL_CNT_UNIQUE_KEY, &((pBindData)->hashId));
    if( CL_OK != rc )
    {
        clHeapFree(pBindData);
        CL_DEBUG_PRINT(CL_DEBUG_ERROR,
                ("clCntHashtblCreate(): rc[0x %x]", rc));
        return rc;
    }    
    pBindData->dsId  = pData->dsId;
    clCksm32bitCompute ((ClUint8T *)(pData->name.value), pData->name.length, &cksum);
    pBindData->cksum = cksum;
    /* Add the entry */
    rc = clCntNodeAdd(pCtxData->hashId,
            (ClCntKeyHandleT)(pBindData),
            (ClCntDataHandleT)(pBindData), NULL);
    if(rc != CL_OK)
    {
        clCntDelete((pBindData)->hashId);
        clHeapFree(pBindData);
        clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_DEBUG, NULL,
                CL_NS_LOG_1_SYNCUP_FAILED, rc);
        return rc;
    }
    pCtxData->entryCount++;

    CL_NAME_DEBUG_TRACE(("Exit"));
    return rc;
}    



/**
 *  Name: clNameFinalize 
 *
 *  This function the cleanup function. It deletes all the NS entries and
 *  contexts and stops NS EO.
 *
 *  @param  pConfig: Configuration information
 *
 *  @returns
 *    CL_OK               - everything is ok <br>
 *    CL_ERR_DUPLICATE    - trying to run another instance of NS on same node
 *    CL_ERR_NULL_POINTER - Null pointer passed

 */

ClRcT clNameFinalize()
{
    return CL_OK;
}


ClUint32T nameSvcMaxNoGlobalContextGet()
{
    return  gpConfig.nsMaxNoGlobalContexts;
}

ClUint32T nameSvcMaxNoLocalContextGet()
{
    return  gpConfig.nsMaxNoLocalContexts;
}

void invokeWalkForDelete(ClNameSvcDeregisInfoT *walkData)
{
    /* take the semaphore */
    if(clOsalMutexLock(gSem)  != CL_OK)
    {
        CL_DEBUG_PRINT(CL_DEBUG_ERROR, 
                  ("\n NS: Could not get Lock successfully------\n"));
    }

    gNameEntryDelete   = 0;
    gDelete            = 0;
 
    clCntWalk(gNSHashTable, _nameSvcContextLevelWalkForDelete, 
                    (ClCntArgHandleT)walkData, sizeof(ClUint32T));

    /* Release the semaphore */
    clOsalMutexUnlock(gSem);
}

                                                                                                                             
ClEoConfigT clEoConfig = {
    "NAM", 
    1,     
    2,     
    CL_IOC_NAME_PORT,
    CL_EO_USER_CLIENT_ID_START,
    CL_EO_USE_THREAD_FOR_RECV,
    nameSvcInitialize,
    clNameFinalize,
    nameSvcStateChange,
    nameSvcHealthCheck,
    NULL
    };

/* What basic and client libraries do we need to use? */
ClUint8T clEoBasicLibs[] = {
    CL_TRUE,    		/* osal */
    CL_TRUE,    		/* timer */
    CL_TRUE,    		/* buffer */
    CL_TRUE,    		/* ioc */
    CL_TRUE,    		/* rmd */
    CL_TRUE,    		/* eo */
    CL_FALSE,    		/* om */
    CL_FALSE,    		/* hal */
    CL_TRUE,    		/* dbal */
};
  
ClUint8T clEoClientLibs[] = {
    CL_FALSE,    		/* cor */
    CL_FALSE,    		/* cm */
    CL_TRUE,    		/* name */
    CL_TRUE,    		/* log */
    CL_FALSE,    		/* trace */
    CL_FALSE,    		/* diag */
    CL_FALSE,    		/* txn */
    CL_FALSE,    		/* hpi */
    CL_FALSE,    		/* cli */
    CL_FALSE,    		/* alarm */
    CL_TRUE,    		/* debug */
    CL_FALSE,    		/* gms */
    CL_FALSE,    		/* pm */
};

