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
 * File        : clCkptDs.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
*
*   This file contains Checkpoint server related data structures
*
*
*****************************************************************************/
#ifndef _CL_CKPT_DS_H_
#define _CL_CKPT_DS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <clCntApi.h>
#include <clTimerApi.h>
#include <clCkptApi.h>
#include <clCkptExtApi.h>
#include <xdrCkptPeerT.h>
#include <xdrCkptPrimInfoT.h>
#include <xdrCkptSyncStateT.h>


/**====================================**/
/**     C O N S T A N T S              **/
/**====================================**/


#define CL_CKPT_HASH_TBL_SIZE           17  
#define CL_CKPT_PEER_HASH_TBL_SIZE      17  
#define CL_CKPT_CLNT_HASH_TBL_SIZE      17  
#define CL_CKPT_MAX_NAME                256 
#define CL_CKPT_INIT_PRIORITY           32067
#define CL_CKPT_PUB_NAME                      "CKPT_PUB_NAME"
#define CL_CKPT_UPDATE_CLIENT_CHANNEL_NAME    "CPM_UPD_CLIENT_CHANNEL"
#define  CL_CKPT_UPDATE_CLIENT_TYPE            0x100

#define CL_CKPT_MAX_NUM_THREADS   5
#define CL_CKPT_MAX_NUM_MUTEXS    50 

/**====================================**/
/**         E N U M S                  **/
/**====================================**/


typedef enum
{
    CKPT_INVALID_SVC_STATE = 0,  /* Invalid state */
    CKPT_NO_SVC_AVAILABLE,       /* Checkpoint service is not available*/
    CKPT_LIMITED_SVC_AVAILABLE,  /* Limited service available*/
    CKPT_FULL_SVC_AVAILABLE,     /* Complete service available */
    CKPT_SVC_STATE_MAX,  
}CkptSvcStateT;

typedef enum
{
    CKPT_INVALID_AVLBL_STATE = 0,  /* Invalid state */
    CKPT_AVLBL_STATE_BEST_EFFORT,  /* Best Effort mode*/
    CKPT_AVLBL_STATE_RELIABLE,     /* Reliable Mode */
    CKPT_AVLBL_STATE_MAX           /* Reliable Mode */
}CkptAvlblModeT;



/**====================================**/
/**       S T R U C T U R E S          **/
/**====================================**/


/* 
 * This structure represents the statistics associated with a checkpoint.
 */
 
typedef struct ckptStats
{
    ClSizeT    ckptSize;       /* Size of the checkpoint in bytes */
    ClTimeT    ckptCreateTime; /* Time at which the checkpoint is created */
    ClTimeT    ckptLastSync;   /* Time at which the checkpoint is created */
    ClUint32T  ckptNumRplcs;   /* Number of replicas */
    ClUint32T  ckptNumCrptScns;/* Number of Corrupt Sections*/
    ClUint32T  ckptNumUsrs;    /* Number of users */
    ClUint32T  ckptNumWrtrs;   /* Number of writers */
    ClUint32T  ckptNumRdrs;    /* Number of readers */
}CkptStatsT;

/*
 * The following structure represents a section key
 */
 typedef struct 
 {
   ClCkptSectionIdT  scnId; 
   ClUint32T         hash;
 }ClCkptSectionKeyT;

/* 
 * The following structure represents a section data 
 */
 
typedef struct ckptSection
{
    //ClCkptSectionIdT    scnId;       /* Section identifier  */
    ClBoolT             used;        /* Flag specifying used/unused section */
    ClTimeT             exprTime;    /* Expiry time */
    ClCkptSectionStateT state;       /* State of a section */
    ClPtrT              pData;       /* Pointer to the data of this section */
    ClSizeT             size;        /* Size of the section */
    ClTimeT             lastUpdated; /* When was the last update */
    ClTimerHandleT      timerHdl;    /* Expiration timer handle */
    ClDifferenceVectorKeyT *differenceVectorKey;
} CkptSectionT;


/*
 * Expiration timer related cookie.
 */
 
typedef struct ckptSecTimer
{
    ClCkptHdlT       ckptHdl; /* Checkpoint handle */
    ClCkptSectionIdT secId;   /* Section identifier */
}CkptSecTimerInfoT;



/* 
 * This structure represents the control plane information 
 * related to a checkpoint
 */
 
typedef struct ckptCtrlInfo
{
    ClUint32T       updateOption; /* Policy deciding how the replicas 
                                     are updated */
    ClCntHandleT    presenceList; /* List of replica Nodes in the system */
    ClCntHandleT    appInfoList; /* Maintaining application info for Immediate
                                    consumption */
    ClUint64T       id;          /*unique checkpoint id*/
}CkptCPlaneInfoT;



/* 
 * This structure represents the data plane information 
 * related to a checkpoint
 */
 
typedef struct ckptDPlaneInfo
{
    ClSizeT       maxCkptSize;     /* Maximum size of checkpoint */
    ClUint32T     maxScns;         /* Maximum sections in this checkpoint */
    ClSizeT       maxScnSize;      /* Maximum size of any section
                                      in this checkpoint */
    ClSizeT       maxScnIdSize;    /* Maximum size of a section identifier */
    ClUint32T     numScns;         /* Number of sections */
    ClUint32T     indexGenScns;    /* Generated section id index */
    //CkptSectionT  *pSections;      /* Sections data */
    ClUint32T     numOfBukts;
    ClCntHandleT  secHashTbl;
}CkptDPlaneInfoT;



/*
 * The following structures are used to represent
 * a checkpoint information
 */

typedef struct ckpt
{
    ClNameT            ckptName;    /* Name of the checkpoint */
    CkptCPlaneInfoT    *pCpInfo;    /* Controlplane information */
    CkptDPlaneInfoT    *pDpInfo;    /* Dataplane info */
    ClOsalMutexIdT     ckptMutex;   /* Mutex to protect the data */
    ClBoolT            isPending;   /* to check server is ready or not */
    ClUint32T          numMutex;   /* Num mutexs need to be created for section */
    ClOsalMutexIdT     secMutex[CL_CKPT_MAX_NUM_MUTEXS]; /* pool of mutexs for section level lock */       
}CkptT;


#ifdef __cplusplus
}
#endif

#endif /* _CL_CKPT_DS_H_ */
