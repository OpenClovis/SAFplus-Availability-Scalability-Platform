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
 * ModuleName  : alarm                                                         
 * File        : clAlarmClient.h
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 * 
 * This module contains alarm Service related APIs
 *
 *****************************************************************************/

#ifndef _CL_ALARM_CLIENT_H_
#define _CL_ALARM_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <clCommon.h>
#include <clAlarmDefinitions.h>
#include <clEventApi.h>
#include <clCorTxnApi.h>
#include <clTxnAgentApi.h>
#include <clCpmApi.h>
#include <clOampRtApi.h>
#include <clAlarmContainer.h>
#include <clCorNotifyApi.h>
#include <xdrClCorAlarmConfiguredDataIDLT.h>
#include <xdrClCorAlarmResourceDataIDLT.h>
#include <xdrClCorAlarmProfileDataIDLT.h>

#include <clAlarmIpi.h>

/****************************************************************************
 * Constants 
 ***************************************************************************/ 
#define CL_MAX_APP_OM_CLASSES 1
#define CL_ALARM_COR_CLASS_NAME_MAXM_LEN    256
#define CL_ALARM_INSTANCE_MAX_LEN    256
#define ALARM_CLIENT_CALLBACK_TIME 100
    
#define CL_ALARM_MAX_ALARM_RULE_IDS 4 /*Max number of alarms participating in
                                        alarm rule*/

#define ALARM_TXN 0

/*************************************************************************/

/** 
 * Assigning values to the alarm attributes which
 * are a part of alarm profile.
 */ 

typedef enum
{
    CL_ALARM_PROFILE_ATTR_START =CL_ALARM_COR_ATTR_END,
    
    CL_ALARM_FETCH_MODE,
    CL_ALARM_ASSERT_SOAKING_TIME,
    CL_ALARM_CLEAR_SOAKING_TIME,

    CL_ALARM_PROFILE_ATTR_END
    
}ClAlarmProfileAttrIdT;

/*************************************************************************/

extern ClUint8T
clAlarmClientEngineAlarmConditionCheck(
    ClCorMOIdPtrT,
    ClCorObjectHandleT hCorHdl,  
    ClUint32T idx,
    ClUint32T lockApplied);

extern ClRcT
clAlarmClientEngineAlarmProcess(
    ClCorMOIdPtrT,
    ClCorObjectHandleT, 
    ClAlarmInfoT*,
    ClAlarmHandleT*,
    ClUint32T);


extern void
clAlarmClientEngineAffectedAlarmsProcess(
    ClCorMOIdPtrT,
    ClCorObjectHandleT hMSOObj, 
    ClUint32T idx,
    ClUint8T isAssert,
    ClUint32T lockApplied,
    ClAlarmHandleT*);


extern ClRcT 
clAlarmClientEngineProfAttrValueGet(
    ClCorMOIdPtrT pMoId, 
    ClCorObjectHandleT hMSOObj, 
    ClUint32T attrId, 
    ClUint32T idx, 
    void* value, 
    ClUint32T* size);


extern ClUint32T
clAlarmClientEngineTimeDiffCalc(
    struct timeval * tm1, 
    struct timeval *tm2);

/*****************************************************************************
 *  Functions
 *****************************************************************************/


/* forward declaration of functions related to MSO & OM */


ClRcT clAlarmClientAlarmOMCreate(
    ClCorMOIdPtrT hMoId);


ClRcT clAlarmClientMOChangeNotifSubscribe(
    ClCorMOIdListPtrT pMoId);


ClRcT clAlarmClientAlarmMsoConfigure(
    ClCorMOIdPtrT hMoId);


ClRcT clAlarmClientSetAffectedAlarm(
    ClCorMOIdPtrT pMoId,
    ClCorObjectHandleT  objH, 
    ClUint32T           index);

ClRcT   
_clAlmBundleAttrListGet(ClCorMOIdPtrT pMoId,
                        ClCorObjectHandleT objHandle, 
                        ClUint32T attrId, 
                        ClUint32T size, 
                        ClCorAttrValueDescriptorListT *pAttrList);

/* forward declaration of functions called at library initialization time */

ClRcT clAlarmLeakyBucketInitialize(void);

ClRcT clAlarmClientNativeClientTableInit(
    ClEoExecutionObjT * eoObject);

ClRcT clAlarmClientNativeClientTableFinalize(
        ClEoExecutionObjT* eoObj);

ClRcT clAlarmClientBaseOmClassInit();


ClRcT clAlarmClientTimerInit();


ClRcT clAlarmClientCorSubscribe(ClCorMOIdListPtrT pMoIdList);


ClRcT clAlarmChannelOpen();


ClRcT clAlarmClientCorNodeAddressToMoIdGet(
    ClCorMOIdPtrT     moId);


ClRcT clAlarmClientResListCreate();


ClInt32T clAlarmResCompare(
    ClCntKeyHandleT key1, 
    ClCntKeyHandleT key2);


void clAlarmResEntryDeleteCallback(
    ClCntKeyHandleT key, 
    ClCntDataHandleT userData);

void clAlarmResEntryDestroyCallback(
    ClCntKeyHandleT key, 
    ClCntDataHandleT userData);


ClRcT clAlarmClientResTableProcess(ClEoExecutionObjT* pEoObj);


ClRcT clAlarmClientResourceTableInfoGet(
    ClCorAddrT* pProvAddr, 
    ClOampRtResourceArrayT* pResourcesArray); 

ClRcT clAlarmClientResListWalk(ClCntKeyHandleT     key,
                   ClCntDataHandleT    pData,
                   void                *dummy,
                   ClUint32T           dataLength);

#if 0
ClRcT clAlarmNodeMoIdGet(
    ClNameT* pNodeSrtingMoId);
#endif

ClRcT clAlarmClientAlarmObjectCreate(
    ClNameT moIdName,
    ClCorMOIdPtrT pFullMoId, 
    ClCorAddrT* pProvAddr, 
    ClUint32T createFlag, 
    ClBoolT autoCreateFlag,
    ClUint32T wildCardFlag);

ClRcT
_clAlarmClientPendingAlmsForMOGet ( ClCorMOIdPtrT const pMoId, 
                                    ClBufferHandleT pendingAlmList);

ClRcT   
_clAlarmAllPendingAlmGet(ClAlarmPendingAlmListPtrT const pPendingAlmList);

ClRcT
_clAlarmGetPendingAlarmFromOwner(ClCorMOIdPtrT const pMoId, 
                                 ClAlarmPendingAlmListPtrT const pPendingAlmList);
/****************************************************************************
 * forward declaration of functions belonging to function list table
 ***************************************************************************/

ClRcT VDECL(clAlarmSvcLibDebugCliAlarmProcess) (ClEoDataT data, 
                                         ClBufferHandleT  inMsgHandle,
                                         ClBufferHandleT  outMsgHandle);

ClRcT VDECL(clAlarmStateGet) (ClEoDataT data, 
                       ClBufferHandleT  inMsgHandle,
                       ClBufferHandleT  outMsgHandle);

ClRcT VDECL(clAlarmPendingAlarmsGet) (ClEoDataT data, 
                               ClBufferHandleT  inMsgHandle,
                               ClBufferHandleT  outMsgHandle);

/****************************************************************************/

/* forward declaration of event callback function part of cor subscription */

ClRcT clAlarmBootUpAlarmMsoProcess(
    ClCorMOIdPtrT       pMoId);

void  clAlarmClientNotificationHandler( 
    ClEventSubscriptionIdT subscriptionId, 
    ClEventHandleT eventHandle, 
    ClSizeT eventDataSize );

/* forward declaration of notification handling functions */

ClRcT clAlarmClientCreateNotifProcess(
    ClCorTxnIdT corTxnId, 
    ClCorTxnJobIdT jobId);


ClRcT clAlarmClientDeleteNotifProcess(
    ClCorTxnIdT corTxnId, 
    ClCorTxnJobIdT jobId);


ClRcT clAlarmSetJobProcess(
    ClCorTxnIdT  corTxnId,
    ClCorTxnJobIdT     jobId);

/* forward declaration of functions called periodically */

ClRcT _clAlarmClientTimerCallback(ClPtrT arg);

ClRcT clAlarmClientPeriodicCallBack(
    void *arg);


void clAlarmClientUpdateSoakingTime(
    ClCorMOIdPtrT pMoId,
    ClCorObjectHandleT hMSOObj);



/* forward declaration of Cor related Apis */

ClRcT clAlarmWildCardMoIdObjWalk(
    void*  pData, void *cookie);


#if ALARM_TXN
ClRcT clAlarmClientTxnValidate(
    CL_IN ClTxnTransactionHandleT txnHandle,
    CL_IN ClTxnJobDefnHandleT     jobDefn, 
    CL_IN ClUint32T               jobDefnSize,
    CL_INOUT ClTxnAgentCookieT       *pCookie);


ClRcT clAlarmClientTxnUpdate(
    CL_IN ClTxnTransactionHandleT txnHandle,
    CL_IN ClTxnJobDefnHandleT     jobDefn, 
    CL_IN ClUint32T               jobDefnSize,
    CL_INOUT ClTxnAgentCookieT       *pCookie);


ClRcT clAlarmClientTxnRollback(
    CL_IN       ClTxnTransactionHandleT txnHandle,
    CL_IN       ClTxnJobDefnHandleT     jobDefn,
    CL_IN       ClUint32T               jobDefnSize,
    CL_INOUT    ClTxnAgentCookieT       *pCookie);


ClRcT clAlarmClientTxnJobWalk(ClCorTxnIdT trans,
                    ClCorTxnJobIdT jobId,
                            void          *arg);
#endif

/* externs */
extern ClOmClassControlBlockT omAlarmClassTbl[];
extern ClOmClassControlBlockT omAppAlarmClassTbl[CL_MAX_APP_OM_CLASSES];


/* Functions used for Alarm MSO configuration */
ClRcT clAlarmClientConfigDataGet(VDECL_VER(ClCorAlarmConfiguredDataIDLT, 4, 1, 0)* pAlarmData);
ClRcT clAlarmClientConfigDataFree(VDECL_VER(ClCorAlarmConfiguredDataIDLT, 4, 1, 0)* pAlarmData);
ClRcT clAlarmClientConfigMOAlarmsGet(ClAlarmComponentResAlarmsT* pAlarmMO, VDECL_VER(ClCorAlarmResourceDataIDLT, 4,1,0)* pResData);
ClRcT clAlarmClientConfigAlarmsGet(ClAlarmProfileT* pAlarmConfig, VDECL_VER(ClCorAlarmProfileDataIDLT, 4,1,0)* pAlarmData);
ClRcT clAlarmClientConfigAlarmRuleSet(ClAlarmComponentResAlarmsT* pAlarmMO, VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)* pRestData);
ClRcT clAlarmClientConfigTableDelete();
ClRcT clAlarmClientConfigTableCreate();
ClInt32T _clAlarmInfoKeyCompare(ClCntKeyHandleT key1, ClCntKeyHandleT key2);
ClUint32T _clAlarmInfoHashFunc(ClCntKeyHandleT key);
ClRcT clAlarmClientConfigTableClassAdd(ClCorClassTypeT classId, VDECL_VER(ClCorAlarmResourceDataIDLT, 4,1,0)* pResData);
ClRcT clAlarmClientConfigTableResAdd(ClCorClassTypeT classId, ClCorMOIdPtrT pMoId);
ClRcT clAlarmClientConfigTableResDataGet(ClCorClassTypeT classId, VDECL_VER(ClCorAlarmResourceDataIDLT, 4, 1, 0)** pAlarmConfig);

#ifdef ALARM_POLL

ClRcT clAlarmClientPolledAlarmUpdate(
    ClCorMOIdPtrT pMoId, 
    ClCorObjectHandleT objH, 
    ClUint8T bootUpTime);


ClRcT clAlarmClientPollDataProcess(
    ClCorMOIdPtrT pMoId,
    ClCorObjectHandleT objH);


void clAlarmClientAlarmPoll(
    ClCorMOIdPtrT pMoId,
    ClCorObjectHandleT hMSOObj);

ClRcT clAlarmClientResAlarmStausGet(
    ClCntKeyHandleT     key,
    ClCntDataHandleT    pData,
    void                *dummy,
    ClUint32T           dataLength);


ClRcT clAlarmClientBootTimeResAlarmGet();


#endif

#ifdef __cplusplus
}
#endif

#endif  /* _CL_ALARM_CLIENT_H_ */
