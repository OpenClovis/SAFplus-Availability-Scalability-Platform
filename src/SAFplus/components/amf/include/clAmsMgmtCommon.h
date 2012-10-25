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
 * ModuleName  : amf
 * File        : clAmsMgmtCommon.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This file contains the function call types used by the client library to
 * call the server and for the server to call its peer. 
 *
 * XXX: This file should not be exposed to users of the client API because
 * it is internal to the API but is currently needed in include/ to satisfy
 * dependencies. These dependencies need to be separated and this file moved 
 * to common/.
 *****************************************************************************/

#ifndef _CL_AMS_MGMT_COMMON_H_
#define _CL_AMS_MGMT_COMMON_H_

# ifdef __cplusplus
extern "C" {
# endif


#include <clCommon.h>

#include <clAmsTypes.h>
#include <clAmsEntities.h>
#include <clLogApi.h>

/*
 * Function IDs to identify client callback function that the server needs to
 * call.
 */

typedef enum
{
    CL_AMS_MGMT_INITIALIZE                       = 1,
    CL_AMS_MGMT_FINALIZE                         = 2,
    CL_AMS_MGMT_ENTITY_CREATE                   = 3,
    CL_AMS_MGMT_ENTITY_DELETE                   = 4,
    CL_AMS_MGMT_ENTITY_SET_CONFIG               = 5,
    CL_AMS_MGMT_ENTITY_LOCK_ASSIGNMENT          = 6,
    CL_AMS_MGMT_ENTITY_LOCK_INSTANTIATION       = 7,
    CL_AMS_MGMT_ENTITY_UNLOCK                   = 8,
    CL_AMS_MGMT_ENTITY_SHUTDOWN                 = 9,
    CL_AMS_MGMT_ENTITY_RESTART                  = 10,
    CL_AMS_MGMT_ENTITY_REPAIRED                 = 11,
    CL_AMS_MGMT_SG_ADJUST_PREFERENCE            = 12,
    CL_AMS_MGMT_SI_SWAP                         = 13,
    CL_AMS_MGMT_ENTITY_LIST_ENTITY_REF_ADD      = 14,
    CL_AMS_MGMT_ENTITY_SET_REF                  = 15,
    CL_AMS_MGMT_CSI_SET_NVP                     = 16,
    CL_AMS_MGMT_DEBUG_ENABLE                    = 17,
    CL_AMS_MGMT_DEBUG_DISABLE                   = 18,
    CL_AMS_MGMT_DEBUG_GET                       = 19,
    CL_AMS_MGMT_DEBUG_ENABLE_LOG_TO_CONSOLE     = 20,
    CL_AMS_MGMT_DEBUG_DISABLE_LOG_TO_CONSOLE    = 21,

    CL_AMS_MGMT_CCB_INITIALIZE                  = 22,
    CL_AMS_MGMT_CCB_FINALIZE                    = 23,
    CL_AMS_MGMT_CCB_ENTITY_SET_CONFIG           = 24,
    CL_AMS_MGMT_CCB_CSI_SET_NVP                 = 25,
    CL_AMS_MGMT_CCB_SET_NODE_DEPENDENCY         = 26,
    CL_AMS_MGMT_CCB_SET_NODE_SU_LIST            = 27,
    CL_AMS_MGMT_CCB_SET_SG_SU_LIST              = 28,
    CL_AMS_MGMT_CCB_SET_SG_SI_LIST              = 29,
    CL_AMS_MGMT_CCB_SET_SU_COMP_LIST            = 30,
    CL_AMS_MGMT_CCB_SET_SI_SU_RANK_LIST         = 31,
    CL_AMS_MGMT_CCB_SET_SI_SI_DEPENDENCY        = 32,
    CL_AMS_MGMT_CCB_SET_SI_CSI_LIST             = 33,
    CL_AMS_MGMT_CCB_ENABLE_ENTITY               = 34,
    CL_AMS_MGMT_CCB_DISABLE_ENTITY              = 35,
    CL_AMS_MGMT_CCB_ENTITY_CREATE               = 36,
    CL_AMS_MGMT_CCB_ENTITY_DELETE               = 37,
    CL_AMS_MGMT_CCB_COMMIT                      = 38, 
    CL_AMS_MGMT_ENTITY_GET                      = 39,
    CL_AMS_MGMT_ENTITY_GET_CONFIG               = 40,
    CL_AMS_MGMT_ENTITY_GET_STATUS               = 41,
    CL_AMS_MGMT_GET_CSI_NVP_LIST                = 42,
    CL_AMS_MGMT_GET_ENTITY_LIST                 = 43,
    CL_AMS_MGMT_GET_OL_ENTITY_LIST              = 44,
    CL_AMS_MGMT_CCB_CSI_DELETE_NVP              = 45,

    CL_AMS_MGMT_CCB_DELETE_NODE_DEPENDENCY         = 46,
    CL_AMS_MGMT_CCB_DELETE_NODE_SU_LIST            = 47,
    CL_AMS_MGMT_CCB_DELETE_SG_SU_LIST              = 48,
    CL_AMS_MGMT_CCB_DELETE_SG_SI_LIST              = 49,
    CL_AMS_MGMT_CCB_DELETE_SU_COMP_LIST            = 50,
    CL_AMS_MGMT_CCB_DELETE_SI_SU_RANK_LIST         = 51,
    CL_AMS_MGMT_CCB_DELETE_SI_SI_DEPENDENCY        = 52,
    CL_AMS_MGMT_CCB_DELETE_SI_CSI_LIST             = 53,

    CL_AMS_MGMT_ENTITY_SET_ALPHA_FACTOR = 54,
    CL_AMS_MGMT_MIGRATE_SG = 55,
    
    CL_AMS_MGMT_ENTITY_USER_DATA_SET = 56,
    CL_AMS_MGMT_ENTITY_USER_DATA_SETKEY = 57,
    CL_AMS_MGMT_ENTITY_USER_DATA_GET = 58,
    CL_AMS_MGMT_ENTITY_USER_DATA_GETKEY = 59,
    CL_AMS_MGMT_ENTITY_USER_DATA_DELETE = 60,
    CL_AMS_MGMT_ENTITY_USER_DATA_DELETEKEY = 61,

    CL_AMS_MGMT_CCB_SET_CSI_CSI_DEPENDENCY        = 62,
    CL_AMS_MGMT_CCB_DELETE_CSI_CSI_DEPENDENCY        = 63,
    CL_AMS_MGMT_SI_ASSIGN_SU_CUSTOM                 = 65,
    CL_AMS_MGMT_ENTITY_SET_BETA_FACTOR = 66,
    CL_AMS_MGMT_ENTITY_FORCE_LOCK     =  67,
    CL_AMS_MGMT_DB_GET     =  68,
    CL_AMS_MGMT_COMPUTED_ADMIN_STATE_GET     =  69,
    CL_AMS_MGMT_ENTITY_FORCE_LOCK_INSTANTIATION     =  70,
    CL_AMS_MGMT_CCB_BATCH_COMMIT = 71,
    CL_AMS_MGMT_ENTITY_SHUTDOWN_WITH_RESTART = 72,
} ClAmsMgmtClientCallbackRmdInterfaceT;

/*
 * Supported TLV types 
 */
typedef enum
{
    CL_AMS_TLV_TYPE_ENTITY       = CL_AMS_ENTITY_TYPE_ENTITY,
    CL_AMS_TLV_TYPE_NODE         = CL_AMS_ENTITY_TYPE_NODE,
    CL_AMS_TLV_TYPE_APP          = CL_AMS_ENTITY_TYPE_APP,
    CL_AMS_TLV_TYPE_SG           = CL_AMS_ENTITY_TYPE_SG,
    CL_AMS_TLV_TYPE_SU           = CL_AMS_ENTITY_TYPE_SU,
    CL_AMS_TLV_TYPE_SI           = CL_AMS_ENTITY_TYPE_SI,
    CL_AMS_TLV_TYPE_COMP         = CL_AMS_ENTITY_TYPE_COMP,
    CL_AMS_TLV_TYPE_CSI          = CL_AMS_ENTITY_TYPE_CSI,
    CL_AMS_TLV_TYPE_CLUSTER      = CL_AMS_ENTITY_TYPE_CLUSTER,
    CL_AMS_TLV_TYPE_START_LIST   = CL_AMS_ENTITY_TYPE_CLUSTER + 1,
    CL_AMS_TLV_TYPE_END_LIST     = CL_AMS_TLV_TYPE_START_LIST + 1, 
} ClAmsTLVTypeT;

/*
 * AMS CCB operations type 
 */

typedef enum
{
    CL_AMS_MGMT_CCB_OPERATION_CREATE = 1,
    CL_AMS_MGMT_CCB_OPERATION_DELETE ,
    CL_AMS_MGMT_CCB_OPERATION_SET_CONFIG,
    CL_AMS_MGMT_CCB_OPERATION_CSI_SET_NVP ,
    CL_AMS_MGMT_CCB_OPERATION_SET_NODE_DEPENDENCY,
    CL_AMS_MGMT_CCB_OPERATION_SET_NODE_SU_LIST ,
    CL_AMS_MGMT_CCB_OPERATION_SET_SG_SU_LIST ,
    CL_AMS_MGMT_CCB_OPERATION_SET_SG_SI_LIST ,
    CL_AMS_MGMT_CCB_OPERATION_SET_SU_COMP_LIST ,
    CL_AMS_MGMT_CCB_OPERATION_SET_SI_SU_RANK_LIST,
    CL_AMS_MGMT_CCB_OPERATION_SET_SI_SI_DEPENDENCY_LIST,
    CL_AMS_MGMT_CCB_OPERATION_SET_SI_CSI_LIST ,
    CL_AMS_MGMT_CCB_OPERATION_SET_CSI_CSI_DEPENDENCY_LIST,
    CL_AMS_MGMT_CCB_OPERATION_CSI_DELETE_NVP,
    CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_DEPENDENCY,
    CL_AMS_MGMT_CCB_OPERATION_DELETE_NODE_SU_LIST,
    CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SU_LIST,
    CL_AMS_MGMT_CCB_OPERATION_DELETE_SG_SI_LIST,
    CL_AMS_MGMT_CCB_OPERATION_DELETE_SU_COMP_LIST,
    CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SU_RANK_LIST,
    CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_SI_DEPENDENCY_LIST,
    CL_AMS_MGMT_CCB_OPERATION_DELETE_CSI_CSI_DEPENDENCY_LIST,
    CL_AMS_MGMT_CCB_OPERATION_DELETE_SI_CSI_LIST,
    CL_AMS_MGMT_CCB_OPERATION_MAX,
}ClAmsMgmtCCBOperationsT;

#define AMS_FUNC_ID(clnt, fn) CL_EO_GET_FULL_FN_NUM(clnt, fn)

/*
 * Default RMD behavior for AMS
 */
#define CL_AMS_RMD_DEFAULT_TIMEOUT        10000  
#define CL_AMS_RMD_DEFAULT_RETRIES           3

/*
 * Structure TLV for passing the pointer types to server
 */

typedef struct
{
    ClAmsTLVTypeT   type;
    ClUint32T       length ;
} ClAmsTLVT;


/*-----------------------------------------------------------------------------
 * Type definitions for RMD client --> server function arguments:
 *---------------------------------------------------------------------------*/

typedef struct
{
    ClHandleT  handle;
}clAmsMgmtDummyResponseT;

typedef struct
{
    ClAmsMgmtHandleT handle;
} clAmsMgmtFinalizeRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtFinalizeResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtEntityInstantiateRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityInstantiateResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtEntityTerminateRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityTerminateResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityRefT                     entityRef;
} clAmsMgmtEntityFindByNameRequestT;    

typedef struct
{
    ClAmsEntityRefT             entityRef;
}clAmsMgmtEntityFindByNameResponseT ;


typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClUint32T                           peInstantiateFlag;
    ClAmsEntityConfigT                  *entityConfig;
} clAmsMgmtEntitySetConfigRequestT;    

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
    ClUint32T                           alphaFactor;
} clAmsMgmtEntitySetAlphaFactorRequestT;    

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
    ClUint32T                           betaFactor;
} clAmsMgmtEntitySetBetaFactorRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntitySetAlphaFactorResponseT;
typedef clAmsMgmtDummyResponseT  clAmsMgmtEntitySetBetaFactorResponseT;
typedef clAmsMgmtDummyResponseT  clAmsMgmtEntitySetConfigResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        sourceEntity;
    ClAmsEntityT                        targetEntity;
}clAmsMgmtEntitySetRefRequestT ;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntitySetRefResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        sourceEntity;
    ClAmsCSINameValuePairT              nvp;
}clAmsMgmtCSISetNVPRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtCSISetNVPResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtEntityLockAssignmentRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityLockAssignmentResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
    ClBoolT                             lock;
} clAmsMgmtEntityForceLockRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityForceLockResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtEntityLockInstantiationRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityLockInstantiationResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtEntityUnlockRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityUnlockResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtEntityShutdownRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityShutdownResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtEntityRestartRequestT;    


typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityRestartResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtEntityRepairedRequestT;    


typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityRepairedResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtEntityStartMonitoringEntityRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityStartMonitoringEntityResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtEntityStopMonitoringEntityRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityStopMonitoringEntityResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
    ClUint32T                           enable;
} clAmsMgmtSGAdjustPreferenceRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtSGAdjustPreferenceResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
} clAmsMgmtSISwapRequestT;    


typedef clAmsMgmtDummyResponseT  clAmsMgmtSISwapResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        sourceEntity;
    ClAmsEntityT                        targetEntity;
    ClAmsEntityListTypeT                entityListName;
} clAmsMgmtEntityListEntityRefAddRequestT;    

typedef clAmsMgmtDummyResponseT  clAmsMgmtEntityListEntityRefAddResponseT;

typedef struct
{
    ClAmsMgmtHandleT                    handle;
    ClAmsEntityT                        entity;
    ClAmsEntityListTypeT                entityListName;
} clAmsMgmtEntityGetEntityListRequestT;    

typedef struct
{
    ClUint32T       numNodes;
    void            *nodeList;
}clAmsMgmtEntityGetEntityListResponseT;

typedef enum
{
    CL_AMS_MGMT_SUB_AREA_MSG            = 1,
    CL_AMS_MGMT_SUB_AREA_STATE_CHANGE   = (1<<1),
    CL_AMS_MGMT_SUB_AREA_FN_CALL        = (1<<2),
    CL_AMS_MGMT_SUB_AREA_TIMER          = (1<<3),
    CL_AMS_MGMT_SUB_AREA_UNDEFINED      = (1<<7),
}ClAmsMgmtSubAreaT;

typedef struct 
{
    ClAmsMgmtHandleT handle;
    ClAmsEntityT     entity;
    ClUint8T         debugFlags; 
}clAmsMgmtDebugEnableRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtDebugEnableResponseT;

typedef struct 
{
    ClAmsMgmtHandleT handle;
    ClAmsEntityT     entity;
    ClUint8T         debugFlags; 
}clAmsMgmtDebugDisableRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtDebugDisableResponseT;

typedef struct 
{
    ClAmsMgmtHandleT handle;
    ClAmsEntityT     entity;
}clAmsMgmtDebugGetRequestT;


typedef struct 
{
    ClUint8T         debugFlags; 
}clAmsMgmtDebugGetResponseT;

typedef struct 
{
    ClAmsMgmtHandleT handle;
}clAmsMgmtDebugEnableLogToConsoleRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtDebugEnableLogToConsoleResponseT;

typedef struct 
{
    ClAmsMgmtHandleT handle;
}clAmsMgmtDebugDisableLogToConsoleRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtDebugDisableLogToConsoleResponseT;

typedef struct
{
    ClCntHandleT  ccbOpListHandle;

}clAmsMgmtCCBT;

typedef struct
{
    ClAmsMgmtHandleT  handle;
} clAmsMgmtCCBInitializeRequestT;

typedef struct
{
    ClAmsMgmtCCBHandleT handle;
} clAmsMgmtCCBInitializeResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
} clAmsMgmtCCBFinalizeRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBFinalizeResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
} clAmsMgmtCCBCommitRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBCommitResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  entity;
}clAmsMgmtCCBEntityCreateRequestT;


typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBEntityCreateResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  entity;
}clAmsMgmtCCBEntityDeleteRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBEntityDeleteResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClUint64T  bitmask;
    ClAmsEntityConfigT  *entityConfig;
}clAmsMgmtCCBEntitySetConfigRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBEntitySetConfigResponseT;

typedef ClAmsCSINameValuePairT ClAmsCSINVPT; 

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  csiName;
    ClAmsCSINVPT  nvp;
}clAmsMgmtCCBCSISetNVPRequestT;


typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBCSISetNVPResponseT;

typedef clAmsMgmtCCBCSISetNVPRequestT  clAmsMgmtCCBCSIDeleteNVPRequestT;
typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBCSIDeleteNVPResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  nodeName;
    ClAmsEntityT  dependencyNodeName;
}clAmsMgmtCCBSetNodeDependencyRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBSetNodeDependencyResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  nodeName;
    ClAmsEntityT  suName;
}clAmsMgmtCCBSetNodeSUListRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBSetNodeSUListResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  sgName;
    ClAmsEntityT  suName;
}clAmsMgmtCCBSetSGSUListRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBSetSGSUListResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  sgName;
    ClAmsEntityT  siName;
}clAmsMgmtCCBSetSGSIListRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBSetSGSIListResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  suName;
    ClAmsEntityT  compName;
}clAmsMgmtCCBSetSUCompListRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBSetSUCompListResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  siName;
    ClAmsEntityT  suName;
    ClUint32T  suRank;
}clAmsMgmtCCBSetSISURankListRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBSetSISURankListResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  siName;
    ClAmsEntityT  dependencySIName;
}clAmsMgmtCCBSetSISIDependencyRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBSetSISIDependencyResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  csiName;
    ClAmsEntityT  dependencyCSIName;
}clAmsMgmtCCBSetCSICSIDependencyRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBSetCSICSIDependencyResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  siName;
    ClAmsEntityT  csiName;
}clAmsMgmtCCBSetSICSIListRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBSetSICSIListResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  entityName;
}clAmsMgmtCCBEnableEntityRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBEnableEntityResponseT;

typedef struct
{
    ClAmsMgmtCCBHandleT  handle;
    ClAmsEntityT  entityName;
}clAmsMgmtCCBDisableEntityRequestT;

typedef clAmsMgmtDummyResponseT  clAmsMgmtCCBDisableEntityResponseT;

typedef struct
{
    ClAmsMgmtHandleT  handle;
    ClAmsEntityT  entity;
}clAmsMgmtEntityGetRequestT;

typedef struct
{
    ClAmsEntityT  *entity;
}clAmsMgmtEntityGetResponseT;

typedef struct
{
    ClAmsMgmtHandleT  handle;
    ClAmsEntityT  entity;
}clAmsMgmtEntityGetConfigRequestT;

typedef struct
{
    ClAmsEntityConfigT  *entityConfig;
}clAmsMgmtEntityGetConfigResponseT;

typedef struct
{
    ClAmsMgmtHandleT  handle;
    ClAmsEntityT  entity;
}clAmsMgmtEntityGetStatusRequestT;

typedef struct
{
    ClAmsEntityT  entity;
    ClAmsEntityStatusT  *entityStatus;
}clAmsMgmtEntityGetStatusResponseT;

typedef struct
{
    ClUint32T  count;
    ClAmsCSINVPT  *nvp;
}ClAmsCSINVPBufferT;

typedef struct
{
    ClAmsMgmtHandleT  handle;
    ClAmsEntityT  csi;
}clAmsMgmtGetCSINVPListRequestT;

typedef ClAmsCSINVPBufferT clAmsMgmtGetCSINVPListResponseT;

typedef struct
{
    ClUint32T  count;
    ClAmsEntityT  *entity;
}ClAmsEntityBufferT;

typedef struct
{
    ClUint32T  count;
    ClAmsEntityRefT  *entityRef;
}ClAmsEntityRefBufferT;

typedef struct
{
    ClUint32T  count;
    ClAmsSUSIRefT  *entityRef;
}ClAmsSUSIRefBufferT;

typedef struct
{
    ClUint32T  count;
    ClAmsSUSIExtendedRefT  *entityRef;
}ClAmsSUSIExtendedRefBufferT;

typedef struct
{
    ClUint32T  count;
    ClAmsSISURefT  *entityRef;
}ClAmsSISURefBufferT;

typedef struct
{
    ClUint32T  count;
    ClAmsSISUExtendedRefT  *entityRef;
}ClAmsSISUExtendedRefBufferT;

typedef struct
{
    ClUint32T  count;
    ClAmsCompCSIRefT  *entityRef;
}ClAmsCompCSIRefBufferT;

typedef struct
{
    ClAmsMgmtHandleT  handle;
    ClAmsEntityT  entity;
    ClAmsEntityListTypeT  entityListName;
}clAmsMgmtGetEntityListRequestT;

typedef struct ClAmsMgmtMigrateList
{
    ClAmsEntityBufferT si;
    ClAmsEntityBufferT csi;
    ClAmsEntityBufferT node;
    ClAmsEntityBufferT su;
    ClAmsEntityBufferT comp;
}ClAmsMgmtMigrateListT;

typedef struct ClAmsMgmtMigrateRequest
{
    ClNameT sg;
    ClNameT prefix;
    ClUint32T activeSUs;
    ClUint32T standbySUs;
}ClAmsMgmtMigrateRequestT;

typedef struct ClAmsMgmtMigrateResponse
{
    ClAmsMgmtMigrateListT migrateList;
}ClAmsMgmtMigrateResponseT;

typedef struct ClAmsMgmtUserDataSetRequest
{
    ClAmsEntityT *entity;
    ClNameT *key;
    ClCharT *data;
    ClUint32T len;
}ClAmsMgmtUserDataSetRequestT;

typedef struct ClAmsMgmtUserDataGetRequest
{
    ClAmsEntityT *entity;
    ClNameT *key;
    ClCharT **data;
    ClUint32T *len;
}ClAmsMgmtUserDataGetRequestT;

typedef struct ClAmsMgmtUserDataDeleteRequest
{
    ClAmsEntityT *entity;
    ClNameT *key;
    ClBoolT clear;
}ClAmsMgmtUserDataDeleteRequestT;

typedef struct ClAmsMgmtSIAssignSUCustomRequest
{
    ClAmsEntityT si;
    ClAmsEntityT activeSU;
    ClAmsEntityT standbySU;
}ClAmsMgmtSIAssignSUCustomRequestT;

typedef struct ClAmsMgmtDBGetResponse
{
    ClUint32T len;
    ClUint8T *buffer;
}ClAmsMgmtDBGetResponseT;

typedef struct ClAmsMgmtCASGetRequest
{
    ClAmsEntityT entity;
    ClAmsAdminStateT computedAdminState;
}ClAmsMgmtCASGetRequestT;

typedef ClAmsEntityBufferT clAmsMgmtGetEntityListResponseT;
typedef clAmsMgmtGetEntityListRequestT clAmsMgmtGetOLEntityListRequestT;
typedef ClAmsEntityRefBufferT clAmsMgmtGetOLEntityListResponseT;
/*
 * Entity configuration attributes
 */

#define CL_AMS_CONFIG_ATTR_ALL                      1 


#define NODE_CONFIG_ADMIN_STATE                     CL_AMS_CONFIG_ATTR_ALL<<1
#define NODE_CONFIG_ID                              CL_AMS_CONFIG_ATTR_ALL<<2
#define NODE_CONFIG_CLASS_TYPE                      CL_AMS_CONFIG_ATTR_ALL<<3
#define NODE_CONFIG_SUB_CLASS_TYPE                  CL_AMS_CONFIG_ATTR_ALL<<4
#define NODE_CONFIG_IS_SWAPPABLE                    CL_AMS_CONFIG_ATTR_ALL<<5
#define NODE_CONFIG_IS_RESTARTABLE                  CL_AMS_CONFIG_ATTR_ALL<<6
#define NODE_CONFIG_AUTO_REPAIR                     CL_AMS_CONFIG_ATTR_ALL<<7
#define NODE_CONFIG_IS_ASP_AWARE                   CL_AMS_CONFIG_ATTR_ALL<<8
#define NODE_CONFIG_SU_FAILOVER_DURATION            CL_AMS_CONFIG_ATTR_ALL<<9
#define NODE_CONFIG_SU_FAILOVER_COUNT_MAX           CL_AMS_CONFIG_ATTR_ALL<<10 

#define SG_CONFIG_ADMIN_STATE                       CL_AMS_CONFIG_ATTR_ALL<<1
#define SG_CONFIG_REDUNDANCY_MODEL                  CL_AMS_CONFIG_ATTR_ALL<<2
#define SG_CONFIG_LOADING_STRATEGY                  CL_AMS_CONFIG_ATTR_ALL<<3
#define SG_CONFIG_FAILBACK_OPTION                   CL_AMS_CONFIG_ATTR_ALL<<4
#define SG_CONFIG_AUTO_REPAIR                       CL_AMS_CONFIG_ATTR_ALL<<5
#define SG_CONFIG_INSTANTIATE_DURATION              CL_AMS_CONFIG_ATTR_ALL<<6
#define SG_CONFIG_NUM_PREF_ACTIVE_SUS               CL_AMS_CONFIG_ATTR_ALL<<7
#define SG_CONFIG_NUM_PREF_STANDBY_SUS              CL_AMS_CONFIG_ATTR_ALL<<8
#define SG_CONFIG_NUM_PREF_INSERVICE_SUS            CL_AMS_CONFIG_ATTR_ALL<<9
#define SG_CONFIG_NUM_PREF_ASSIGNED_SUS             CL_AMS_CONFIG_ATTR_ALL<<10 
#define SG_CONFIG_NUM_PREF_ACTIVE_SUS_PER_SI        CL_AMS_CONFIG_ATTR_ALL<<11 
#define SG_CONFIG_MAX_ACTIVE_SIS_PER_SU             CL_AMS_CONFIG_ATTR_ALL<<12 
#define SG_CONFIG_MAX_STANDBY_SIS_PER_SU            CL_AMS_CONFIG_ATTR_ALL<<13 
#define SG_CONFIG_COMP_RESTART_DURATION             CL_AMS_CONFIG_ATTR_ALL<<14 
#define SG_CONFIG_COMP_RESTART_COUNT_MAX            CL_AMS_CONFIG_ATTR_ALL<<15 
#define SG_CONFIG_SU_RESTART_DURATION               CL_AMS_CONFIG_ATTR_ALL<<16 
#define SG_CONFIG_SU_RESTART_COUNT_MAX              CL_AMS_CONFIG_ATTR_ALL<<17 
#define SG_CONFIG_REDUCTION_PROCEDURE               CL_AMS_CONFIG_ATTR_ALL<<18
#define SG_CONFIG_COLOCATION_ALLOWED                CL_AMS_CONFIG_ATTR_ALL<<19
#define SG_CONFIG_AUTO_ADJUST                       CL_AMS_CONFIG_ATTR_ALL<<20
#define SG_CONFIG_AUTO_ADJUST_PROBATION             CL_AMS_CONFIG_ATTR_ALL<<21
#define SG_CONFIG_ALPHA_FACTOR                      CL_AMS_CONFIG_ATTR_ALL<<22
#define SG_CONFIG_MAX_FAILOVERS                     CL_AMS_CONFIG_ATTR_ALL<<23
#define SG_CONFIG_FAILOVER_DURATION                 CL_AMS_CONFIG_ATTR_ALL<<24
#define SG_CONFIG_BETA_FACTOR                       CL_AMS_CONFIG_ATTR_ALL<<25

#define SU_CONFIG_ADMIN_STATE                       CL_AMS_CONFIG_ATTR_ALL<<1
#define SU_CONFIG_RANK                              CL_AMS_CONFIG_ATTR_ALL<<2
#define SU_CONFIG_NUM_COMPONENTS                    CL_AMS_CONFIG_ATTR_ALL<<3
#define SU_CONFIG_IS_PREINSTANTIABLE                CL_AMS_CONFIG_ATTR_ALL<<4
#define SU_CONFIG_IS_RESTARTABLE                    CL_AMS_CONFIG_ATTR_ALL<<5
#define SU_CONFIG_IS_CONTAINER_SU                   CL_AMS_CONFIG_ATTR_ALL<<6

#define SI_CONFIG_ADMIN_STATE                       CL_AMS_CONFIG_ATTR_ALL<<1
#define SI_CONFIG_RANK                              CL_AMS_CONFIG_ATTR_ALL<<2
#define SI_CONFIG_NUM_CSIS                          CL_AMS_CONFIG_ATTR_ALL<<3
#define SI_CONFIG_NUM_STANDBY_ASSIGNMENTS           CL_AMS_CONFIG_ATTR_ALL<<4
#define SI_CONFIG_STANDBY_ASSIGNMENT_ORDER          CL_AMS_CONFIG_ATTR_ALL<<5

#define COMP_CONFIG_SUPPORTED_CSI_TYPE              CL_AMS_CONFIG_ATTR_ALL<<1 
#define COMP_CONFIG_PROXY_CSI_TYPE                  CL_AMS_CONFIG_ATTR_ALL<<2 
#define COMP_CONFIG_CAPABILITY_MODEL                CL_AMS_CONFIG_ATTR_ALL<<3 
#define COMP_CONFIG_PROPERTY                        CL_AMS_CONFIG_ATTR_ALL<<4 
#define COMP_CONFIG_IS_RESTARTABLE                  CL_AMS_CONFIG_ATTR_ALL<<5 
#define COMP_CONFIG_NODE_REBOOT_CLEANUP_FAIL        CL_AMS_CONFIG_ATTR_ALL<<6 
#define COMP_CONFIG_INSTANTIATE_LEVEL               CL_AMS_CONFIG_ATTR_ALL<<7 
#define COMP_CONFIG_NUM_MAX_INSTANTIATE             CL_AMS_CONFIG_ATTR_ALL<<8 
#define COMP_CONFIG_NUM_MAX_INSTANTIATE_WITH_DELAY  CL_AMS_CONFIG_ATTR_ALL<<9 
#define COMP_CONFIG_NUM_MAX_TERMINATE               CL_AMS_CONFIG_ATTR_ALL<<10 
#define COMP_CONFIG_NUM_MAX_AM_START                CL_AMS_CONFIG_ATTR_ALL<<11 
#define COMP_CONFIG_NUM_MAX_AM_STOP                 CL_AMS_CONFIG_ATTR_ALL<<12 
#define COMP_CONFIG_NUM_MAX_ACTIVE_CSIS             CL_AMS_CONFIG_ATTR_ALL<<13 
#define COMP_CONFIG_NUM_MAX_STANDBY_CSIS            CL_AMS_CONFIG_ATTR_ALL<<14 
#define COMP_CONFIG_TIMEOUTS                        CL_AMS_CONFIG_ATTR_ALL<<15 
#define COMP_CONFIG_RECOVERY_ON_TIMEOUT             CL_AMS_CONFIG_ATTR_ALL<<16 
#define COMP_CONFIG_PARENT_SU                       CL_AMS_CONFIG_ATTR_ALL<<17
#define COMP_CONFIG_INSTANTIATE_COMMAND             CL_AMS_CONFIG_ATTR_ALL<<18

#define CSI_CONFIG_TYPE                             CL_AMS_CONFIG_ATTR_ALL<<1
#define CSI_CONFIG_IS_PROXY_CSI                     CL_AMS_CONFIG_ATTR_ALL<<2
#define CSI_CONFIG_RANK                             CL_AMS_CONFIG_ATTR_ALL<<3

#ifdef  __cplusplus
}
#endif

#endif  /* _CL_AMS_MGMT_COMMON_H_ */

