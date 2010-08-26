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
 * ModuleName  : prov
 * File        : clProvMainIpi.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *          This header file contains Provision Internal defenations and function prototypes.
 *
 *
 *****************************************************************************/

#ifndef _CL_PROV_MAIN_IPI_H_
#define _CL_PROV_MAIN_IPI_H_

#include <clCorTxnApi.h>
#include <clTxnAgentApi.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern ClCntHandleT    gClProvOmHandleInfo;
extern ClOsalMutexIdT  gClProvMutex;

typedef enum ClProvTxnState
{
        CL_PROV_VALIDATE_STATE,
        CL_PROV_UPDATE_STATE,
        CL_PROV_ROLLBACK_STATE,
        CL_PROV_READ_STATE,
}ClProvTxnStateT;

typedef struct ClProvObjAttrArgs
{
    ClCorMOIdPtrT pMoId;
    ClProvTxnDataT *pProvTxnData;
    ClUint32T txnDataEntries;
}ClProvObjAttrArgsT;

/* Macros for the corresponding log messages */    
#define  CL_PROV_LOG_1_MOID_GET             gProvLogMsgDb[0]
#define  CL_PROV_LOG_1_RULE_SET             gProvLogMsgDb[1]
#define  CL_PROV_LOG_1_MOID_ALLOC           gProvLogMsgDb[2]
#define  CL_PROV_LOG_1_MOID_INIT            gProvLogMsgDb[3]
#define  CL_PROV_LOG_1_MO_CLASS_GET         gProvLogMsgDb[4] 
#define  CL_PROV_LOG_1_OM_OBJ_CREATE        gProvLogMsgDb[5]
#define  CL_PROV_LOG_1_CONTAINER_ADD        gProvLogMsgDb[6]
#define  CL_PROV_LOG_1_MAP_MOID_OM          gProvLogMsgDb[7] 
#define  CL_PROV_LOG_1_OM_HANDLE_GET        gProvLogMsgDb[8] 
#define  CL_PROV_LOG_1_MOID_OM_GET          gProvLogMsgDb[9] 
#define  CL_PROV_LOG_1_SET_VALIDATE         gProvLogMsgDb[10] 
#define  CL_PROV_LOG_1_SET_UPDATE           gProvLogMsgDb[11] 
#define  CL_PROV_LOG_1_SET_PREVALIDATE      gProvLogMsgDb[12] 
#define  CL_PROV_LOG_1_MOID_GET_OBJ_HANDLE  gProvLogMsgDb[13] 
#define  CL_PROV_LOG_1_OM_PREPARE           gProvLogMsgDb[14] 
#define  CL_PROV_LOG_1_CREATING_CONTAINER   gProvLogMsgDb[15]     
#define  CL_PROV_LOG_1_GET_OM_CLASS         gProvLogMsgDb[16] 
#define  CL_PROV_LOG_1_GETING_EO_OBJECT     gProvLogMsgDb[17]     
#define  CL_PROV_LOG_1_CREATING_OBJECT      gProvLogMsgDb[18]     
#define  CL_PROV_LOG_1_SERVICE_SET          gProvLogMsgDb[19] 
#define  CL_PROV_LOG_1_PROV_FINALIZE        gProvLogMsgDb[20] 
#define  CL_PROV_LOG_1_NODE_MOID_GET_FROM_LOGICAL_SLOT       gProvLogMsgDb[21]
#define  CL_PROV_LOG_1_PORT_GET              gProvLogMsgDb[22]         
#define  CL_PROV_LOG_1_COMP_HANDLE_GET       gProvLogMsgDb[23] 
#define  CL_PROV_LOG_1_COMP_NAME_GET         gProvLogMsgDb[24] 
#define  CL_PROV_LOG_1_RESOURCE_INFO_GET     gProvLogMsgDb[25] 
#define  CL_PROV_LOG_1_NODE_MOID_GET         gProvLogMsgDb[26] 
#define  CL_PROV_LOG_1_NODE_MOID_STRING_MOID gProvLogMsgDb[27] 
#define  CL_PROV_LOG_1_PROV_SERVICE_SET      gProvLogMsgDb[28] 
#define  CL_PROV_LOG_1_PROV_OM_INIT          gProvLogMsgDb[29] 
#define  CL_PROV_LOG_1_MOID_STRING_MOID      gProvLogMsgDb[30]     
#define  CL_PROV_LOG_0_PROV_INIT_DONE        gProvLogMsgDb[31]     
#define  CL_PROV_LOG_0_PROV_CLEAN_DONE       gProvLogMsgDb[32] 
#define  CL_PROV_LOG_1_CREATE_UPDATE         gProvLogMsgDb[33] 
#define  CL_PROV_LOG_1_OM_OBJ_REF_GET        gProvLogMsgDb[34] 
#define  CL_PROV_LOG_1_ATTR_INFO_GET         gProvLogMsgDb[35] 
#define  CL_PROV_LOG_1_CREATE_ROLLBACK       gProvLogMsgDb[36] 
#define  CL_PROV_LOG_1_SET_ROLLBACK          gProvLogMsgDb[37] 
#define  CL_PROV_LOG_1_CREATE_VALIDATE       gProvLogMsgDb[38] 
#define  CL_PROV_LOG_1_DELETE_VALIDATE       gProvLogMsgDb[39] 
#define  CL_PROV_LOG_1_DELETE_ROLLBACK       gProvLogMsgDb[40] 
#define  CL_PROV_LOG_1_DELETE_UPDATE         gProvLogMsgDb[41] 

#define CL_PROV_LIB_NAME "PROVISION"

#define CL_PROV_CHECK_RC_UPDATE(rc, expectedRc, corTxnId, jobId)\
do{\
    if (rc != expectedRc)\
    {\
        clCorTxnJobStatusSet(corTxnId, jobId, rc);\
    }\
}while(0);

#define CL_PROV_CHECK_RC0_LOG1(rc,expectedRc,logSev,logMsg,arg)\
do{\
    if(rc != expectedRc)\
    {\
        CL_FUNC_EXIT();\
        clLogError("PRV", CL_LOG_CONTEXT_UNSPECIFIED, (ClCharT*)logMsg, (arg));\
        return;\
   }\
}while(0);

#define CL_PROV_CHECK_RC1_LOG1(rc,expectedRc,retVal,logSev,logMsg,arg)\
do{\
    if(rc != expectedRc)\
    {\
        CL_FUNC_EXIT();\
        clLogError("PRV", CL_LOG_CONTEXT_UNSPECIFIED, (ClCharT*)logMsg,(arg));\
        return retVal;\
   }\
}while(0);

#define CL_PROV_CHECK_RC1_LOG2(rc,expectedRc,retVal,logSev,logMsg,arg, pProvData)\
do{\
    if(rc != expectedRc)\
    {\
        CL_FUNC_EXIT();\
        clLogError("PRV", CL_LOG_CONTEXT_UNSPECIFIED, (ClCharT*)logMsg,(arg));\
   }\
}while(0);


#define CL_PROV_CHECK_RC1_EAQUAL_LOG0(rc,expectedRc,retVal,logSev,logMsg)\
do{\
    if(rc == expectedRc)\
    {\
        CL_FUNC_EXIT();\
        clLogError("PRV", CL_LOG_CONTEXT_UNSPECIFIED, (ClCharT*)logMsg);\
        return retVal;\
   }\
}while(0);

ClRcT   clProvObjAttrWalk( ClCorAttrPathPtrT pAttrPath,
                           ClCorAttrIdT attrId,
                           ClCorAttrTypeT attrType,
                           ClCorTypeT attrDataType,
                           void* value,
                           ClUint32T size,
                           ClCorAttrFlagT attrFlag,
                           void* cookie );

void clProvObjAttrFree(void *cookie);

ClRcT   clProvAttrWalk( ClCorAttrPathPtrT pAttrPath,
                        ClCorAttrIdT attrId,
                        ClCorAttrTypeT attrType,
                        ClCorTypeT attrDataType,
                        void* value,
                        ClUint32T size,
                        ClCorAttrFlagT attrFlag,
                        void* cookie );

ClRcT   clCorStringToMoIdXlate( ClNameT* pStringMoId,
                                ClCorMOIdPtrT  moIdh );
    
ClRcT   clProvTxnJobWalk( ClCorTxnIdT     corTxnId,
                          ClCorTxnJobIdT  jobId,
                          void           *arg );

ClRcT   clProvTxnListJobWalk( ClCorTxnIdT     corTxnId,
                              ClCorTxnJobIdT  jobId,
                              void           *arg );


/**
 * Transaction Agent callback for prepare/validate
 */
ClRcT   clProvTxnValidate( CL_IN       ClTxnTransactionHandleT txnHandle,
                           CL_IN       ClTxnJobDefnHandleT     jobDefn,
                           CL_IN       ClUint32T               jobDefnSize,
                           CL_IN       ClCorTxnIdT             corTxnId);

/**
 * Transaction agent callback for commit/update
 */
ClRcT   clProvTxnUpdate( CL_IN       ClTxnTransactionHandleT txnHandle,
                         CL_IN       ClTxnJobDefnHandleT     jobDefn,
                         CL_IN       ClUint32T               jobDefnSize,
                         CL_IN       ClCorTxnIdT             corTxnId);

/**
 * Transaction Agent callback for rollback 
 */
ClRcT   clProvTxnRollback( CL_IN       ClTxnTransactionHandleT txnHandle,
                           CL_IN       ClTxnJobDefnHandleT     jobDefn,
                           CL_IN       ClUint32T               jobDefnSize,
                           CL_IN       ClCorTxnIdT             corTxnId);
/**
 * Transaction Agent callback for read operation 
 */
ClRcT   clProvReadSession( CL_IN       ClTxnTransactionHandleT txnHandle,
                           CL_IN       ClTxnJobDefnHandleT     jobDefn,
                           CL_IN       ClUint32T               jobDefnSize,
                           CL_IN       ClCorTxnIdT             corSessionId);


/**
 * Transaction Agent callback for transaction start.
 */
ClRcT clProvTxnStart(
        CL_IN       ClTxnTransactionHandleT txnHandle,
        CL_INOUT    ClTxnAgentCookieT*      pCookie);

/**
 * Transaction Agent callback for transaction end. 
 */
ClRcT clProvTxnEnd(
        CL_IN       ClTxnTransactionHandleT txnHandle,
        CL_INOUT    ClTxnAgentCookieT*      pCookie);

void    clProvArrayAttrInfoGet( ClCorAttrDefT* pAttrDef,
                                ClUint32T* pDataLen );

void    clProvTxnDeregister();
ClRcT   clProvIfCreateFlagIsTrue( ClCorMOIdPtrT pFullMoId,
                                  ClCorAddrT* pProvAddr ,
                                  ClUint32T   readOIFlag,
                                  ClBoolT     autoCreateFlag);
ClRcT   clProvIfCreateFlagIsFalse( ClCorMOIdPtrT pFullMoId,
                                   ClCorAddrT* pProvAddr, 
                                   ClUint32T   readOIFlag);
ClRcT   clProvOmClassDelete();
ClRcT   clCpmComponentHandle( ClCpmHandleT* pCpmHandle );
ClRcT   clCorNodeAddressToMoIdGet( ClIocNodeAddressT iocNodeAddress,
                                   ClCorMOIdPtrT moId );
ClRcT   clCorMoIdToStringXlate( ClCorMOIdPtrT moIdh,
                                ClNameT* pStringMoId );
ClRcT   clCorStringToMoIdXlate( ClNameT* pStringMoId,
                                ClCorMOIdPtrT  moIdh );
ClRcT   clProvRuleAdd( ClCorMOIdPtrT fullMoId,
                       ClCorAddrT* pProvAddr );
ClRcT   clProvOmClassCreate();
ClRcT   clProvProvisionOmCreateAndRuleAdd( ClNameT* pResourceStringMoId,
                                           ClCorMOIdPtrT pFullMoId,
                                           ClCorAddrT* pProvAddr,
                                           ClUint32T createFlag,
                                           ClBoolT   autoCreateFlag,
                                           ClUint32T readFlag,
                                           ClUint32T wildCardFlag );
ClRcT   clProvOMClassConstructor( void* pThis,
                                void* pUsrData,
                                ClUint32T usrDataLen );
ClRcT   clProvOMClassDestructor( void* pThis,
                               void* pUsrData,
                               ClUint32T usrDataLen );
ClRcT   clProvOmObjectPrepare( ClCorMOIdPtrT pFullMoId );

ClRcT clProvOmObjectCreate(ClCorTxnIdT corTxnId, void** ppOmObj);
ClRcT clProvOmObjectDelete(ClCorTxnIdT corTxnId);


/** 
 *  Function to do un-register for all the route entries during library finalize
 *  for which the registration was done during initialization phase.
 */
ClRcT _clProvOIUnregister();

typedef struct ClProvAttWalkCookie
{
    ClCorObjectHandleT  corObjHandle;
    ClCorMOIdPtrT       moId;
}ClProvAttWalkCookieT;

/**
 * Structure to store the transaction job walk specific data.
 */
struct ClProvTxnJobWalkData
{
    ClHandleT handle;
    ClProvTxnStateT op;
};

typedef struct ClProvTxnJobWalkData ClProvTxnJobWalkDataT;
typedef ClProvTxnJobWalkDataT * ClProvTxnJobWalkDataPtrT;

struct ClProvTxnListJobWalkData
{
    ClProvTxnJobWalkDataT txnJobData;
    ClProvTxnDataT *pTxnDataList;
    ClUint32T txnDataEntries;
};

typedef struct ClProvTxnListJobWalkData ClProvTxnListJobWalkDataT;
typedef ClProvTxnListJobWalkDataT * ClProvTxnListJobWalkDataPtrT;

#ifdef __cplusplus
}
#endif

#endif /* _CL_PROV_MAIN_IPI_H_ */

