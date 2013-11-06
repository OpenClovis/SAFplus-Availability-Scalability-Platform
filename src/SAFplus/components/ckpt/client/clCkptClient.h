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
 * File        : clCkptClient.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 *   This file contains Checkpoint client related data structures
 *
 *
 *****************************************************************************/
#ifndef _CL_CKPT_CLIENT_H_
#define _CL_CKPT_CLIENT_H_

# ifdef __cplusplus
extern "C" {
# endif

#include <clCommon.h>
#include <clCntApi.h>
#include <clOsalApi.h>
#include <clEoApi.h>

#include <clCkptExtApi.h>
#include <clCkptApi.h>
#include <clIdlApi.h>
#include <clDbalApi.h>
#include <clCkptUtils.h>
#include <clCkptCommon.h>
#include  <clEventExtApi.h>

struct clnCkpt; /* Spoof the compiler */

/**====================================**/
/**     S T R U C T U R E S            **/
/**====================================**/

#define  CL_CKPT_AREA_CLIENT       "CLN"
#define  CL_CKPT_CTX_REG           "REG"
#define  CL_CKPT_CTX_NOTIFICATION  "NOT"

/*
 * Checkpoint client related data structure.
 */
 
typedef struct ClCkptClntInfo
{
    ClHandleDatabaseHandleT  ckptDbHdl;         /* Handle to database storing
                                                   info about ckpt/service/
                                                   iteration hdls */
    ClCntHandleT             ckptHdlList;       /* Ckpt Handle list (one hdl 
                                                   per open) */
    ClUint32T                ckptSvcHdlCount;   /* No. of service handles i.e
                                                   no. of client initialize */
    ClEventInitHandleT       ckptEvtHdl;        /* Event handle */
    ClEventChannelHandleT    ckptChannelSubHdl; /* Channel handle */
    ClOsalMutexT             ckptClntMutex;     /* Mutex to proctect the 
                                                   client info */
    ClIocPhysicalAddressT    ckptOwnAddr;
    
    ClBoolT                  ckptClientCacheDisable;
}ClCkptClntInfoT;



/*
 * Per client initialize instance related control block. i.e. Service handle
 * information.
 */
typedef struct initInfo
{
    ClUint8T           hdlType;         /* Handle type */
    ClIdlHandleT       ckptIdlHdl;      /* Idl Handle */
    ClIdlHandleT       ckptClientIdlHdl; /* ckpt peer idl handle */
    ClIocNodeAddressT  mastNodeAddr;    /* Master server address */
    ClCntHandleT       hdlList;         /* Iteration Handle list */
    ClCkptCallbacksT   *pCallback;      /* Callback functions */
    ClUint32T          readFd;          /* Selection Object read fd */
    ClUint32T          writeFd;         /* Selection Object write fd */
    ClUint8T           cbFlag;          /* Selection obj get called or not */ 
    ClQueueT           cbQueue;         /* Selection obj related queue */
    ClOsalMutexIdT     ckptSvcMutex;    /* Mutex to protect control block */
    ClOsalMutexIdT     ckptSelObjMutex; /* Selection obj related mutex */
    ClHandleT          ckptNotificationHandle; /* notification callback handle */
}CkptInitInfoT;



/*
 * Open (async flavour) related cookie info.
 */
 
typedef struct ckptOpenCbInfo
{
    ClInvocationT     invocation;  /* Invocation id */
    ClCkptHdlT        ckptHdl;     /* Checkpoint hdl */
    ClRcT             error;       /* Error */
}CkptOpenCbInfoT;



/*
 * Synchronize (async flavour) related cookie info.
 */
 
typedef struct ckptSyncCbInfo
{
    ClInvocationT     invocation;   /* Invocation id */
    ClRcT             error;        /* Error */   
}CkptSyncCbInfoT;



/*
 * Selection Object's queue related info.
 */

typedef struct ckptQueueInfo
{
    ClUint32T  callbackId;          /* Callback id */
    void       *pData;              /* Data to be stored */
}CkptQueueInfoT;



/*
 * Structure containing section iteration related info.
 */

typedef struct secIterateInfo
{
    ClUint8T           hdlType;     /* Handle type */ 
    ClCkptHdlT         ckptHdl;     /* Ckpt handle (Checkpoint Hdl) */
    ClCkptHdlT         localHdl;    /* Local handle to be passed to user */
    ClIdlHandleT       idlHdl ;     /* Idl handle */
    ClUint32T          secCount;    /* No. of sections (Iteration Hdl)*/
    ClCkptSectionIdT   *pSecId;     /* Section Id (Iteration Hdl) */
    ClUint32T          nextSec;     /* Next Section (Iteration Hdl) */
}CkptIterationInfoT;



/**====================================**/
/**     C O N S T A N T S              **/
/**====================================**/


#define CL_CKPT_OPEN_SYNC      0     /* Open call sync flavour */
#define CL_CKPT_OPEN_ASYNC     1     /* Open call async flavour */
#define CL_CKPT_MAX_FLAGS      0x07  /* Combination of all possible 
                                        open flags */
                                        
#define CL_CKPT_ITER_HDL       0     /* Handle type = Iteration handle */
#define CL_CKPT_CHECKPOINT_HDL 1     /* Handle type = Checkpoint handle */
#define CL_CKPT_SERVICE_HDL    2     /* Handle type = Service Handle */

#define CL_CKPT_OPEN_CALLBACK  0x01  /* Open callback */
#define CL_CKPT_SYNC_CALLBACK  0x02  /* Synchronize callback */

#define CL_CKPT_MAX_RETRY      2     /* Max rmd retries */
#define CL_CKPT_NO_RETRY       0     /* No retry */ 

/* This numer so far has no significance so far in GDBM and Berkely */
#define CKPT_MAX_NUMBER_RECORD 1000 
/* This numer so far has no significance so far in GDBM and Berkely */
#define CKPT_MAX_SIZE_RECORD   40000   

#define CKPT_DB_KEY            20         /* Key for all Ckpt Lib records */  
#define CL_CKPT_DB_NAME        "CKPT_DB"   /* Name for Ckpt Lib records */


/*
 * Macro for logging and setting proper return code in case of 
 * version mismatch.
 */
#define CL_CKPT_VERSION_VALD(releaseCode, rc)\
do\
{\
if (releaseCode != '\0' && (rc == CL_OK))\
{\
    rc = CL_CKPT_ERR_VERSION_MISMATCH;\
    clLogWrite(CL_LOG_HANDLE_APP, CL_LOG_SEV_ERROR, NULL,\
                    CL_CKPT_LOG_4_CLNT_VERSION_NACK, "Close call failed",\
                    version.releaseCode,version.majorVersion,\
                    version.minorVersion);\
}\
}while(0)



/**====================================**/
/**          E X T E R N S             **/
/**====================================**/


ClRcT ckptLocalHandleCreate(ClCkptHdlT *pCkptHdl);
ClRcT ckptHandleInfoSet(ClCkptHdlT ckptLocalHdl,CkptHdlDbT *pData);
ClRcT ckptActiveAddressGet(ClCkptHdlT ckptLocalHdl,ClIocNodeAddressT *pNodeAddr);
ClRcT ckptMasterHandleGet(ClCkptHdlT ckptLocalHdl,ClCkptHdlT *pCkptMastHdl);
ClRcT ckptLocalHandleDelete(ClCkptHdlT ckptLocalHdl);
ClRcT ckptActiveHandleGet(ClCkptHdlT ckptLocalHdl,ClCkptHdlT *pCkptActHdl);
ClRcT ckptContVarGet(ClCkptHdlT ckptLocalHdl,ClBoolT *retFlag);
ClRcT ckptContVarSet(ClCkptHdlT ckptLocalHdl,ClBoolT retFlag);
ClRcT ckptHandleRcSet(ClCkptHdlT ckptLocalHdl,ClRcT returnCode);
ClRcT ckptHandleRcGet(ClCkptHdlT ckptLocalHdl,ClRcT *returnCode);
ClRcT ckptSvcHandleSet(ClCkptHdlT ckptLocalHdl,ClCkptSvcHdlT svcHdl);
ClRcT ckptSvcHandleGet(ClCkptHdlT ckptLocalHdl,ClCkptSvcHdlT *pSvcHdl);
ClRcT ckptHandleCheckout( ClHandleT ckptHdl,
                          ClInt32T  hdlType,
                          void     **ppData); 

#ifdef __cplusplus
}
#endif

#endif							/* _CL_CKPT_CLIENT_H_ */ 
