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

/**
 * This header file contains definitions of internal data types used
 * by the CPM server.
 */

#ifndef _CL_CPM_INTERNAL_H_
#define _CL_CPM_INTERNAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * ASP include files 
 */
#include <clCommon.h>
#include <clCommonErrors.h>

#include <clCntApi.h>
#include <clOsalApi.h>
#include <clEoApi.h>
#include <clCpmApi.h>
#include <clCpmConfigApi.h>
#include <clCkptApi.h>
#include <clClmApi.h>
#include <clEventApi.h>
#include <clCorMetaData.h>
/*
 * CPM internal header files 
 */
#include <clCpmCommon.h>
#include <clCpmAms.h>
#include <clCpmBoot.h>
#include <clTaskPool.h>
/*
 * XDR header files 
 */
#include "xdrClCpmLocalInfoT.h"

extern ClCharT clCpmNodeName[];
extern ClCharT gAspInstallInfo[];
extern ClCharT clCpmBootProfile[];
extern ClUint32T clCpmDaemonize;
extern ClUint32T clAspLocalId;
extern ClTaskPoolHandleT gCpmFaultPool;

#define ClEoHandleT                      ClUint32T
#define NULL_STRING                      "\0"

#define CPM_LOG_CTX_CPM_AMS              "AMS"    
#define CPM_LOG_CTX_CPM_BOOT             "BOO"    
#define CPM_LOG_CTX_CPM_CKP              "CKP"
#define CPM_LOG_CTX_CPM_CLI              "CLI"
#define CPM_LOG_CTX_CPM_CM               "CHM"    
#define CPM_LOG_CTX_CPM_CPM              "CPM"    
#define CPM_LOG_CTX_CPM_EO               "_EO"    
#define CPM_LOG_CTX_CPM_EVT              "EVT"
#define CPM_LOG_CTX_CPM_GMS              "GMS"
#define CPM_LOG_CTX_CPM_HB               "_HB"    
#define CPM_LOG_CTX_CPM_LCM              "LCM"
#define CPM_LOG_CTX_CPM_TL               "_TL"
#define CPM_LOG_CTX_CPM_MGM              "MGM"

/**
 * Configuration files read by the CPM.
 */
#define CL_IOC_MAP_FILE_NAME            "map.xml"
#define CL_CPM_CONFIG_FILE_NAME         "clAmfConfig.xml"
#define CL_CPM_DEF_FILE_NAME            "clAmfDefinitions.xml"
#define CL_CPM_ASP_DEF_FILE_NAME        "clAspDefinitions.xml"
#define CL_CPM_ASP_INST_FILE_NAME       "clAspInstances.xml"
#define CL_CPM_SLOT_FILE_NAME           "clSlotInfo.xml"
#define CL_CPM_GMS_CONF_FILE_NAME       "clGmsConfig.xml"

#define CL_CPM_COMPONENT_EVENT_NAME     "eventServer"
#define CL_CPM_COMPONENT_COR_NAME       "corServer"
#define CL_CPM_COMPONENT_LOG_NAME       "log"

#define CL_CPM_COMPONENT_DEFAULT_TIMEOUT    10000  /* milli seconds */
#define CL_CPM_COMPONENT_DEFAULT_HC_TIMEOUT 30000
#define CL_CPM_COMPONENT_MIN_HC_TIMEOUT     5000
#define CL_CPM_DFLT_TIMEOUT                 3000
#define CL_CPM_DFLT_RETRIES                 3
#define CL_CPM_DFLT_PRIORITY                CL_IOC_DEFAULT_PRIORITY
#define CL_CPM_DEFAULT_TIMEOUT              5000
#define CL_CPM_DEFAULT_MAX_TIMEOUT          32000
#define CL_CPM_GMS_DEFAULT_TIMEOUT          CL_GMS_DEFAULT_BOOT_ELECTION_TIMEOUT

/*
 * CPM restart flags for ASP and node.
 */
#define CL_CPM_RESTART_NONE 0x0
#define CL_CPM_RESTART_ASP  0x1
#define CL_CPM_RESTART_NODE 0x2
#define CL_CPM_HALT_ASP     0x3

#define CL_CPM_SET_RESTART_OVERRIDE(flag) ( ((flag) & 0xffff) << 16 )
#define CL_CPM_GET_RESTART_OVERRIDE(flag) ( ( (flag) >> 16) & 0xffff )

#define CL_CPM_RESTART_FLAG_STR(flag)   ( (flag) == CL_CPM_RESTART_ASP ? "ASP_RESTART" : \
                                        ( (flag) == CL_CPM_RESTART_NODE ) ? "NODE_RESET" : \
                                        ( (flag) == CL_CPM_HALT_ASP ) ? "NODE_HALT" : "NODE_STOPPED" )
/*
 * Disable watchdog ASP restarts on node failover/failfast with 
 * ASP_NODE_REBOOT_DISABLE set.
 */
#define CL_CPM_RESTART_DISABLE_FILE "safplus_restart_disable"

/*
 * Bug 4690:
 * Added the default values for boot profile, maximum boot level
 * and default boot level.
 */
#define CL_CPM_DEFAULT_BOOT_PROFILE         "OPENCLOVIS_DEFAULT_BOOT_PROFILE"
#define CL_CPM_DEFAULT_MAX_BOOT_LEVEL       5
#define CL_CPM_DEFAULT_DEFAULT_BOOT_LEVEL   5

/*
 * Seconds to wait before trying to start after cleanup/terminate 
 */
#define CL_CPM_RESTART_SLEEP    50   /* milli seconds */

/*
 * Container size
 */
#define CL_CPM_COMPTABLE_BUCKET_SIZE    73
#define CL_CPM_INVOCATION_BUCKET_SIZE   73
#define CL_CPM_CPML_TABLE_BUCKET_SIZE   19

#define CL_CPM_EVENT_CHANNEL_TYPE       0x1
#define CL_CPM_COMPONENT_NAME           "cpmServer"
#define CL_CPM_EO_NAME                  "AMF"

#define CL_CPM_MAX_PING_TRIES           3

#define CL_CPM_TIMEOUT_DELTA            800
#define CL_CPM_EXPBKOFF_FACTOR          2

#define CL_CPM_ASP_COMP_RESTART_ATTEMPT    3
#define CL_CPM_ASP_COMP_RESTART_WINDOW     20

#define CPM_RESPOND_TO_ACTIVE 0xFFFF    

#define CL_CPM_IS_ACTIVE()                                  \
    ((gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL &&      \
      gpClCpm->haState == CL_AMS_HA_STATE_ACTIVE) ? 1 : 0)

#define CL_CPM_IS_STANDBY()                                 \
    ((gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL &&      \
      gpClCpm->haState == CL_AMS_HA_STATE_STANDBY) ? 1 : 0)

#define CL_CPM_IS_SC() ((gpClCpm->pCpmConfig->cpmType == CL_CPM_GLOBAL) ? 1 : 0)
#define CL_CPM_IS_WB() ((gpClCpm->pCpmConfig->cpmType == CL_CPM_LOCAL) ? 1 : 0)

#define CL_CPM_IS_STANDBY_PRESENT() \
    ((gpClCpm->deputyNodeId == -1) ? CL_FALSE : CL_TRUE)

/* 
 * macro for error checking, logging.
 */
#define CL_CPM_CHECK_0(X, Z, retCode, logSeverity, logHandle)   \
    if(CL_GET_ERROR_CODE(retCode) != CL_OK)                     \
    {                                                           \
        CL_DEBUG_PRINT(X, (Z));                                 \
        clLogWrite((logHandle), (logSeverity), NULL, Z);        \
        rc = retCode;                                           \
        goto failure;                                           \
    }
 
#define CL_CPM_CHECK_1(X, Z, argv1, retCode, logSeverity, logHandle)    \
    if(CL_GET_ERROR_CODE(retCode) != CL_OK)                             \
    {                                                                   \
        CL_DEBUG_PRINT(X, (Z, argv1));                                  \
        clLogWrite((logHandle), (logSeverity), NULL, Z, argv1);         \
        rc = retCode;                                                   \
        goto failure;                                                   \
    }

#define CL_CPM_CHECK_2(X, Z, argv1, argv2, retCode, logSeverity, logHandle) \
    if(CL_GET_ERROR_CODE(retCode) != CL_OK)                             \
    {                                                                   \
        CL_DEBUG_PRINT(X, (Z, argv1, argv2));                           \
        clLogWrite((logHandle), (logSeverity), NULL, Z, (argv1), (argv2)); \
        rc = retCode;                                                   \
        goto failure;                                                   \
    }

#define CL_CPM_CHECK_3(X, Z, argv1, argv2, argv3, retCode, logSeverity, logHandle) \
    if(CL_GET_ERROR_CODE(retCode) != CL_OK)                             \
    {                                                                   \
        CL_DEBUG_PRINT(X, (Z, argv1, argv2, argv3));                    \
        clLogWrite(logHandle, logSeverity, NULL, Z, argv1, argv2, argv3); \
        rc = retCode;                                                   \
        goto failure;                                                   \
    }

#define CL_CPM_CHECK(X, Z, retCode)             \
    if(CL_GET_ERROR_CODE(retCode) != CL_OK)     \
    {                                           \
        CL_DEBUG_PRINT(X,Z);                    \
        rc = retCode;                           \
        goto failure;                           \
    }

#define CL_CPM_LOCK_CHECK(X, Z, retCode)        \
    if(retCode != CL_OK)                        \
    {                                           \
     CL_DEBUG_PRINT(X,Z);                       \
    rc = retCode;                               \
    goto withlock;                              \
    }

#define IS_ASP_COMP(comp) ((comp)->compConfig->isAspComp)

typedef enum
{
    /**
     * Not used for now.
     */
    CPM_INITIALIZE_FN_ID = 0,
    /**
     * Component healthcheck callbck.
     */
    CPM_HEALTH_CHECK_FN_ID =
        CL_EO_GET_FULL_FN_NUM(CL_CPM_MGR_CLIENT_TABLE_ID, 1),
    /**
     * Component terminate callback.
     */
    CPM_TERMINATE_FN_ID =
        CL_EO_GET_FULL_FN_NUM(CL_CPM_MGR_CLIENT_TABLE_ID, 2),
    /**
     * CSI set callback.
     */
    CPM_CSI_SET_FN_ID =
        CL_EO_GET_FULL_FN_NUM(CL_CPM_MGR_CLIENT_TABLE_ID, 3),
    /**
     * CSI remove callback
     */
    CPM_CSI_RMV_FN_ID =
        CL_EO_GET_FULL_FN_NUM(CL_CPM_MGR_CLIENT_TABLE_ID, 4),
    /**
     * Protection group callback.
     */
    CPM_PROTECTION_GROUP_TRACK_FN_ID =
        CL_EO_GET_FULL_FN_NUM(CL_CPM_MGR_CLIENT_TABLE_ID, 5),
    /**
     * Proxied component instantiation callback.
     */
    CPM_PROXIED_COMP_INSTANTIATION_FN_ID =
        CL_EO_GET_FULL_FN_NUM(CL_CPM_MGR_CLIENT_TABLE_ID, 6),
    /**
     * Proxied component termination/cleanup callback.
     */
    CPM_PROXIED_CL_CPM_CLEANUP_FN_ID =
        CL_EO_GET_FULL_FN_NUM(CL_CPM_MGR_CLIENT_TABLE_ID, 7),

    /*
     * End of EO Function 
     */
    CPM_LAST_FN_ID
} clCpmFunctionsIdT;
    

typedef struct
{
    ClCpmComponentT *comp;
    ClCpmEOListNodeT *eoHandle;
} CpmheartBeatFailedCompT;


typedef struct clCpmComponentRef
{
    ClCpmComponentT *ref;
    struct clCpmComponentRef *pNext;
} ClCpmComponentRefT;        

/**
 * Structures used while parsing component information from the XML
 * file.
 */
typedef struct clCpmCompInfo ClCpmCompInfoT;
struct clCpmCompInfo
{
    ClCpmCompConfigT compConfig;
    ClCharT compType[CL_MAX_NAME_LENGTH];
    ClCpmCompInfoT *pNext;
};

/**
 * Structure used in LCM operation.
 */
typedef struct
{
    ClCpmComponentT *comp;
    ClCpmCompRequestTypeT requestType;
} ClCpmCompRequestT;

typedef struct
{
    ClInvocationT invocation;
    ClNameT compName;
    ClAmsCompHealthcheckKeyT healthCheckKey;
} ClCpmCompHealthCheckSendT;

typedef struct
{
    ClNameT compName;
    ClAmsPGNotificationBufferT notificationBuffer;
    ClUint32T numberOfMembers;
    ClUint32T error;
} ClCpmCompProtectionGroupTrackT;

typedef struct
{
    ClInvocationT invocation;
    ClNameT proxiedCompName;
} ClCpmCompProxiedCompOperationT;

typedef struct {
    ClCmCpmMsgT cmMsg;
    ClNameT     nodeName;
}ClCpmCmQueuedataT;

extern ClBoolT gClAmsSwitchoverInline;
extern ClBoolT gClAmsPayloadResetDisable;

/**
 * CM.
 */
extern ClRcT cpmParseSlotFile(void);
extern ClRcT cpmSlotClassTypesGet(ClCntHandleT slotTable, 
                                  ClUint32T slotNumber, 
                                  ClCpmSlotClassTypesT *slotClassTypes);
extern ClRcT cpmSlotClassAdd(ClNameT *type, ClNameT *identifier, ClUint32T slot);
extern ClRcT cpmSlotClassDelete(const ClCharT *type);

/**
 * Boot manager.
 */
extern void *cpmBMResponse(ClCpmLcmResponseT *response);

/**
 * Misc
 */
extern ClRcT cpmCompInitialize(ClCpmComponentT **comp);
extern ClRcT cpmCompConfigure(ClCpmCompConfigT *compCfg,
                              ClCpmComponentT **component);
extern ClRcT cpmCompFinalize(ClCpmComponentT *comp);
extern ClRcT cpmGetConfig(void);
extern ClRcT cpmParseIocFile(void);
extern void cpmCompConfigFinalize(void);

extern ClRcT _cpmComponentInstantiate(ClCharT *compName,
                                      ClCharT *proxyCompName,
                                      ClUint64T instantiateCookie,
                                      ClCharT *nodeName,
                                      ClIocPhysicalAddressT *srcAddress,
                                      ClUint32T rmdNumber);

extern ClRcT _cpmComponentTerminate(ClCharT *compName,
                                    ClCharT *proxyCompName,
                                    ClCharT *nodeName,
                                    ClIocPhysicalAddressT *srcAddress,
                                    ClUint32T rmdNumber);

extern ClRcT _cpmComponentCleanup(ClCharT *compName,
                                  ClCharT *proxyCompName,
                                  ClCharT *nodeName,
                                  ClIocPhysicalAddressT *srcAddress,
                                  ClUint32T rmdNumber,
                                  ClUint32T requestType);

extern ClRcT _cpmLocalComponentCleanup(ClCpmComponentT *comp,
                                       ClCharT *compName,
                                       ClCharT *proxyCompName,
                                       ClCharT *nodeName);

extern ClRcT _cpmComponentRestart(ClCharT *compName,
                                  ClCharT *proxyCompName,
                                  ClCharT *nodeName,
                                  ClIocPhysicalAddressT *srcAddress,
                                  ClUint32T rmdNumber);

extern ClRcT cpmCompFind(ClCharT *name,
                         ClCntHandleT compTable,
                         ClCpmComponentT **comp);

extern ClRcT cpmCompFindWithLock(ClCharT *name,
                                 ClCntHandleT compTable,
                                 ClCpmComponentT **comp);

extern ClRcT cpmEventInitialize(void);

extern ClUint32T cpmNodeFindByIocAddress(ClIocNodeAddressT nodeAddress, ClCpmLT **cpmL);

extern void compMgrStateStatusGet(ClUint32T  status,
                                  ClEoStateT state,
                                  ClCharT   *pStatus,
                                  ClUint32T  pStatusSize,
                                  ClCharT   *pState,
                                  ClUint32T  pStateSize);

extern ClCharT *_cpmOperStateNameGet(ClAmsOperStateT operState);
extern ClCharT *_cpmPresenceStateNameGet(ClAmsPresenceStateT presenceState);

extern ClCharT *_cpmReadinessStateNameGet(ClAmsReadinessStateT readinessState);

extern ClRcT cpmRequestFailedResponse(ClCharT *name,
                                      ClCharT *nodeName,
                                      ClIocPhysicalAddressT *srcAddress,
                                      ClUint32T rmdNumber,
                                      ClCpmCompRequestTypeT requestType,
                                      ClRcT retCode);

extern ClRcT clCpmCpmLocalRegister(ClCpmLocalInfoT *cpmInfo);

extern ClRcT clCpmCpmLocalDeregister(ClCpmLocalInfoT *cpmInfo);

extern ClRcT CL_CPM_CALL_RMD_ASYNC(ClIocNodeAddressT destAddr,
                                   ClIocPortT destPort,
                                   ClUint32T fcnId,
                                   ClUint8T *pInBuf,
                                   ClUint32T inBufLen,
                                   ClUint8T *pOutBuf,
                                   ClUint32T *pOutBufLen,
                                   ClUint32T flags,
                                   ClUint32T timeOut,
                                   ClUint32T maxRetries,
                                   ClUint8T priority,
                                   void *cookie,
                                   ClRmdAsyncCallbackT pFuncCB);

extern ClRcT CL_CPM_CALL_RMD_ASYNC_NEW(ClIocNodeAddressT destAddr,
                                       ClIocPortT destPort,
                                       ClUint32T fcnId,
                                       ClUint8T *pInBuf,
                                       ClUint32T inBufLen,
                                       ClUint8T *pOutBuf,
                                       ClUint32T *pOutBufLen,
                                       ClUint32T flags,
                                       ClUint32T timeOut,
                                       ClUint32T maxRetries,
                                       ClUint8T priority,
                                       void *cookie,
                                       ClRmdAsyncCallbackT pFuncCB,
                                       ClCpmMarshallT marshallFunction);

extern ClRcT CL_CPM_CALL_RMD_SYNC(ClIocNodeAddressT destAddr,
                                  ClIocPortT eoPort,
                                  ClUint32T fcnId,
                                  ClUint8T *pInBuf,
                                  ClUint32T inBufLen,
                                  ClUint8T *pOutBuf,
                                  ClUint32T *pOutBufLen,
                                  ClUint32T flags,
                                  ClUint32T timeOut,
                                  ClUint32T maxRetries,
                                  ClUint8T priority);

extern ClRcT CL_CPM_CALL_RMD_SYNC_NEW(ClIocNodeAddressT destAddr,
                                      ClIocPortT eoPort,
                                      ClUint32T fcnId,
                                      ClUint8T *pInBuf,
                                      ClUint32T inBufLen,
                                      ClUint8T *pOutBuf,
                                      ClUint32T *pOutBufLen,
                                      ClUint32T flags,
                                      ClUint32T timeOut,
                                      ClUint32T maxRetries,
                                      ClUint8T priority,
                                      ClCpmMarshallT marshallFunction,
                                      ClCpmUnmarshallT unmarshallFunction);

extern ClRcT nodeArrivalDeparturePublish(ClIocNodeAddressT iocAddress,
                                         ClNameT nodeName,
                                         ClCpmNodeEventT operation);

extern ClRcT clEventCpmCleanup(ClCpmEventPayLoadT *pEvtCpmCleanupInfo);

extern ClRcT cpmDebugRegister(void);

extern ClRcT cpmDebugDeregister(void);

extern void cpmResponse(ClRcT retCode,
                        ClCpmComponentT *comp,
                        ClCpmCompRequestTypeT requestType);

extern ClRcT cpmUpdateTL(ClAmsHAStateT haState);

extern ClRcT _cpmNodeDepartureAllowed(ClNameT *nodeName, ClCpmNodeLeaveT nodeLeave);

extern ClRcT _cpmIocAddressForNodeGet(ClNameT *nodeName, 
                                      ClIocAddressT *pIocAddress);

extern ClRcT _cpmNodeNameForNodeAddressGet(ClIocNodeAddressT nodeAddress,
                                           ClNameT *pNodeName);

extern ClRcT cpmCmRequestDSInitialize();

extern ClRcT cpmCmRequestDSFinalize();

extern ClRcT cpmEnqueueCmRequest(ClNameT *pNodeName, ClCmCpmMsgT *pRequest);

extern ClRcT cpmDequeueCmRequest(ClNameT *pNodeName, ClCmCpmMsgT *pRequest);

extern ClRcT cpmEnqueueAspRequest(ClNameT *pNodeName, ClIocNodeAddressT nodeAddress, 
                                  ClUint32T nodeRequest);

extern ClRcT cpmDequeueAspRequest(ClNameT *pNodeName, ClUint32T *nodeRequest);

extern void  cpmResetDeleteRequest(const ClNameT *pNodeName);

/**
 ************************************
 *  \page pageCPM129 clCpmComponentLogicalAddressUpdate
 *
 *  \par Synopsis:
 *  Updates the logical address of a component.
 * 
 *  \par Header File:
 *  clCpmApi.h
 * 
 *  \par Library Files:
 *  libClAmfClient 
 *
 *  \par Syntax:
 *  \code     extern ClRcT clCpmComponentLogicalAddressUpdate(
 *                CL_IN ClNameT         *pCompName, 
 *                CL_IN ClIocAddressT   *pLogicalAddress);
 *  \endcode
 *
 *  \param pCompName: Pointer to the component name for which logical address 
 *  is updated.
 *  \param pLogicalAddress: Logical address of the component.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_ERR_NULL_POINTER: On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to update and maintain the logical address of a given 
 *  component name \e pCompName.
 *   
 *  \b Note: This is a synchronous API.
 * 
 *  \par Related APIs:
 * 
 *
 */
extern ClRcT
clCpmComponentLogicalAddressUpdate(CL_IN ClNameT *pCompName,
                                   CL_IN ClIocAddressT *pLogicalAddress);

/**
 ************************************
 *  \page pageCPM101 clCpmExecutionObjectRegister
 *
 *  \par Synopsis:
 *  Registers the EO with the Component Manager.
 * 
 *  \par Header File:
 *  clCpmApi.h
 * 
 *  \par Library Files:
 *  libClAmfClient 
 *
 *  \par Syntax:
 *  \code    extern ClRcT clCpmExecutionObjectRegister(
 *                CL_IN ClEoExecutionObjT* pThis);
 *  \endcode
 *
 *  \param pThis:  Pointer to the EO.    
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_CPM_ERR_NULL_PTR: On passing a NULL pointer.
 *  \retval CL_CPM_ERR_NO_MEMORY: If Component Manager Server runs 
 *  out-of-memory.
 *  \retval CL_ERR_VERSION_MISMATCH: On version mismatch.
 *
 *  \par Description:
 *  This API is used to register an EO with the Component Manager. It is 
 *  called whenever an EO is created. The newly created EO gets added to the 
 *  \e EOList by the Component Manager.
 *
 *  \b Note: This is a synchronous API.
 * 
 *  \par Related APIs:
 * 
 *
 */
extern ClRcT clCpmExecutionObjectRegister(CL_IN ClEoExecutionObjT *pThis);

/**
 ************************************
 *  \page pageCPM104 clCpmExecutionObjectStateUpdate
 *
 *  \par Synopsis:
 *  Updates the state of an EO.
 * 
 *  \par Header File:
 *  clCpmApi.h
 * 
 *  \par Library Files:
 *  libClAmfClient 
 *
 *  \par Syntax:
 *  \code     extern ClRcT clCpmExecutionObjectStateUpdate(
 *                CL_IN ClEoExecutionObjT *pEOptr);
 *  \endcode
 *
 *  \param pEOptr: Pointer to the EO for which state is updated.
 *
 *  \retval CL_OK: The API executed successfully.
 *  \retval CL_CPM_ERR_NULL_PTR: On passing a NULL pointer.
 *
 *  \par Description:
 *  This API is used to update the state of an EO registered with the 
 *  Component Manager.
 *   
 *  \b Note: This is a synchronous API.
 * 
 *  \par Related APIs:
 * 
 *  clCpmExecutionObjectStateSet()
 *
 */
extern ClRcT clCpmExecutionObjectStateUpdate(CL_IN ClEoExecutionObjT *pEOptr);

extern ClRcT cpmInitiatedSwitchOver(ClBoolT checkDeputy);
 
extern ClRcT cpmGmsTimerCallback(void *arg);

extern ClBoolT cpmIsInfrastructureComponent(ClNameT *compName);

extern ClRcT cpmSelfShutDown(void);

extern ClRcT cpmComponentEventPublish(ClCpmComponentT *comp,
                                      ClCpmCompEventT compEvent,
                                      ClBoolT eventCleanup);

extern ClRcT cpmComponentEventCleanup(ClCpmComponentT *comp);

/*
 * Bug 4424:
 * Added the declaration for cpmUpdateNodeState().
 */
extern ClRcT cpmUpdateNodeState(ClCpmLocalInfoT *cpmL);

extern void cpmWriteNodeStatToFile(const ClCharT *who, ClBoolT isUp);

extern ClRcT cpmFailoverNode(ClGmsNodeIdT nodeId, ClBoolT scFailover);

extern void cpmShutdownAllNodes(void);

extern void cpmEOHBFailure(ClCpmEOListNodeT *pThis);

extern void cpmCompHeartBeatFailure(ClIocPortT iocPort);

extern void cpmRegisterWithActive(void);

extern void cpmGoBackToRegister(void);

extern void cpmBmRespTimerStart(void);

extern void cpmBmRespTimerStop(void);

extern void cpmCommitSuicide(void);

extern void cpmShutdownHeartbeat(void);

extern void cpmEnableHeartbeat(void);

extern void cpmDisableHeartbeat(void);

extern void cpmActive2Standby(ClBoolT shouldRestartWorkers);

extern ClRcT cpmStandby2Active(ClGmsNodeIdT prevMasterNodeId,
                               ClGmsNodeIdT deputyNodeId);

extern void cpmStandbyRecover(const ClGmsClusterNotificationBufferT
                              *notificationBuffer);

extern ClBoolT cpmStatusHeartbeat(void);

extern void cpmRestart(ClTimerTimeOutT *pDelay, const ClCharT *pPersonality);

extern void cpmReset(ClTimerTimeOutT *pDelay, const ClCharT *pPersonality);

extern ClBoolT cpmIsValgrindBuild(ClCharT *instantiationCMD);

extern void cpmModifyCompArgs(ClCpmCompConfigT *newConfig, ClUint32T *pArgIndex);

extern ClBoolT cpmIsAspSULoaded(const ClCharT *su);

extern ClRcT cpmLoggerRotateEnable(void);

extern ClRcT cpmProxiedHealthcheckStop(ClNameT *compName);

extern ClRcT cpmCompHealthcheckStop(ClNameT *compName);

extern void cpmResetNodeElseCommitSuicide(ClUint32T restartFlag);

extern ClRcT cpmCompCleanup(ClCharT *compName);

extern void cpmKillAllUserComponents(void);

extern void cpmSwitchoverActive(void);

extern ClBoolT clCpmSwitchoverInline(void);

extern ClRcT clCpmCompPreCleanupInvoke(ClCpmComponentT *comp);

extern ClRcT cpmCompParseArgs(ClCpmCompConfigT *compConfig, ClCharT *cmd, ClUint32T *pArgIndex);

extern ClRcT cpmEventPublishQueueInit();

#ifdef __cplusplus
}
#endif

#endif /* _CL_CPM_INTERNAL_H_ */
