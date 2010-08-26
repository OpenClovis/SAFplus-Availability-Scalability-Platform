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
 * ModuleName  : cor
 * File        : clCorTxnClientIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Defines internal APIs used by COR-Transaction Management.
 *
 *
 *****************************************************************************/

#ifndef _CL_COR_TXN_CLIENT_IPI_H_
#define _CL_COR_TXN_CLIENT_IPI_H_

#include <clCommon.h>
#include "clCorTxnJobStream.h"
#include "clCorClient.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *  Constant and Macro Definitions
 *****************************************************************************/

#define CL_COR_TXN_SESSIONS_BUCKETS             10

#define     CL_COR_TXN_JOB_ADJUST(header, jobNode)              \
do{                                                             \
    if((header)->head != (header)->tail)                        \
    {                                                           \
        if((jobNode) == (header)->head) /* First Node */        \
        {                                                       \
            (header)->head = (jobNode)->next;                   \
            ((jobNode)->next)->prev = NULL;                     \
        }                                                       \
        else if ((jobNode) == (header)->tail) /* Last Node */   \
        {                                                       \
            (header)->tail = (jobNode)->prev;                   \
            ((jobNode)->prev)->next = NULL;                     \
        }                                                       \
        else  /* Intermidiate node */                           \
        {                                                       \
            ((jobNode)->next)->prev = (jobNode)->prev;          \
            ((jobNode)->prev)->next = (jobNode)->next;          \
        }                                                       \
    }                                                           \
    else                                                        \
    {                                                           \
        (header)->head = (header)->tail = NULL;  /*Single node */ \
    }                                                           \
    (jobNode) = (jobNode)->next;                                \
}                                                               \
while(0)


/* Macro for adjusting the job */
#define CL_COR_BUNDLE_JOB_ADJUST(header, jobNode) CL_COR_TXN_JOB_ADJUST(header, jobNode)
/******************************************************************************
 *  Data Types 
 *****************************************************************************/
/**
 * COR Transaction Mode
 */
typedef enum {
/**
 *  Open edit-session having single job or operation.
 */
    CL_COR_TXN_MODE_SIMPLE,

/**
 *  Open edit-session supporting multiple job or operations
 */
    CL_COR_TXN_MODE_COMPLEX
} ClCorTxnModeT;

/**
 * Enumeration to identify type of object-id passed by application
 * to identify COR Object
 */
typedef enum {
    CL_COR_TXN_OBJECT_HANDLE,
    CL_COR_TXN_OBJECT_MOID
} ClCorTxnObjectTypeT;

/**
 * Common structure to hold any type object-id of COR Objects
 */
typedef union {
    ClCorObjectHandleT      oh;
    ClCorMOIdT              moId;
} ClCorTxnObjectIdT;

/**
 * Structure to define Object-Id of COR Objects
 */
typedef struct clCorTxnObjectHandle
{
    ClCorTxnObjectTypeT     type;
    ClCorTxnObjectIdT       obj;
} ClCorTxnObjectHandleT;


/**
 * Internal structure defining transaction-session.
 */
typedef struct clCorTxnSession
{
    ClCorTxnSessionIdT     txnSessionId;
    ClCorTxnModeT          txnMode;
} ClCorTxnSessionT;


/* Attribute Set Job.*/
struct ClCorTxnAttrSetJobNode{
    struct ClCorTxnAttrSetJobNode   *next;
    struct ClCorTxnAttrSetJobNode   *prev;
    ClCorTxnAttrSetJobT              job;
};

typedef struct ClCorTxnAttrSetJobNode ClCorTxnAttrSetJobNodeT;

/* Object Job  */
struct ClCorTxnObjJobNode {
    struct ClCorTxnObjJobNode     *next;
    struct ClCorTxnObjJobNode     *prev;
    ClCorTxnAttrSetJobNodeT       *head;
    ClCorTxnAttrSetJobNodeT       *tail;
    ClCorTxnObjJobT                job;
};
typedef struct ClCorTxnObjJobNode    ClCorTxnObjJobNodeT;

/*
   Header for COR's txn job.
   Here key is moId or oh, so one header per Object.
   There can be multiple jobs per object though.
   A Job Header of COR represents one Transaction Job. 
*/
struct ClCorTxnJobHeaderNode {
    ClCntHandleT            objJobTblHdl;  /* Container handle for the jobs. Helps in serializing the jobs. */
    ClCorObjectHandleT     *pReturnHdl;    /* User supplied reference for writing object handle for CREATE request. */
    ClCorTxnObjJobNodeT    *head;	   /* Last job on the list */
    ClCorTxnObjJobNodeT    *tail;	   /* First job on the list */
    ClCorTxnJobHeaderT      jobHdr;
};
typedef struct ClCorTxnJobHeaderNode ClCorTxnJobHeaderNodeT;

/* Structure to capture user  entered SET info */
struct ClCorUserSetInfo{
    ClCorAttrPathT   *pAttrPath;
    ClCorAttrIdT      attrId;
    ClUint32T         index;
    ClUint32T         size;
    void             *value;
    ClCorJobStatusT  *pJobStatus;
};
typedef struct ClCorUserSetInfo ClCorUserSetInfoT;


/* Defines */
/* Macro to add a Object Job to a COR-txn header.*/

#define CL_COR_TXN_OBJ_JOB_ADD(jobHdr, job)\
do{\
    if ((jobHdr)->head == NULL)\
    {\
        (job)->next = (job)->prev = NULL;\
        (jobHdr)->head  = (jobHdr)->tail = job;\
    }\
    else\
    {\
        (job)->next         = NULL;\
        (job)->prev         = (jobHdr)->tail;\
        ((jobHdr)->tail)->next = job;\
        (jobHdr)->tail         = job;\
    }\
 }\
 while(0)

/**
 * Macro for adding the jobs in the ascending order of the attribute Id.
 */

#define CL_COR_BUNDLE_JOB_SORT_AND_ADD(objJob, attrJob)\
do{\
    if ((objJob)->head == NULL)\
    {\
        (attrJob)->next = (attrJob)->prev = NULL;\
        (objJob)->head  = (objJob)->tail = attrJob;\
    }\
    else\
    {\
        ClCorTxnAttrSetJobNodeT *tempJobNode = NULL; \
        tempJobNode         = (objJob)->head; \
        while((tempJobNode)) \
        {\
            if(((tempJobNode)->job).attrId < ((attrJob)->job).attrId) \
                (tempJobNode) = (tempJobNode)->next; \
            else if(((tempJobNode)->job).attrId == ((attrJob)->job).attrId)\
            {\
                if(((tempJobNode)->job).index != ((attrJob)->job).index) /* Index is Different */\
                    (tempJobNode) = (tempJobNode)->next; \
                else \
                { \
                    if(((tempJobNode)->job).size != ((attrJob)->job).size) /* Size is Different*/\
                        (tempJobNode) = (tempJobNode)->next; \
                    else { \
                        clHeapFree(attrJob); \
                        CL_DEBUG_PRINT(CL_DEBUG_ERROR,("Cannot add same attribute")); \
                        return CL_COR_SET_RC(CL_COR_ERR_DUPLICATE);\
                    }\
                } \
            }\
            else \
                break; /* If attribute id is less */\
        }\
        if(NULL == (tempJobNode)) \
        {\
            (attrJob)->next             = NULL;\
            (attrJob)->prev             = (objJob)->tail;\
            ((objJob)->tail)->next  = attrJob;\
            (objJob)->tail          = attrJob;\
        }\
        else\
        {\
            (attrJob)->prev               = (tempJobNode)->prev; \
            (attrJob)->next               = tempJobNode;  \
            if(NULL == (tempJobNode)->prev) \
                (objJob)->head = attrJob; \
            else \
                ((tempJobNode)->prev)->next = attrJob;\
            (tempJobNode)->prev           = attrJob;  \
        }\
    }\
}\
while(0)

/* Macro to add a AttrSet Job to a Object Job.*/
#define CL_COR_TXN_ATTR_SET_JOB_ADD(objJob, attrSetJob) CL_COR_TXN_OBJ_JOB_ADD(objJob, attrSetJob)

/* Job Id manipulation Macros. */
#define CL_COR_TXN_FORM_JOB_ID(objJobId, attrSetJobId) \
                   (((objJobId) << 16) | ((attrSetJobId) & 0x0000FFFF))

#define CL_COR_TXN_JOB_ID_TO_OBJ_JOB_ID_GET(jobId)\
                   ((jobId) >> 16)

#define CL_COR_TXN_JOB_ID_TO_ATTRSET_JOB_ID_GET(jobId)\
                   ((jobId) & 0x0000FFFF)

#define CL_COR_TXN_OBJ_JOB_ID_SET(jobId, objJobId)\
                  jobId = (((jobId) & 0x0000FFFF) | ((objJobId) << 16))

#define CL_COR_TXN_ATTRSET_JOB_ID_SET(jobId, attrSetJobId)\
                   jobId = (((jobId) & 0xFFFF0000) | ((attrSetJobId) & 0x0000FFFF))


/******************************************************************************
 *  Functions
 *****************************************************************************/
 

/**
 *
 *  This function gets the Job Header from container, with MoId as key.
 *  If its not found, a new Job Header is created.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clCorTxnJobHeaderNodeGet(
    CL_IN     ClCorTxnSessionT       *pTxnSession,
    CL_IN     ClCorTxnObjectHandleT   txnObject,
    CL_OUT    ClCorTxnJobHeaderNodeT **pJobHdrNode);


/**
 *
 *  This function gets the Job Header from container, with MoId as key.
 *  If its not found, a new Job Header is created.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clCorBundleJobHeaderNodeGet(
    CL_IN     ClCorBundleInfoPtrT    pBundleInfo,
    CL_IN     ClCorTxnObjectHandleT   txnObject,
    CL_OUT    ClCorTxnJobHeaderNodeT **pJobHdrNode);
/**
 *
 *  This function inserts a job to a transaction.
 *
 *  @returns CL_OK  - Success<br>
 */
ClRcT
clCorTxnObjJobNodeInsert(ClCorTxnJobHeaderNodeT* pJobHdr, ClCorOpsT op, void *jobData);


/**
 *
 *  This function to create a object job table.
 *  The table is indexed with attrPath as key.
 *
 *  @returns CL_OK  - Success<br>
 */

ClRcT clCorTxnObjJobTblCreate(ClCntHandleT *pContHandle);


/*
 *  This function displays all the jobs in the Txn.
 *
 */
void
clCorTxnShow(CL_IN   ClCorTxnSessionT *pTxnSession);

/**
 * Internal Function
 *
 * This method creates new COR-Txn job Header defining
 * all operations on a given COR-Object.
 */
extern ClRcT clCorTxnJobHeaderNodeCreate( 
                  CL_OUT ClCorTxnJobHeaderNodeT  **pJobHdrNode);

/*
 *  This function frees the all the contenst of job-header.
 *  
*/
extern ClRcT clCorTxnJobHeaderNodeDelete(
                  CL_IN ClCorTxnJobHeaderNodeT   *pJobHdrNode);


/**
 * Internal function to allocate memory for new cor txn-job
 */
extern ClRcT clCorTxnJobNodeAllocate(
            CL_IN   ClCorOpsT               op,
            CL_OUT  ClCorTxnObjJobNodeT*   *pNewJob);
                                                                                                             
/**
 * Internal function to remove first available job from current session
 */
extern ClRcT clCorTxnFirstJobNodeRemove(
            CL_IN   ClCorTxnJobHeaderNodeT*   pThis);
                                                                                                             
/**
 * Internal function to free cor-txn job description
 */
extern ClRcT clCorTxnJobNodeFree(
            CL_IN   ClCorTxnObjJobNodeT*      job);

/**
 * Internal function to commit the transaction
 */
extern ClRcT _clCorTxnSessionCommit(
            CL_IN   ClCorTxnSessionT        txnSession);

extern ClRcT clCorTxnSessionDelete(
         CL_IN   ClCorTxnSessionT txnSession);

/* extern ClRcT clCorTxnJobStreamUnpack(
        CL_IN   ClCorTxnJobStreamT*  txnStream); */

extern ClRcT 
clCorTxnJobStreamUnpack(
    CL_IN   ClBufferHandleT    inMsgHdl,
    CL_OUT  ClCorTxnJobHeaderNodeT  **pJobHeaderNode);

/*
extern ClRcT clCorTxnJobStreamPack(
        CL_IN   ClCorTxnJobHeaderNodeT*    pThis,
        CL_OUT  ClCorTxnJobStreamT*  txnStream); */

extern ClRcT
clCorTxnJobStreamPack(
        CL_IN    ClCorTxnJobHeaderNodeT*    pThis, 
        CL_OUT   ClBufferHandleT     msgHdl);


extern ClRcT  clCorTxnJobToJobExtract(
              CL_IN   ClCorTxnJobHeaderNodeT   *pJobHdrNode,
              CL_IN   ClCorTxnObjJobNodeT      *pObjJobNode,
              CL_OUT  ClCorTxnJobHeaderNodeT  **pNewTxnStream);

/*
 *  API to get the SET Parameters, if the operation is SET.
 *  @param: Flag = 0, get the value in Network order.
 *          Flag = 1, get the value in Host order
 *  The pValue returned here, would be in Network Order.
 */
extern ClRcT _clCorTxnJobSetParamsGet(
          CL_IN   ClUint8T             flag,
          CL_IN   ClCorTxnIdT          txnId,
          CL_IN   ClCorTxnJobIdT       jobId,
          CL_OUT  ClCorAttrIdT        *pAttrId,
          CL_OUT  ClInt32T            *pIndex,
          CL_OUT  void               **pValue,
          CL_OUT  ClUint32T           *pSize);


/* Container to store the failed job information */
extern ClRcT clCorTxnInfoStoreContainerCreate();

extern ClRcT clCorTxnInfoStoreContainerFinalize();

/* Container to store the txn-id name to txn-id value mapping */

extern ClRcT clCorCliTxnIdMapCntCreate();
extern ClRcT clCorCliTxnIdMapCntFinalize();

extern ClRcT clCorCliTxnFailedJobLlistCreate();
extern ClRcT clCorCliTxnFailedJobLlistFinalize();

/* To remove the failed job information stored in the container */
extern ClRcT clCorTxnFailedJobCleanUp(ClCorTxnSessionIdT txnSession);

/**
 * Apply the session. 
 */

extern ClRcT
_clCorBundleStart ( CL_OUT ClCorBundleHandleT sessionHandle, 
                           CL_IN ClCorBundleCallbackPtrT fptr,
                           CL_IN ClPtrT userArg);

/**
 * Initialize the session.
 */
extern ClRcT
_clCorBundleInitialize ( CL_OUT ClCorBundleHandlePtrT pBundleHandle,
                                 CL_IN  ClCorBundleConfigPtrT pBundleConfig);


/**
 *
 */
extern ClRcT
_clCorBundleFinalize(CL_OUT ClCorBundleHandleT sessionHandle );


/**
 * Function to get the session info structure corresponding to this session.
 */
extern ClRcT     
clCorBundleHandleGet( CL_IN  ClHandleT dBhandle,
                         CL_OUT ClCorBundleInfoPtrT *pBundleInfo);

/**
 * Update the data for this handle.
 */
extern ClRcT     
clCorBundleHandleUpdate( CL_IN ClHandleT dbHandle,
                        CL_IN ClCorBundleInfoPtrT pBundleInfo);
/**
 * Function for locating the job in the job table.
 */

extern ClRcT clCorTxnObjJobNodeGet(
               ClCorTxnJobHeaderNodeT    *pJobHdrNode,
               ClCorAttrPathT        *pAttrPath,
               ClCorTxnObjJobNodeT      **ppObjJobNode);

/**
 * Function for adding the job in the job table.
 */
extern ClRcT clCorTxnObjJobNodeAdd(
               ClCorTxnJobHeaderNodeT *pJobHdrNode,
               ClCorTxnObjJobNodeT    *pObjJobNode);

#ifdef __cplusplus
}
#endif


#endif
