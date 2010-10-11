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
 * ModuleName  : ckpt                                                          
 * File        : clCkptUtils.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains Checkpoint service utility routines
*
*
*****************************************************************************/
/* System includes */
#include <string.h>

/* Clovis Common includes */
#include <clCommon.h>
#include <clCntApi.h>
#include <clCksmApi.h>
#include <clDebugApi.h>
#include <clVersionApi.h>
#include <clIocApi.h>
#include <clLogApi.h>
#include <clIdlApi.h>
#include <clVersion.h>

/* Ckpt specific includes */
#include "clCkptSvr.h"
#include "clCkptUtils.h"
#include "clCkptErrors.h"
#include "clCkptIpi.h"
#include "clCkptLog.h"
#include "clCkptMaster.h"
#include "xdrCkptXlationDBEntryT.h"
#include "xdrCkptMasterDBClientInfoT.h"
#include <ckptEockptServerCliServerFuncServer.h>
#include <ckptEockptServerPeerPeerClient.h>

/* Handle related headers */
#include <clHandleApi.h>
#include <ipi/clHandleIpi.h>

/* #include "inc/ckptSvrCompId.h"*/
extern CkptSvrCbT  *gCkptSvr;


#define CL_CKPT_MAX_SCN_BKTS     1024
ClHandleDatabaseHandleT gMasterDBHdl;

/* Elements compare routines */
ClInt32T  ckptNameCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
ClInt32T  ckptHdlNameCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
ClInt32T  ckptPeerCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
ClInt32T  ckptPrefNodeCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);

/* Delete callbacks */
void ckptHdlInfoDelete();
void    ckptSvrHdlDeleteCallback(ClCntKeyHandleT userKey, ClCntDataHandleT userData);
void    ckptPeerDeleteCallback( ClCntKeyHandleT userKey,
                                ClCntDataHandleT userData);
void    ckptClntDeleteCallback(ClCntKeyHandleT userKey,
                               ClCntDataHandleT userData);

ClInt32T ckptHdlNameCompare(ClCntKeyHandleT key1,
                                   ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}



/*
 * Hashing function to distribute peers in the hash table.
 */
ClUint32T   ckptPeerHashFunc(ClCntKeyHandleT userKey)
{
         return 0;
}
        


/*
 * Function to compare two checkpoints identified by a name
 */
 
ClInt32T  ckptNameCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClNameT       *pName1 = (ClNameT *)key1;
    ClNameT       *pName2 = (ClNameT *)key2;
    ClUint32T    minLen = 0;
    ClInt32T     result = 0; 
    
    if( (pName1 == NULL) || (pName2 == NULL))
        return -1;

    /* 
     * minLen is the smallest string 
     */
    minLen =(pName1->length  < pName2->length) ? 
                 pName1->length: pName2->length;

    result = memcmp(pName1->value, pName2->value, minLen);

    if (result == 0)  /* String1 and string2 are same upto minLen */
        return (pName1->length  - pName2->length);
    else 
        return result;
    return 0;
}



/*
 * Function to compare two peers identified by a nodeId
 */
 
ClInt32T  ckptPeerCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key1 - (ClWordT)key2);
}



ClInt32T  ckptPrefNodeCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    return ((ClWordT)key2-(ClWordT)key1);
}

ClInt32T  clCkptAppKeyInfoCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2)
{
    ClCkptAppInfoT  *pKey1 = (ClCkptAppInfoT *)(ClWordT) key1;
    ClCkptAppInfoT  *pKey2 = (ClCkptAppInfoT *)(ClWordT) key2;

    if( pKey2->nodeAddress == pKey1->nodeAddress )
    {
        if( pKey2->portId == pKey1->portId )
        {
            return 0;
        }
    }
    return -1; 
}
void clCkptAppKeyDelete(ClCntKeyHandleT key, ClCntDataHandleT data)
{
    clHeapFree((ClCkptAppInfoT *)(ClWordT) key);
    clHeapFree((ClUint32T *)(ClWordT) data);
    return;
}
/*
   Function to compare two clients dentified by a port 
 */




/* 
 * Function to delete a checkpoint whenever a container is 
 * destroyed/an element is deleted from the container
 */
 
void  ckptMasterHdlDeleteCallback( void *pData)
{
    CkptMasterDBEntryT  *pMasterDBEntry = (CkptMasterDBEntryT *)pData;

    clDbgCheckNull(pData,CL_CID_CKPT);
    if (NULL != pData)
    {
        /*
         * Delete the associated replica list.
         */
        if(pMasterDBEntry->replicaList != 0)
        {
          CL_DEBUG_PRINT(CL_DEBUG_INFO,("Deleting checkpoint [%s]'s replica list in the master handle delete callback",pMasterDBEntry->name.value));

          clCntDelete(pMasterDBEntry->replicaList);
          pMasterDBEntry->replicaList = 0;
        }
    }
    return;
}



/*
 * Delete/destroy callback function for checkpoint handle list
 * maintained at the active server. This callback will be invoked 
 * whenever a checkpoint is removed form that node.
 */
 
void    ckptSvrHdlDeleteCallback(ClCntKeyHandleT userKey, 
                                 ClCntDataHandleT userData)
{
    ClCkptHdlT  ckptHdl = *(ClCkptHdlT *)userData;
    CkptT       *pCkpt  = NULL;
    ClRcT       rc      = CL_OK;
    ClOsalMutexIdT ckptMutex;
    rc = clHandleCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);
    if( (CL_OK != rc) || (NULL == pCkpt) )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_DEL, 
                "Failed to checkout handle [%#llX] while deleting rc [0x %x]",
                ckptHdl, rc);
        return;
    }
    ckptMutex = pCkpt->ckptMutex;
    CKPT_LOCK(ckptMutex);
    pCkpt->ckptMutex = CL_HANDLE_INVALID_VALUE;
    ckptEntryFree(pCkpt);
    CKPT_UNLOCK(ckptMutex);
    clOsalMutexDelete(ckptMutex);
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    clHandleDestroy(gCkptSvr->ckptHdl,ckptHdl);
    clLogDebug(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_DEL, 
               "Deleted the checkpoint handle [%#llX]", ckptHdl);
    
    clHeapFree(userData);
    return;
}



/* 
 * Function to delete a checkpoint peer  whenever a container is 
 * destroyed/an element is deleted from the container. This container
 * is maintained by active server.
 */
 
void  ckptPeerDeleteCallback(ClCntKeyHandleT userKey,
                             ClCntDataHandleT userData)
{
    CkptPeerT   *pPeer = (CkptPeerT *)userData;
    clHeapFree(pPeer);
    return;
}



/* 
 * Function to delete a checkpoint peer  whenever a container is 
 * destroyed/an element is deleted from the container. This container
 * is maintained by master server.
 */
 
void    ckptPeerListDeleteCallback(ClCntKeyHandleT  userKey,
                                   ClCntDataHandleT userData)
{
    CkptPeerInfoT   *pPeer = userData;
    
    /*
     * Delete the client handle list and master handle list.
     */

    if(pPeer->ckptList != 0)
    {
        clCntDelete(pPeer->ckptList);
        pPeer->ckptList = 0;
    }

    if(pPeer->mastHdlList != 0)
    {
        clCntDelete(pPeer->mastHdlList);
        pPeer->mastHdlList = 0;
    }

    clHeapFree(pPeer);
    return;
}



/* 
 *  Function to delete a checkpoint client whenever a container is 
 *  destroyed/an element is deleted from the container of both
 *  master and active server.
 */
 
void    ckptClntDeleteCallback( ClCntKeyHandleT userKey,
                                ClCntDataHandleT userData)
{
    return;
}



/*
 * Key compare function associated with the name to handle Xlation table.
 */
 
static ClInt32T ckptXlationHashKeyCmp(ClCntKeyHandleT key1,
                                      ClCntKeyHandleT key2)
{
    CkptXlationDBEntryT  *pInfo1 = (CkptXlationDBEntryT*)key1;
    CkptXlationLookupT   *pInfo2 = (CkptXlationLookupT*)key2;
    
    /*
     * Compare the name as well as the associated checksum.
     */
    if((pInfo1->cksum == pInfo2->cksum) &&
       !strcmp(pInfo1->name.value, pInfo2->name.value))
    {
        return 0;
    }
    else
        return 1;
}



/*
 * Hash function associated with the name to handle Xlation table.
 */
 
static ClUint32T ckptXlationHashFunction(ClCntKeyHandleT key)
{
    CkptXlationLookupT  *pInfo = (CkptXlationLookupT*)key;
    return(ClInt32T)((pInfo->cksum>>8)%(CL_CKPT_MAX_NAME));
}



/*
 * Delete callback function associated with the name to handle 
 * Xlation table.
 */
 
static void ckpXlationtHashDeleteCallback(ClCntKeyHandleT userKey,
            ClCntDataHandleT userData)
{
    CkptXlationDBEntryT *pInfo = (CkptXlationDBEntryT*)userData;
    
    if(pInfo)
    {
        clHeapFree(pInfo);
    }
    return;
}



/*
 * Funtion to destroy the passed handle from master's clientHdl database.
 */
 
ClRcT _ckptMasterDBInfoDeleteCallback(ClHandleDatabaseHandleT databaseHandle,
                                      ClHandleT               handle, 
                                      void                    *pCookie)
{
    ClRcT                   rc              = CL_OK;
    CkptMasterDBEntryT      *pStoredData    = NULL;
    
    /*
     * Retrieve the information associated with the passed handle.
     */
    rc = clHandleCheckout(gCkptSvr->masterInfo.masterDBHdl,
                          handle, (void **)&pStoredData);
    
    /*
     * Delete the associated retention timer.
     */
    if(pStoredData->retenTimerHdl)
        clTimerDelete(&pStoredData->retenTimerHdl);
        
    rc = clHandleCheckin(gCkptSvr->masterInfo.masterDBHdl, handle);
    rc = clHandleDestroy(gCkptSvr->masterInfo.masterDBHdl, handle);

    /*
     * Decrement the count of master handles.
     */
    gCkptSvr->masterInfo.masterHdlCount--;
    return CL_OK;
}



/*
 * Function to delete all the entries in master's masterHdl database.
 * This function will usually be called during cleanup operations.
 */
 
void ckptMasterDBInfoDelete()
{
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    
    /*
     * Walk through the database and call the function to destroy
     * the created master handles.
     */
    clHandleWalk (gCkptSvr->masterInfo.masterDBHdl,
                   _ckptMasterDBInfoDeleteCallback,
                   NULL);
                   
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return;
}



/*
 * Funtion to destroy the passed handle from master's clientHdl database.
 */
 
ClRcT _ckptClientDBInfoDeleteCallback(ClHandleDatabaseHandleT databaseHandle,
                                      ClHandleT               handle,     
                                      void                    *pCookie)
{
    ClRcT rc = CL_OK;
    
    rc = clHandleDestroy(gCkptSvr->masterInfo.clientDBHdl, handle);
    
    /*
     * Decrement the count of client handles.
     */
    gCkptSvr->masterInfo.clientHdlCount--;
    return CL_OK;
}



/*
 * Function to delete all the entries in master's clientHdl database.
 * This function will usually be called during cleanup operations.
 */
 
void ckptClientDBInfoDelete()
{
    CKPT_LOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    
    /*
     * Walk through the database and call the function to destroy
     * the created client handles.
     */
    clHandleWalk (gCkptSvr->masterInfo.clientDBHdl,
                   _ckptClientDBInfoDeleteCallback,
                   NULL);
    CKPT_UNLOCK(gCkptSvr->masterInfo.ckptMasterDBSem);
    return;
}


static CkptSvrCbT  gCkptSvrEntry = {0};

/*
 * Supported version information.
 */
static ClVersionT clVersionSupported[]={
{'B',0x01 , 0x01}
};


/**========================================================================**/
/**                     Allocation related routines                        **/
/**========================================================================**/



/*
 * This routine allocates a server control block and initialises it 
 */
 
ClRcT  ckptSvrCbAlloc(CkptSvrCbT **ppSvrCb)
{
    ClRcT            rc            = CL_OK;
    CkptSvrCbT       *pSvrCb       = &gCkptSvrEntry;
    ClVersionT       *pTempVersion = NULL;
    ClUint32T        count         = 0;
    ClIdlHandleObjT  idlObj        = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlAddressT    address       ;

    /*
     * Allocate the memory for the server control block.
     */
    memset(&address, '\0', sizeof(address));
    pSvrCb->localAddr = clIocLocalAddressGet();

    /* 
     * Initialize the event related info.
     */
    memcpy(pSvrCb->evtChName.value, CL_CKPT_EVT_CH, strlen(CL_CKPT_EVT_CH));
    pSvrCb->evtChName.length = strlen(CL_CKPT_EVT_CH);

    rc = clOsalMutexInit(&pSvrCb->ckptClusterSem);
    CKPT_ERR_CHECK( CL_CKPT_SVR, CL_DEBUG_ERROR,
                    ("Init failed rc [%#x]", rc), rc);
    /*
     * Create the mutex to guard the active server DB.
     */
    rc = clOsalMutexCreate( &(pSvrCb->ckptActiveSem));
    CKPT_ERR_CHECK( CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Init failed rc[0x %x]\n", rc), rc);
            
    /*
     * Create the Ckpt list at the active server. The handles of all the ckpt 
     * that are opened on that node are maintained in this list. Each handle
     * will have checkpoint related info associated with it and stored as 
     * handle database entry.
     */
    rc = clCntLlistCreate(ckptHdlNameCompare,
                         ckptSvrHdlDeleteCallback,
                         ckptSvrHdlDeleteCallback,
                         CL_CNT_NON_UNIQUE_KEY,
                         &pSvrCb->ckptHdlList);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
       ("Linked list create for checkpoint handles failed rc[0x %x]\n", 
       rc), rc);
       
    rc = clHandleDatabaseCreate(NULL,&pSvrCb->ckptHdl);
    
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
       ("Database create for checkpoint handles failed rc[0x %x]\n",
       rc), rc);
            
    /*
     * Initialize the version database.
     */
    pSvrCb->versionDatabase.versionCount = sizeof(clVersionSupported)/
                                           sizeof(ClVersionT);
    pSvrCb->versionDatabase.versionsSupported = 
         (ClVersionT *)clHeapAllocate(pSvrCb->versionDatabase.versionCount * 
                                    sizeof(ClVersionT));
                                    
    pTempVersion = pSvrCb->versionDatabase.versionsSupported;
    for(count = 0;count <  pSvrCb->versionDatabase.versionCount ;count++)
    {
        memcpy(pSvrCb->versionDatabase.versionsSupported, 
               &clVersionSupported[count],sizeof(ClVersionT));
        pSvrCb->versionDatabase.versionsSupported++;
    }
    pSvrCb->versionDatabase.versionsSupported = pTempVersion;

    /*
     * Create the peer list associated with the active server.
     */
    rc = clCntHashtblCreate( CL_CKPT_PEER_HASH_TBL_SIZE,
                             ckptPeerCompare,
                             ckptPeerHashFunc, 
                             ckptPeerDeleteCallback,
                             ckptPeerDeleteCallback,
                             CL_CNT_UNIQUE_KEY,
                             &pSvrCb->peerList);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Hash table create for checkpoint peers failed rc[0x %x]\n", rc),
             rc);

    pSvrCb->masterInfo.availPeerCount = CL_CKPT_INIT_INDEX;
    
    /* 
     * Create database for name to global Hdl translation.
     */
    rc = clCntHashtblCreate(CL_CKPT_MAX_NAME, ckptXlationHashKeyCmp,
                            ckptXlationHashFunction,
                            ckpXlationtHashDeleteCallback,
                            ckpXlationtHashDeleteCallback,
                            CL_CNT_UNIQUE_KEY, 
                            &pSvrCb->masterInfo.nameXlationDBHdl);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Name to global handle DB creation failed rc[0x %x]\n",rc),
             rc);

    /*
     * Create the peer list associated with the master server.
     */
    rc = clCntHashtblCreate( CL_CKPT_PEER_HASH_TBL_SIZE,
                             ckptPeerCompare,
                             ckptPeerHashFunc, 
                             ckptPeerListDeleteCallback,
                             ckptPeerListDeleteCallback,
                             CL_CNT_UNIQUE_KEY,
                             &pSvrCb->masterInfo.peerList);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Hash table create for checkpoint peers failed rc[0x %x]\n", rc),
             rc);
             
    /* 
     * If Master, allocate DB for ckpt meta data.
     */
    rc = clHandleDatabaseCreate( ckptMasterHdlDeleteCallback, 
                                 &pSvrCb->masterInfo.masterDBHdl);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Master DB creation failed rc[0x %x]\n",rc),
             rc);
    rc = clHandleDatabaseCreate( NULL, 
                                 &pSvrCb->masterInfo.clientDBHdl);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Master DB creation failed rc[0x %x]\n",rc),
             rc);
             
    clOsalMutexCreate(&pSvrCb->masterInfo.ckptMasterDBSem); 
    
    /* 
     * Initialize with Idl object to contact other servers.
     */
    memset(&address,'\0',sizeof(ClIdlAddressT));
    address.addressType = CL_IDL_ADDRESSTYPE_IOC ;
    address.address.iocAddress.iocPhyAddress.nodeAddress  = 
                                               pSvrCb->localAddr;
    address.address.iocAddress.iocPhyAddress.portId       =
                                               CL_IOC_CKPT_PORT;
    idlObj.address          = address;
    idlObj.flags            = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout  = 5000;
    idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries  = 0;
    
    rc = clIdlHandleInitialize(&idlObj,&pSvrCb->ckptIdlHdl);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Idl handle initialize is failed rc[0x %x]\n",rc),
             rc);
    *ppSvrCb = pSvrCb;
    
exitOnError:
    {
        return rc;
    }
}



/*
 * This routine frees a server control block and initialises it 
 */
 
ClRcT  ckptSvrCbFree(CkptSvrCbT *pSvrCb)
{
    clCntDelete(pSvrCb->ckptHdlList);
    clHandleDatabaseDestroy(pSvrCb->ckptHdl);
    clCntDelete(pSvrCb->peerList);
    ckptClientDBInfoDelete();
    ckptMasterDBInfoDelete();
    clOsalCondDelete(gCkptSvr->condVar);
    clOsalMutexDelete(gCkptSvr->mutexVar);
    clHandleDatabaseDestroy(pSvrCb->masterInfo.clientDBHdl);
    clHandleDatabaseDestroy(pSvrCb->masterInfo.masterDBHdl);
    clCntDelete(pSvrCb->masterInfo.peerList);
    clCntDelete(pSvrCb->masterInfo.nameXlationDBHdl);
    clOsalMutexDelete(pSvrCb->masterInfo.ckptMasterDBSem);
    clHeapFree(pSvrCb->versionDatabase.versionsSupported);
    return CL_OK;
}

void
clCkptSectionLevelMutexDelete(ClOsalMutexIdT  *pSecMutexArray, 
                             ClUint32T       numMutex)
{
    ClInt32T i = 0;
    /* for default section no need to create */
    if( numMutex == 1 ) return ;

    for(i = 0; i < numMutex; i++ )
    {
        if( CL_HANDLE_INVALID_VALUE != pSecMutexArray[i] )
        {
            clOsalMutexDelete(pSecMutexArray[i]);
            pSecMutexArray[i] = CL_HANDLE_INVALID_VALUE;
        }
    }
    return;
}

ClRcT
clCkptSectionLevelMutexCreate(ClOsalMutexIdT  *pSecMutexArray, 
                              ClUint32T       numMutex)
{
    ClRcT  rc = CL_OK;
    ClInt32T i = 0;
    
    /* for default section no need to create */
    if( numMutex == 1 ) return CL_OK;

    for(i = 0; i < numMutex; i++ )
    {
        rc = clOsalMutexCreate(&pSecMutexArray[i]);
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                    "Failed to create mutex count [%d]", i);
            goto freeMutexNExit;
        }
    }
    return CL_OK;
freeMutexNExit:
    clCkptSectionLevelMutexDelete(pSecMutexArray, numMutex);
    return rc;
}

ClUint32T
clCkptSectionIndexGet(CkptT             *pCkpt,
                      ClCkptSectionIdT  *pSecId)
{
    ClUint32T  index = -1; 
    ClUint32T  cksum = 0;
    ClRcT      rc    = CL_OK;

    rc = clCksm32bitCompute(pSecId->id, pSecId->idLen, &cksum);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Failed to compute cksum for sectionId [%.*s]", 
                pSecId->idLen, pSecId->id);
        return index;
    }
    index = cksum % pCkpt->numMutex;

    return index;
}

ClRcT
clCkptSectionLevelLock(CkptT             *pCkpt,
                       ClCkptSectionIdT  *pSectionId,
                       ClBoolT *pSectionLockTaken)
{
    ClRcT     rc    = CL_OK;
    ClUint32T index = 0;

    *pSectionLockTaken = CL_TRUE;
    if( (NULL == pSectionId->id) || (0 == pCkpt->numMutex) || (1 == pCkpt->numMutex) )
    {
        /* for default section, no need to take lock */
        *pSectionLockTaken = CL_FALSE;
        return CL_OK;
    }
    index = clCkptSectionIndexGet(pCkpt, pSectionId);
    CL_ASSERT(index != -1);

    rc = clOsalMutexLock((pCkpt->secMutex[index]));
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                "Failed to acquire the mutex for sectionId [%.*s] index [%d]", 
                pSectionId->idLen, pSectionId->id, index);
        return rc;
    }
    return rc;
}

ClRcT
clCkptSectionLevelUnlock(CkptT             *pCkpt,
                         ClCkptSectionIdT  *pSectionId,
                         ClBoolT sectionLockTaken)
{
    ClRcT     rc    = CL_OK;

    if(sectionLockTaken == CL_TRUE)
    {
        ClUint32T index = 0;

        if( (NULL == pSectionId->id) || (0 == pCkpt->numMutex) || (1 == pCkpt->numMutex) )
        {
            /* for default section, no need to take lock */
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                       "Section lock taken for default section");
            return CL_OK;
        }
        index = clCkptSectionIndexGet(pCkpt, pSectionId);
        CL_ASSERT(index != -1);

        rc = clOsalMutexUnlock((pCkpt->secMutex[index]));
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN,
                       "Failed to release the mutex for sectionId [%.*s] index [%d]", 
                       pSectionId->idLen, pSectionId->id, index);
            return rc;
        }
    }
    return rc;
}

/* 
 * This routine frees a checkpoint entry
 */
 
ClRcT  ckptEntryFree(CkptT  *pCkpt)
{
    if (pCkpt != NULL)
    {
        /*
         * Free the associated control plane info, data plane info.
         */
        if (pCkpt->pCpInfo != NULL)
        {
            _ckptCplaneInfoFree(pCkpt->pCpInfo);
            pCkpt->pCpInfo = NULL;
        }
        if (pCkpt->pDpInfo != NULL)
        {
            _ckptDplaneInfoFree(pCkpt->pDpInfo);
            pCkpt->pDpInfo = NULL;
        }
        if( CL_HANDLE_INVALID_VALUE != pCkpt->ckptMutex )
        {
            clOsalMutexDelete(pCkpt->ckptMutex);
            pCkpt->ckptMutex = CL_HANDLE_INVALID_VALUE;
        }
        clCkptSectionLevelMutexDelete(pCkpt->secMutex, pCkpt->numMutex);
    }
    return CL_OK;
}

/*
 * Section Id compare function, compares the section ids are same or not.
 */
ClInt32T
clCkptSectionCompareCb(ClCntKeyHandleT  key1, ClCntKeyHandleT key2) 
{
    ClCkptSectionKeyT  *pSecKey1 = (ClCkptSectionKeyT *) key1;
    ClCkptSectionKeyT  *pSecKey2 = (ClCkptSectionKeyT *) key2;

    if( pSecKey2->scnId.idLen == pSecKey1->scnId.idLen )
    {
        if( ! memcmp( pSecKey2->scnId.id, pSecKey1->scnId.id,
                    pSecKey1->scnId.idLen) )
        {
            return 0;
        }
    }
    return 1;
}

/*
 * Section hash function returns the hash value from the key.
 * hash values is calculated from the cksum value of section id modulo by
 * num of buckets.
 */ 
ClUint32T
clCkptSectionHashFn(ClCntKeyHandleT  key)
{
    ClCkptSectionKeyT  *pSecKey = (ClCkptSectionKeyT *) key;
    return pSecKey->hash;
}

/*
 * Section delete callback function deletes the pointers of key and data.
 */
void
clCkptSectionDeleteCb(ClCntDataHandleT  key,
                      ClCntKeyHandleT   data)
{
    ClCkptSectionKeyT  *pSecKey = (ClCkptSectionKeyT *) key;
    CkptSectionT       *pSec    = (CkptSectionT *) data;
 
    /* free the data */
    pSec->used = CL_FALSE;
    if( 0 != pSec->timerHdl )
    {
        clTimerDelete(&pSec->timerHdl);
        pSec->timerHdl = 0;
    }
    pSec->exprTime = 0;
    clHeapFree(pSec->pData);
    pSec->pData = NULL;
    clHeapFree(pSec);
    /* free the key */
    clHeapFree(pSecKey->scnId.id);
    clHeapFree(pSecKey);

    return;
}
/* 
 * This routine allocates memory for storing a checkpoint's  Data plane 
 * information.
 */
 
ClRcT  _ckptDplaneInfoAlloc(CkptDPlaneInfoT  **ppDplaneInfo,
                            ClUint32T        scnCount)
{
    ClRcT            rc      = CL_OK;
    CkptDPlaneInfoT  *pTmpDp = NULL;

    /* 
     * Allocate memory for data plane information.
     */
    if (NULL == (pTmpDp = clHeapCalloc(1,sizeof(CkptDPlaneInfoT))))
        return CL_CKPT_ERR_NO_MEMORY;
        
    /* 
     * Create a hash table for sections. If the num of sections are lesser
     * than CL_CKPT_MAX_SCN_BKTS, then it will be num of sections, otherwise
     * its CL_CKPT_MAX_SCN_BKTS
     */
     if( scnCount < CL_CKPT_MAX_SCN_BKTS )
     {
        rc = clCntHashtblCreate(scnCount, clCkptSectionCompareCb, 
                                clCkptSectionHashFn, clCkptSectionDeleteCb,
                                clCkptSectionDeleteCb, CL_CNT_UNIQUE_KEY, 
                                &pTmpDp->secHashTbl);
        pTmpDp->numOfBukts = scnCount;
     }
     else
     {
        rc = clCntHashtblCreate(CL_CKPT_MAX_SCN_BKTS, clCkptSectionCompareCb, 
                                clCkptSectionHashFn, clCkptSectionDeleteCb,
                                clCkptSectionDeleteCb, CL_CNT_UNIQUE_KEY, 
                                &pTmpDp->secHashTbl);
        pTmpDp->numOfBukts = CL_CKPT_MAX_SCN_BKTS; 
     }
     if( CL_OK != rc )
     {
         clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                    "Failed to create hash table for sections rc[0x %x]", rc);
         return rc;
     }
    *ppDplaneInfo = pTmpDp;

    return rc;
}



/* 
 * This routine frees memory representing  a checkpoint's Dataplane 
 * information.
 */
 
ClRcT  _ckptDplaneInfoFree (CkptDPlaneInfoT  *pDpInfo)
{
    if (pDpInfo != NULL)
    {
        clCntDelete(pDpInfo->secHashTbl);
        clHeapFree(pDpInfo);
        pDpInfo = NULL;
    }
    return CL_OK;
}



/* 
 * This routine allocates memory for storing a checkpoint's Control plane
 * information.
 */
 
ClRcT  _ckptCplaneInfoAlloc(CkptCPlaneInfoT  **pCplaneInfo)
{
    ClRcT            rc      = CL_OK;
    CkptCPlaneInfoT  *pTmpCp = NULL;  

    /*
     * Allocate memory for data plane information.
     */
    if (NULL == (pTmpCp = clHeapCalloc(1,sizeof(CkptCPlaneInfoT))))
        return CL_CKPT_ERR_NO_MEMORY;
    
    /*
     * Create the presence list for storing where all the checkpoint is being
     * replicated.
     */
    rc = clCntLlistCreate (ckptPrefNodeCompare, NULL, NULL,
                       CL_CNT_UNIQUE_KEY, &(pTmpCp->presenceList));
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("Presence list creation failed rc[0x %x]\n", rc), rc);
    rc = clCntLlistCreate(clCkptAppKeyInfoCompare, clCkptAppKeyDelete,
                          clCkptAppKeyDelete, CL_CNT_UNIQUE_KEY, &(pTmpCp->appInfoList));
    if( CL_OK != rc )
    {
        clCntDelete(pTmpCp->presenceList);
    }
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                     ("App info list creation failed rc[0x %x]\n", rc), rc);
                     
    *pCplaneInfo = pTmpCp;
    return rc;
exitOnError:
    {
        /*
         * Do the necessary cleanup.
         */
        if (pTmpCp != NULL) clHeapFree(pTmpCp);
        return rc;
    }
}


/* 
 *  This routine frees memory associated with a checkpoint's control plane 
 *  information. 
 */
 
ClRcT  _ckptCplaneInfoFree (CkptCPlaneInfoT  *pCpInfo)
{
    /*
     * Delete the presence list associated with the checkpoint.
     */
    if (pCpInfo != NULL)
    {
        if (pCpInfo->presenceList != 0) clCntDelete(pCpInfo->presenceList);
        pCpInfo->presenceList = 0;
        if (pCpInfo->appInfoList != 0) clCntDelete(pCpInfo->appInfoList);
        pCpInfo->appInfoList = 0;
        clHeapFree(pCpInfo);
        pCpInfo = NULL;
    }

    return CL_OK;
}



/* 
 * This routine finds a section in a given checkpoint.
 */
 
#if 0
ClRcT  _ckptSectionFind(CkptT             *pCkpt, 
                        ClCkptSectionIdT  *pKey, 
                        CkptSectionT      **ppOpSec,
                        ClUint32T         *pCount)
{
    ClRcT           rc       = CL_OK;
    ClUint32T       secCount = 0;
    CkptSectionT    *pSec    = NULL;  

    /*
     * Validate the input parameters.
     */
    if ((pKey == NULL) || (pCkpt == NULL) || ((ppOpSec) == NULL)) 
            return CL_CKPT_ERR_NULL_POINTER;

    /* 
     * Validate the data plane info.
     */
    if ((pCkpt->pDpInfo == NULL))
        rc = CL_CKPT_ERR_INVALID_STATE;
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Dataplane information not present for %s rc[0x %x]\n",
             pCkpt->ckptName.value, rc), rc);

    pSec = pCkpt->pDpInfo->pSections;
    for ( secCount = 0; secCount < pCkpt->pDpInfo->maxScns; secCount++)
    {
        if (pSec == NULL)
            return CL_CKPT_ERR_NULL_POINTER;

        /*
         * Check whether the section is in use and the idLens match.
         */
        if ((pSec->used == CL_TRUE) &&
            (pSec->scnId.idLen ==  pKey->idLen))
        {
            if ((pSec->scnId.id != NULL) && (pKey->id != NULL))
            {
                if (0 == memcmp(pSec->scnId.id, pKey->id, pSec->scnId.idLen))
                {
                    /* 
                     * Match found.
                     */
                    *ppOpSec = pSec;
                    *pCount = secCount;
                    return CL_OK;
                }
            }
        }
        pSec++ ;
    }
    return CL_CKPT_ERR_NOT_EXIST;
exitOnError:
    {
        return rc;
    }
}
#endif


/*
 * This routine adds the passed address to the presence list if the
 * checkpoint.
 */
ClRcT _ckptPresenceListUpdate(CkptT                *pCkpt, 
                              ClIocNodeAddressT    peerAddr)
{
    ClRcT  rc = CL_OK;

    /*
     * Validate the input parameters.
     */
    if(peerAddr == 0) return CL_OK;
    if(pCkpt->pCpInfo != NULL)
    {
        /*
         * Add the node to the presence list.
         */
        rc = clCntNodeAdd(pCkpt->pCpInfo->presenceList, 
        (ClPtrT)(ClWordT)peerAddr, 0, NULL);
        if (CL_GET_ERROR_CODE(rc) == CL_ERR_DUPLICATE)   rc = CL_OK;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("Presence list add failed rc[0x %x]\n",rc), rc);
    }
exitOnError:
    {
        return rc;
    }
}


ClRcT VDECL_VER(clCkptNackReceive, 4, 0, 0)(ClVersionT  version,
                        ClUint32T   nackId)
{
  ClRcT      rc = CL_OK;
  ClIocAddressT srcAddr;

  rc = clRmdSourceAddressGet(&srcAddr.iocPhyAddress);
  CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
          ("Fail: Nack Receive failed [0x %x]\n", rc), rc);
  switch(nackId)
  {
      case CKPT_REM_CKPT_DEL:
      {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_6_VERSION_NACK, "RemoteSvrCkptAdd",
                    srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId, version.releaseCode,
                    version.majorVersion, version.minorVersion);

          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Ckpt nack recieved from "
                        "NODE[0x%x:0x%x], rc=[0x%x]\n", 
                        srcAddr.iocPhyAddress.nodeAddress,
                        srcAddr.iocPhyAddress.portId,
                        CL_CKPT_ERR_VERSION_MISMATCH));
            break;

      }
      case CKPT_REM_SEC_ADD:
      {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_6_VERSION_NACK, "RemoteSvrSectionAdd",
                    srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId, version.releaseCode,
                    version.majorVersion, version.minorVersion);

          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Ckpt nack recieved from "
                        "NODE[0x%x:0x%x], rc=[0x%x]\n", 
                        srcAddr.iocPhyAddress.nodeAddress,
                        srcAddr.iocPhyAddress.portId,
                        CL_CKPT_ERR_VERSION_MISMATCH));
            break;

      }
      case CKPT_REM_SEC_DEL:
      {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_6_VERSION_NACK, "RemoteSvrSectionDelete",
                    srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId, version.releaseCode,
                    version.majorVersion, version.minorVersion);

          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Ckpt nack recieved from "
                        "NODE[0x%x:0x%x], rc=[0x%x]\n", 
                        srcAddr.iocPhyAddress.nodeAddress,
                        srcAddr.iocPhyAddress.portId,
                        CL_CKPT_ERR_VERSION_MISMATCH));
            break;

      }
      case CKPT_REM_SEC_OVERWRITE:
      {
          clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_ERROR, NULL,
                    CL_CKPT_LOG_6_VERSION_NACK, "RemoteSvrSectionOverwrite",
                    srcAddr.iocPhyAddress.nodeAddress,
                    srcAddr.iocPhyAddress.portId, version.releaseCode,
                    version.majorVersion, version.minorVersion);

          CL_DEBUG_PRINT(CL_DEBUG_ERROR, ("Ckpt nack recieved from "
                        "NODE[0x%x:0x%x], rc=[0x%x]\n", 
                        srcAddr.iocPhyAddress.nodeAddress,
                        srcAddr.iocPhyAddress.portId,
                        CL_CKPT_ERR_VERSION_MISMATCH));
            break;

      }
  }
   return rc; 
exitOnError:
  return rc;
}



/*
 * Funtion to write to the checkpoint locally. This function is called with
 * ckptMutex is held. If error is occuring before the we are unlocking the
 * mutex and returing from here, so the caller of the function should just
 * exit, no need to unlock again. This is because race condition between
 * checkpoint delete & this call. 
 */
ClRcT _ckptCheckpointLocalWrite(ClCkptHdlT             ckptHdl,
                                ClUint32T              numberOfElements,
                                ClCkptIOVectorElementT *pIoVector,
                                ClUint32T              *pError,
                                ClIocNodeAddressT      nodeAddr,
                                ClIocPortT             portId)
{
    ClRcT        rc          = CL_OK;
    ClUint32T    count       = 0;
    CkptT        *pCkpt      = NULL;
    ClUint32T    tmpDataSize = 0;
    ClUint8T     *pTmpBuf    = NULL;
    CkptSectionT *pSec       = NULL;
    ClCkptIOVectorElementT *pVec = pIoVector;
    ClBoolT sectionLockTaken = CL_TRUE;
    /*
     * Retrieve the data associated with the active handle.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl,ckptHdl,(void **)&pCkpt);
    if( CL_OK != rc )
    {
        /* this should never occur */
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE,
                "Handle [%#llX] checkout failed rc [0x %x]", ckptHdl, rc);
        CL_ASSERT(CL_FALSE);
        return CL_CKPT_ERR_INVALID_STATE;
    }

    /*
     * Read from iovector and copy to the corresponding sections one by one.
     */
    for (count = 0; count < numberOfElements; count ++)
    {
        /*
         * Validate the iovector data.
         */
        if ((pCkpt->pDpInfo->maxScnSize != 0) && ((pIoVector->dataSize + pIoVector->dataOffset) > 
                pCkpt->pDpInfo->maxScnSize) )
        {
            /*
             * Return ERROR as data size + offset exceeds the max section
             * size.
             */
            rc = CL_CKPT_ERR_INVALID_PARAMETER;
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Trying to write beyond the section's"
                     " maximum size %.*s rc[0x %x]\n",
                     pCkpt->ckptName.length,
                     pCkpt->ckptName.value,rc), rc);
        }   
        if((pCkpt->pDpInfo->maxScnSize != 0) && (pIoVector->dataOffset > pCkpt->pDpInfo->maxScnSize))
        {
            /*
             * Return ERROR as offset exceeds the max section size. 
             */
            rc = CL_CKPT_ERR_INVALID_PARAMETER;
            CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                    ("Invalid Offset %.*s rc[0x %x]\n",
                     pCkpt->ckptName.length, 
                     pCkpt->ckptName.value,rc), rc);
        }         
        /* Acquire the section level mutex, release the ckpt mutex */
        rc = clCkptSectionLevelLock(pCkpt, &pIoVector->sectionId, 
                                    &sectionLockTaken);
        if( CL_OK != rc )
        {
            goto exitOnError;
        }
        if(sectionLockTaken == CL_TRUE)
        {
            CKPT_UNLOCK(pCkpt->ckptMutex);
        }
        if(pIoVector->sectionId.id == NULL)
        {
            if(pCkpt->pDpInfo->maxScns > 1)
            {
                /*
                 * Return ERROR as id == NULL and max sec > 1
                 */
                rc = CL_CKPT_ERR_INVALID_PARAMETER;
                clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_SEC_OVERWRITE, 
                        "Passed section id is NULL");
                /* release the section mutex, acquire ckpt mutex */
                goto sectionUnlockNExit;
            }
            rc = clCkptDefaultSectionInfoGet(pCkpt, &pSec);
        }
        else
        {
            /*
             * Find the section and return ERROR if the section doesnt exist.
             */
            rc = clCkptSectionInfoGet(pCkpt, &pIoVector->sectionId, &pSec);
        }   
        if( CL_OK != rc )
        {
            clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                    "Failed to find section [%.*s] in ckpt [%.*s]", 
                    pIoVector->sectionId.idLen, pIoVector->sectionId.id,
                    pCkpt->ckptName.length, pCkpt->ckptName.value);
            /* release the section mutex, acquire ckpt mutex */
            goto sectionUnlockNExit;
        }
        if(pSec->size < (pIoVector->dataOffset + pIoVector->dataSize))
        {
            /* 
             * Realloc the section memory and copy the old data
             */
            tmpDataSize = pIoVector->dataOffset + pIoVector->dataSize; 
            pTmpBuf = (ClUint8T *)clHeapCalloc(1,tmpDataSize);
            if(pTmpBuf == NULL)
            {
                rc = CL_CKPT_ERR_NO_MEMORY;
                clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                        "Memory allocation failed [%.*s] rc [0x %x]", 
                        pCkpt->ckptName.length, pCkpt->ckptName.value, 
                        rc);
                /* release the section mutex, acquire ckpt mutex */
                goto sectionUnlockNExit;
            }
            memcpy(pTmpBuf,pSec->pData,pSec->size);
            clHeapFree(pSec->pData);
            pSec->pData = pTmpBuf;
            pSec->size = tmpDataSize;
        }
        /*
         * Copy the new data,
         */
        pTmpBuf = (ClUint8T*)(pSec->pData) + pIoVector->dataOffset;
        memcpy((void*)pTmpBuf,pIoVector->dataBuffer,pIoVector->dataSize);
        /*
         * Update lastUpdate field of section, not checking error as this is not as important for
         * Section creation.
         */
         clCkptSectionTimeUpdate(&pSec->lastUpdated);
        /* release the section mutex, acquire ckpt mutex */
        clCkptSectionLevelUnlock(pCkpt, &pIoVector->sectionId, 
                                 sectionLockTaken);
        if(sectionLockTaken == CL_TRUE)
        {
            CKPT_LOCK(pCkpt->ckptMutex);
        }
        pIoVector++;
    }

    /*
     * Everything is success. so pError is 0.
     */
    count = 0;
    if(pCkpt->pCpInfo->updateOption & CL_CKPT_DISTRIBUTED )
    {
        clCkptClntWriteNotify(pCkpt, pVec, numberOfElements, nodeAddr, portId);
    }
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    /* Success, Note we are returning with LOCK held as CL_OK, 
     * NOTE, it should be returned with LOCK held..Please chk the caller 
     * function before modifying*/
    return CL_OK;
exitOnError:
    /*
     * pError carries back the section whose updation failed.
     * so returning error without being held ckpt mutex 
     */
    CKPT_UNLOCK(pCkpt->ckptMutex);
    *pError = count;
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    return rc;
sectionUnlockNExit:
    clCkptSectionLevelUnlock(pCkpt, &pIoVector->sectionId, sectionLockTaken);
    if(sectionLockTaken == CL_FALSE)
    {
        CKPT_UNLOCK(pCkpt->ckptMutex);
    }
    /* Already released CKPT_LOCK, error occured so release section lock */
    *pError = count;
    clHandleCheckin(gCkptSvr->ckptHdl,ckptHdl);
    return rc;
}


/*
 * Function to checkout the passed handle and do basic validations
 * on the associated data.
 */
 
ClRcT ckptSvrHdlCheckout( ClHandleDatabaseHandleT ckptDbHdl,
                          ClHandleT               ckptHdl,
                          void                    **pData)
{
    ClRcT rc = CL_OK;

    /*
     * Handle checkout.
     */
    rc = clHandleCheckout(ckptDbHdl, ckptHdl, pData);
    if( CL_OK != rc)
    {
        return rc;
    }

    /*
     * Associated data validation.
     */
    if( pData == NULL)
    {
        clHandleCheckin(ckptDbHdl, ckptHdl);
        return CL_CKPT_ERR_INVALID_HANDLE;
    }
    
    return CL_OK;
}



/*
 * Fucntion to free packed info about a checkpoint.
 */
 
ClRcT  ckptPackInfoFree( CkptInfoT  *pCkptInfo)
{
    CkptSectionInfoT     *pSec     = NULL;
    ClUint32T             count    = 0;

    if( NULL != pCkptInfo->pName )
    {
        clHeapFree(pCkptInfo->pName);
    }
    /*
     * Free the control plane related info.
     */
    if( pCkptInfo->pCpInfo != NULL)
    {
        if( pCkptInfo->pCpInfo->presenceList != NULL)
        {
            clHeapFree( pCkptInfo->pCpInfo->presenceList);
            pCkptInfo->pCpInfo->presenceList = NULL;
        }   
        if( pCkptInfo->pCpInfo->pAppInfo != NULL )
        {
            clHeapFree( pCkptInfo->pCpInfo->pAppInfo);
            pCkptInfo->pCpInfo->pAppInfo = NULL;
        }
        clHeapFree(pCkptInfo->pCpInfo);
        pCkptInfo->pCpInfo = NULL;
    }    
    
    /*
     * Free the data plane related info.
     */
    if( pCkptInfo->pDpInfo != NULL)
    {
        if( pCkptInfo->pDpInfo->pSection != NULL)
        {
            pSec = pCkptInfo->pDpInfo->pSection;
            for( count = 0; count < pCkptInfo->pDpInfo->numScns; count++)
            {
                if( pSec->secId.id != NULL) 
                {
                    clHeapFree( pSec->secId.id );
                    pSec->secId.id = NULL;
                }    
                if( pSec->pData != NULL)
                {
                    clHeapFree( pSec->pData);
                    pSec->pData = NULL;
                }    
                pSec++;
            }
            clHeapFree(pCkptInfo->pDpInfo->pSection);
            pCkptInfo->pDpInfo->pSection = NULL;
        }
        clHeapFree(pCkptInfo->pDpInfo);
        pCkptInfo->pDpInfo = NULL;
    }    
    return CL_OK;
}



/*
 * Function to get the expiration time in the format understood by
 * OpenClovis timer library.
 */
 
ClRcT _ckptSectionExpiryTimeoutGet(ClTimeT expiryTime, 
                                   ClTimerTimeOutT *timeOut)
{
    ClTimerTimeOutT currentTime = {0};
    ClRcT           rc          = CL_OK;  

    /*
     * Compute the absolute time.
     */
    rc = clOsalTimeOfDayGet(&currentTime);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Can not get the current time of day rc[0x %x]\n",
             rc), rc);
    timeOut->tsSec      = expiryTime / CL_CKPT_NANOS_IN_SEC;
    timeOut->tsMilliSec = (expiryTime % CL_CKPT_NANOS_IN_SEC)/ 
                            CL_CKPT_NANOS_IN_MILI;
    if( timeOut->tsSec < currentTime.tsSec )
    {
        /* 
         * Return ERROR as section has already expired.
         */
        rc = CL_CKPT_ERR_INVALID_PARAMETER;
        CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
                ("The expiration time has already "
                 "passed rc[0x %x]\n", rc), rc);
    }

    if( timeOut->tsMilliSec >= currentTime.tsMilliSec )
    {
        timeOut->tsSec -= currentTime.tsSec;
        timeOut->tsMilliSec -= currentTime.tsMilliSec;
    }
    else
    {
        timeOut->tsSec -= (currentTime.tsSec + 1);
        timeOut->tsMilliSec += CL_CKPT_MILIS_IN_SEC - 
            currentTime.tsMilliSec; 
    }
exitOnError:
    return rc;
}



/*
 * Function to create the spawn the expiration timer.
 */
 
ClRcT
_ckptExpiryTimerStart(ClHandleT         ckptHdl,
                     ClCkptSectionIdT   *pSecId, 
                     CkptSectionT       *pSec,
                     ClTimerTimeOutT    timeOut)
{
    CkptSecTimerInfoT *pSecTimerInfo = NULL;
    ClRcT              rc            = CL_OK;

    /*
     * Prepare the cookie to be passed to the timer expiry callback.
     */
    if( NULL == (pSecTimerInfo = (CkptSecTimerInfoT *)clHeapCalloc(
                    1, sizeof(CkptSecTimerInfoT))))
    {
        CKPT_DEBUG_E(("Failed to allocate memory for TimerInfo\n"));
        return CL_CKPT_ERR_NO_MEMORY;
    }

    pSecTimerInfo->ckptHdl     = ckptHdl;
    pSecTimerInfo->secId.idLen = pSecId->idLen;
    if( NULL == (pSecTimerInfo->secId.id = (ClUint8T *)clHeapAllocate(
                    pSecId->idLen)))
    {
        clHeapFree(pSecTimerInfo);
        return CL_CKPT_ERR_NO_MEMORY;
    }
    
    memset( pSecTimerInfo->secId.id,'\0',
            pSecId->idLen);
    memcpy( pSecTimerInfo->secId.id, pSecId->id, pSecId->idLen);

    /*
     * Create and start the timer.
     */
    rc = clTimerCreateAndStart(timeOut,
            CL_TIMER_ONE_SHOT,
            CL_TIMER_SEPARATE_CONTEXT,
            _ckptSectionTimerCallback,  
            pSecTimerInfo,
            &pSec->timerHdl); 
    return rc;
}

/*
 * Callback funtion that would be invoked whenever a section expires.
 */
ClRcT _ckptSectionTimerCallback(void *pArg)
{
    CkptSecTimerInfoT   *pSecInfo  = (CkptSecTimerInfoT *)pArg;
    CkptT               *pCkpt     = NULL;
    ClRcT                rc        = CL_OK;

    if( (gCkptSvr == NULL) || ( (gCkptSvr != NULL) && (gCkptSvr->serverUp ==
                    CL_FALSE)) )
    {
        goto exitOnErrorBeforeHdlCheckout; 
    }
    clLogInfo(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_DEL, 
            "Section timer expired for section [%.*s]", 
             pSecInfo->secId.idLen, pSecInfo->secId.id);
    /*
     * Get the associated EO object.
     */
    rc = clEoMyEoObjectSet(gCkptSvr->eoHdl);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Eo object set failed rc[0x %x]\n",rc), rc);

    /*
     * Retrieve the data associated with the active handle that is passed as 
     * the cookie.
     */
    rc = clHandleCheckout(gCkptSvr->ckptHdl, pSecInfo->ckptHdl, 
            (void **)&pCkpt);
    CKPT_ERR_CHECK_BEFORE_HDL_CHK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Handle is invalid  rc[0x %x]\n",rc), rc);

    rc = clCkptSectionLevelDelete(pSecInfo->ckptHdl, pCkpt, &pSecInfo->secId, 0);
    if( CL_OK != rc )
    {
        clLogError(CL_CKPT_AREA_ACTIVE, CL_CKPT_CTX_CKPT_OPEN, 
                "Failed to delete the section [%.*s]", 
                pSecInfo->secId.idLen, pSecInfo->secId.id);
    }
    clHandleCheckin(gCkptSvr->ckptHdl, pSecInfo->ckptHdl);
exitOnErrorBeforeHdlCheckout:
    if(pSecInfo != NULL)
    {
        clHeapFree(pSecInfo->secId.id);
        clHeapFree(pSecInfo);
        pSecInfo = NULL;
    }
    return rc;
}

ClRcT clCkptAppIdlHandleInit(ClIdlHandleT  *pHdl)
{
    ClIdlHandleObjT  idlObj  = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlAddressT    address = {0};
    ClRcT            rc      = CL_OK;
	 
    /*
     * Set the idlObj.
     */
    memset(&address,'\0',sizeof(ClIdlAddressT));
    address.addressType     = CL_IDL_ADDRESSTYPE_IOC ;
    address.address.iocAddress.iocPhyAddress.nodeAddress  = 0;
    address.address.iocAddress.iocPhyAddress.portId       = CL_IOC_CKPT_PORT;
    idlObj.address          = address;
    idlObj.flags            = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout  = CKPT_RMD_DFLT_TIMEOUT;
    idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries  = 0;
    
    rc = clIdlHandleInitialize(&idlObj, pHdl);
    
    return rc;
}

ClRcT clCkptAppIdlHandleUpdate(ClIdlHandleT       idlHdl,
                               ClIocNodeAddressT  nodeAddress,
                               ClIocPortT         portId,
                               ClUint32T          numRetries)
{
    ClIdlHandleObjT  idlObj  = CL_IDL_HANDLE_INVALID_VALUE;
    ClIdlAddressT    address = {0};
    ClRcT            rc      = CL_OK;
	 
    /*
     * Set the idlObj.
     */
    memset(&address,'\0',sizeof(ClIdlAddressT));
    address.addressType     = CL_IDL_ADDRESSTYPE_IOC ;
    address.address.iocAddress.iocPhyAddress.nodeAddress  = nodeAddress;
    address.address.iocAddress.iocPhyAddress.portId       = portId;
    idlObj.address          = address;
    idlObj.flags            = CL_RMD_CALL_DO_NOT_OPTIMIZE;
    idlObj.options.timeout  = CKPT_RMD_DFLT_TIMEOUT;
    idlObj.options.priority = CL_RMD_DEFAULT_PRIORITY;
    idlObj.options.retries  = numRetries;
    
    rc = clIdlHandleUpdate(idlHdl, &idlObj);
    
    return rc;
}

/*
 * Called from the client to indicate that application has finalized 
 * the client library.
 */
ClRcT VDECL_VER(clCkptServerFinalize, 4, 0, 0)(ClVersionT        *pVersion,
                           ClIocNodeAddressT nodeAddr,
                           ClIocPortT        iocPort)
{
    ClRcT     rc   = CL_OK;
    ClUint32T flag = CL_CKPT_COMP_DOWN;
    
    /*
     * Check whether the server is fully up or not.
     */
    CL_CKPT_SVR_EXISTENCE_CHECK; 
    
    /*
     * Version verification.
     */
    rc = clVersionVerify(&gCkptSvr->versionDatabase,pVersion);
    if(rc != CL_OK)
    {   
        return CL_OK;
    }
    
    /*
     * The scenario is similar to COMP going down.
     */
    ckptPeerDown(nodeAddr, flag, iocPort); 
    
    /*
     * Broadcase the departure to all the nodes in the cluster.
     */
    rc = ckptIdlHandleUpdate(CL_IOC_BROADCAST_ADDRESS,
                             gCkptSvr->ckptIdlHdl,0);
    CKPT_ERR_CHECK(CL_CKPT_SVR,CL_DEBUG_ERROR,
            ("Failed to update the idl handle rc[0x %x]\n",rc), rc);
    rc = VDECL_VER(clCkptRemSvrByeClientAsync, 4, 0, 0)(gCkptSvr->ckptIdlHdl,
                                    *pVersion,
                                    nodeAddr,
                                    iocPort,
                                    flag, 
                                    NULL,0);
    memset(pVersion,'\0',sizeof(ClVersionT));
exitOnError:    
    return rc;
}
                           
ClInt32T ckptHdlNonUniqueKeyCompare(ClCntDataHandleT givenData, ClCntDataHandleT data)
{
    ClNameT *pName = (ClNameT*)givenData;
    ClCkptHdlT ckptHdl = *(ClCkptHdlT*)data;
    CkptT *pCkpt = NULL;
    ClRcT rc;
    ClInt32T ret = 1;

    rc = clHandleCheckout(gCkptSvr->ckptHdl, ckptHdl, (void**)&pCkpt);
    if(rc != CL_OK)
    {
        clLogError("KEY", "CMP", "CKP handle non unique key compare checkout failed for handle [%#llx]",
                   ckptHdl);
        return ret;
    }

    if(pCkpt->ckptName.length != pName->length) 
        goto out;

    ret = strncmp((const ClCharT*)pCkpt->ckptName.value, (const ClCharT*)pName->value, pName->length);

    out:
    clHandleCheckin(gCkptSvr->ckptHdl, ckptHdl);
    return ret;
}
