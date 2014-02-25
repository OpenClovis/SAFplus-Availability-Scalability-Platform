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
 * File        : clAmsEntities.h
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * This header file defines the various entities that are tracked by AMS.
 * These definitions are exposed via the Management API. This file is Clovis 
 * internal if the management API is not directly exposed to a ASP customer. 
 * However, if the management API is exposed to a ASP customer, this file is 
 * not Clovis internal.
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/
 
#ifndef _CL_AMS_ENTITIES_H_
#define _CL_AMS_ENTITIES_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clCommon.h>
#include <clCntApi.h>
#include <clTimerApi.h>
#include <clIocApi.h>

#include <clAmsTypes.h>
#include <clAmsSAClientApi.h>
#include <clCpmApi.h>
#include <clList.h>

/******************************************************************************
 * AMS Entities
 *
 * AMS keeps track of various entities such as Cluster Node, Application,
 * SG, SU, SI, Component and CSI.
 *
 * Each AMS entity has an associated object of the form ClAms<Entity>T. The
 * object is further divided into config and status objects of the form
 * ClAmsEntityConfigT and ClAmsEntityStatusT. The aim of this separation is
 * three fold: (1) to clearly separate out attributes that are configurable
 * from those that change as a result of AMS activity (2) to simply certain
 * instantiation, termination and modification operations (3) make it easier
 * in future to checkpoint only status.
 *
 *****************************************************************************/

/**
 * This enumeration defines all of the types of entities the AMF manages
 */
typedef enum
{
    CL_AMS_ENTITY_TYPE_ENTITY       = 0, /**< unused */
    CL_AMS_ENTITY_TYPE_NODE         = 1, /**< A Cluster Node (system,computer) */
    CL_AMS_ENTITY_TYPE_APP          = 2, /**< A SAF application */
    CL_AMS_ENTITY_TYPE_SG           = 3, /**< A SAF service group */
    CL_AMS_ENTITY_TYPE_SU           = 4, /**< A SAF service unit */
    CL_AMS_ENTITY_TYPE_SI           = 5, /**< A SAF service instance (work assignment) */
    CL_AMS_ENTITY_TYPE_COMP         = 6, /**< A SAF component (program) */
    CL_AMS_ENTITY_TYPE_CSI          = 7, /**< A SAF component service instance (work assigned to a particular program) */
    CL_AMS_ENTITY_TYPE_CLUSTER      = 8, /**< A cluster */
} ClAmsEntityTypeT;

#define CL_AMS_ENTITY_TYPE_MAX      7   // not ready for cluster yet

/******************************************************************************
 * Entity Operations
 *****************************************************************************/

typedef struct ClAmsEntityOpStack
{
    ClInt32T numOps;
    ClListHeadT opList;
} ClAmsEntityOpStackT;

/******************************************************************************
 * AMS Entity - Generic Base Class
 *****************************************************************************/

/** This structure represents a reference to an AMF entity
 */
typedef struct
{
    ClAmsEntityTypeT        type;                   /**< Type of entity (SG,SU,SI,CSI, etc) */
    ClNameT                 name;                   /**< unique name of entity */
    ClUint8T                debugFlags;             /* debug sub area flags  */

#if defined (CL_AMS_MGMT_HOOKS)
    ClUint64T               hookId;
#endif

} ClAmsEntityConfigT;

/** This structure is the common fields in all AMF entities' status
 */
typedef struct
{
    ClTimeT                 epoch;                  /**< time when started     */
    ClUint32T               timerCount;             /**< # timers running      */
    ClAmsEntityOpStackT     opStack;
} ClAmsEntityStatusT;

/** This structure represents a reference to an AMF entity
 */
typedef ClAmsEntityConfigT  ClAmsEntityT;

typedef ClRcT (*ClAmsEntityCallbackT)(ClAmsEntityT *);
typedef ClRcT (*ClAmsEntityCallbackExtendedT)(ClAmsEntityT *, ClPtrT userArg);

typedef struct
{
    ClAmsEntityCallbackT    printOut;
    ClAmsEntityCallbackT    validateConfig;
    ClAmsEntityCallbackT    validateRelationships;
} ClAmsEntityMethodsT;


typedef struct ClAmsEntityOp
{
#define CL_AMS_ENTITY_OP_REMOVE_MPLUSN                0x1
#define CL_AMS_ENTITY_OP_SWAP_REMOVE_MPLUSN           0x2
#define CL_AMS_ENTITY_OP_REDUCE_REMOVE_MPLUSN         0x4
#define CL_AMS_ENTITY_OP_ACTIVE_REMOVE_MPLUSN         0x8

#define CL_AMS_ENTITY_OP_REMOVES_MPLUSN               (CL_AMS_ENTITY_OP_REMOVE_MPLUSN | \
                                                       CL_AMS_ENTITY_OP_SWAP_REMOVE_MPLUSN   |\
                                                       CL_AMS_ENTITY_OP_REDUCE_REMOVE_MPLUSN )

#define CL_AMS_ENTITY_OP_SWAP_ACTIVE_MPLUSN           0x10
#define CL_AMS_ENTITY_OP_SI_REASSIGN_MPLUSN           0x20
#define CL_AMS_ENTITY_OP_ACTIVE_REMOVE_REF_MPLUSN     0x40
    ClUint32T op;
    ClUint32T dataSize;
    void *data;
    ClListHeadT list;
} ClAmsEntityOpT;

typedef struct ClAmsEntityRemoveOp
{
    ClAmsEntityT entity;
    ClInt32T sisRemoved;
    ClUint32T switchoverMode;
    ClUint32T error;
}ClAmsEntityRemoveOpT;

typedef struct ClAmsEntitySwapRemoveOp
{
    ClAmsEntityT entity; /*standby SU pending other SI removal*/
    ClInt32T sisRemoved; /*pending extended SI removals*/
    ClUint32T numOtherSIs;
    ClAmsEntityT *otherSIs; /*pending list of other SIs awaiting removal*/
} ClAmsEntitySwapRemoveOpT;

typedef struct ClAmsEntityReduceRemoveOp
{
    ClInt32T sisRemoved;
}ClAmsEntityReduceRemoveOpT;

typedef struct ClAmsEntitySwapActiveOp
{
    ClInt32T sisReassigned;
} ClAmsEntitySwapActiveOpT;

typedef struct ClAmsSIReassignOp
{
    ClAmsEntityT su;
}ClAmsSIReassignOpT;


/******************************************************************************
 * Timers for entities
 *****************************************************************************/

#define CL_AMS_SG_ADJUST_DURATION (3000) /* For auto adjust in millisecs*/
#define CL_AMS_SG_ADJUST_PROBATION CL_AMS_SG_ADJUST_DURATION
#define CL_AMS_SU_ASSIGNMENT_DELAY (3000) /* For SI preference only*/
typedef enum
{
    CL_AMS_NODE_TIMER_SUFAILOVER                        = 1,

    CL_AMS_SG_TIMER_INSTANTIATE                         = 10,
    CL_AMS_SG_TIMER_ADJUST                              = 11,
    CL_AMS_SG_TIMER_ADJUST_PROBATION                    = 12,
    CL_AMS_SU_TIMER_COMPRESTART                         = 20,
    CL_AMS_SU_TIMER_SURESTART                           = 21,
    CL_AMS_SU_TIMER_PROBATION                           = 22,
    CL_AMS_SU_TIMER_ASSIGNMENT                          = 23,

    CL_AMS_COMP_TIMER_INSTANTIATE                       = 40,
    CL_AMS_COMP_TIMER_TERMINATE                         = 41,
    CL_AMS_COMP_TIMER_CLEANUP                           = 42,
    CL_AMS_COMP_TIMER_AMSTART                           = 43,
    CL_AMS_COMP_TIMER_AMSTOP                            = 44,
    CL_AMS_COMP_TIMER_QUIESCINGCOMPLETE                 = 45,
    CL_AMS_COMP_TIMER_CSISET                            = 46,
    CL_AMS_COMP_TIMER_CSIREMOVE                         = 47,
    CL_AMS_COMP_TIMER_PROXIEDCOMPINSTANTIATE            = 48,
    CL_AMS_COMP_TIMER_PROXIEDCOMPCLEANUP                = 49,
    CL_AMS_COMP_TIMER_INSTANTIATEDELAY                  = 50,
    CL_AMS_COMP_TIMER_MAX,
} ClAmsEntityTimerTypeT;

typedef struct
{
    ClAmsEntityTimerTypeT   type;                   /* timer type            */
    ClTimeT                 count;                  /* num of times started  */
    ClTimerHandleT          handle;                 /* osal timer handle     */
    ClAmsEntityT            *entity;                /* timer is for entity   */

    ClUint32T               currentOp;              /* started by op id      */

} ClAmsEntityTimerT;

typedef ClRcT (*ClAmsEntityTimerCallbackT)(ClAmsEntityTimerT *);

/******************************************************************************
 * References to entities
 *****************************************************************************/

typedef struct
{
    ClAmsEntityT            entity;                 /* type/name of target   */
    ClAmsEntityT            *ptr;                   /* ptr to target         */
    ClCntNodeHandleT        nodeHandle;             /* target's cnt handle   */
} ClAmsEntityRefT;


typedef enum
{
    CL_AMS_ENTITY_REF_TYPE_ENTITY   = 0,
    CL_AMS_ENTITY_REF_TYPE_NODE     = 1,
    CL_AMS_ENTITY_REF_TYPE_APP      = 2,
    CL_AMS_ENTITY_REF_TYPE_SG       = 3,
    CL_AMS_ENTITY_REF_TYPE_SU       = 4,
    CL_AMS_ENTITY_REF_TYPE_SI       = 5,
    CL_AMS_ENTITY_REF_TYPE_COMP     = 6,
    CL_AMS_ENTITY_REF_TYPE_CSI      = 7,
    CL_AMS_ENTITY_REF_TYPE_SUSI     = 8,
    CL_AMS_ENTITY_REF_TYPE_SISU     = 9,
    CL_AMS_ENTITY_REF_TYPE_COMPCSI  = 10,
    CL_AMS_ENTITY_REF_TYPE_CSICOMP  = 11,
} ClAmsEntityRefTypeT;

#define CL_AMS_ENTITY_REF_TYPE_MAX      11

/******************************************************************************
 * A list of entity references
 *****************************************************************************/

typedef struct
{
    ClAmsEntityTypeT        type;                   /* entity type in list   */
    ClBoolT                 isRankedList;           /* Ranked list or Unranked list */
    ClBoolT                 isValid;                /* is list valid         */
    ClUint32T               numEntities;            /* list count            */
    ClCntHandleT            list;                   /* list container        */
} ClAmsEntityListT;



    /**
     * Entity Lists
     */
typedef enum
{
    CL_AMS_START_LIST                                    ,

    /*
     * Config lists
     */
    CL_AMS_CONFIG_LIST_START                              ,
    CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST                ,
    CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST             ,
    CL_AMS_NODE_CONFIG_SU_LIST                            ,
    CL_AMS_SG_CONFIG_SU_LIST                              ,
    CL_AMS_SG_CONFIG_SI_LIST                              ,
    CL_AMS_SU_CONFIG_COMP_LIST                            ,
    CL_AMS_SI_CONFIG_SU_RANK_LIST                         ,
    CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST                   ,
    CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST                 ,
    CL_AMS_SI_CONFIG_CSI_LIST                             ,
    CL_AMS_CSI_CONFIG_NVP_LIST                            ,
    CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST                 ,
    CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST               ,
    CL_AMS_CONFIG_LIST_END                                ,
    /*
     * Status lists
     */

    CL_AMS_SG_STATUS_INSTANTIABLE_SU_LIST                 ,
    CL_AMS_SG_STATUS_INSTANTIATED_SU_LIST                 ,
    CL_AMS_SG_STATUS_IN_SERVICE_SPARE_SU_LIST             ,
    CL_AMS_SG_STATUS_ASSIGNED_SU_LIST                     ,
    CL_AMS_SG_STATUS_FAULTY_SU_LIST                       ,
    CL_AMS_SU_STATUS_SI_LIST                              ,
    CL_AMS_SI_STATUS_SU_LIST                              ,
    CL_AMS_COMP_STATUS_CSI_LIST                           ,
    CL_AMS_CSI_STATUS_PG_LIST                             ,

    /*
     * Start of entity all list types.
     */
    CL_AMS_ENTITY_LIST_ALL_START                          ,

    CL_AMS_SG_LIST                                            , /**< List of all service groups */
    CL_AMS_SI_LIST                                            , /**< List of all service instances (work assignment) */
    CL_AMS_NODE_LIST                                          , /**< List of all nodes (computers) */
    CL_AMS_SU_LIST                                            , /**< List of all service units */
    CL_AMS_COMP_LIST                                          , /**< List of all components (program) */
    CL_AMS_CSI_LIST                                           , /**< List of all component service instances (work assigned to one program) */
    /*
     * End of entity all list types.
     */
    CL_AMS_ENTITY_LIST_ALL_END                                ,

    /* Internal entity list types.*/
    CL_AMS_CSI_PGTRACK_CLIENT_LIST,

    CL_AMS_SU_STATUS_SI_EXTENDED_LIST                              ,

    CL_AMS_SI_STATUS_SU_EXTENDED_LIST                              ,

    CL_AMS_END_LIST                                      ,

}ClAmsEntityListTypeT;


typedef ClRcT (*ClAmsEntityRefCallbackT)(
                    ClAmsEntityRefT *entityRef,
                    ClAmsEntityListTypeT listName);

/**
 * A structure for aggregating default parameters for an entity
 *****************************************************************************/

typedef struct
{
    ClNameT         typeString;                     /**< string representation */
    void            *defaultConfig;                 /**< default config        */
    ClUint32T       configSize;                     /**< size of config struct */
    void            *defaultMethods;                /**< default methods       */
    ClUint32T       methodSize;                     /**< size of method struct */
    ClUint32T       entitySize;                     /**< size of entity struct */
} ClAmsEntityParamsT;

/******************************************************************************
 * AMS NODE
 *****************************************************************************/

    /**
     * Node configuration information
     */
typedef struct
{
    ClAmsEntityConfigT      entity;                 /**< base class            */

    ClAmsAdminStateT        adminState;             /**< can AMS use entity?   */
    ClUint32T               id;                     /**< unique id in cluster  */
    ClAmsNodeClassT         classType;              /**< profile (A,B,C,D)     */
    ClNameT                 subClassType;           /**< eg: SDH_OC48C_V3      */
    ClBoolT                 isSwappable;            /**< is node a FRU ?       */
    ClBoolT                 isRestartable;          /**< is node restartable ? */
    ClBoolT                 autoRepair;             /**< does node autorepair  */
    ClBoolT                 isASPAware;             /**< ASP running on node? */
    ClTimeT                 suFailoverDuration;     /**< escalation timeout ms */
    ClUint32T               suFailoverCountMax;     /**< failures to tolerate  */
    ClAmsEntityListT        nodeDependentsList;     /**< list of dependents    */
    ClAmsEntityListT        nodeDependenciesList;   /**< list of dependencies  */
    ClAmsEntityListT        suList;                 /**< list of SU in node    */
} ClAmsNodeConfigT;

    /**
     * Node state information
     */    
typedef struct
{
    ClAmsEntityStatusT      entity;                 /** base class            */

    ClAmsPresenceStateT     presenceState;          /**< ams internal (not saf)*/
    ClAmsOperStateT         operState;              /**< Oper state            */
    ClAmsNodeClusterMemberT isClusterMember;        /**< cluster member status */
    ClBoolT                 wasMemberBefore;        /**< was it here before ?  */
    ClAmsLocalRecoveryT     recovery;               /**< recovery action       */
    ClUint32T               alarmHandle;            /**< handle for fault manager*/
    ClUint32T               suFailoverCount;        /**< current failure count */
    ClAmsEntityTimerT       suFailoverTimer;        /**< keep track of timer   */
    ClUint32T               numInstantiatedSUs;     /**< SUs instantiated      */
    ClUint32T               numAssignedSUs;         /**< SUs with assignments  */
} ClAmsNodeStatusT;

typedef struct
{
    ClAmsEntityMethodsT     entity;                 /* base methods          */

    ClAmsEntityTimerCallbackT suFailoverTimeout;    /* call this on timeout  */
} ClAmsNodeMethodsT;

typedef struct
{
    ClAmsNodeConfigT        config;
    ClAmsNodeStatusT        status;
    ClAmsNodeMethodsT       methods;
} ClAmsNodeT;

/******************************************************************************
 * AMS APPLICATION 
 *****************************************************************************/

typedef struct
{
    ClAmsEntityConfigT      entity;                 /* base class            */
} ClAmsAppConfigT;

typedef struct
{
    ClAmsEntityStatusT      entity;                 /* base class            */
} ClAmsAppStatusT;

typedef struct
{
    ClAmsEntityMethodsT     entity;                 /* base methods          */
} ClAmsAppMethodsT;

typedef struct
{
    ClAmsAppConfigT         config;
    ClAmsAppStatusT         status;
    ClAmsAppMethodsT        methods;
} ClAmsAppT;

/******************************************************************************
 * AMS SERVICE GROUP
 *****************************************************************************/

    /**
     * Service group configuration information
     */       
typedef struct
{
    ClAmsEntityConfigT      entity;                 /**< base class            */

    ClAmsAdminStateT        adminState;             /**< can AMS use entity?   */
    ClAmsSGRedundancyModelT redundancyModel;        /**< None, 2N, M+N, etc    */
    ClAmsSGLoadingStrategyT loadingStrategy;        /**< SU loading scheme     */
    ClBoolT                 failbackOption;         /**< revert SUs ?          */
    ClBoolT                 autoRepair;             /**< auto repair failed SU */
    ClTimeT                 instantiateDuration;    /**< Delay between SG instantation and comp start */
    ClUint32T               numPrefActiveSUs;       /**< 2N, M+N               */
    ClUint32T               numPrefStandbySUs;      /**< 2N, M+N               */
    ClUint32T               numPrefInserviceSUs;    /**< >Active|Assigned+Stdby*/
    ClUint32T               numPrefAssignedSUs;     /**< N-Way-* models only   */
    ClUint32T               numPrefActiveSUsPerSI;  /**< N-Way-Active only     */
    ClUint32T               maxActiveSIsPerSU;      /**< max active SI per SU  */
    ClUint32T               maxStandbySIsPerSU;     /**< max standby SI per SU */
    ClTimeT                 compRestartDuration;    /**< escalation timeout    */
    ClUint32T               compRestartCountMax;    /**< failover threshold    */
    ClTimeT                 suRestartDuration;      /**< escalation timeout    */
    ClUint32T               suRestartCountMax;      /**< failover threshold    */
    ClBoolT                 isCollocationAllowed;    /**< collocated is allowed */
    ClUint32T               alpha;                   /**< % of actives SUs in M+N when the preferred configuration can not be met */
    ClBoolT                 autoAdjust;              /**< sg adjust or realignment config*/
    ClTimeT                 autoAdjustProbation;    /**< sg adjust timer for realingnment on recovery/faults*/
    ClBoolT                 reductionProcedure;     /*enable reduction procedure*/
    ClAmsEntityRefT         parentApp;              /**< member of this app    */
    ClUint32T               maxFailovers;           /* max failovers configured for the SG*/
    ClTimeT                 failoverDuration;       /* failover protection duration*/
    ClUint32T               beta;                   /** % of standby SUs based on the current cluster config */
    ClAmsEntityListT        suList;                 /**< all configured SU     */
    ClAmsEntityListT        siList;                 /**< all configured SI     */
} ClAmsSGConfigT;

typedef ClAmsSGConfigT VDECL_VER(ClAmsSGConfigT, 4, 1, 0);
typedef ClAmsSGConfigT VDECL_VER(ClAmsSGConfigT, 5, 0, 0);

typedef struct ClAmsSGFailoverHistoryKey
{
    ClAmsEntityT entity;
    ClUint32T index;
}ClAmsSGFailoverHistoryKeyT;

typedef struct ClAmsSGFailoverHistory
{
    ClListHeadT list;
    ClAmsEntityT   entity;
    ClUint32T  index;
    ClTimerHandleT timer;
    ClUint32T      numFailovers;
}ClAmsSGFailoverHistoryT;

    /**
     * Service group state information
     */  
typedef struct
{
    ClAmsEntityStatusT      entity;                 /**< base class            */
    ClBoolT                 isStarted;              /**< has SG started ?      */
    ClAmsEntityTimerT       instantiateTimer;       /**< restart timer         */
    ClAmsEntityTimerT       adjustTimer;            /**< adjustment incase of pending invocations */
    ClAmsEntityTimerT       adjustProbationTimer;   /*** adjust probation timer*/
    ClAmsEntityTimerT       assignmentTimer;        /** assignment delay for SI preference*/
    ClAmsEntityListT        instantiableSUList;     /**< all usable SUs        */
    ClAmsEntityListT        instantiatedSUList;     /**< unlocked, usable SUs  */
    ClAmsEntityListT        inserviceSpareSUList;   /**< SUs ready, w/wo work  */
    ClAmsEntityListT        assignedSUList;         /**< SUs with work         */
    ClAmsEntityListT        faultySUList;           /**< SUs awaiting repair   */
                                                    /**< not used for now      */

    ClUint32T               numCurrActiveSUs;       /**< current active SUs    */
    ClUint32T               numCurrStandbySUs;      /**< current standby SUs   */
    ClUint32T               failoverHistoryIndex;   /** running history index counter **/
    ClInt32T                failoverHistoryCount;   /*present count of failover history entries*/
    ClListHeadT             failoverHistory;        /** sg failover history for failover duration **/
} ClAmsSGStatusT;

typedef ClAmsSGStatusT VDECL_VER(ClAmsSGStatusT, 4, 1, 0);

typedef struct
{
    ClAmsEntityMethodsT     entity;                 /* base methods          */

    ClAmsEntityTimerCallbackT instantiateTimeout;
    ClAmsEntityTimerCallbackT adjustTimeout;
    ClAmsEntityTimerCallbackT adjustProbationTimeout;
} ClAmsSGMethodsT;

typedef struct
{
    ClAmsSGConfigT          config;
    ClAmsSGStatusT          status;
    ClAmsSGMethodsT         methods;
} ClAmsSGT;

typedef ClAmsSGT VDECL_VER(ClAmsSGT, 4, 1, 0);
typedef ClAmsSGT VDECL_VER(ClAmsSGT, 5, 0, 0);

/******************************************************************************
 * AMS SERVICE UNIT
 *****************************************************************************/

/**
 * Service Instances assigned to this Service Unit
 *
 * Each SU has a list of SIs that can be assigned to it and for each SI
 * there is a HA state. The following struct is used to track the SIs
 * assigned to this SU (both active and standby) as part of the siList.
 */
typedef struct
{
    ClAmsEntityRefT         entityRef;
    ClAmsHAStateT           haState;
    ClUint32T               numActiveCSIs;      /**< active csis in this si    */
    ClUint32T               numStandbyCSIs;     /**< standby csis in this si   */
    ClUint32T               numQuiescedCSIs;    /**< quiesced csis in this si  */
    ClUint32T               numQuiescingCSIs;   /**< quiescing csis in this si */
    ClUint32T               rank;               /**< rank associated with SI   */
} ClAmsSUSIRefT;

typedef struct
{
    ClAmsEntityRefT         entityRef;
    ClAmsHAStateT           haState;
    ClUint32T               numActiveCSIs;      /**< active csis in this si    */
    ClUint32T               numStandbyCSIs;     /**< standby csis in this si   */
    ClUint32T               numQuiescedCSIs;    /**< quiesced csis in this si  */
    ClUint32T               numQuiescingCSIs;   /**< quiescing csis in this si */
    ClUint32T               numCSIs;            /*   num configured csis in this si*/
    ClUint32T               rank;               /**< rank associated with SI   */
    ClUint32T               pendingInvocations; /* number of pending invocations for this si*/
} ClAmsSUSIExtendedRefT;


/**
 * Service Unit configuration information
 */  
typedef struct
{
    ClAmsEntityConfigT      entity;             /**< base class                */

    ClAmsAdminStateT        adminState;         /**< can AMS use entity?       */
    ClUint32T               rank;               /**< preference 1..4294967295  */
    ClUint32T               numComponents;      /**< components in SU          */
    ClBoolT                 isPreinstantiable;  /**< true if any comp is prei  */
    ClBoolT                 isRestartable;      /**< is SU restartable?        */
                                                /**< not used for now          */
    ClBoolT                 isContainerSU;      /**< does SU contain other SUs */
                                                /* not used for now          */
    ClAmsEntityListT        compList;           /**< list of components in SU  */
    ClAmsEntityRefT         parentSG;           /**< SU is part of this SG     */
    ClAmsEntityRefT         parentNode;         /**< SU is part of this node   */
} ClAmsSUConfigT;

    /**
     * Service Unit status information
     */  
typedef struct
{
    ClAmsEntityStatusT      entity;             /**< base class                */

    ClAmsPresenceStateT     presenceState;      /**< presence state (saf)      */
    ClAmsOperStateT         operState;          /**< operational state (saf)   */
    ClAmsReadinessStateT    readinessState;     /**< readiness state (saf)     */
    ClAmsLocalRecoveryT     recovery;           /**< recovery action           */
    ClUint32T               numActiveSIs;       /**< active SI count for SU    */
    ClUint32T               numStandbySIs;      /**< standby SI count for SU   */
    ClUint32T               numQuiescedSIs;     /**< quiesced SI count for SU  */
    ClUint32T               compRestartCount;   /**< current SU failure count  */
    ClAmsEntityTimerT       compRestartTimer;   /**< timer details             */
    ClUint32T               suRestartCount;     /**< current SU failure count  */
    ClAmsEntityTimerT       suRestartTimer;     /**< timer details             */
    ClAmsEntityTimerT       suProbationTimer;   /**<probation timer details*/
    ClAmsEntityTimerT       suAssignmentTimer;  /** pref.assignment timer*/
    ClUint32T               numInstantiatedComp;/**< num instantiated comps    */
    ClUint32T               numPIComp;          /**< preinstantiable comps     */
    ClUint32T               instantiateLevel;  /** SUs current instantiate level */
    ClUint32T               numWaitAdjustments; /**< adjustments waited for higher rank realignments for active*/
    ClUint32T               numDelayAssignments;    /** Assignment delays for SI preference  **/
    ClAmsEntityListT        siList;             /**< list of SI for this SU    */
} ClAmsSUStatusT;

typedef struct
{
    ClAmsEntityMethodsT     entity;             /**< Base Methods              */

    ClAmsEntityTimerCallbackT     suRestartTimeout;   /* Fn to call          */
    ClAmsEntityTimerCallbackT     compRestartTimeout; /* Fn to call          */
    ClAmsEntityTimerCallbackT     suProbationTimeout; 
    ClAmsEntityTimerCallbackT     suAssignmentTimeout;
} ClAmsSUMethodsT;

typedef struct
{
    ClAmsSUConfigT          config;
    ClAmsSUStatusT          status;
    ClAmsSUMethodsT         methods;
} ClAmsSUT;

/******************************************************************************
 * AMS SERVICE INSTANCE
 *****************************************************************************/

/**
 * Service Instance preferred ranking
 *
 * In the N-Way and N-Way-Active models, each SI can have a preferred ranking
 * for the SUs to which it can be assigned. The following struct is used in a
 * list to track this preference as part of the suRankList.
 */

typedef struct
{
    ClAmsEntityRefT         entityRef;          /**< base reference to SU      */
    ClUint32T               rank;               /**< rank associated with SU   */
    ClAmsHAStateT           haState;            /**< hastate assigned to SU    */
} ClAmsSISURefT;

typedef struct
{
    ClAmsEntityRefT         entityRef;          /**< base reference to SU      */
    ClUint32T               rank;               /**< rank associated with SU   */
    ClAmsHAStateT           haState;            /**< hastate assigned to SU    */
    ClUint32T               pendingInvocations; /* pending invocations against this SU */
} ClAmsSISUExtendedRefT;

    /**
     * Service Unit configuration information
     */  
typedef struct
{
    ClAmsEntityConfigT      entity;             /**< base class                */

    ClAmsAdminStateT        adminState;         /**< can AMS use entity?       */
    ClUint32T               rank;               /**< preference for this SI    */
    ClUint32T               numCSIs;            /**< number of CSIs in this SI */
    ClUint32T               numStandbyAssignments;/**< n-way model only        */
    ClUint32T               standbyAssignmentOrder; /*standby assignment order for CSI dep.*/
    ClAmsEntityRefT         parentSG;           /**< SI is part of this SG     */
    ClAmsEntityListT        suList;             /**< ordered SUs for this SI   */
    ClAmsEntityListT        siDependentsList;   /**< clusterwide dependents    */
    ClAmsEntityListT        siDependenciesList; /**< clusterwide dependencies  */
    ClAmsEntityListT        csiList;            /**< CSIs part of this SI       */
} ClAmsSIConfigT;


    /**
     * Service Unit state information
     */  
typedef struct
{
    ClAmsEntityStatusT      entity;             /**< base class                */

    ClAmsOperStateT         operState;          /**< operation state (saf)     */
    ClUint32T               numActiveAssignments;   /**< active assignments    */
    ClUint32T               numStandbyAssignments;  /**< standby assignments   */
    ClAmsEntityListT        suList;             /**< assigned to these SUs     */
} ClAmsSIStatusT;

typedef struct
{
    ClAmsEntityMethodsT     entity;             /* base methods              */
} ClAmsSIMethodsT;

typedef struct
{
    ClAmsSIConfigT          config;
    ClAmsSIStatusT          status;
    ClAmsSIMethodsT         methods;
} ClAmsSIT;

/******************************************************************************
 * AMS COMPONENT
 *****************************************************************************/

typedef struct
{
    ClAmsEntityRefT         entityRef;          /**< base reference to CSI     */
    ClAmsHAStateT           haState;            /**< ha state for CSI          */
    ClAmsCSITransitionDescriptorT tdescriptor;  /**< transition descriptor     */
    ClUint32T               rank;               /**< rank of the assignment    */
                                                /**< active=0, standby=1..N    */
    ClAmsEntityT            *activeComp;        /**< comp with active ha state */
    ClUint16T               pendingOp;          /**< pending op on CSI         */
} ClAmsCompCSIRefT;

typedef struct
{
    ClTimeT                 instantiate;
    ClTimeT                 terminate;
    ClTimeT                 cleanup;
    ClTimeT                 amStart;
    ClTimeT                 amStop;
    ClTimeT                 quiescingComplete;
    ClTimeT                 csiSet;
    ClTimeT                 csiRemove;
    ClTimeT                 proxiedCompInstantiate;
    ClTimeT                 proxiedCompCleanup;
    ClTimeT                 instantiateDelay;
} ClAmsCompTimerDurationsT;

typedef struct
{
    ClAmsEntityTimerT       instantiate;
    ClAmsEntityTimerT       terminate;
    ClAmsEntityTimerT       cleanup;
    ClAmsEntityTimerT       amStart;
    ClAmsEntityTimerT       amStop;
    ClAmsEntityTimerT       quiescingComplete;
    ClAmsEntityTimerT       csiSet;
    ClAmsEntityTimerT       csiRemove;
    ClAmsEntityTimerT       proxiedCompInstantiate;
    ClAmsEntityTimerT       proxiedCompCleanup;
    ClAmsEntityTimerT       instantiateDelay;
} ClAmsCompTimersT;        

    /**
     * Component configuration information
     */  
typedef struct
{
    ClAmsEntityConfigT      entity;             /**< base class                */
    ClUint32T               numSupportedCSITypes; /**<supported csi type count*/
    ClNameT                 *pSupportedCSITypes;  /**< CSI types supported */
    ClNameT                 proxyCSIType;       /**< CSI type of proxy, if any */
    ClAmsCompCapModelT      capabilityModel;    /**< how to assign CSIs        */
    ClAmsCompPropertyT      property;           /**< component type/property   */
    ClBoolT                 isRestartable;      /**< is component restart ok ? */
    ClBoolT                 nodeRebootCleanupFail;/**< escalate cleanup failure*/
    ClUint32T               instantiateLevel;   /**< when to start comp        */
    ClUint32T               numMaxInstantiate;  /**< max instantiation attempts*/
    ClUint32T               numMaxInstantiateWithDelay; /**<                   */
    ClUint32T               numMaxTerminate;    /**< -- not used               */
    ClUint32T               numMaxAmStart;      /**< max amstart attempts      */
    ClUint32T               numMaxAmStop;       /**< max amstop attempts       */
    ClUint32T               numMaxActiveCSIs;   /**< used as per capability    */
    ClUint32T               numMaxStandbyCSIs;  /**< used as per capability    */
    ClAmsCompTimerDurationsT timeouts;          /**< comp operation durations  */
    ClAmsLocalRecoveryT     recoveryOnTimeout;  /**< recovery on error         */
    ClAmsEntityRefT         parentSU;           /**< member of this SU         */
    ClCharT                 instantiateCommand[CL_MAX_NAME_LENGTH];
    /*  
     * No need to add terminate and cleanup as those aren't supported
     */
} ClAmsCompConfigT;

    /**
     * Component state information
     */
typedef struct
{
    ClAmsEntityStatusT      entity;             /**< base class                */
    ClAmsPresenceStateT     presenceState;      /**< presence state (saf)      */
    ClAmsOperStateT         operState;          /**< operational state (saf)   */
    ClAmsReadinessStateT    readinessState;     /**< readiness state (saf)     */
    ClAmsLocalRecoveryT     recovery;           /**< recovery action for comp  */
    ClUint32T               alarmHandle;        /**< handle for fault manager  */
    ClUint32T               numActiveCSIs;      /**< num active CSIs assigned  */
    ClUint32T               numStandbyCSIs;     /**< num standby CSIs assigned */
    ClUint32T               numQuiescingCSIs;   /**< num quiescing CSIs        */
    ClUint32T               numQuiescedCSIs;    /**< num quiesced CSIs         */
    ClUint32T               restartCount;       /**< current comp failure count*/
    ClUint32T               failoverCount;      /**< current comp failure count*/
    ClUint32T               instantiateCount;   /**< current inst count        */
    ClUint32T               instantiateDelayCount; /**< inst count with delay  */
    ClUint32T               amStartCount;       /**< current am start count    */
    ClUint32T               amStopCount;        /**< current am stop count     */
    ClUint64T               instantiateCookie; /**a cookie to correlate component faults */
    ClAmsCompTimersT        timers;             /**< comp operation timeouts   */
    ClAmsEntityT            *proxyComp;         /**< proxy for this component  */
    ClAmsEntityListT        csiList;            /**< assigned act/standby CSIs */
    ClAmsSAClientCallbacksT clientCallbacks;    /**< fns registered by client  */
} ClAmsCompStatusT;

typedef ClAmsCompStatusT VDECL_VER(ClAmsCompStatusT, 5, 1, 0);

typedef struct
{
    ClAmsEntityMethodsT         entity;

    ClAmsEntityTimerCallbackT   instantiateTimeout;
    ClAmsEntityTimerCallbackT   terminateTimeout;
    ClAmsEntityTimerCallbackT   cleanupTimeout;
    ClAmsEntityTimerCallbackT   amStartTimeout;
    ClAmsEntityTimerCallbackT   amStopTimeout;
    ClAmsEntityTimerCallbackT   quiescingCompleteTimeout;
    ClAmsEntityTimerCallbackT   csiSetTimeout;
    ClAmsEntityTimerCallbackT   csiRemoveTimeout;
    ClAmsEntityTimerCallbackT   proxiedCompInstantiateTimeout;
    ClAmsEntityTimerCallbackT   proxiedCompCleanupTimeout;
    ClAmsEntityTimerCallbackT   instantiateDelayTimeout;
} ClAmsCompMethodsT;

typedef struct
{
    ClAmsCompConfigT        config;
    ClAmsCompStatusT        status;
    ClAmsCompMethodsT       methods;
} ClAmsCompT;

typedef ClAmsCompT VDECL_VER(ClAmsCompT, 5, 1, 0);

/******************************************************************************
 * AMS COMPONENT SERVICE INSTANCE
 *****************************************************************************/

/*
 * This structure is used as the list element in pgList
 */

typedef struct
{
    ClAmsEntityRefT         entityRef;          /**< base reference to Comp    */
    ClAmsHAStateT           haState;            /**< ha state for CSI          */
    ClUint32T               rank;               /**< rank of the assignment    */
                                                /**< active=0, standby=1..N    */
} ClAmsCSICompRefT;

typedef struct
{
    ClIocAddressT           address;
    ClAmsPGTrackFlagT       trackFlags;
    ClCpmHandleT            cpmHandle;
} ClAmsCSIPGTrackClientT;


/**
 * Name Value Pair definition for CSIs
 *
 * The following struct defines a CSI, which is identified by a name value
 * pair.
 */
typedef struct
{
    ClNameT                 csiName;            /**< What CSI this NVP is associated with */
    ClNameT                 paramName;          /**< String name of the parameter   */
    ClNameT                 paramValue;         /**< String value of the parameter  */
} ClAmsCSINameValuePairT;

    /**
     * Component service instance configuration information
     */
typedef struct
{
    ClAmsEntityConfigT      entity;             /**< base class                */

    ClNameT                 type;               /**< type of CSI in SNMP       */
    ClBoolT                 isProxyCSI;         /**< Is this a proxy CSI?      */
                                                /**< Future: Set this in CW    */
                                                /**< For now we compute it     */
    ClUint32T               rank;               /**< order of CSI within SI    */
    ClCntHandleT            nameValuePairList;  /**< List of name value pairs  */
    ClAmsEntityRefT         parentSI;           /**< Part of this SI           */
    ClAmsEntityListT        csiDependentsList;    /* list of dependents of this CSI*/
    ClAmsEntityListT        csiDependenciesList;  /* dependencies of this CSI */
} ClAmsCSIConfigT; 

    /**
     * Component service instance state information
     */
typedef struct
{
    ClAmsEntityStatusT      entity;             /**< base class                */
    ClAmsEntityListT        pgList;             /**< list of pg components     */
    ClCntHandleT            pgTrackList;        /**< interested clients        */
} ClAmsCSIStatusT;

typedef struct
{
    ClAmsEntityMethodsT     entity;             /**< base methods              */
} ClAmsCSIMethodsT;

typedef struct
{
    ClAmsCSIConfigT         config;
    ClAmsCSIStatusT         status;
    ClAmsCSIMethodsT        methods;
} ClAmsCSIT; 

typedef struct ClAmsSUReassignOp
{
    ClInt32T numSIs;
    ClAmsEntityT *sis;
}ClAmsSUReassignOpT;

typedef struct ClAmsSIReassignEntry
{
    ClAmsSIT *si;
    ClListHeadT list;
}ClAmsSIReassignEntryT;

#define CL_AMS_TIMER_CONVERT(x,y)       \
{                                       \
    (y).tsSec = (x) / 1000;             \
    (y).tsMilliSec = (x) % 1000;        \
}

/******************************************************************************
 * Macros and functions for validating entities
 *****************************************************************************/

#define AMS_VALIDATE_ADMINSTATE(x)                                      \
{                                                                       \
    if ( ((x)->config.adminState < 1) ||                                \
         ((x)->config.adminState > 4) )                                 \
    {                                                                   \
        AMS_LOG(CL_DEBUG_ERROR,                                         \
                ("Entity[%s] fails adminState validation.\n",           \
                 (x)->config.entity.name.value));                       \
        return CL_AMS_ERR_INVALID_ENTITY;                               \
    }                                                                   \
}

#define AMS_VALIDATE_OPERSTATE(x)                                       \
{                                                                       \
    if ( ((x)->status.operState < 1) ||                                 \
         ((x)->status.operState > 2) )                                  \
    {                                                                   \
        AMS_LOG(CL_DEBUG_ERROR,                                         \
                ("Entity[%s] fails operState validation.\n",            \
                 (x)->config.entity.name.value));                       \
    }                                                                   \
}

#define AMS_VALIDATE_PRESENCESTATE(x)                                   \
{                                                                       \
    if ( ((x)->status.presenceState < 1) ||                             \
         ((x)->status.presenceState > 10) )                             \
    {                                                                   \
        AMS_LOG(CL_DEBUG_ERROR,                                         \
                ("Entity[%s] fails presenceState validation.\n",        \
                 (x)->config.entity.name.value));                       \
    }                                                                   \
}

#define AMS_VALIDATE_READINESSSTATE(x)                                  \
{                                                                       \
    if ( ((x)->status.readinessState < 1) ||                            \
         ((x)->status.readinessState > 10) )                            \
    {                                                                   \
        AMS_LOG(CL_DEBUG_ERROR,                                         \
                ("Entity[%s] fails readinessState validation.\n",       \
                 (x)->config.entity.name.value));                       \
    }                                                                   \
}

#define AMS_VALIDATE_NODE_CLASS_TYPE(x)                                 \
{                                                                       \
    if ( ((x)->config.classType) < CL_AMS_NODE_CLASS_NONE ||            \
            ((x)->config.classType) > CL_AMS_NODE_CLASS_D )             \
    {                                                                   \
        AMS_LOG(CL_DEBUG_ERROR,                                         \
                ("Entity[%s] fails node class type validation.\n",      \
                 (x)->config.entity.name.value));                       \
        return CL_AMS_ERR_INVALID_ENTITY;                                \
    }                                                                   \
}

#define AMS_VALIDATE_COMP_CAPABILITY_MODEL(x)                           \
{                                                                       \
    if ( ((x)->config.capabilityModel <                                 \
                CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY) ||              \
         ((x)->config.capabilityModel >                                 \
          CL_AMS_COMP_CAP_NON_PREINSTANTIABLE) )                        \
    {                                                                   \
        AMS_LOG(CL_DEBUG_ERROR,                                         \
                ("Entity[%s] fails comp capability model validation.\n",\
                 (x)->config.entity.name.value));                       \
        return CL_AMS_ERR_INVALID_ENTITY;                               \
    }                                                                   \
}


#define AMS_VALIDATE_COMP_RECOVERY_ON_ERROR(x)                          \
{                                                                       \
    if ( ((x)->config.recoveryOnTimeout <                               \
                CL_AMS_RECOVERY_NO_RECOMMENDATION) ||                   \
         ((x)->config.recoveryOnTimeout >                               \
          CL_AMS_RECOVERY_SU_RESTART) )                              \
    {                                                                   \
        AMS_LOG(CL_DEBUG_ERROR,                                         \
                ("Entity[%s] fails comp recoveryOnError validation.\n", \
                 (x)->config.entity.name.value));                       \
        return CL_AMS_ERR_INVALID_ENTITY;                               \
    }                                                                   \
}



#define AMS_VALIDATE_COMP_PROPERTY(x)                                   \
{                                                                       \
    if ( ((x)->config.property < CL_AMS_COMP_PROPERTY_SA_AWARE ) ||     \
            ((x)->config.property >                                     \
             CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE ))    \
    {                                                                   \
        AMS_LOG(CL_DEBUG_ERROR,                                         \
                ("Entity[%s] fails comp property validation.\n",        \
                 (x)->config.entity.name.value));                       \
        return CL_AMS_ERR_INVALID_ENTITY;                               \
    }                                                                   \
}


#define AMS_VALIDATE_BOOL_VALUE(x)                                  \
{                                                                   \
    if ( ! ( (x) == CL_TRUE || (x) == CL_FALSE) )                   \
    {                                                               \
        AMS_LOG(CL_DEBUG_ERROR,                                     \
                ("Expecting boolean value, received value %d \n",   \
                 x));                                               \
        return CL_ERR_INVALID_PARAMETER;                            \
    }                                                               \
}                                                                       

#define AMS_VALIDATE_RESTART_COUNT(x)                       do {        \
    if( !(x) )                                                          \
    {                                                                   \
        AMS_LOG(CL_DEBUG_ERROR,("Expecting non-zero restart count\n")); \
        return CL_ERR_INVALID_PARAMETER;                                \
    }                                                                   \
}while(0)


#define AMS_VALIDATE_RESTART_DURATION(x)                       do {         \
    if( !(x) )                                                              \
    {                                                                       \
        AMS_LOG(CL_DEBUG_ERROR,("Expecting non-zero restart duration\n"));  \
        return CL_ERR_INVALID_PARAMETER;                                    \
    }                                                                       \
}while(0)

#ifdef __cplusplus
}
#endif

#endif /* _CL_AMS_ENTITIES_H_ */
