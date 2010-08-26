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
 * ModuleName  : med
 * File        : clMedDs.h
 *******************************************************************************/

#ifndef _MED_DS_H_
#define _MED_DS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES */
#include <clCorTxnApi.h>
#include "clMedApi.h"
#include "clEoApi.h"
#define CL_MED_NOTIFY_CALBACK  (1)
#define CL_MED_INSTANCE_NTFY_CALBACK  (5)

#define CL_MED_RMD_PARAM_COUNT                    1
#define CL_MED_RMD_PAYLOAD_PARAM_COUNT            1
#define CL_MED_RMD_PAYLOAD_RET_PARAM_COUNT        1
#define CL_MED_DFLT_TIMEOUT                       TIMEOUT_FOREVER
#define CL_MED_MOID_BUCKET_SIZE                   10

extern ClEventInitHandleT gMedEevtHandle;
extern ClEventChannelHandleT gCorEvtChannelHandle;
extern ClEventChannelHandleT gNotificationEvtChannelHandle;

/* Structure representing a mediation object */
typedef struct clMedObj
{

    ClCntHandleT   idXlationTbl;       /* Identifier's translation */ 
    ClCntHandleT   opCodeXlationTbl;   /* Opcode translation */
    ClCntHandleT   errIdXlationTbl;    /* Error code translation */
    ClCntHandleT   watchAttrList;      /* Watch attributes */
    ClCntHandleT   moIdStore;          /* List of all moIds */


    ClMedNotifyHandlerCallbackT             fpNotifyHdler;      /* Notify handler */
    ClMedInstXlatorCallbackT             fpInstXlator;       /* Instance translator */
    ClCntKeyCompareCallbackT  fpInstCompare;      /* Instance Compare routine */


    ClUint32T             traceEnable;        /* Tracing retuired??*/
    ClOsalMutexIdT mmSem;              /* Semaphor */

}ClMedObjT;

/* Translation of native identifier to target identifier */
typedef struct
{
    ClMedAgentIdT           nativeId;      /* Native Identifier */
    ClMedTgtIdT            *tgtId;        /* Target id list */
    ClUint32T            tgtIdCount;    /* Number of target identifiers */
    ClMedObjT              *pMed;         /* Reference to Mediatopn Object*/
    ClCorNotifyHandleT      subHandle;    /* COR Subscription handle */
    ClCntHandleT  moIdTbl;/* Instance Translation table */
    ClCntHandleT  instXlationTbl;/* Instance Translation table */
}ClMedIdXlationT;

/* Structure to represent instance translation */
typedef struct 
{
    void*       pAgntInst;   /* Instance in Agent language */
    ClUint32T   instLen;     /* Instance length */
    ClCorMOIdT  moId;        /* MO Id handle */
    ClInt32T   attrId[10];
} ClMedInstXlationT;

/* Target error info*/
typedef struct
{
    ClUint32T     type; /* Native error type. CLOVIS/NON_CLOVIS */
    ClUint32T     id;   /* Target Error Id*/
}ClMedTgtErrInfoT;

/* Translation of target Error code to native error code */
typedef struct
{
    ClMedTgtErrInfoT     tgtError;        /* Target Error */
    ClUint32T       nativeErrorId;   /* Native error Id*/
}ClMedErrIdXlationT;


/* Structure to represent operation code translation */
typedef struct
{ 
    ClMedAgntOpCodeT     nativeOpCode;    /* Operation Id in native form*/
    ClMedTgtOpCodeT      *tgtOpCode;   /* Operation Id(s) in target form*/
    ClUint32T          tgtOpCount;   /* Number of target opCodes */
} ClMedOpCodeXlationT;

/* Structure to represent notify subscriptions */
typedef struct
{
    ClMedTgtIdT             tgtId;        /* Target id list */
    ClMedAgentIdT            *pNativeId;   /* Native Identifier */
    ClCntHandleT   sessionList;  /* Session List */ 
    ClMedObjT               *pMed;        /* Reference to Mediatopn Object*/
    ClCorNotifyHandleT      subHandle;    /* COR Subscription handle */
}ClMedWatchAttrT;

typedef struct
{
    ClUint32T     attrId;
    ClUint32T     size;
    ClUint8T      value[1];
}ClMedCorJobInfoT;

/* MOID Store element
 *   All the MOIds of all the identifier translations are stored here
 */
typedef struct
{
    ClCorMOIdT                moId;        /* MOID*/
    ClUint32T            refCount;    /* Ref count */
}ClMedMOIdInfoT;


typedef struct
{
    ClEventHandleT eventHandle;
    ClSizeT        eventDataSize;
}ClMedAttrPathInfoT;
/***********************************************************/ 
/*              FUNCTION PROTOTYPES                        */
/***********************************************************/ 

/* Destructor for identifier translation object*/
void clMedIdXlationFree(ClMedIdXlationT*);

/* Routine to unsubscribe for change notifications
   on an attribute per a session*/
ClRcT clMedWatchStop(ClMedObjT*, ClMedTgtIdT *,  ClUint32T);

/* Utility routine to actually delete watch */
ClRcT clMedWatchSessionDel(ClMedObjT     *pMm,
                          ClMedWatchAttrT  *pWatchAttr,
                          ClUint32T    sessionId);


/* Routine to compare two session ids.*/
int clMedSessionCompare(ClCntKeyHandleT ,ClCntKeyHandleT);

/* Routine to compare two Opcodes */
int clMedOpCodeCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);

/* Routine to compare two attribute under watch */
int clMedWatchIdCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);

/* Routine to compare two Error Ids */
int clMedErrorIdCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);

/* Routine to compare two Attribute Ids */
int clMedAttrIdCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);


/*   Session free  */
ClRcT clMedSessionRelease(ClCntKeyHandleT    key,
                        ClCntDataHandleT   pSession,
                        void                  *dummy,
                        ClUint32T dataLength);

/* Routine to delete all Op code translation*/
ClRcT  clMedAllOpCodeXlationDel(ClMedObjT  *pMm);

/* Routine to delete all Error Id xlation*/
ClRcT  clMedAllErrIdXlationDel(ClMedObjT  *pMm);

/* Routine to delete all Identifier translation */
ClRcT  clMedAllIdXlationDel(ClMedObjT  *pMm);

/* Delete an opcode translation  */
void  clMedOpCodeXlationFree(ClMedOpCodeXlationT *pOpCodeXlation);

/* Callback for Containers */
void clMedDummyCallback(ClCntKeyHandleT , ClCntDataHandleT );

/* Routine to free Target ID (ClCorMOId/Sys Id) */
void clMedTgtIdFree(ClMedTgtIdT   *pTgtId);

/* Routine to free Error translation ) */
ClRcT clMedErrXlationRelease(ClCntKeyHandleT     key,
                           ClCntDataHandleT    pErr,
                           void* dummy,
                           ClUint32T dataLength);

/* Routine to free identifier translation ) */
ClRcT  clMedIdXlationRelease(ClCntKeyHandleT     key,
                          ClCntDataHandleT    pIdXlation,
                          void* dummy,
                          ClUint32T dataLength);

/* Routine to free opCode translation ) */
ClRcT  clMedOpCodeXlationRelease(ClCntKeyHandleT     key,
                              ClCntDataHandleT    pIdXlation,
                              void* dummy,
                              ClUint32T dataLength);

/* Routine to notify the changes */
/* ClRcT clMedNotifyHandler(ClEventHandleT eventHandle, ClSizeT eventDataSize); */
ClRcT clMedNotifyHandler(ClCorTxnIdT corTxnId, ClCorTxnJobIdT jobId, void *pCookie);

/* Routine to perform cor operation  */
ClRcT clMedCorOpPerform (ClMedObjT          *pMm,
                        ClMedOpCodeXlationT   *pOpXlation,
                        ClMedOpT           *pOpInfo);
/* Routine to perform non cor operation  */
#if MED_REL2
ClRcT clMedNonCorOpPerform (ClMedObjT          *pMm,
                           ClMedOpCodeXlationT   *pOpXlation,
                           ClMedVarBindT           *pOpInfo);
#endif
/* Routine to perform the agent operation */
ClRcT  clMedOpPerform(ClMedObjT          *pMm,
                     ClMedOpCodeXlationT   *pOpXlation,
                     ClMedOpT           *pOpInfo);

/* Routine to Create a watch attribute */
ClRcT clMedWatchAttrCreate(ClMedObjT     *pMm,
                          ClMedAgentIdT  *pNativeId,
                          ClMedTgtIdT   *pTgtId,
                          ClUint32T    sessionId);

/* Routine to fill in the instance ids*/
ClRcT clMedInstFill(ClMedIdXlationT    *pIdx, 
                   ClMedCorOpEnumT        opCode,
                   void*         pInst,
                   ClCorMOIdPtrT         pMoId,
                   ClCorAttrPathPtrT    pContainedPath); 

/* Routine to listen to obj create delets and add instance translations*/
/* ClRcT clMedCorObjChangeHdlr(ClEventHandleT eventHandle, ClSizeT eventDataSize, ClCorOpsT operation); */
ClRcT clMedCorObjChangeHdlr(ClCorTxnIdT corTxnId, ClCorTxnJobIdT  jobId, void  *pCookie);
/* Routine to free the instance translation*/
void medInstanceXlnDel(ClCntKeyHandleT    key,
                       ClCntDataHandleT   pData);

void clMedMoIdDelete(ClCntKeyHandleT key, ClCntDataHandleT pData);
void clMedAllWatchAttrDel(ClCntKeyHandleT key, ClCntDataHandleT pData);

/* Routine to replace an instance translation */
ClRcT clMedInstXlnReplace(ClMedIdXlationT  *pIdXln,
                     ClCorMOIdPtrT  hMoId,
                     ClCorAttrPathPtrT pAttrPath,
                     ClMedInstXlationOpEnumT  instXlnOp);

/* Routine to Add an instance translation */
ClRcT clMedInstXlnAdd(ClMedIdXlationT  *pIdXln,
                     ClCorMOIdPtrT  hMoId,
                     ClCorAttrPathPtrT pAttrPath,
                     ClMedInstXlationOpEnumT  instXlnOp);

/* Routine to Delete an instance translation */
ClRcT clMedInstXlnDelete(ClMedIdXlationT  *pIdXln, 
                     ClCorMOIdPtrT  hMoId,
                     ClCorAttrPathPtrT pAttrPath,
                     ClMedInstXlationOpEnumT  instXlnOp);

ClRcT clMedInstXlnDataGet(ClMedIdXlationT  *pIdXln,
        ClCorMOIdPtrT  hMoId,
        ClCorAttrPathPtrT pAttrPath,
        ClMedInstXlationOpEnumT  instXlnOp,
        ClMedInstXlationT   **pInstXln);

ClRcT clMedCorIndexValueChangeHdlr(ClCorTxnIdT corTxnId, void* pCookie);

/* Cor Obj Walk handler */
ClRcT clMedCorObjWalkHdler(void* pData, void *objCookie);


ClRcT clMedCorConaintedAttrWalkHdler(ClCorAttrPathPtrT pAttrPath, ClCorAttrIdT attrId, ClCorAttrTypeT attrType,
              ClCorTypeT attrDataType, void *value, ClUint32T size, ClCorAttrFlagT attrFlag, void *cookie);
    
/* Routine to walk all the precreated objects for notifying the client */
ClRcT clMedPreCreatedObjNtfy(ClMedObjT   *pMm, ClMedIdXlationT  *pIdXln);

ClRcT clMedTransJobWalk(ClCorTxnIdT    corTxnId,
                       ClCorTxnJobIdT  jobId,
                       void           *cookie);

/* Utility routine to Compate the transaction formalities */
ClRcT  clMedTxnFinish(ClCorTxnIdT  hTxnId);

int  clMedMoIdCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);

void clMedNotificationHandler( ClEventSubscriptionIdT subscriptionId, ClEventHandleT eventHandle, ClSizeT eventDataSize );

ClRcT clMedSetOpTxnJobWalk(ClCorTxnIdT       corTxnId,
                            ClCorTxnJobIdT   jobId,
                            void            *pCookie);

ClRcT clMedContainmentAttrNotify(ClCorTxnIdT corTxnId, ClCorTxnJobIdT jobId, void *pCookie);

ClRcT clMedEventSubscribe(ClMedIdXlationT *pIdXlation, ClCorAttrPathPtrT pAttrPath, ClCorAttrIdT     corAttrId);

void medErrIdXlate(ClMedObjT      *pMm,
    ClUint32T    nativeId,
    ClMedVarBindT  *pVarBind);

#define CL_MED_LIB_NAME  "libClMedClient"

#ifdef CL_DEBUG
#define clMedprintf(x) printf x
#endif


#ifdef __cplusplus
}
#endif
#endif	/* _MED_DS_H_*/
