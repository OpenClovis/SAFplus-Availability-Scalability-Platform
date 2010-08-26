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
 * File        : clCkptMaster.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains ckpt master related data structures
*
*
*****************************************************************************/
#ifndef _CL_CKPT_MASTER_H_
#define _CL_CKPT_MASTER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clCkptSvr.h"
#include "clCkptDs.h"
#include <clDebugApi.h>
#include <clCkptCommon.h>
#include <ckptEockptServerCliServerFuncServer.h>
#include "xdrCkptCreateInfoT.h"
#include "xdrCkptXlationDBEntryT.h"
#include "xdrCkptDynamicSyncupT.h"
#include "xdrCkptUpdateInfoT.h"
#include "xdrCkptOpenInfoT.h"


/**====================================**/
/**     C O N S T A N T S              **/
/**====================================**/



#define CL_CKPT_INIT_INDEX   0   /* Initial index */
#define CL_CKPT_CLIENT_HDL   0   /* Client handle */
#define CL_CKPT_MASTER_HDL   1   /* Master handle */

#define CL_CKPT_CREAT        1   /* Created */
#define CL_CKPT_OPEN         2   /* Opened for read/write */

#define CL_CKPT_RETRY        1   /* Try finding new replica */
#define CL_CKPT_DONT_RETRY   0   /* Dont try to find new replica */

#define CL_CKPT_ADD_TO_MASTHDL_LIST 1 /* Add to masterHdl list */
#define CL_CKPT_REPLICA_DEL         0XFFFFFFFE

/**====================================**/
/**     S T R U C T U R E S            **/
/**====================================**/



/*
 * Meta data maintained by master per checkpoint.
 */
 
typedef struct ckptMasterDBEntry
{
    ClNameT             name;              /* ckpt name */   
    ClCkptCheckpointCreationAttributesT 
                        attrib;            /* creation attributes */
    ClUint32T           refCount;          /* No. of opens per ckpt */
    ClUint8T            markedDelete;      /* Whether delete has been 
                                              called or not */
    ClHandleT           activeRepHdl;      /* Active replica hdl */
    ClIocNodeAddressT   activeRepAddr;     /* Active address */
    ClIocNodeAddressT   prevActiveRepAddr; /* Prev active address */
    ClCntHandleT        replicaList;       /* Where all the ckpt is 
                                              replicated */
    ClTimerHandleT      retenTimerHdl;     /* Retention timer handle */
}CkptMasterDBEntryT;



/*
 * Info required for looking up the name to master hdl Xlation table
 */
 
typedef struct ckptXlationLookup
{
    ClUint32T cksum; /* Checksum of the name */
    ClNameT   name;  /* Ckpt name */
}CkptXlationLookupT;



/*
 * Info needed while choosing replica node.
 */
 
typedef struct ckptPeerWalkInfo
{
    ClIocNodeAddressT inAddr;    /* Address of active replica. Hence this 
                                    should not be selected */
    ClIocNodeAddressT outAddr;   /* To carry back the seleted address */
    ClIocNodeAddressT prevAddr;  /* Already tried storing at this node. Hence
                                    should not be selected */
    ClUint32T         count;     /* No. of replicas stored on that node */
}CkptPeerWalkInfoT;



/*
 * Info stored per peer by the master.
 */
 
typedef struct CkptPeerInfo
{
   ClIocNodeAddressT  addr;         /* Peer node */
   ClUint8T           credential;   /* Whether it can store replicas 
                                       or not */
   ClUint32T          available;    /* Whether ckpt server is up on that 
                                       node or not */
   ClCntHandleT       ckptList;     /* List of all opens on that node */
   ClCntHandleT       mastHdlList;  /* list of all creates on that node */
   ClUint32T          replicaCount; /* Count of replicas stored on 
                                       that node */
}CkptPeerInfoT;



/*
 * This structure contains the replica information.
 */
 
typedef struct ClCkptReplicateInfo
{
    ClIocNodeAddressT  destAddr;     /* Destination node*/
    ClBoolT            tryOtherNode; /* Whetehr we need to try finding another
                                        replica or not */
    ClUint32T          addToList;    /* Whether we neeed to add to master Hdl
                                        list or not */
    ClIdlHandleT       idlHdl;
    ClIocNodeAddressT  activeAddr;
    ClHandleT          clientHdl;
    ClHandleT          activeHdl;
    ClCkptCreationFlagsT creationFlags;
}ClCkptReplicateInfoT;
    

typedef struct ClCkptReplicateTimerArgs
{
    ClIocNodeAddressT nodeAddress;
    ClTimerHandleT *pTimerHandle;
} ClCkptReplicateTimerArgsT;
    
/**====================================**/
/**         E X T E R N S              **/
/**====================================**/


extern void 	ckptCkptListDeleteCallback();
extern ClInt32T ckptCkptListKeyComp();
extern ClInt32T ckptMastHdlListtKeyComp();

extern ClRcT clCkpMastertReplicaInfoUpdate(ClHandleT hdl, ClNameT *pName,
                                           ClIocNodeAddressT *pActAddr);
                                            
extern ClRcT clCkptReplicaNodeGet(ClIocNodeAddressT *pNodeAddr);
extern ClRcT _clCkptMasterClose(ClHandleT clientHdl, ClIocNodeAddressT localAddr, ClUint32T flag);
extern ClRcT _clCkptMasterCloseNoLock(ClHandleT clientHdl, ClIocNodeAddressT localAddr, ClUint32T flag);

extern ClRcT _ckptCkptReplicaNodeAddrGet(ClIocNodeAddressT* pNodeAddr,
                                  ClCntHandleT replicaListHdl);
extern ClRcT ckptMasterDatabaseSyncup();
extern ClRcT ckptMasterDatabasePack(ClBufferHandleT  outMsg);
extern ClRcT ckptMasterDatabaseUnpack(ClBufferHandleT  outMsg);
extern ClRcT ckptPersistentMemoryRead();
extern ClRcT ckptDataBackupInit(ClUint8T *pFlag);
extern ClRcT clCkptMasterReplicaListUpdate(ClIocNodeAddressT peerAddr);
extern ClRcT clCkptMasterReplicaListUpdateNoLock(ClIocNodeAddressT peerAddr);
extern void ckptDataBackupFinalize();
extern ClRcT _ckptMasterDBEntryPack(ClHandleDatabaseHandleT databaseHandle,
                                    ClHandleT handle, void *pCookie);
extern ClRcT  ckptMasterDBEntryCopy( ClHandleT handle,
                        CkptMasterDBEntryIDLT *pMasterEntry);
extern ClRcT ckptDynamicDeputyUpdate(CkptDynamicSyncupT updateFlag,
                              ClHandleT          clientHdl,
                              ClHandleT          masterHdl,
                              CkptUpdateInfoT    *pUpdateInfo);
extern ClRcT ckptDeputyUpdateForOpen(CkptCreateInfoT createInfo,
                              ClHandleT       masterHdl,
                              ClUint32T         openType);
extern ClRcT
_ckptMastHdlListWalk(ClCntKeyHandleT   masterHdl,
                     ClCntDataHandleT  pData,
                     ClCntArgHandleT   userArg,
                     ClUint32T         size);

extern ClRcT _ckptMasterCkptsReplicateTimerExpiry(void *pAddr);                     
extern ClRcT _ckptIsReplicationPossible(ClBoolT* pFlag);

extern ClRcT _ckptCheckpointLoadBalancing();
extern ClRcT
clCkptAppInfoReplicaNotify(CkptMasterDBEntryT  *pMasterData,
                           ClHandleT           masterHdl,
                           ClIocNodeAddressT   appAddr,
                           ClIocPortT          appPortId);
extern ClRcT clCkptReplicaCopy(ClIocNodeAddressT destAddr,
                        ClIocNodeAddressT replicaAddr,
                        ClHandleT         activeHdl, 
                        ClHandleT         clientHdl,
                        ClIocNodeAddressT activeAddr,
                        CkptHdlDbT        *pHdlInfo);
extern ClRcT
_ckptMasterPeerListInfoCreate(ClIocNodeAddressT nodeAddr,
                              ClUint32T         credential,
                              ClUint32T         replicaCount);

#ifdef __cplusplus
}
#endif
#endif  /* _CL_CKPT_MASTER_H_ */


