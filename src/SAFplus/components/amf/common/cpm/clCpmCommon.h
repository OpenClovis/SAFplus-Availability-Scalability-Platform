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
 * This header file contains definitions of all the internal data
 * types and data structures used by the CPM client and server code.
 */

#ifndef _CL_CPM_COMMON_H_
#define _CL_CPM_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * ASP header files 
 */
#include <clCommon.h>
#include <clCommonErrors.h>
#include <clBufferApi.h>
#include <clIocApi.h>    
#include <clCpmAms.h>    
#include <clCorMetaData.h>
#include <clEventApi.h>
#include <clClmApi.h>
    
struct _ClCpmLocalInfoT_4_0_0;
    

#define CPM_LOG_AREA_CPM               "CPM"
    
#define CL_ASP_CONFIG_PATH             "ASP_CONFIG"
#define CL_ASP_BINDIR_PATH             "ASP_BINDIR"
    
#define CL_CPM_CLIENT_DFLT_TIMEOUT     CL_RMD_DEFAULT_TIMEOUT
#define CL_CPM_CLIENT_DFLT_RETRIES     CL_RMD_DEFAULT_RETRIES

/**
 * Invocation ids for SAF callbacks. Callback base should not clash
 * with AMS max callbacks
 */
#define __CPM_CALLBACK_BASE (0x20)
#define __CPM_CALLBACK(cmd) (__CPM_CALLBACK_BASE + (cmd))
#define CL_CPM_HB_CALLBACK                  __CPM_CALLBACK(0x1)
#define CL_CPM_TERMINATE_CALLBACK           __CPM_CALLBACK(0x2)
#define CL_CPM_CSI_SET_CALLBACK             __CPM_CALLBACK(0x3)
#define CL_CPM_CSI_RMV_CALLBACK             __CPM_CALLBACK(0x4)
#define CL_CPM_PG_TRACK_CALLBACK            __CPM_CALLBACK(0x5)
#define CL_CPM_PROXIED_INSTANTIATE_CALLBACK __CPM_CALLBACK(0x6)
#define CL_CPM_PROXIED_CLEANUP_CALLBACK     __CPM_CALLBACK(0x7)
#define CL_CPM_CSI_QUIESCING_CALLBACK       __CPM_CALLBACK(0x8)
#define CL_CPM_INSTANTIATE_REGISTER_CALLBACK __CPM_CALLBACK(0x9)
#define CL_CPM_MAX_CALLBACK                 __CPM_CALLBACK(0x10)

/*
 * CPM invocation entry flags.
 */
#define CL_CPM_INVOCATION_CPM               0x1
#define CL_CPM_INVOCATION_AMS               0x2
#define CL_CPM_INVOCATION_DATA_SHARED       0x4
#define CL_CPM_INVOCATION_DATA_COPIED       0x8

/*
 * AMF priorities
 */
#define CL_IOC_CPM_PRIORITY(pri) (CL_IOC_MAX_PRIORITIES + (pri))
#define CL_IOC_CPM_INSTANTIATE_PRIORITY CL_IOC_CPM_PRIORITY(CL_CPM_INSTANTIATE_REGISTER_CALLBACK)
#define CL_IOC_CPM_TERMINATE_PRIORITY CL_IOC_CPM_PRIORITY(CL_CPM_TERMINATE_CALLBACK)
#define CL_IOC_CPM_CSISET_PRIORITY CL_IOC_CPM_PRIORITY(CL_CPM_CSI_SET_CALLBACK)
#define CL_IOC_CPM_CSIRMV_PRIORITY CL_IOC_CPM_PRIORITY(CL_CPM_CSI_RMV_CALLBACK)
#define CL_IOC_CPM_CSIQUIESCING_PRIORITY CL_IOC_CPM_PRIORITY(CL_CPM_CSI_QUIESCING_CALLBACK)

/* 
 * Macro for error checking and logging. Should be refined someday if
 * possible :-(
 */
#define CL_CPM_CHECK_0(X, Z, retCode,logHandle)			\
    if(CL_GET_ERROR_CODE(retCode) != CL_OK)                     \
    {                                                           \
        /* CL_DEBUG_PRINT(X, (Z));                   */         \
        clLogWrite((logHandle),X, NULL, Z);		        \
        rc = retCode;                                           \
        goto failure;                                           \
    }
 
#define CL_CPM_CHECK_1(X, Z, argv1, retCode, logHandle)			\
    if(CL_GET_ERROR_CODE(retCode) != CL_OK)                             \
    {                                                                   \
        /* CL_DEBUG_PRINT(X, (Z, argv1));              */               \
        clLogWrite((logHandle), X, NULL, Z, argv1);			\
        rc = retCode;                                                   \
        goto failure;                                                   \
    }

#define CL_CPM_CHECK_2(X, Z, argv1, argv2, retCode,logHandle) \
    if(CL_GET_ERROR_CODE(retCode) != CL_OK)                             \
    {                                                                   \
        /* CL_DEBUG_PRINT(X, (Z, argv1, argv2));      */                \
        clLogWrite((logHandle), X, NULL, Z, (argv1), (argv2));		\
        rc = retCode;                                                   \
        goto failure;                                                   \
    }

#define CL_CPM_CHECK_3(X, Z, argv1, argv2, argv3, retCode, logHandle)	\
    if(CL_GET_ERROR_CODE(retCode) != CL_OK)                             \
    {                                                                   \
        /* CL_DEBUG_PRINT(X, (Z, argv1, argv2, argv3));    */           \
        clLogWrite(logHandle, X, NULL, Z, argv1, argv2, argv3);		\
        rc = retCode;                                                   \
        goto failure;                                                   \
    }

#define CL_CPM_LOG_SP(...)  __VA_ARGS__

#define CL_CPM_CHECK(X, Z, retCode)					 \
    if(CL_GET_ERROR_CODE(retCode) != CL_OK)				 \
    {									 \
        clLog(X,CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,CL_CPM_LOG_SP Z);\
        rc = retCode;							 \
        goto failure;							 \
    }
    
#define CL_CPM_LOCK_CHECK(X, Z, retCode)			        \
    if(retCode != CL_OK)			                        \
    {					                                \
     clLog(X,CL_LOG_AREA_UNSPECIFIED,CL_LOG_CONTEXT_UNSPECIFIED,CL_CPM_LOG_SP Z);\
    rc = retCode;							\
    goto withlock;							\
    }

#define CL_CPM_INVOCATION_ID_GET(CBType, invocation, invocationId)  \
{                                                                   \
    invocationId = ( ( (CBType) << 32) | (invocation));             \
}

#define CL_CPM_INVOCATION_CB_TYPE_GET(invocationId, cbType) \
{                                                           \
    cbType = ((invocationId) >> 32);                        \
}

#define CL_CPM_INVOCATION_KEY_GET(invocationId, invocation) \
{                                                           \
    invocation = ((invocationId) & 0x00000000ffffffffLL);   \
}

#define MARSHALL_FN(dataType, rel, major, minor) VDECL_VER(clXdrMarshall ## dataType, rel, major, minor)
#define UNMARSHALL_FN(dataType, rel, major, minor) VDECL_VER(clXdrUnmarshall ## dataType, rel, major, minor)

typedef ClRcT (*ClCpmMarshallT) (void *, ClBufferHandleT, ClUint32T);
typedef ClRcT (*ClCpmUnmarshallT) (ClBufferHandleT, void *);

extern void cpmGetOwnLogicalAddress(ClIocLogicalAddressT * logicalAddress);


struct cpmBM;
    
typedef enum
{
    CL_CPM_LOCAL = 0,
    CL_CPM_GLOBAL = 1
} ClCpmTypeT;

#define CPM_MAX_ARGS                     256
#define CPM_MAX_ENVS                     256


typedef struct clCpmL
{
    /**
     * Name of the node.
     */
    ClCharT nodeName[CL_MAX_NAME_LENGTH];

    /**
     * Node information which is also used when registation with CPM/G
     * active.
     */
    struct _ClCpmLocalInfoT_4_0_0 *pCpmLocalInfo;

    /**
     * List of components on this node.
     */
    ClCntHandleT compTable;

    /**
     * Number of components on this node.
     */
    ClUint32T noOfComponent;

    ClOsalMutexIdT compTableMutex;

    SaNameT nodeType;
    SaNameT nodeIdentifier;
    SaNameT nodeMoIdStr;
    ClCharT classType[CL_MAX_NAME_LENGTH];
    ClUint32T chassisID;
    ClUint32T slotID;
} ClCpmLT;
    
typedef struct clCpmCfg
{

    /**
     * This is the default frequency at which CPM will send heartbeat
     * signals (isAlive) to the component.
     */
    ClUint32T defaultFreq;

    /**
     * This is the maximum frequency at which CPM will send heartbeat
     * signals (isAlive) to the component.
     */
    ClUint32T threshFreq;

    /**
     * This indicates whether the Component Manager type is \e Local
     * or \e Global.
     */
    ClCpmTypeT cpmType;
    
    /**
     * This indicate the name of the node.
     */
    ClCharT nodeName[CL_MAX_NAME_LENGTH];
} ClCpmCfgT;
    
/**
 * Per node global data structure.
 */
typedef struct clCpm
{
    /**
     * CPMs Own information 
     */
    struct _ClCpmLocalInfoT_4_0_0 *pCpmLocalInfo;

    /**
     * CPM configuration.
     */
    ClCpmCfgT *pCpmConfig;

    /**
     * HA state of the node.
     */
    ClAmsHAStateT haState;

    ClGmsNodeIdT activeMasterNodeId;
    ClGmsNodeIdT deputyNodeId;

    /**
     * Name used for event.
     */
    SaNameT name;

    /**
     * Name used for checkpoint service.
     */
    SaNameT ckptCpmLName;

    /**
     * Server based checkpoint handles.
     */
    ClHandleT ckptHandle;
    ClHandleT ckptOpenHandle;

    ClUint32T registeredCPML;

    /**
     * Handle to CPM EO 
     */
    ClEoExecutionObjT *cpmEoObj;

    /**
     * Polling Thread ID, this is the thread which periodically keep
     * sending HB to all the EOs. In case of CPM/G, this will also
     * send HB to CPM/Ls.
     */
    ClOsalTaskIdT taskId;

    /**
     * If 0, should stop doing the HB and exit from the polling thread.
     */
    ClInt32T polling;
    
    /**
     * If 0, disable heartbeating globally, as its unnecesarry backlog
     * when we already have much faster reliable mechanisms in place.
     * We disable heartbeating by default and let the administrator
     * enable it.
     */
    ClBoolT enableHeartbeat;

    /**
     * Flag to indicate wether the event server is up. If the flag is
     * not set then the CPM will not publish then CPM will not publish
     * any event.
     */
    ClUint32T emUp;

    ClUint32T emInitDone;

    /**
     * Flag to indicate wether the COR is up/down. Should go away.
     */
    ClUint32T corUp;

    /**
     * EO id allocator, used for dynamically assigning the EO ids.
     */
    ClEoIdT eoIdAlloc;

    /**
     * Component id allocator, used for dynamically assigning the
     * component ids.
     */
    ClUint32T compIdAlloc;

    ClUint32T nodeEventPublished;

    ClOsalMutexT cpmMutex;

    ClOsalMutexT heartbeatMutex;
    ClOsalCondT  heartbeatCond;

    /**
     * Mutex to have exclusive access to the eo list.
     */
    ClOsalMutexIdT eoListMutex;

    /**
     * Number of EOs on the node. Should go away.
     */
    ClUint32T noOfEo;

    /**
     * Information related to the components on this node.
     */

    /**
     * List of components.
     */
    ClCntHandleT compTable;
    /**
     * Number of components on this node.
     */
    ClUint32T noOfComponent;
    /**
     * Mutex to protect the list of components on this node.
     */
    ClOsalMutexIdT compTableMutex;

    /**
     * Information related to the service units on this node.
     */

    /**
     * List of CPM/G attributes.
     */

    /**
     * List of all the nodes in the cluster, with their registration
     * information.
     */
    ClCntHandleT cpmTable;

    /**
     * Number of nodes existing in the cluster.
     */
    ClUint32T noOfCpm;

    /**
     * Mutex to protect list of nodes in the cluster.
     */
    ClOsalMutexIdT cpmTableMutex;

    /**
     * Boot manager related.
     */

    ClOsalTaskIdT bmTaskId;
    struct cpmBM *bmTable;

    /**
     * AMS.
     */
    ClCpmAmsToCpmCallT *amsToCpmCallback;
    ClCpmCpmToAmsCallT *cpmToAmsCallback;
    ClCntHandleT invocationTable;
    ClOsalMutexIdT invocationMutex;
    ClUint32T invocationKey;

    /**
     * Event.
     */
    ClEventInitHandleT cpmEvtHandle;
    ClEventCallbacksT cpmEvtCallbacks;
    /**
     * Handle to opened component event channel.
     */
    ClEventChannelHandleT cpmEvtChannelHandle;
    /**
     * Handle to component event.
     */
    ClEventHandleT cpmEventHandle;
    /**
     * Identifier for component event.
     */
    ClEventIdT cpmEvtId;

    /**
     * Similar things for node event.
     */
    ClEventChannelHandleT cpmEvtNodeChannelHandle;
    ClEventHandleT cpmEventNodeHandle;
    ClEventIdT cpmNodeEvtId;

    /**
     * AMF version.
     */
    ClVersionT version;

    ClUint32T cpmShutDown;

    /**
     * COR.
     */
    ClCorMOIdPtrT nodeMoId;

    /**
     * GMS.
     */
    ClGmsHandleT cpmGmsHdl;
    ClOsalMutexT cpmGmsMutex;
    ClOsalCondT cpmGmsCondVar;
    ClUint32T cpmGmsTimeout;

    /**
     * Should go away.
     */
    ClTimerHandleT gmsTimerHandle;

    /* 
     * CM.
     */
    ClCntHandleT slotTable;
    ClCntHandleT cmRequestTable;
    ClOsalMutexIdT cmRequestMutex;

    ClCharT logFilePath[CL_MAX_NAME_LENGTH];
    ClCharT corServerName[CL_MAX_NAME_LENGTH];
    ClCharT eventServerName[CL_MAX_NAME_LENGTH];
    ClCharT logServerName[CL_MAX_NAME_LENGTH];

    ClTimerHandleT cpmLogRollTimer;
    ClBoolT nodeLeaving;
    ClOsalMutexT clusterMutex;
    ClOsalMutexT compTerminateMutex;
    ClOsalMutexT cpmShutdownMutex;
    ClBoolT trackCallbackInProgress;
    ClTimeT bootTime;
} ClCpmT;

typedef struct
{
    /**
     * In case of framework invoked healthcheck means :
     * Interval at which AMF will invoke the healthcheck
     * callback of the component.
     *
     * In case of client invoked healthcheck means :
     * Client should call clCpmHealthcheckConfirm() every
     * `period' interval to indicate that it is alive.
     */
    ClUint32T period;
    
    /**
     * In case of framework invoked healthcheck means :
     * Interval at which AMF will invoke the healthcheck
     * callback of the component.
     *
     * In case of client invoked healthcheck it is ignored.
     * 
     */
    ClUint32T maxDuration;

    /**
     * Recovery action to take in case the healthcheck fails.
     */
    ClAmsLocalRecoveryT recommendedRecovery;
} ClCpmCompHealthCheckConfigT;

/**
 * This is the definition of an environment variable for a
 * component.
 */
typedef struct
{
    /**
     * Environment variable name.
     */
    ClCharT envName[CL_MAX_NAME_LENGTH];
    /**
     * Environment variable value.
     */
    ClCharT envValue[CL_MAX_NAME_LENGTH];
} ClCpmEnvVarT;
        
/**
 * This structure contains a list of the configuration parameters,
 * with default some values.  It is required for the component
 * definition.
 */
typedef struct clCpmCompConfig
{
    /**
     * The component name must be considered. There is no default
     * value of this attribute.
     */
    ClCharT compName[CL_MAX_NAME_LENGTH];

    /**
     * Is this an ASP component ?
     */
    ClBoolT isAspComp;

    /**
     * This must be configured as it indicates whether the component
     * is Pre-Instantiabe or Non-PreInstantiable or Proxied or
     * SA-Aware.
     */
    ClAmsCompPropertyT compProperty;

    /**
     * This displays the component-process relationship. The Default
     * value is \c COMP_SINGLE_PROCESS.
     */
    ClCpmCompProcessRelT compProcessRel;

    /**
     * This is the instantiation command which you must know.
     */
    ClCharT instantiationCMD[CL_MAX_NAME_LENGTH];

    /**
     * These argument are passed to the component. The default Value
     * is NULL.
     */
    ClCharT *argv[CPM_MAX_ARGS];

    /**
     * This is an environment variable passed to the component. The
     * default Value is NULL.
     */
    ClCpmEnvVarT *env[CPM_MAX_ENVS];

    /**
     * This is the command for component termination.
     */
    ClCharT terminationCMD[CL_MAX_NAME_LENGTH];

    /**
     * This is the command for component cleanup which you must know.
     */
    ClCharT cleanupCMD[CL_MAX_NAME_LENGTH];

    /**
     *  The default value is \c CL_CPM_COMPONENT_DEFAULT_TIMEOUT.
     */
    ClUint32T compInstantiateTimeout;

    /**
     * The default value is \c CL_CPM_COMPONENT_DEFAULT_TIMEOUT.
     */
    ClUint32T compCleanupTimeout;

    /**
     * The default value is \c CL_CPM_COMPONENT_DEFAULT_TIMEOUT.
     */
    ClUint32T compTerminateCallbackTimeOut;

    /**
     * The default value is \c CL_CPM_COMPONENT_DEFAULT_TIMEOUT.
     */
    ClUint32T compProxiedCompInstantiateCallbackTimeout;

    /**
     * The default value is \c CL_CPM_COMPONENT_DEFAULT_TIMEOUT.
     */
    ClUint32T compProxiedCompCleanupCallbackTimeout;

    /**
     * Component healthcheck configuration.
     */
    ClCpmCompHealthCheckConfigT healthCheckConfig;
} ClCpmCompConfigT;

typedef struct clCpmComponent ClCpmComponentT;
typedef struct EOListNode ClCpmEOListNodeT;
    
struct clCpmComponent
{
    /**
     * Component configuration.
     */
    ClCpmCompConfigT *compConfig;

    /**
     * Name of the proxy, proxying this component.
     */
    ClCharT proxyCompName[CL_MAX_NAME_LENGTH];

    /**
     * Reference to the proxy component. Should go away.
     */
    ClCpmComponentT *proxyCompRef;

    /**
     * If it is a proxy component, the number of proxied components.
     */
    ClUint32T numProxiedComps;

    /**
     * Component id.
     */
    ClUint32T compId;

    /**
     * EO port of the component.
     */
    ClIocPortT eoPort;

    /**
     * Earlier there was concept of multiple EOs per component. This
     * should be simplified.
     */
    ClUint32T eoCount;
    ClCpmEOListNodeT *eoHandle;

    /**
     * In case of life cycle management(LCM) operation, source address
     * where the response RMD should be invoked after completing the
     * LCM operation.
     */
    ClIocPhysicalAddressT requestSrcAddress;
    ClUint32T requestRmdNumber;

    /**
     * Type of LCM operation.
     */
    ClUint32T requestType;
    ClUint32T restartPending;

    /**
     * Process id of the component.
     */
    ClUint32T processId;

    /**
     * Indicates the number of times the component is restarted after
     * ASP has started on this node. Used in LCM operations only.
     */
    ClUint32T compRestartCount;

    /**
     * Indicates the number of times the component is restarted in the
     * given window after HB failure. Only used for ASP components.
     */
    ClUint32T compRestartRecoveryCount;

    /**
     * The time when component was last restarted.
     */
    ClTimeT compRestartTime;

    /**
     * Operational state of the component.
     */
    ClAmsOperStateT compOperState;

    /**
     * Presence state of the component.
     */
    ClAmsPresenceStateT compPresenceState;

    /**
     * Readiness state of the component.
     */
    ClAmsReadinessStateT compReadinessState;

    /**
     * The component timers maintained by CPM. Many of these are
     * obsolete and must go away.
     */
    ClTimerHandleT cpmInstantiateTimerHandle;
    ClTimerHandleT cpmTerminateTimerHandle;
    ClTimerHandleT cpmHealthcheckTimerHandle;
    ClTimerHandleT cpmProxiedInstTimerHandle;
    ClTimerHandleT cpmProxiedCleanupTimerHandle;

    ClCorMOIdPtrT compMoId;
    ClCorObjectHandleT compObjHdl;
    ClOsalMutexIdT compMutex;
    ClTimerHandleT hbTimerHandle;
    ClBoolT hbInvocationPending;
    ClBoolT hcConfirmed;
    
    /*
     * EO id of the component.
     */
    ClEoIdT eoID; 
    ClEoIdT lastEoID; /*last EO id of the component before restart*/
    ClBoolT compEventPublished;
    ClBoolT compTerminated;
    ClUint64T instantiateCookie; /*cluster wide component instantiate cookie*/
};

struct EOListNode
{
    /**
     * Pointer to the EO.
     */
    ClEoExecutionObjT *eoptr;

    ClUint32T freq;
    ClUint32T nextPoll;

    ClRcT status;

    ClUint32T dontPoll;

    ClCpmComponentT *compRef;

    ClCpmEOListNodeT *pNext;
};
        
extern ClCpmT *gpClCpm;

extern ClUint32T cpmNodeFind(SaUint8T *name, ClCpmLT **cpmL);
extern ClUint32T cpmNodeFindLocked(SaUint8T *name, ClCpmLT **cpmL);
extern ClUint32T cpmNodeFindByNodeId(ClGmsNodeIdT nodeId, ClCpmLT **cpmL);
extern ClRcT cpmNodeDelByNodeId(ClUint32T nodeId);

#ifdef __cplusplus
}
#endif

#endif /* _CL_CPM_COMMON_H_ */
