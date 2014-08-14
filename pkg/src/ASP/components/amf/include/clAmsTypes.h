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
 * ModuleName  : amf
 * File        : clAmsTypes.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file contains types that are exposed to AMS client, event 
 * and management API users. Some types conform to SAF specs while others
 * are ASP proprietary.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_TYPES_H_
#define _CL_AMS_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>

/******************************************************************************
 * Different types of states associated with AMS entities. Note, each AMS
 * entity need not have all of the following states.
 *****************************************************************************/

/*
 * The management state is defined for all AMS entities. It is akin to the
 * rowStatus attribute in the AMF MIB and is exposed via the AMS management
 * API.
 */

typedef enum
{
    CL_AMS_MGMT_STATE_NONE                      = 0,
    CL_AMS_MGMT_STATE_DISABLED                  = 1,
    CL_AMS_MGMT_STATE_ENABLED                   = 2,
} ClAmsMgmtStateT;

/*
 * The admin state is defined for node, application, sg, su and si entities. 
 * It is exposed via the ams management api and managed by the management
 * entities.
 *
 */

typedef enum
{
    CL_AMS_ADMIN_STATE_NONE                     = 0,
    CL_AMS_ADMIN_STATE_UNLOCKED                 = 1,
    CL_AMS_ADMIN_STATE_LOCKED_A                 = 2,
    CL_AMS_ADMIN_STATE_LOCKED_I                 = 3,
    CL_AMS_ADMIN_STATE_SHUTTINGDOWN             = 4,
    CL_AMS_ADMIN_STATE_SHUTTINGDOWN_RESTART     = 5,
    CL_AMS_ADMIN_STATE_MAX,
} ClAmsAdminStateT;

/*
 * The presence state is defined for node, su and component entities. It is
 * exposed via the AMS event and management apis. Note, the SAF specs do not
 * define presence state for node entities.
 */

typedef enum
{
    CL_AMS_PRESENCE_STATE_NONE                  = 0,
    CL_AMS_PRESENCE_STATE_UNINSTANTIATED        = 1,
    CL_AMS_PRESENCE_STATE_INSTANTIATING         = 2,
    CL_AMS_PRESENCE_STATE_INSTANTIATED          = 3,
    CL_AMS_PRESENCE_STATE_TERMINATING           = 4,
    CL_AMS_PRESENCE_STATE_RESTARTING            = 5,
    CL_AMS_PRESENCE_STATE_INSTANTIATION_FAILED  = 6,
    CL_AMS_PRESENCE_STATE_TERMINATION_FAILED    = 7,

    // The following are not defined by SAF

    CL_AMS_PRESENCE_STATE_ABSENT                = 0,
    CL_AMS_PRESENCE_STATE_FAULT                 = 8,
    CL_AMS_PRESENCE_STATE_FAULT_WTC             = 9,
    CL_AMS_PRESENCE_STATE_FAULT_WTR             = 10
} ClAmsPresenceStateT;

/*
 * The operational state is defined for node, su and component entities.
 * It is managed by AMS and exposed via the AMS management api.
 */

typedef enum
{
    CL_AMS_OPER_STATE_NONE                      = 0,
    CL_AMS_OPER_STATE_ENABLED                   = 1,
    CL_AMS_OPER_STATE_DISABLED                  = 2
} ClAmsOperStateT;

/*
 * The ha state is defined for the si and csi entities. It is exposed via
 * the AMS client and management apis.
 */
 
typedef enum
{
    CL_AMS_HA_STATE_NONE                        = 0,
    CL_AMS_HA_STATE_ACTIVE                      = 1,
    CL_AMS_HA_STATE_STANDBY                     = 2,
    CL_AMS_HA_STATE_QUIESCED                    = 3,
    CL_AMS_HA_STATE_QUIESCING                   = 4
} ClAmsHAStateT;

/*
 * The readiness state is defined for su and component entities. It is
 * exposed via the ams management api. It is not exposed via the client
 * api.
 */
 
typedef enum
{
    CL_AMS_READINESS_STATE_NONE                 = 0,
    CL_AMS_READINESS_STATE_OUTOFSERVICE         = 1,
    CL_AMS_READINESS_STATE_INSERVICE            = 2,
    CL_AMS_READINESS_STATE_STOPPING             = 3
} ClAmsReadinessStateT;

/*
 * The service state is defined for ams as a whole. It is exposed via
 * the management api.
 */
 
typedef enum 
{
    CL_AMS_SERVICE_STATE_NONE                   = 0,
    CL_AMS_SERVICE_STATE_RUNNING                = 1,
    CL_AMS_SERVICE_STATE_STOPPED                = 2,
    CL_AMS_SERVICE_STATE_STARTINGUP             = 3,
    CL_AMS_SERVICE_STATE_SHUTTINGDOWN           = 4,
    CL_AMS_SERVICE_STATE_UNAVAILABLE            = 5,
    CL_AMS_SERVICE_STATE_HOT_STANDBY            = 6,
} ClAmsServiceStateT;

/******************************************************************************
 * Node related types
 *****************************************************************************/

/*
 * Classes of nodes known to AMS as defined in ASP HLA. Use of this
 * information is RC2+.
 */

typedef enum
{
    CL_AMS_NODE_CLASS_NONE                      = 0,
    CL_AMS_NODE_CLASS_A                         = 1,
    CL_AMS_NODE_CLASS_B                         = 2,
    CL_AMS_NODE_CLASS_C                         = 3,
    CL_AMS_NODE_CLASS_D                         = 4,
    CL_AMS_NODE_CLASS_MAX,
} ClAmsNodeClassT;

/*
 * Cluster Membership Status
 */

typedef enum
{
    CL_AMS_NODE_IS_NOT_CLUSTER_MEMBER           = 0,
    CL_AMS_NODE_IS_CLUSTER_MEMBER               = 1,
    CL_AMS_NODE_IS_LEAVING_CLUSTER              = 2,
} ClAmsNodeClusterMemberT;

/******************************************************************************
 * Application related types
 *****************************************************************************/

/* nothing for now, to be added in RC2+ */
 
/******************************************************************************
 * Service Group related types
 *****************************************************************************/

/*
 * Types of redundancy models
 */

typedef enum
{
    CL_AMS_SG_REDUNDANCY_MODEL_NONE             = 0,
    CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY    = 1,
    CL_AMS_SG_REDUNDANCY_MODEL_TWO_N            = 2,
    CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N         = 3,
    CL_AMS_SG_REDUNDANCY_MODEL_N_WAY            = 4,
    CL_AMS_SG_REDUNDANCY_MODEL_N_WAY_ACTIVE     = 5,
    CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM           = 6, /*user controlled redundancy mode*/
    CL_AMS_SG_REDUNDANCY_MODEL_MAX,
} ClAmsSGRedundancyModelT;

/*
 * Loading strategy to use when deciding how to assign SIs to SUs.
 *
 * LEAST_SI_PER_SU (M+N)
 *
 * In this strategy, the assignment algorithm tries to maximize the 
 * number of SUs assigned active and standby assignments, thus 
 * resulting in the least possible SI assignments per SU. This 
 * strategy is meaningful for the M+N redundancy model. (2N is a 
 * trivial case). This is the default strategy specified in the
 * AMF specification.
 *
 * LEAST_SU_ASSIGNED (M+N)
 *
 * In this strategy, the assignment algorithm tries to minimize the
 * number of SUs that are assigned the active or standby SIs. This
 * strategy is meaningful for the M+N redundancy model (2N is a
 * trivial case) and is useful when assigning more SUs is costlier 
 * than adding new SIs to existing SUs and it is acceptable not to
 * have the additional resiliency provided by not having the max
 * possible number of SUs assigned.
 *
 * LEAST_LOAD_PER_SU (M+N, N-Way)
 *
 * In this strategy, the assignment algorithm picks the SU with the
 * lowest load to be the next candidate. The definition of load is
 * intended to be customizable for a particular deployment.
 *
 * BY_SI_PREFERENCE (M+N, N-Way)
 *
 * In this strategy, the assignment algorithm tries to assign the
 * SIs to the SUs based on the preference list associated with each
 * SI. SIs with higher rank have first dibs. The preference is only 
 * for the active assignment and if the preferred nodes are not 
 * available, the algorithm defaults to LEAST_SI_PER_SU.
 *
 * USER_DEFINED (M+N, N-Way)
 *
 * This is intended to be an user customizable strategy that is
 * plugged in during system deployment. (RC2+)
 *
 */

typedef enum
{
    CL_AMS_SG_LOADING_STRATEGY_NONE               = 0,  /* invalid           */
    CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU    = 1,  /* all models        */
    CL_AMS_SG_LOADING_STRATEGY_LEAST_SU_ASSIGNED  = 2,  /* all models        */
    CL_AMS_SG_LOADING_STRATEGY_LEAST_LOAD_PER_SU  = 3,  /* all models        */
    CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE   = 4,  /* n-way-* only      */
    CL_AMS_SG_LOADING_STRATEGY_USER_DEFINED       = 5,   /* user-callout      */
    CL_AMS_SG_LOADING_STRATEGY_MAX,               
} ClAmsSGLoadingStrategyT;

/******************************************************************************
 * Component related types
 *****************************************************************************/

/*
 * Types of component capabilities. These determine how many active and
 * standby CSIs can be assigned to a component.
 */
 
typedef enum
{
    CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY      = 1,
    CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY       = 2,
    CL_AMS_COMP_CAP_ONE_ACTIVE_OR_X_STANDBY     = 3,
    CL_AMS_COMP_CAP_ONE_ACTIVE_OR_ONE_STANDBY   = 4,
    CL_AMS_COMP_CAP_X_ACTIVE                    = 5,
    CL_AMS_COMP_CAP_ONE_ACTIVE                  = 6,
    CL_AMS_COMP_CAP_NON_PREINSTANTIABLE         = 7,
    CL_AMS_COMP_CAP_MAX,
} ClAmsCompCapModelT;

#define CL_AMS_COMP_CAP_ONE_HA_STATE(X)                         \
{                                                               \
    ( ( ((X) == CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY)     ||   \
        ((X) == CL_AMS_COMP_CAP_ONE_ACTIVE_OR_ONE_STANDBY) ||   \
        ((X) == CL_AMS_COMP_CAP_ONE_ACTIVE_OR_X_STANDBY) ) ?    \
        CL_TRUE : CL_FALSE )                                    \
}

/*
 * Types of component properties.
 */

typedef enum
{
    CL_AMS_COMP_PROPERTY_SA_AWARE                           = 1,
    CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE            = 2,
    CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE        = 3,
    CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE    = 4,
    CL_AMS_COMP_PROPERTY_MAX,
} ClAmsCompPropertyT;

/*
 * Types of health checks performed by the AMF.
 */

typedef enum
{
    CL_AMS_COMP_HEALTHCHECK_AMF_INVOKED         = 1,
    CL_AMS_COMP_HEALTHCHECK_CLIENT_INVOKED      = 2
} ClAmsCompHealthcheckInvocationT;

#define CL_AMS_HEALTHCHECK_KEY_MAX 32
typedef struct
{
    ClUint8T       key[CL_AMS_HEALTHCHECK_KEY_MAX];
    ClUint16T      keyLen;
} ClAmsCompHealthcheckKeyT;

typedef enum
{
    /*
     * Recommended recovery actions for local components as defined by SAF
     */

    CL_AMS_RECOVERY_NONE                        = 0,
    CL_AMS_RECOVERY_NO_RECOMMENDATION           = 1,
    CL_AMS_RECOVERY_COMP_RESTART                = 2,
    CL_AMS_RECOVERY_COMP_FAILOVER               = 3,
    CL_AMS_RECOVERY_NODE_SWITCHOVER             = 4,
    CL_AMS_RECOVERY_NODE_FAILOVER               = 5,
    CL_AMS_RECOVERY_NODE_FAILFAST               = 6,
    CL_AMS_RECOVERY_CLUSTER_RESET               = 7,
    CL_AMS_RECOVERY_APP_RESTART                 = 8,

    /*
     * Recommeded recovery actions defined by Clovis
     */

    CL_AMS_RECOVERY_INTERNALLY_RECOVERED        = 20,
    CL_AMS_RECOVERY_SU_RESTART                  = 21,
    CL_AMS_RECOVERY_NODE_HALT                   = 22,

    /*
     * Recommended recovery actions for external components
     */

    CL_AMS_EXTERNAL_RECOVERY_RESET              = 30,
    CL_AMS_EXTERNAL_RECOVERY_REBOOT             = 31,
    CL_AMS_EXTERNAL_RECOVERY_POWER_ON           = 32,
    CL_AMS_EXTERNAL_RECOVERY_POWER_OFF          = 33,
    
} ClAmsRecoveryT;

typedef ClAmsRecoveryT ClAmsLocalRecoveryT;

/******************************************************************************
 * Component Service Instance related types
 *****************************************************************************/

#define CL_AMS_CSI_FLAG_ADD_ONE                 0x1
#define CL_AMS_CSI_FLAG_TARGET_ONE              0x2
#define CL_AMS_CSI_FLAG_TARGET_ALL              0x4

typedef ClUint32T   ClAmsCSIFlagsT;
typedef ClNameT     ClAmsCSITypeT;

typedef enum
{
    CL_AMS_CSI_NEW_ASSIGN                       = 1,
    CL_AMS_CSI_QUIESCED                         = 2,
    CL_AMS_CSI_NOT_QUIESCED                     = 3,
    CL_AMS_CSI_STILL_ACTIVE                     = 4
} ClAmsCSITransitionDescriptorT;

typedef struct
{
    ClAmsCSITransitionDescriptorT   transitionDescriptor;
    ClNameT                         activeCompName;
} ClAmsCSIActiveDescriptorT;

typedef struct
{
    ClNameT                         activeCompName;
    ClUint32T                       standbyRank;
} ClAmsCSIStandbyDescriptorT;

typedef union
{
    ClAmsCSIActiveDescriptorT       activeDescriptor;
    ClAmsCSIStandbyDescriptorT      standbyDescriptor;
} ClAmsCSIStateDescriptorT;

typedef struct
{
    /* As per the SAF AMF spec:
       These are UTF-8 encoded, null terminated strings.  UTF-8 is the same 
       as 7 bit (printable) ASCII, but the high bit indicates UTF-8 encoding.
       Normal C strxxx() functions will work if you cast these to char*.
    */
    ClUint8T                        *attributeName;
    ClUint8T                        *attributeValue;
} ClAmsCSIAttributeT;

typedef struct
{
    ClAmsCSIAttributeT              *attribute;
    ClUint32T                       numAttributes;
} ClAmsCSIAttributeListT;

typedef struct
{
    ClAmsCSIFlagsT                  csiFlags;
    ClNameT                         csiName;
    ClAmsCSIStateDescriptorT        csiStateDescriptor;
    ClAmsCSIAttributeListT          csiAttributeList;
} ClAmsCSIDescriptorT;

typedef struct
{
    ClAmsCSIDescriptorT             csiDescriptor;
    ClAmsCSITypeT                   csiType;
    ClNameT                         compName;
}ClAmsCSITypeDescriptorT;

/******************************************************************************
 * Protection group related types
 *****************************************************************************/

typedef enum
{
    CL_AMS_PG_TRACK_CURRENT                     = 1,
    CL_AMS_PG_TRACK_CHANGES                     = 2,
    CL_AMS_PG_TRACK_CHANGES_ONLY                = 4
} ClAmsPGTrackFlagT;

typedef enum
{
    CL_AMS_PG_NO_CHANGE                         = 1,
    CL_AMS_PG_ADDED                             = 2,
    CL_AMS_PG_REMOVED                           = 3,
    CL_AMS_PG_STATE_CHANGE                      = 4
} ClAmsPGChangeT;

typedef struct
{
    ClNameT                         compName;   /* component for CSI         */
    ClAmsHAStateT                   haState;    /* ha state for CSI          */
    ClUint32T                       rank;       /* rank, if state is standby */
} ClAmsPGMemberT;

typedef struct
{
    ClAmsPGMemberT                  member;
    ClAmsPGChangeT                  change;
} ClAmsPGNotificationT;

typedef struct
{
    ClUint32T                       numItems;
    ClAmsPGNotificationT            *notification;
} ClAmsPGNotificationBufferT;

/******************************************************************************
 * Types for handles used by various AMS APIs
 *****************************************************************************/
typedef ClHandleT                   ClAmsMgmtCCBHandleT;
typedef ClHandleT                   ClAmsMgmtHandleT;
typedef ClHandleT                   ClAmsClientHandleT;
typedef ClHandleT                   ClAmsEventHandleT;
typedef ClHandleT                   ClAmsFaultHandleT;
typedef ClHandleT                   ClAmsEntityHandleT;
typedef ClPtrT                      ClAmsMgmtDBHandleT;
typedef ClPtrT                      ClAmsMgmtCCBBatchHandleT;

/******************************************************************************
 * Temporary Notes
 *****************************************************************************/

// this is not defined in saAmf.h

typedef struct
{
    ClUint32T                  numberOfItems;
    ClAmsCSITypeDescriptorT    *csiDefinition;
}ClAmsSIDescriptorT;



#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_TYPES_H_ */
