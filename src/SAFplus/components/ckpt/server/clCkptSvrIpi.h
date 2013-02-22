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
 * ModuleName  : ckpt                                                          
 * File        : clCkptIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains Checkpoint  service IPIs
*
*
*****************************************************************************/
#ifndef _CL_CKPT_IPI_H_
#define _CL_CKPT_IPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clCkptSvr.h"
#include "clCkptDs.h"
#include "clCkptPeer.h"
#include <clDebugApi.h>
#include <clCkptCommon.h>
#include <clCkptExtApi.h>
#include <xdrCkptUpdateFlagT.h>
#include "xdrCkptXlationDBEntryT.h"
#include "xdrCkptMasterDBInfoIDLT.h"
#include "xdrCkptMasterDBEntryIDLT.h"
#include "xdrCkptPeerListInfoT.h"
#include "xdrCkptMasterDBClientInfoT.h"

#define CL_CKPT_RC(ERROR_CODE)  CL_RC(CL_CID_CKPT, (ERROR_CODE))

#define  CL_CKPT_AREA_MASTER          "MAS"    
#define  CL_CKPT_AREA_ACTIVE          "ACT"    
#define  CL_CKPT_AREA_PEER            "PER"    
#define  CL_CKPT_AREA_DEPUTY          "DEP"
#define  CL_CKPT_AREA_MAS_DEP        "MDP"

#define  CL_CKPT_CTX_CKPT_OPEN        "OPE"
#define  CL_CKPT_CTX_CKPT_CLOSE        "CLO"
#define  CL_CKPT_CTX_CKPT_DEL         "DEL"
#define  CL_CKPT_CTX_SEC_CREATE       "SEC"
#define  CL_CKPT_CTX_SEC_OVERWRITE    "OVR"
#define  CL_CKPT_CTX_REPL_NOTIFY      "NOT"
#define  CL_CKPT_CTX_REPL_UPDATE      "UPD"
#define  CL_CKPT_CTX_PEER_DOWN        "PDN"
#define  CL_CKPT_CTX_PEER_WELCOME     "WEL"
#define  CL_CKPT_CTX_PEER_ANNOUNCE    "ANN"    
#define  CL_CKPT_CTX_ACTADDR_SET      "SET"
#define  CL_CKPT_CTX_HDL_DEL          "HDL"
#define  CL_CKPT_CTX_DEP_SYNCUP       "SYN"

/**==================================================================**/
/**     Server side implementations                                  **/
/**==================================================================*/
ClRcT clHandleCreateSpecifiedHandle (
    ClHandleDatabaseHandleT databaseHandle,
        ClInt32T instance_size,
    ClHandleT handle);
ClRcT clCkptMasterAddressesSet();
ClRcT _ckptCheckpointDelete(ClCkptHdlT     ckptHdl);
ClRcT clCkptMasterPeerUpdate(ClIocPortT portId, ClUint32T flag, ClIocNodeAddressT localAddr,
                             ClUint8T credentials); 
ClRcT clCkptMasterPeerUpdateNoLock(ClIocPortT portId, ClUint32T flag, ClIocNodeAddressT localAddr,
                                   ClUint8T credentials); 

ClRcT _ckptRemSvrInitialWelcome(ClEoDataT data,
                                ClBufferHandleT  inMsg,
                                ClBufferHandleT  outMsg);
ClRcT _ckptCheckpointOpenAsync(
       ClCkptHdlT     mastHdl,
       ClInvocationT  invocation,
       ClNameT        *pName,
       ClCkptCheckpointCreationAttributesT *pCreateAttr,
       ClCkptOpenFlagsT        checkpointOpenFlags,
       ClCkptHdlT   *pCkptActHdl );
ClRcT  ckptSvrCbAlloc(CkptSvrCbT **pSvrCb);
ClRcT  ckptSvrCbFree(CkptSvrCbT *pSvrCb);

ClRcT  ckptEntryAlloc(const ClNameT  *ckptName,  CkptT   **pCkpt);
ClRcT  ckptEntryFree(CkptT   *pCkpt);
ClRcT  _ckptSvrIterationInitialize(ClEoDataT data,
                                   ClBufferHandleT inMsg,
                                   ClBufferHandleT  outMsg);
ClRcT _ckptSectionIterationInitialize( ClCkptHdlT              ckptHdl,
                                       ClCkptSectionsChosenT   sectionsChosen,
                                       ClTimeT                 expirationTime,
                                       ClBufferHandleT  *pMsg);

ClRcT  _ckptRemSvrNotify(ClEoDataT data,
                         ClBufferHandleT  inMsg,
                         ClBufferHandleT  outMsg);
ClRcT  _ckptDplaneInfoAlloc(CkptDPlaneInfoT  **pDplaneInfo, ClUint32T  scnCount);
ClRcT  _ckptDplaneInfoFree (CkptDPlaneInfoT  *pDpInfo);

ClRcT  _ckptCplaneInfoAlloc(CkptCPlaneInfoT  **pCplaneInfo);
ClRcT  _ckptCplaneInfoFree (CkptCPlaneInfoT  *pCpInfo);
ClRcT clCkptSvrInitialize();
ClRcT ckptEOInit();
void ckptIDSet(CkptT *pCkpt);
ClRcT  ckptEntriesShow();
/* This routine finds a section in a given checkpoint */
ClRcT  _ckptSectionFind(CkptT                 *pCkpt, 
                        ClCkptSectionIdT      *pKey, 
                        CkptSectionT          **ppOpSec,
                        ClUint32T             *pCount);

/* This routine deletes a section in a given checkpoint */
ClRcT  ckptSectionDelete (CkptT               *pCkpt, 
                        ClCkptSectionIdT      *pKey, 
                        ClBoolT                ckptDelete,
                        ClBoolT                *pFlag);

/* This routine deletes a checkpoint from the network */
ClRcT   ckptNetworkWideCheckpointDelete(CkptT    *pCkpt);

void ckptInfoVersionConvertForward(VDECL_VER(CkptInfoT, 5, 0, 0) *pDstCkptInfo,
                                   CkptInfoT *pCkptInfo);
    
void ckptInfoVersionConvertBackward(CkptInfoT *pDstCkptInfo, VDECL_VER(CkptInfoT, 5, 0, 0) *pCkptInfo);

/* This routine unpacks the complete checkpoint DB */
ClRcT   ckptDbUnpack(ClBufferHandleT   inMsg);

/* This routine packs the complete checkpoint DB */
ClRcT   ckptDbPack(ClBufferHandleT   *pOutMsg, 
                   ClIocNodeAddressT        peerAddr);
ClRcT  clCkptCheckpointAsync(ClEoDataT data,
                        ClBufferHandleT  inMsg,
                        ClBufferHandleT  outMsg);
ClRcT  clCkptSvrInitInfoPack(ClEoDataT data,
                        ClBufferHandleT  inMsg,
                        ClBufferHandleT  outMsg);
/* This routine replies to the new coming server  */
ClRcT   _ckptSvrPeerWelcome (ClEoDataT data, 
                        ClBufferHandleT  inMsg,
                        ClBufferHandleT  outMsg);

/* Function to add checkpoint from a remote svr */    
ClRcT   _ckptRemSvrCkptAdd (ClEoDataT data, 
                           ClBufferHandleT  inMsg,
                           ClBufferHandleT  outMsg);

#if 0                           
/* Function to add checkpoint from a remote svr */    
ClRcT   _ckptRemSvrCheckpointDel (ClEoDataT data, 
                                  ClBufferHandleT  inMsg,
                                  ClBufferHandleT  outMsg);
#endif

ClRcT   _ckptActiveReplicaSyncUp( ClEoDataT data,
                              ClBufferHandleT  inMsg,
                              ClBufferHandleT  outMsg);


ClRcT _ckptCheckpointOpen(ClCkptHdlT           mastHdl,
			              ClNameT             *pName,
                          ClCkptCheckpointCreationAttributesT *pCreateAttr,
                          ClCkptOpenFlagsT     checkpointOpenFlags,
                          ClCkptHdlT           *pCkptActHdl);


ClRcT  _ckptRemSvrCheckpointAdd(CkptT *pCkpt,
                                ClCkptHdlT ckptHdl,
                                ClRcT *pRetCode, ClOsalCondIdT condVar);

/* Function to delete a checkpoint on another server */
ClRcT   ckptRemSvrCheckpointDel(ClCkptHdlT     ckptHdl);

/* This routine announces the arrival of a checkpoint server */
ClRcT   ckptSvrArrvlAnnounce();

ClRcT   _ckptSvrArrvlAnnounce();

/* This routine announces the departure of a checkpoint server */
ClRcT   ckptSvrDepartureAnnounce();
ClRcT   ckptInitialConsume(ClBufferHandleT inMsg);

#if 0
/* This routine accept the db from existing servers in the system */
ClRcT   _ckptSvrPeerDbConsume(ClEoDataT data, 
                        ClBufferHandleT  inMsg,
                        ClBufferHandleT  outMsg);
/* This routine replies to the out going server  */
ClRcT   _ckptSvrPeerBye(ClEoDataT data, 
                        ClBufferHandleT  inMsg,
                        ClBufferHandleT  outMsg);

/* Function to create a section from a remote svr */    
ClRcT   _ckptRemSvrSectionCreate (ClEoDataT data, 
                        ClBufferHandleT  inMsg,
                        ClBufferHandleT  outMsg);

/* Function to create a section from a remote svr */    
ClRcT   _ckptRemSvrSectionDelete (ClEoDataT data, 
                        ClBufferHandleT  inMsg,
                        ClBufferHandleT  outMsg);

/* Function to overwrite a section from a remote svr */    
ClRcT   _ckptRemSvrSectionOverwrite(ClEoDataT data, 
                        ClBufferHandleT  inMsg,
                        ClBufferHandleT  outMsg);
#endif
/* Timer expiry callback . Would be packing the data and send it to the server*/
ClRcT ckptSyncTimerCallback(void *);

/* Function to Add a section on another server */
ClRcT   ckptRemSvrSectionAdd( CkptT              *pCkpt, 
                              CkptSectionT       *pSec,
                              ClBoolT             *pFlag);
void ckptSectionCreateCallback( ClRcT rc,void *pData,
                                ClBufferHandleT inMsg,
                                ClBufferHandleT  outMsg);

/* Function to delete  a section on another server */
ClRcT   ckptRemSvrSectionDel( CkptT              *pCkpt, 
                              CkptSectionT       *pSec,
                              ClBoolT             *pFlag);

/* Function to Add a section on another server */
ClRcT   ckptRemSvrSectionOverwrite ( CkptT              *pCkpt, 
                                     CkptSectionT       *pSec,
                                      ClBoolT             *pFlag);

/* This routine updates the presence list for a checkpoint */
ClRcT   _ckptPresenceListUpdate( CkptT                *pCkpt, 
                                 ClIocNodeAddressT    peerAddr);

/*
   This routine updates the presence list
   by walking thru all the peers for a checkpoint
 */
ClRcT   ckptPresenceListRefresh ( CkptT                *pCkpt);

/* Routine to shutdown a CKPT server*/
ClRcT   ckptShutDown();

/* Config read API */
ClRcT ckptDebugRegister( ClEoExecutionObjT* pEoObj);

/* This routine initializes the event service library to be used by ckpt */
ClRcT   ckptEventSvcInitialize();

/* This routine finalizes the event service library used by ckpt */
ClRcT   ckptEventSvcFinalize();


/* This routine deregisters with debug CLI */
ClRcT ckptDebugDeregister( ClEoExecutionObjT* pEoObj);

/* Function to update the ownership info */
ClRcT   ckptOwnershipInfoUpdate(CkptT   *pCkpt);

/* This routine prints a string on the debug server */
void ckptCliPrint( char *str,
                   ClCharT ** ret);

/* This routine updates the presence list */
void ckptPeerDown(ClIocNodeAddressT   peerAddr, ClUint32T flag, ClIocPortT portId); /* Gopal */

/* Routine to get the next most prefered node */
ClRcT ckptMostPrefNodeGet(ClNameT *pName, ClIocNodeAddressT *pPrefNode);

/* Routine for getting status relted to a ckpt */
ClRcT clMasterStatusInfoGet(ClEoDataT data,
                            ClBufferHandleT  inMsg,
                            ClBufferHandleT  outMsg);

/* Routine to pack the master database */
ClRcT clMasterDatabasePack(ClEoDataT data,
                           ClBufferHandleT  inMsg,
                           ClBufferHandleT  outMsg);

/* Routine to update the deputy master */
ClRcT clDeputyMasterUpdate(ClEoDataT data,
                           ClBufferHandleT  inMsg,
                           ClBufferHandleT  outMsg);

ClRcT ckptActiveRepAddressUpdate(ClIocNodeAddressT newAddr);

ClRcT   clCkptNackSend(ClVersionT version, ClUint32T funId);                                        
ClRcT   _ckptAddAndUpdatePeerList(ClCkptHdlT ckptActHdl,ClNameT *pCkptName,ClIocNodeAddressT nodeAddr);
ClRcT ckptCheckpointEntryPack( CkptT                    *pCkpt,
                               ClCkptHdlT                ckptHdl,
                               ClBufferHandleT   *pMsgHdl,
                               ClIocNodeAddressT         peerAddr);

ClRcT  _ckptActPeerListUpdate(ClEoDataT data,
                              ClBufferHandleT  inMsg,
                              ClBufferHandleT  outMsg);
                              
ClRcT clCkptPresenceListUpdate(ClEoDataT data,
                        ClBufferHandleT inMsg,
                        ClBufferHandleT outMsg);
ClRcT  ckptReplicaNodeGet(ClIocNodeAddressT peerAddr,
                        ClIocNodeAddressT *pNodeAddr);                   


ClRcT  _ckptMasterSvrFinalize(ClEoDataT data, 
                        ClBufferHandleT  inMsg,
                        ClBufferHandleT  outMsg);
                        
ClRcT ckptMasterIntimatePrevActive(ClEoDataT data,
                           ClBufferHandleT  inMsg,
                           ClBufferHandleT  outMsg);

ClRcT ckptLeaderAddrUpdate(ClEoDataT data,
                           ClBufferHandleT  inMsg,
                           ClBufferHandleT  outMsg);
ClRcT   ckptRemSvrSectionWrite (ClBufferHandleT msg, 
                               CkptT                   *pCkpt,
                                CkptCbInfoT        *pCallInfo);
#if 0                                
ClRcT   ckptSectionDetailedPack( ClBufferHandleT   msg,
                                 CkptT                   *pCkpt,
                                 CkptSectionT            *pSec);
#endif                                 
ClRcT ckptMasterActiveAddressGet(ClEoDataT eoData,
                                 ClBufferHandleT inMsg,
                                 ClBufferHandleT outMsg);

ClRcT _ckptLocalDataUpdate(ClCkptHdlT         ckptHdl,
                           ClNameT            *pName,
      ClCkptCheckpointCreationAttributesT     *pCreateAttr,
                           ClUint32T          cksum,
                           ClIocNodeAddressT  appAddr,
                           ClIocPortT         appPort);
ClRcT _ckptReplicaIntimation(ClIocNodeAddressT replicaAddr,
                            ClIocNodeAddressT activeAddr,
                            ClHandleT         activeHdl,
                            ClBoolT           tryOtherNode,
                            ClUint32T         flag);
ClRcT _ckptCheckpointInfoPack( CkptT       *pCkpt,
                               VDECL_VER(CkptCPInfoT, 5, 0, 0) *pCpInfo,
                               CkptDPInfoT *pDpInfo);
ClRcT   _ckptCPlaneInfoPack(CkptCPlaneInfoT  *pCpInfo,
                            VDECL_VER(CkptCPInfoT, 5, 0, 0) *pCpOutInfo);
ClRcT   _ckptDPlaneInfoPack( CkptT  *pCkpt, 
                             CkptDPlaneInfoT *pDpInfo,
                             CkptDPInfoT     *pDpOutInfo);
ClRcT _ckptReplicaInfoUpdate( ClHandleT   ckptHdl,
                              VDECL_VER(CkptInfoT, 5, 0, 0)   *pCkptInfo);
ClRcT   _ckptRemSvrSectionInfoUpdate(ClHandleT    ckptHdl, 
                              CkptT              *pCkpt,
                              CkptSectionT       *pSec,
                              CkptUpdateFlagT     updateFlag,
                              ClBoolT            isDefer,
                              ClUint32T           count);
ClRcT _ckptSectionDataPack(CkptSectionT     *pSec,
                           ClCkptSectionIdT *pSecId,
                           CkptSectionInfoT *pSection,
                           CkptUpdateFlagT   updateFlag,
                           ClUint32T         count);
ClRcT _ckptSectionInfoPack( CkptT           *pCkpt,
			    CkptDPlaneInfoT *pDpInfo,
                            CkptSectionInfoT *pSection,
                            ClUint32T maxScns);
ClRcT ckptSectionDataUnpack(CkptSectionInfoT *pSection,
                            CkptUpdateFlagT   updateFlag,
                            CkptSectionT     *pSec,
                            CkptT            *pCkpt, 
                            ClUint32T         flag);
ClRcT _ckptPresenceListPeerUpdate(CkptT            *pCkpt,
                                  ClHandleT         ckptHdl,
                                  ClIocNodeAddressT peerAddr);
ClRcT _ckptCollocatedActAddrUpdate(ClHandleT handle, void *pCookie);
ClRcT ckptMasterAllCkptInfoPack(ClIocNodeAddressT peerAddr);

#if 0
ClRcT ckptListWalkForInfoPack( ClCntKeyHandleT   key,
                            ClCntDataHandleT  hdl,
                            void              *pData,
                            ClUint32T         dataLength);
#endif                            
ClRcT ckptPeerListInfoUnpack(ClUint32T ckptCount,
                             CkptPeerListInfoT  *pPeerListInfo);
ClRcT ckptMasterDBInfoUnpack(ClUint32T mastHdlCount,
                             CkptMasterDBEntryIDLT *pMasterDBInfo);
ClRcT ckptClientDBInfoUnpack(ClUint32T clientHdlCount,
                             CkptMasterDBClientInfoT *pClientDBInfo);
ClRcT ckptXlatioEntryUnpack( ClUint32T ckptCount,
                             CkptXlationDBEntryT *pXlationInfo);
ClRcT _ckptRetentionTimerExpiry (void *pRetenInfo);
ClRcT _ckptPeerListWalk(ClCntKeyHandleT    userKey,
        ClCntDataHandleT   hashTable,
        ClCntArgHandleT    userArg,
        ClUint32T          dataLength);
ClRcT _ckptPeerMasterHdlWalk(ClCntKeyHandleT    userKey,
        ClCntDataHandleT   hashTable,
        ClCntArgHandleT    userArg,
        ClUint32T          dataLength);
ClRcT _ckptCheckpointLocalWrite(ClCkptHdlT  ckptHdl,
                               ClUint32T   numberOfElements,
                               ClCkptIOVectorElementT *pIoVector,
                               ClUint32T   *pError,
                               ClIocNodeAddressT  nodeAddr,
                               ClIocPortT         portId);
ClRcT _ckptCheckpointLocalWriteVector(ClCkptHdlT  ckptHdl,
                                      ClUint32T   numberOfElements,
                                      ClCkptDifferenceIOVectorElementT *pDifferenceIoVector,
                                      ClUint32T   *pError,
                                      ClIocNodeAddressT  nodeAddr,
                                      ClIocPortT         portId,
                                      ClCkptDifferenceIOVectorElementT *pDifferenceReplicaVector,
                                      ClBoolT *pReplicate,
                                      ClPtrT *pStaleSectionsData);

ClRcT ckptSvrHdlCheckout( ClHandleDatabaseHandleT ckptDbHdl,
                          ClHandleT               ckptHdl,
                          void                    **pData);
    
ClRcT ckptPackInfoFree( VDECL_VER(CkptInfoT, 5, 0, 0)  *pCkptInfo);                          

ClRcT _ckptActiveCkptDeleteCall( ClVersionT version,
                                ClHandleT  ckptHdl);

ClRcT
_ckptMasterPeerListInfoCreate(ClIocNodeAddressT nodeAddr,
                             ClUint32T         credential,
                             ClUint32T         replicaCount);
ClRcT ckptPresenceListNodeDelete( ClCntKeyHandleT    key,
                                         ClCntDataHandleT   hdl,
                                         void              *pData,
                                         ClUint32T          dataLength);
ClRcT _clCkpMastertReplicaAddressUpdate(ClHandleT         mastHdl,
                                        ClIocNodeAddressT newActAddr);
ClRcT _ckptMasterCkptsReplicate(ClHandleDatabaseHandleT databaseHandle,
                                ClHandleT masterHdl,void *pData);

ClRcT
_ckptSectionExpiryTimeoutGet(ClTimeT expiryTime, ClTimerTimeOutT *timeOut);

ClRcT
_ckptExpiryTimerStart(ClHandleT       ckptHdl,
                     ClCkptSectionIdT  *pSecId, 
                     CkptSectionT    *pSec,
                     ClTimerTimeOutT timeOut);
ClRcT
_ckptSectionTimerCallback(void *pArg);

ClRcT _ckptLocalSectionExpirationTimeSet(ClVersionT        *pVersion,
                                         ClCkptHdlT        ckptHdl,
                                         ClCkptSectionIdT  *pSectionId,
                                         ClTimeT           exprTime);
                                         
void ckptMastHdlListDeleteCallback(ClCntKeyHandleT  userKey,
                                   ClCntDataHandleT userData);

ClRcT clCkptIsServerRunning(ClIocNodeAddressT addr, ClUint8T* isRunning);

extern ClRcT
clCkptRemSvrSectionCreate(ClCkptHdlT                        ckptHdl, 
                          CkptT                             *pCkpt, 
                          ClCkptSectionCreationAttributesT  *pSecCreateAttr,
                          ClUint32T                         initialDataSize,
                          ClUint8T                          *pInitialData);
extern ClRcT
clCkptRemSvrSectionOverwrite(ClCkptHdlT        ckptHdl, 
                             CkptT             *pCkpt, 
                             ClCkptSectionIdT  *pSectionId, 
                             CkptSectionT      *pSec,
                             ClUint8T          *pData,
                             ClUint32T         dataSize,
                             ClIocNodeAddressT nodeAddr,
                             ClIocPortT        portId);

extern ClRcT
clCkptRemSvrSectionOverwriteVector(ClCkptHdlT        ckptHdl, 
                                   CkptT             *pCkpt, 
                                   ClCkptSectionIdT  *pSectionId, 
                                   CkptSectionT      *pSec,
                                   ClUint8T          *pData,
                                   ClUint32T         dataSize,
                                   ClIocNodeAddressT nodeAddr,
                                   ClIocPortT        portId,
                                   ClDifferenceVectorT    *differenceVector);

extern ClRcT
clCkptDefaultSectionInfoGet(CkptT         *pCkpt,
                            CkptSectionT  **ppSec);
extern ClRcT
clCkptSectionInfoGet(CkptT             *pCkpt,
                     ClCkptSectionIdT  *pSectionId,
                     CkptSectionT      **ppSec);
extern ClRcT
clCkptRemSvrSectionDelete(ClCkptHdlT        ckptHdl,
                          CkptT             *pCkpt, 
                          ClCkptSectionIdT  *pSecId);
extern ClRcT
clCkptSecFindNDelete(CkptT             *pCkpt, 
                     ClCkptSectionIdT  *pSecId);
extern ClRcT
clCkptSectionChkNAdd(ClCkptHdlT  ckptHdl,
             CkptT               *pCkpt, 
             ClCkptSectionIdT    *pSectionId, 
             ClTimeT             expiryTime,
             ClSizeT             initialDataSize,
             ClUint8T            *pInitialData);
extern ClRcT
clCkptDefaultSectionAdd(CkptT      *pCkpt,
                        ClUint32T  initialDataSize,
                        ClUint8T   *pInitialData);
extern ClRcT
clCkptSectionCreateInfoPull(ClCkptHdlT        ckptHdl, 
                            CkptT             *pCkpt,
                            ClCkptSectionIdT  *pSecId);
extern ClRcT
clCkptClntWriteNotify(CkptT                   *pCkpt,
                      ClCkptIOVectorElementT  *pVec,
                      ClUint32T               numSections,
                      ClIocNodeAddressT       nodeAddr, 
                      ClIocPortT              iocPort);
extern ClRcT
clCkptClntSecOverwriteNotify(CkptT             *pCkpt,
                             ClCkptSectionIdT  *pSecId,
                             CkptSectionT      *pSec,
                             ClIocNodeAddressT nodeAddr,
                             ClIocPortT        portId);
extern ClRcT
clCkptActiveAppInfoUpdate(CkptT              *pCkpt,
                          ClIocNodeAddressT  appAddr,
                          ClIocPortT         appPort,
                          ClUint32T          **ppData);
extern ClRcT
clCkptSectionLevelMutexCreate(ClOsalMutexIdT  *pSecMutexArray, 
                              ClUint32T       numMutexs);
extern void
clCkptSectionLevelMutexDelete(ClOsalMutexIdT  *pSecMutexArray, 
                              ClUint32T       numMutex);
extern ClRcT
clCkptSectionLevelUnlock(CkptT             *pCkpt,
                         ClCkptSectionIdT  *pSectionId,
                         ClBoolT sectionLockTaken);
extern ClRcT
clCkptSectionLevelLock(CkptT             *pCkpt,
                       ClCkptSectionIdT  *pSectionId,
                       ClBoolT *pSectionLockTaken);
extern ClRcT
clCkptSectionLevelDelete(ClCkptHdlT        ckptHdl,
                         CkptT             *pCkpt, 
                         ClCkptSectionIdT  *pSecId,
                         ClBoolT           srcClient);
extern ClUint32T
clCkptSectionIndexGet(CkptT             *pCkpt,
                      ClCkptSectionIdT  *pSecId);
extern ClRcT
clCkptSvrReplicaDelete(CkptT       *pCkpt, 
                       ClCkptHdlT  ckptHdl,
                       ClBoolT     isActive);
extern ClRcT
clCkptIocCallbackUpdate(void);

extern ClRcT
clCkptMasterAddressUpdate(ClIocNodeAddressT  leader, 
                          ClIocNodeAddressT  deputy);
extern ClRcT
clCkptSectionTimeUpdate(ClTimeT   *pLastUpdate);

extern
void ckptDifferenceVectorGet(CkptT *pCkpt, ClCkptSectionIdT *pSectionId, CkptSectionT *pSec, ClUint8T *pData,
                             ClOffsetT offset, ClSizeT dataSize, ClDifferenceVectorT *differenceVector,
                             ClBoolT reset);

extern
ClUint8T *ckptDifferenceVectorMerge(CkptT *pCkpt, ClCkptSectionIdT *pSectionId, CkptSectionT *pSec, 
                                    ClDifferenceVectorT *differenceVector, ClOffsetT offset, ClSizeT dataSize);

extern void
clCkptDifferenceIOVectorFree(ClCkptDifferenceIOVectorElementT *pDifferenceIoVector, ClUint32T numElements);

extern
ClInt32T ckptHdlNonUniqueKeyCompare(ClCntDataHandleT givenData, ClCntDataHandleT data);

#ifdef __cplusplus
}
#endif
#endif         /* _CL_CKPT_IPI_H_ */  
