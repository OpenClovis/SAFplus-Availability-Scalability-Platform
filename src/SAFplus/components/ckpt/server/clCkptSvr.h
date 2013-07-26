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
 * File        : clCkptSvr.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains Checkpoint server data structures
*
*
*****************************************************************************/
#ifndef _CL_CKPT_SVR_H_
#define _CL_CKPT_SVR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clOsalApi.h>
#include <clCntApi.h>
#include <clClmApi.h>
#include <clCpmApi.h>
#include <clIdlApi.h>
#include <clEventApi.h>
#include <clHandleApi.h>
#include "clCkptDs.h"

#define   CL_CKPT_EVT_CH     "CL_CKPT_EVENTS"
#define   CL_CKPT_NODE_DOWN  0
#define   CL_CKPT_SERVER_UP  1
#define   CL_CKPT_SVR_DOWN   2 
#define   CL_CKPT_COMP_DOWN  3  

typedef struct ckptMasterDBInfo
{
    ClIocNodeAddressT        masterAddr;   /* Address of node containing master 
                                            meta data database */
    ClIocNodeAddressT        deputyAddr;   /* back up master address */
    ClIocNodeAddressT        prevMasterAddr;   /* previous master address */
    ClHandleDatabaseHandleT  masterDBHdl;     /* Meta data DB at master */
    ClHandleDatabaseHandleT  clientDBHdl;     /* client handle DB at master */
    ClCntHandleT             nameXlationDBHdl;     /* Xlation data DB at master */
    ClOsalMutexIdT           ckptMasterDBSem;  /* Semaphore guarding the  data structures */
    ClCntHandleT             peerList;     /* List of checkpoint servers in the system.*/
    ClInt8T                  availPeerCount; /* No.of peers ready to replicate */
    ClUint32T                clientHdlCount;
    ClUint32T                masterHdlCount;
    ClUint32T                compId; /* CompId of the master */
}CkptMasterDBInfoT;

/* Each checkpointing Server has
   A Control block
        - A handle for the EO
        - Semaphore
        - Service State
        - Availability State
        - Node Level Constraints
            CheckpointReplicaCreationAllowed
            CheckpointMaxReplicasAllowed
            CheckpointMaxMemory

        - List of checkpoints (Each checkpoint has)
            - Name
            - Length of the name
            - Replica Semantics  
                  DesiredNumReplicas
                  ListOfNodes
                  ReplicaDistributionPolicy
            - Checkpoint properties
                  CheckpointUpdateOption
                  CheckpointIsCollocated
                  CheckpointRetDuration
                  CheckpointMaxSections
                  CheckpointMaxSectionSize
                  CheckpointMaxSectionIdSize
                  List of sections

            - List of replicaDetails (Each replica data contains)
                  NodeIdLoc
                  NodeReplicaType

            - Checkpoint statistics 
                  CheckpointSize
                  CheckpointCreateTime
                  CheckpointLastSynchronized
                  CheckpointNumUsers
                  CheckpointNumReaders
                  CheckpointNumWriters
                  CheckpointNumReplica
                  CheckpointNumSections
                  CheckpointNumCorruptSections
        - List of clients 
                  client comm port Identification
        - List of Peers  
                  peer comm port Identification

 */

typedef struct ckptSvrCb 
{
    ClEoExecutionObjT      *eoHdl;       /* EO handle for al EO related
                                            operations */
    ClIocPortT             eoPort;       /* EO port */
    ClCpmHandleT           cpmHdl;       /* CPM handle */
    ClGmsHandleT           gmsHdl;       /* GMS handle */
    ClEventInitHandleT     evtSvcHdl;    /* Event Service Handle */
    ClEventChannelHandleT  evtChHdl;     /* Event Channel handle */
    ClEventChannelHandleT  clntUpdHdl;   /* Event Channel handle */
    ClEventHandleT         clntUpdEvtHdl;   /* Event Channel handle */
    ClNameT                evtChName;    /* Event channel name */ 
    ClEventHandleT         pubHdl;       /* Event handle */
    ClEventChannelHandleT  compChHdl;    /* CPM Channel handle 
                                            for component related events */
    ClEventChannelHandleT  nodeChHdl;    /* CPM Channel handle  for
                                            node related events */
    ClIocNodeAddressT      localAddr;    /* Server IOC address */
    CkptMasterDBInfoT      masterInfo;   /* Info related to master server */
    ClOsalMutexIdT         ckptActiveSem;  /* Semaphore guarding the 
                                            active server related DB */
    ClCntHandleT           ckptHdlList;  /* Hdl list at active server*/
    ClHandleDatabaseHandleT  ckptHdl;    /* Handle database at active server */
    ClCntHandleT           peerList;     /* List of checkpoint servers in 
                                            the system */
    ClIdlHandleT           ckptIdlHdl;
    ClUint8T               condVarWaiting;
    ClOsalCondIdT          condVar;
    ClOsalMutexIdT         mutexVar;
    ClVersionDatabaseT     versionDatabase;
    ClBoolT                serverUp;
    ClBoolT                isAnnounced;
    ClBoolT                isSynced; /* synced with master*/
    ClUint32T              compId;
    ClOsalMutexT           ckptClusterSem;

#define _CKPT_COLLOCATE_REPLICA_ON_OPEN (0x1)
#define _CKPT_DIFFERENTIAL_REPLICA      (0x2)
#define CKPT_COLLOCATE_REPLICA_ON_OPEN(flag) ( (flag) & _CKPT_COLLOCATE_REPLICA_ON_OPEN )
#define CKPT_DIFFERENTIAL_REPLICA(flag)  ( (flag) & _CKPT_DIFFERENTIAL_REPLICA )

    ClUint32T              replicationFlags;
}CkptSvrCbT; 

#ifdef __cplusplus
}
#endif
#endif    /* _CL_CKPT_SVR_H_ */ 
