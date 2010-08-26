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
 * File        : clAmsDefaultConfig.c
 *******************************************************************************/

/*******************************************************************************
 * Description :                                                                
 *
 * This file holds the default configuration structures for AMS.
 *
 *
 ***************************** Editor Commands ********************************
 * For vi/vim
 * :set shiftwidth=4
 * :set softtabstop=4
 * :set expandtab
 *****************************************************************************/

/******************************************************************************
 * Include files needed to compile this file
 *****************************************************************************/

#include <clAmsEntities.h> 
#include <clAmsMgmtCommon.h> 
#include <clAmsDefaultConfig.h> 
#include <stdio.h> 

/******************************************************************************
 * Default Config for AMS Entities
 *****************************************************************************/

ClAmsEntityConfigT gClAmsEntityDefaultConfig =
{
        CL_AMS_ENTITY_TYPE_ENTITY,              /* entity.type               */
        {
            sizeof("EntityNameUndefined"),      /* entity.name.length        */
            "EntityNameUndefined"               /* entity.name.value         */
        },
        CL_AMS_DEBUG_FLAGS_DEFAULT              /* debug flags               */
};

ClAmsNodeConfigT gClAmsNodeDefaultConfig =
{
    {
        CL_AMS_ENTITY_TYPE_NODE,                /* entity.type               */
        {
            sizeof("NodeNameUndefined"),        /* entity.name.length        */
            "NodeNameUndefined"                 /* entity.name.value         */
        },
        CL_AMS_DEBUG_FLAGS_DEFAULT              /* debug flags               */
    },
    CL_AMS_ADMIN_STATE_LOCKED_I,                /* adminState                */
    0,                                          /* id                        */
    CL_AMS_NODE_CLASS_C,                        /* classType                 */
    {
        sizeof("NodeSubClassUndefined"),        /* subClassType.length       */
        "NodeSubClassUndefined"                 /* subClassType.value        */
    },
    CL_TRUE,                                    /* isSwappable               */
    CL_TRUE,                                    /* isRestartable             */
    CL_TRUE,                                    /* autoRepair                */
    CL_TRUE,                                    /* isASPAware                */
    60000,                                      /* suFailoverDuration=60s    */
    10,                                         /* suFailoverCountMax        */
    {                                           /* nodeDependentsList        */
        CL_AMS_ENTITY_TYPE_NODE,                /*      .type                */
        CL_FALSE,                               /*      .isRanked            */
        CL_FALSE,                               /*      .isValid             */
        0,                                      /*      .numEntities         */
        (ClCntHandleT) NULL                     /*      .list                */
    },
    {                                           /* nodeDependenciesList      */
        CL_AMS_ENTITY_TYPE_NODE,                /*      .type                */
        CL_FALSE,                               /*      .isRanked            */
        CL_FALSE,                               /*      .isValid             */
        0,                                      /*      .numEntities         */
        (ClCntHandleT) NULL                     /*      .list                */
    },
    {                                           /* suList                    */
        CL_AMS_ENTITY_TYPE_SU,                  /*      .type                */
        CL_FALSE,                               /*      .isRanked            */
        CL_FALSE,                               /*      .isValid             */
        0,                                      /*      .numEntities         */
        (ClCntHandleT) NULL                     /*      .list                */
    }
};

ClAmsAppConfigT gClAmsAppDefaultConfig =
{
    {
        CL_AMS_ENTITY_TYPE_APP,                 /* entity.type               */
        {
            sizeof("AppNameUndefined"),         /* entity.name.length        */
            "AppNameUndefined"                  /* entity.name.value         */
        },
        CL_AMS_DEBUG_FLAGS_DEFAULT              /* debug flags               */
    } 
};

ClAmsSGConfigT gClAmsSGDefaultConfig =
{
    {
        CL_AMS_ENTITY_TYPE_SG,                  /* entity.type               */
        {
            sizeof("SGNameUndefined"),          /* entity.name.length        */
            "SGNameUndefined"                   /* entity.name.value         */
        },
        CL_AMS_DEBUG_FLAGS_DEFAULT              /* debug flags               */
    },
    CL_AMS_ADMIN_STATE_LOCKED_I,                /* adminState                */
    CL_AMS_SG_REDUNDANCY_MODEL_TWO_N,           /* redundancyModel           */
    CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU, /* loadingStrategy           */
    CL_FALSE,                                   /* failbackOption            */
    CL_TRUE,                                    /* autoRepair                */
    10000,                                      /* instantiateTimeout (10s)   */
    1,                                          /* numPrefActiveSUs          */
    1,                                          /* numPrefStandbySUs         */
    2,                                          /* numPrefInserviceSUs       */
    2,                                          /* numPrefAssignedSUs        */
    1,                                          /* numPrefActiveSUsPerSI     */
    1,                                          /* maxActiveSIsPerSU         */
    1,                                          /* maxStandbySIsPerSU        */
    10000,                                      /* compRestartDuration       */
    5,                                          /* compRestartCountMax       */
    20000,                                      /* suRestartDuration         */
    5,                                          /* suRestartCountMax         */
    CL_TRUE,                                    /* isCollocationAllowed      */
    100,                                        /* alpha                     */
    CL_FALSE,                                   /* adjust */
    10000,                                      /* probation timeout*/
    CL_FALSE,                                   /* reduction procedure*/ 
    {                                           /* parentApp                 */
        {
            CL_AMS_ENTITY_TYPE_APP,
            {
                sizeof("ParentAppUndefined"),   /* entity.name.length        */
                "ParentAppUndefined"            /* entity.name.value         */
            },
            CL_AMS_DEBUG_FLAGS_DEFAULT          /* debug flags               */
        },
        NULL
    },
    0,                                           /* max failovers */
    300000,                                     /*  failover duration */
    {                                           /* suList                    */
        CL_AMS_ENTITY_TYPE_SU,                  /*      .type                */
        CL_TRUE,                                /*      .isRanked            */
        CL_FALSE,                               /*      .isValid             */
        0,                                      /*      .numEntities         */
        (ClCntHandleT) NULL                     /*      .list                */
    },
    {                                           /* siList                    */
        CL_AMS_ENTITY_TYPE_SI,                  /*      .type                */
        CL_TRUE,                                /*      .isRanked            */
        CL_FALSE,                               /*      .isValid             */
        0,                                      /*      .numEntities         */
        (ClCntHandleT) NULL                     /*      .list                */
    }
};

ClAmsSUConfigT gClAmsSUDefaultConfig =
{
    {
        CL_AMS_ENTITY_TYPE_SU,                  /* entity.type               */
        {
            sizeof("SUNameUndefined"),          /* entity.name.length        */
            "SUNameUndefined"                   /* entity.name.value         */
        },
        CL_AMS_DEBUG_FLAGS_DEFAULT              /* debug flags               */
    },
    CL_AMS_ADMIN_STATE_LOCKED_I,                /* adminState                */
    0,                                          /* rank (1..4294967295)      */
    0,                                          /* numComponents             */
    CL_TRUE,                                    /* isPreinstantiable         */
    CL_TRUE,                                    /* isRestartable             */
    CL_FALSE,                                   /* isContainerSU             */
    {                                           /* compList                  */
        CL_AMS_ENTITY_TYPE_COMP,                /*      .type                */
        CL_TRUE,                                /*      .isRanked            */
        CL_FALSE,                               /*      .isValid             */
        0,                                      /*      .numEntities         */
        (ClCntHandleT) NULL                     /*      .list                */
    },
    {                                           /* parentSG                  */
        {
            CL_AMS_ENTITY_TYPE_SG,
            {
                sizeof("ParentSGUndefined"),    /* entity.name.length        */
                "ParentSGUndefined"             /* entity.name.value         */
            },
            CL_AMS_DEBUG_FLAGS_DEFAULT          /* debug flags               */
        },
        NULL,                                   /* entity.ptr                */
        0                                       /* nodeHandle                */
    },
    {                                           /* parentNode                */
        {
            CL_AMS_ENTITY_TYPE_NODE,
            {
                sizeof("ParentNodeUndefined"),  /* entity.name.length        */
                "ParentNodeUndefined"           /* entity.name.value         */
            },
            CL_AMS_DEBUG_FLAGS_DEFAULT          /* debug flags               */
        },
        NULL,                                   /* entity.ptr                */
        0                                       /* nodeHandle                */
    }
};

ClAmsSIConfigT gClAmsSIDefaultConfig =
{
    {
        CL_AMS_ENTITY_TYPE_SI,                  /* entity.type               */
        {
            sizeof("SINameUndefined"),          /* entity.name.length        */
            "SINameUndefined"                   /* entity.name.value         */
        },
        CL_AMS_DEBUG_FLAGS_DEFAULT              /* debug flags               */
    },
    CL_AMS_ADMIN_STATE_LOCKED_A,                /* adminState                */
    0,                                          /* rank (1..4294967295)      */
    0,                                          /* numCSIs                   */
    0,                                          /* numStandbyAssignments     */
    0,                                          /* standby assignment order */
    {                                           /* parentSG                  */
        {
            CL_AMS_ENTITY_TYPE_SG,
            {
                sizeof("ParentSGUndefined"),    /* entity.name.length        */
                "ParentSGUndefined"             /* entity.name.value         */
            },
            CL_AMS_DEBUG_FLAGS_DEFAULT          /* debug flags               */
        },
        NULL,                                   /* ptr                       */
        0                                       /* nodeHandle                */
    },
    {                                           /* suRankList                */
        CL_AMS_ENTITY_TYPE_SU,                  /*      .type                */
        CL_TRUE,                                /*      .isRanked            */
        CL_FALSE,                               /*      .isValid             */
        0,                                      /*      .numEntities         */
        (ClCntHandleT) NULL                     /*      .list                */
    },
    {                                           /* siDependentsList          */
        CL_AMS_ENTITY_TYPE_SI,                  /*      .type                */
        CL_FALSE,                               /*      .isRanked            */
        CL_FALSE,                               /*      .isValid             */
        0,                                      /*      .numEntities         */
        (ClCntHandleT) NULL                     /*      .list                */
    },
    {                                           /* siDependenciesList        */
        CL_AMS_ENTITY_TYPE_SI,                  /*      .type                */
        CL_FALSE,                               /*      .isRanked            */
        CL_FALSE,                               /*      .isValid             */
        0,                                      /*      .numEntities         */
        (ClCntHandleT) NULL                     /*      .list                */
    },
    {                                           /* csiList                   */
        CL_AMS_ENTITY_TYPE_CSI,                 /*      .type                */
        CL_FALSE,                               /*      .isRanked            */
        CL_FALSE,                               /*      .isValid             */
        0,                                      /*      .numEntities         */
        (ClCntHandleT) NULL                     /*      .list                */
    },
};

ClAmsCompConfigT gClAmsCompDefaultConfig =
{
    {
        CL_AMS_ENTITY_TYPE_COMP,                /* entity.type               */
        {
            sizeof("ComponentNameUndefined"),   /* entity.name.length        */
            "ComponentNameUndefined"            /* entity.name.value         */
        },
        CL_AMS_DEBUG_FLAGS_DEFAULT              /* debug flags               */
    },
    0,
    NULL,
    {
        sizeof("ProxyCSITypeUndefined"),        /* proxyCSIType.name.length  */
        "ProxyCSITypeUndefined"                 /* proxyCSIType.name.value   */
    },
    CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY,       /* capabilityModel           */
    CL_AMS_COMP_PROPERTY_SA_AWARE,              /* property                  */
    CL_TRUE,                                    /* isRestartable ?           */
    CL_TRUE,                                    /* nodeRebootCleanupFail     */
    1,                                          /* instantiateLevel          */
    1,                                          /* numMaxInstantiate         */
    1,                                          /* numMaxInstantiateWithDelay*/
    1,                                          /* numMaxTerminate           */
    3,                                          /* numMaxAmStart             */
    3,                                          /* numMaxAmStop              */
    1,                                          /* numMaxActiveCSIs          */
    1,                                          /* numMaxStandbyCSIs         */
    {
        20000,                                   /* instantiate = 20s          */
        20000,                                   /* terminate = 20             */
        20000,                                   /* cleanup = 20               */
        20000,                                   /* amStart = 20               */
        20000,                                   /* amStop = 20                */
        20000,                                   /* quiescingComplete = 20     */
        20000,                                   /* csiSetCallback = 20        */
        20000,                                   /* csiRemoveCallback = 20     */
        20000,                                   /* proxiedCompInstantiateCB  */
        20000,                                   /* proxiedCompCleanupCB      */
        10000,                                   /* instantiateDelay = 10s     */
    },
    CL_AMS_RECOVERY_COMP_FAILOVER,               /* recoveryonTimeout         */

    {                                           /* parentSU                  */
        {
            CL_AMS_ENTITY_TYPE_SU,
            {
                sizeof("ParentSUUndefined"),    /* entity.name.length        */
                "ParentSUUndefined"             /* entity.name.value         */
            },
            CL_AMS_DEBUG_FLAGS_DEFAULT          /* debug flags               */
        },
        NULL,                                   /* entity.ptr                */
        0                                       /* nodeHandle                */
    },
    "Undefined",                                /* instantiate command       */
};

ClAmsCSIConfigT gClAmsCSIDefaultConfig =
{
    {
        CL_AMS_ENTITY_TYPE_CSI,                 /* entity.type               */
        {
            sizeof("CSINameUndefined"),         /* entity.name.length        */
            "CSINameUndefined"                  /* entity.name.value         */
        },
        CL_AMS_DEBUG_FLAGS_DEFAULT              /* debug flags               */
    },
    {
        sizeof("CSITypeUndefined"),             /* type.name.length          */
        "CSITypeUndefined"                      /* type.name.value           */
    },
    CL_FALSE,                                   /* isProxyCSI                */
    1,                                          /* rank (1..4294967295)      */
    0,                                          /* nameValuePairList handle  */
    {                                           /* parentSI                  */
        {
            CL_AMS_ENTITY_TYPE_SI,
            {
                sizeof("ParentSIUndefined"),    /* entity.name.length        */
                "ParentSIUndefined"             /* entity.name.value         */
            },
            CL_AMS_DEBUG_FLAGS_DEFAULT          /* debug flags               */
        },
        NULL,                                   /* ptr                       */
        0                                       /* nodeHandle                */
    }
};

