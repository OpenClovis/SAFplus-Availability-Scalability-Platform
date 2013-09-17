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
 * File        : clAmsParser.c
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

#include <clParserApi.h>
#include <clCommonErrors.h>
#include <clAmsEntities.h>
#include <clOsalApi.h>
#include <clAmsMgmtClientApi.h>
#include <clAmsMgmtClientIpi.h>
#include <clBufferApi.h>
#include <clAmsUtils.h>
#include <clAmsParser.h>
#include <clAmsErrors.h>
#include <clCpmMgmt.h>
#include <string.h>

/******************************************************************************/
/*
 * ASP configuration environment variable name 
 */

#define CL_ASP_CONFIG_PATH                 "ASP_CONFIG"

/*
 * AMS logging related tags
 */

#define AMS_CONFIG  "amsConfig"
#define AMS_LOGGING  "amsLogging"
#define ENABLE_AMS_CONSOLE_LOGGING  "enableConsoleLogging"
#define AMS_LOG_MESSAGE_TYPES  "amsLogMessageTypes"
#define ENABLE_AMS_EVENTS_MESSAGES  "enableAmsEventsMessages"
#define ENABLE_AMS_STATE_CHANGE_MESSAGES  "enableAmsStateChangeMessages"
#define ENABLE_AMS_TIMER_MESSAGES  "enableAmsTimerMessages"
#define ENABLE_AMS_FUNCTION_TRACE_MESSAGES  "enableAmsFunctionTraceMessages"


/*
 * Node Entity specific XML tags
 */

#define NODE_TAG_NAME  "node"
#define NODE_TYPES_TAG_NAME  "nodeTypes"
#define NODE_TYPE_TAG_NAME  "nodeType"
#define NODE_INSTANCES_TAG_NAME  "nodeInstances"
#define NODE_INSTANCE_TAG_NAME  "nodeInstance"
#define NODE_DEPENDENCIES_TAG_NAME  "dependencies"
#define NODE_CLASS_TYPES_NAME  "nodeClassTypes" 
#define NODE_CLASS_TYPE_NAME  "nodeClassType" 

/*
 * Appication Entity specific XML tags
 */

#define APP_TYPES_TAG_NAME  "appTypes"
#define APP_TYPE_TAG_NAME   "appType"

/*
 * Service Group Entity specific XML tags
 */

#define SG_TYPES_TAG_NAME "sgTypes"
#define SG_TYPE_TAG_NAME "sgType"
#define SG_INSTANCES_TAG_NAME "serviceGroups"
#define SG_INSTANCE_TAG_NAME "serviceGroup"

/*
 * Service Unit Entity specific XML tags
 */

#define SU_TYPES_TAG_NAME  "suTypes"
#define SU_TYPE_TAG_NAME  "suType"
#define SU_INSTANCES_TAG_NAME  "serviceUnitInstances"
#define SU_INSTANCE_TAG_NAME  "serviceUnitInstance"
#define ASSOCIATED_SERVICE_UNITS  "associatedServiceUnits" 
#define ASSOCIATED_SERVICE_UNIT  "associatedServiceUnit" 

/*
 * Service Instance  Entity specific XML tags
 */

#define SI_TYPES_TAG_NAME  "siTypes"
#define SI_TYPE_TAG_NAME  "siType"
#define SI_DEPENDENCIES_TAG_NAME  "dependencies"
#define SI_PREFFERED_SERVICE_UNITS_TAG  "prefferedServiceUnits"
#define SI_PREFFERED_SERVICE_UNIT_TAG  "prefferedServiceUnit"
#define SERVICE_INSTANCES_TAG_NAME  "serviceInstances"
#define SERVICE_INSTANCE_TAG_NAME  "serviceInstance"

/*
 * Component Instance Entity specific XML tags
 */

#define COMP_TYPES_TAG_NAME  "compTypes"
#define COMP_TYPE_TAG_NAME   "compType"
#define COMP_INSTANCES_TAG_NAME  "componentInstances"
#define COMP_INSTANCE_TAG_NAME  "componentInstance"

/*
 * CSI Instance Entity specific XML tags
 */
#define CSI_TYPES_TAG_NAME  "csiTypes"
#define CSI_TYPE_TAG_NAME   "csiType"
#define CSI_INSTANCES_TAG_NAME  "componentServiceInstances"
#define CSI_INSTANCE_TAG_NAME  "componentServiceInstance"
#define CSI_DEPENDENCIES_TAG_NAME "dependencies"

/******************************************************************************/
/*
 * Default Values for the AMS entities
 */

extern ClAmsEntityConfigT  gClAmsEntityDefaultConfig;
extern ClAmsNodeConfigT  gClAmsNodeDefaultConfig;
extern ClAmsAppConfigT  gClAmsAppDefaultConfig;
extern ClAmsSGConfigT  gClAmsSGDefaultConfig;
extern ClAmsSUConfigT  gClAmsSUDefaultConfig;
extern ClAmsSIConfigT  gClAmsSIDefaultConfig;
extern ClAmsCompConfigT  gClAmsCompDefaultConfig;
extern ClAmsCSIConfigT  gClAmsCSIDefaultConfig;

/******************************************************************************/
/*
 * AMS Managment API related declaration and initialization
 */

ClAmsParserConfigTypeT  *gAmfConfigTypeList= NULL;
static ClAmsMgmtCallbacksT  *gAmsMgmtCallbacks = NULL;
static ClAmsMgmtHandleT  gHandle = -1;
ClVersionT  gVersion = {'B', 01, 01 };

/******************************************************************************/
/*
 * Function to parse the boolean values in the XML File 
 */

ClRcT 
clAmsParserBooleanParser(
        CL_OUT ClBoolT *data_ptr,
        CL_IN ClParserPtrT ptr,
        CL_IN ClCharT *str )
{
    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT ( !data_ptr || !str || !ptr );

    data = clParserChild( ptr, str);

    AMS_CHECKPTR_SILENT (!data || !data->txt);

    if ( !strcmp ( data->txt, "CL_TRUE" ))
    {
        *data_ptr = CL_TRUE;
    }
    else
    {
        *data_ptr = CL_FALSE;
    }

    return CL_OK;
}


/******************************************************************************/

/*
 * Function to parse the Uint32 values in the XML File 
 */

ClRcT 
clAmsParserUint32Parser(
        CL_OUT ClUint32T *data_ptr, 
        CL_IN ClParserPtrT ptr, 
        CL_IN ClCharT *str )
{

    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT (!data_ptr || !ptr || !str);

    data = clParserChild( ptr, str);

    AMS_CHECKPTR_SILENT ( !data  || !data->txt );

    *data_ptr = atoi(data->txt);

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the Uint64 values in the XML File 
 */

ClRcT 
clAmsParserTimeoutParser ( 
        CL_OUT ClInt64T *data_ptr, 
        CL_IN ClParserPtrT ptr, 
        CL_IN ClCharT *str,
        CL_IN ClCharT *entityTypeName )
{
    ClParserPtrT data = NULL;
    ClTimeT defaultTimeout = 0;

    AMS_CHECKPTR_SILENT (!data_ptr || !ptr || !str || !entityTypeName);

    /*
     * Since all entity configs are initialized with the default values
     * data_ptr will contain the default timeout value. This can be used 
     * to set the value incase of zero timeout value in the configuration
     */

    defaultTimeout = *data_ptr;

    data = clParserChild( ptr, str);

    AMS_CHECKPTR_SILENT (!data || !data->txt);

    *data_ptr = strtoll(data->txt, NULL, 10);

    if ( (*data_ptr) == 0 )
    {
        AMS_LOG ( CL_DEBUG_WARN, ("Configuration Timeout Value for "
                    "entityType [%s] and timer [%s] is Zero, Resetting "
                    "it to default value \n",entityTypeName,str)); 

        (*data_ptr) = defaultTimeout;

    }

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the classType value for a node in the XML File 
 */

ClRcT clAmsParserNodeClassTypeParser ( 
        CL_OUT ClAmsNodeConfigT *nodeConfig,
        CL_IN ClParserPtrT ptr )
{
    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT ( !nodeConfig || !ptr );

    data = clParserChild( ptr, "classType");

    AMS_CHECKPTR_SILENT (!data );

    if ( !strcmp( data->txt, "CL_AMS_NODE_CLASS_NONE" )) 
    {
        nodeConfig->classType = CL_AMS_NODE_CLASS_NONE;
    }

    else if ( !strcmp( data->txt, "CL_AMS_NODE_CLASS_A" )) 
    {
        nodeConfig->classType = CL_AMS_NODE_CLASS_A;
    }

    else if ( !strcmp( data->txt, "CL_AMS_NODE_CLASS_B" )) 
    {
        nodeConfig->classType = CL_AMS_NODE_CLASS_B;
    }

    else if ( !strcmp( data->txt, "CL_AMS_NODE_CLASS_C" )) 
    {
        nodeConfig->classType = CL_AMS_NODE_CLASS_C;
    }

    else if ( !strcmp( data->txt, "CL_AMS_NODE_CLASS_D" )) 
    {
        nodeConfig->classType = CL_AMS_NODE_CLASS_D;
    }

    else
    {
        return  CL_AMS_ERR_BAD_CONFIG;
    }

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the loading strategy for a given SG in the XML File 
 */

ClRcT 
clAmsParserSGLoadingStrategyParser( 
        CL_OUT ClAmsSGConfigT *sgConfig,
        CL_IN ClParserPtrT ptr )
{

    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT (!sgConfig || !ptr );

    data = clParserChild( ptr, "loadingStrategy");

    AMS_CHECKPTR_SILENT (!data);

    if ( !strcmp( data->txt, "CL_AMS_SG_LOADING_STRATEGY_NONE" )) 
    {
        sgConfig->loadingStrategy= CL_AMS_SG_LOADING_STRATEGY_NONE;
    }

    else if ( !strcmp( data->txt, "CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU" )) 
    {
        sgConfig->loadingStrategy= CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU;
    }

    else if ( !strcmp( data->txt, "CL_AMS_SG_LOADING_STRATEGY_LEAST_SU_ASSIGNED" )) 
    {
        sgConfig->loadingStrategy= CL_AMS_SG_LOADING_STRATEGY_LEAST_SU_ASSIGNED;
    }
    
    else if ( !strcmp( data->txt, "CL_AMS_SG_LOADING_STRATEGY_LEAST_LOAD_PER_SU" )) 
    {
        sgConfig->loadingStrategy= CL_AMS_SG_LOADING_STRATEGY_LEAST_LOAD_PER_SU;
    }
    
    else if ( !strcmp( data->txt, "CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE" )) 
    {
        sgConfig->loadingStrategy= CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE;
    }

    else if ( !strcmp( data->txt, "CL_AMS_SG_LOADING_STRATEGY_USER_DEFINED" )) 
    {
        sgConfig->loadingStrategy= CL_AMS_SG_LOADING_STRATEGY_USER_DEFINED;
    }
    else
    {
        return CL_AMS_ERR_BAD_CONFIG;
    }

    return CL_OK;

}

/******************************************************************************/

/*
 * Function to parse the redundacny model for a SG the XML File 
 */

ClRcT 
clAmsParserSGRedundancyModelParser ( 
        CL_OUT ClAmsSGConfigT *sgConfig,
        CL_IN ClParserPtrT ptr)
{
    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT (!sgConfig || !ptr);

    data = clParserChild( ptr, "redundancyModel");

    AMS_CHECKPTR_SILENT (!data);

    if ( !strcmp( data->txt, "CL_AMS_SG_REDUNDANCY_MODEL_NONE" )) 
    {
        sgConfig->redundancyModel= CL_AMS_SG_REDUNDANCY_MODEL_NONE;
    }

    else if ( !strcmp( data->txt, "CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY" )) 
    {
        sgConfig->redundancyModel= CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY;
    }

    else if ( !strcmp( data->txt, "CL_AMS_SG_REDUNDANCY_MODEL_TWO_N" )) 
    {
        sgConfig->redundancyModel= CL_AMS_SG_REDUNDANCY_MODEL_TWO_N;
    }

    else if ( !strcmp( data->txt, "CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N" )) 
    {
        sgConfig->redundancyModel= CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N;
    }

    else if ( !strcmp( data->txt, "CL_AMS_SG_REDUNDANCY_MODEL_N_WAY" )) 
    {
        sgConfig->redundancyModel= CL_AMS_SG_REDUNDANCY_MODEL_N_WAY;
    }
    
    else if ( !strcmp( data->txt, "CL_AMS_SG_REDUNDANCY_MODEL_N_WAY_ACTIVE" )) 
    {
        sgConfig->redundancyModel= CL_AMS_SG_REDUNDANCY_MODEL_N_WAY_ACTIVE;
    }

    else if ( !strcmp( data->txt, "CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM" )) 
    {
        sgConfig->redundancyModel= CL_AMS_SG_REDUNDANCY_MODEL_CUSTOM;
    }

    else
    {
        return CL_AMS_ERR_BAD_CONFIG;
    }

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the component property in the XML File 
 */

ClRcT 
clAmsParserCompPropertyParser(
        CL_OUT ClUint32T *property,
        CL_IN ClParserPtrT ptr)
{
    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT ( !property || !ptr );

    data = clParserChild( ptr, "property");

    AMS_CHECKPTR_SILENT (!data || !data->txt );

    if ( !strcmp (data->txt,"CL_AMS_SA_AWARE"))
    {
        *property = CL_AMS_COMP_PROPERTY_SA_AWARE;
    }

    else if ( !strcmp (data->txt,"CL_AMS_PROXIED_PREINSTANTIABLE"))
    {
        *property = CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE;
    }

    else if ( !strcmp (data->txt,"CL_AMS_PROXIED_NON_PREINSTANTIABLE"))
    {
        *property = CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE;
    }

    else if ( !strcmp (data->txt,"CL_AMS_NON_PROXIED_NON_PREINSTANTIABLE"))
    {
        *property = CL_AMS_COMP_PROPERTY_NON_PROXIED_NON_PREINSTANTIABLE;
    }

    else
    {
        return CL_AMS_ERR_BAD_CONFIG;
    }

    return CL_OK;
}

/*
 * Function to parse the component instantiate command in the XML File
 */

ClRcT 
clAmsParserCompInstantiateCommandParser(
        CL_OUT ClCharT *instantiateCommand,
        CL_IN ClParserPtrT ptr)
{
    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT ( !instantiateCommand || !ptr );

    data = clParserChild( ptr, "instantiateCommand");

    AMS_CHECKPTR_SILENT (!data || !data->txt );

    strncpy(instantiateCommand, data->txt, CL_MAX_NAME_LENGTH-1);

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the component timeouts in the XML File 
 */

ClRcT 
clAmsParserCompTimeoutsParser( 
        CL_OUT ClAmsCompConfigT *compConfig,
        CL_IN ClParserPtrT ptr )
{
    ClParserPtrT data = NULL;
    ClCharT *compName = NULL;

    compName = (ClCharT *)(((ClAmsEntityConfigT *)compConfig)->name.value);

    AMS_CHECKPTR_SILENT (!compConfig || !ptr );

    data = clParserChild( ptr, "timeouts");

    AMS_CHECKPTR_SILENT (!data);

    clAmsParserTimeoutParser(
            &compConfig->timeouts.instantiate,
            data,
            "instantiateTimeout",
            compName);

    /* XXX Uncomment after ClovisWorks change
    clAmsParserTimeoutParser(
            &compConfig->timeouts.instantiateDelay,
            data,
            "instantiateDelay",
            compName);
            */

    clAmsParserTimeoutParser(
            &compConfig->timeouts.terminate,
            data,
            "terminateTimeout",
            compName);

    clAmsParserTimeoutParser(
            &compConfig->timeouts.cleanup,
            data,
            "cleanupTimeout",
            compName);

    clAmsParserTimeoutParser(
            &compConfig->timeouts.amStart,
            data,
            "amStartTimeout",
            compName);

    clAmsParserTimeoutParser(
            &compConfig->timeouts.amStop,
            data,
            "amStopTimeout",
            compName);

    clAmsParserTimeoutParser(
            &compConfig->timeouts.quiescingComplete,
            data,
            "quiescingCompleteTimeout",
            compName);

    clAmsParserTimeoutParser(
            &compConfig->timeouts.csiSet,
            data,
            "csiSetTimeout",
            compName);

    clAmsParserTimeoutParser(
            &compConfig->timeouts.csiRemove,
            data,
            "csiRemoveTimeout",
            compName);

    clAmsParserTimeoutParser(
            &compConfig->timeouts.proxiedCompInstantiate,
            data,
            "proxiedCompInstantiateTimeout",
            compName);

    clAmsParserTimeoutParser(
            &compConfig->timeouts.proxiedCompCleanup,
            data,
            "proxiedCompCleanupTimeout",
            compName);

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the Recovery on timeout for a component in the XML File 
 */

ClRcT 
clAmsParserRecoveryOnTmOutParser( 
        CL_OUT ClAmsCompConfigT *compConfig,
        CL_IN ClParserPtrT ptr)
{

    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT ( !compConfig || !ptr );

    data = clParserChild( ptr, "recoveryOnTimeout");

    AMS_CHECKPTR_SILENT (!data);

    if ( !strcmp (data->txt, "CL_AMS_RECOVERY_NO_RECOMMENDATION"))
    {
        compConfig->recoveryOnTimeout = CL_AMS_RECOVERY_NO_RECOMMENDATION;
    }

    else if ( !strcmp (data->txt, "CL_AMS_RECOVERY_INTERNALLY_RECOVERED"))
    {
        compConfig->recoveryOnTimeout = CL_AMS_RECOVERY_INTERNALLY_RECOVERED;
    }

    else if ( !strcmp (data->txt, "CL_AMS_RECOVERY_COMP_RESTART"))
    {
        compConfig->recoveryOnTimeout = CL_AMS_RECOVERY_COMP_RESTART;
    }

    else if ( !strcmp (data->txt, "CL_AMS_RECOVERY_SU_RESTART"))
    {
        compConfig->recoveryOnTimeout = CL_AMS_RECOVERY_SU_RESTART;
    }

    else if ( !strcmp (data->txt, "CL_AMS_RECOVERY_COMP_FAILOVER"))
    {
        compConfig->recoveryOnTimeout = CL_AMS_RECOVERY_COMP_FAILOVER;
    }

    else if ( !strcmp (data->txt, "CL_AMS_RECOVERY_NODE_SWITCHOVER"))
    {
        compConfig->recoveryOnTimeout = CL_AMS_RECOVERY_NODE_SWITCHOVER;
    }

    else if ( !strcmp (data->txt, "CL_AMS_RECOVERY_NODE_FAILOVER"))
    {
        compConfig->recoveryOnTimeout = CL_AMS_RECOVERY_NODE_FAILOVER;
    }

    else if ( !strcmp (data->txt, "CL_AMS_RECOVERY_NODE_FAILFAST"))
    {
        compConfig->recoveryOnTimeout = CL_AMS_RECOVERY_NODE_FAILFAST;
    }

    else if ( !strcmp (data->txt, "CL_AMS_RECOVERY_CLUSTER_RESET"))
    {
        compConfig->recoveryOnTimeout = CL_AMS_RECOVERY_CLUSTER_RESET;
    }

    else
    {
        return CL_AMS_ERR_BAD_CONFIG;
    }

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the capability model for a component in the XML File 
 */

ClRcT 
clAmsParserCompCapabilityModelParser ( 
        CL_OUT ClAmsCompConfigT *compConfig,
        CL_IN ClParserPtrT ptr)
{
    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT ( !compConfig || !ptr );
 
    data = clParserChild( ptr, "capabilityModel");

    AMS_CHECKPTR_SILENT (!data || !data->txt );

    if ( !strcmp (data->txt,"CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY"))
    {
        compConfig->capabilityModel= CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY;
    }

    else if ( !strcmp (data->txt,"CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY"))
    {
        compConfig->capabilityModel= CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY;
    }

    else if ( !strcmp (data->txt,"CL_AMS_COMP_CAP_ONE_ACTIVE_OR_X_STANDBY"))
    {
        compConfig->capabilityModel= CL_AMS_COMP_CAP_ONE_ACTIVE_OR_X_STANDBY;
    }

    else if ( !strcmp (data->txt,"CL_AMS_COMP_CAP_ONE_ACTIVE_OR_ONE_STANDBY"))
    {
        compConfig->capabilityModel= CL_AMS_COMP_CAP_ONE_ACTIVE_OR_ONE_STANDBY;
    }

    else if ( !strcmp (data->txt,"CL_AMS_COMP_CAP_X_ACTIVE"))
    {
        compConfig->capabilityModel= CL_AMS_COMP_CAP_X_ACTIVE;
    }

    else if ( !strcmp (data->txt,"CL_AMS_COMP_CAP_ONE_ACTIVE"))
    {
        compConfig->capabilityModel= CL_AMS_COMP_CAP_ONE_ACTIVE ;
    }

    else if ( !strcmp (data->txt,"CL_AMS_COMP_CAP_NON_PREINSTANTIABLE"))
    {
        compConfig->capabilityModel= CL_AMS_COMP_CAP_NON_PREINSTANTIABLE; 
    }

    else
    {
        return CL_AMS_ERR_BAD_CONFIG;
    }

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the admin state of an entity in the XML File 
 */

ClRcT 
clAmsParserEntityAdminStateParser( 
        CL_OUT ClAmsAdminStateT *adminState,
        CL_IN ClParserPtrT ptr )
{
    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT (!adminState || !ptr);

    data = clParserChild( ptr, "adminState");

    AMS_CHECKPTR_SILENT (!data || !data->txt);

    if ( !strcmp( data->txt, "CL_AMS_ADMIN_STATE_NONE" )) 
    {
        *adminState = CL_AMS_ADMIN_STATE_NONE;
    }

    else if ( !strcmp( data->txt, "CL_AMS_ADMIN_STATE_UNLOCKED" )) 
    {
        *adminState = CL_AMS_ADMIN_STATE_UNLOCKED;
    }

    else if ( !strcmp( data->txt, "CL_AMS_ADMIN_STATE_LOCKED_A" )) 
    {
        *adminState = CL_AMS_ADMIN_STATE_LOCKED_A;
    }

    else if ( !strcmp( data->txt, "CL_AMS_ADMIN_STATE_LOCKED_I" )) 
    {
        *adminState = CL_AMS_ADMIN_STATE_LOCKED_I;
    }

    else if ( !strcmp( data->txt, "CL_AMS_ADMIN_STATE_SHUTTINGDOWN" )) 
    {
        *adminState = CL_AMS_ADMIN_STATE_SHUTTINGDOWN;
    }

    else if ( !strcmp( data->txt, "CL_AMS_ADMIN_STATE_SHUTTINGDOWN_RESTART" )) 
    {
        *adminState = CL_AMS_ADMIN_STATE_SHUTTINGDOWN_RESTART;
    }

    else
    {
       return   CL_AMS_ERR_BAD_CONFIG;
    }

    return CL_OK;
}
/******************************************************************************/

/*
 * Function to parse an attribute string in the XML File 
 */

ClRcT 
clAmsParserStringParser(
        ClCharT **data_ptr, 
        ClParserPtrT ptr, 
        ClCharT *str )
{
    const ClCharT *data = NULL;
    ClUint32T size=0;

    AMS_CHECKPTR_SILENT ( !data_ptr||!ptr||!str );

    data = clParserAttr( ptr, str);

    AMS_CHECKPTR_SILENT (!data);

    size = strlen (data) +1;
    *data_ptr = clHeapAllocate (size);
    memset (*data_ptr,0,size);
    strcpy (*data_ptr,data);

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the child with a given name in the XML File and copy it to
 * SaNameT structure
 */

ClRcT 
clAmsParserSaNameParser (
        CL_OUT SaNameT *saName,
        CL_IN ClParserPtrT ptr, 
        CL_IN ClCharT *str )
{

    ClParserPtrT data = NULL;

    AMS_CHECKPTR_SILENT ( !saName||!ptr||!str );

    data = clParserChild( ptr, str);

    AMS_CHECKPTR_SILENT ( !data||!data->txt );

    memset ((ClCharT *)saName,0,sizeof(SaNameT));
    strcpy ((ClCharT *)saName->value, data->txt);
    saName->length = strlen (data->txt) + 1;

    return CL_OK;
}

/******************************************************************************/

/*
 * This function parses the entity name attributes in the XML file and makes
 * a entity type structure based on the entity type and name values
 */

ClRcT clAmsParserEntityAttrParser(
        CL_OUT ClAmsEntityConfigT *entityConfig,
        CL_IN ClAmsEntityTypeT entityType,
        CL_IN ClParserPtrT ptr,
        CL_IN ClCharT *str)
{

    const ClCharT *attr = NULL;

    AMS_CHECKPTR_SILENT (!entityConfig||!ptr||!str);

    attr = clParserAttr( ptr, str);

    AMS_CHECKPTR_SILENT (!attr);

    memset (&entityConfig->name,0,sizeof (SaNameT));

    switch (entityType)
    {
        case CL_AMS_ENTITY_TYPE_ENTITY:
            break;

        case CL_AMS_ENTITY_TYPE_NODE:
            {
                ClAmsNodeConfigT    *config = (ClAmsNodeConfigT *)entityConfig;
                config->entity.type = CL_AMS_ENTITY_TYPE_NODE;
                strcpy ((ClCharT *)config->entity.name.value, attr );
                config->entity.name.length = strlen ((const ClCharT *)config->entity.name.value)+1;

                break;
            }

        case CL_AMS_ENTITY_TYPE_APP:
            {
                ClAmsAppConfigT *config = (ClAmsAppConfigT *)entityConfig;
                config->entity.type = CL_AMS_ENTITY_TYPE_APP;
                strcpy ((ClCharT *)config->entity.name.value, attr);
                config->entity.name.length = strlen ((const ClCharT *)config->entity.name.value)+1;

                break;
            }
        case CL_AMS_ENTITY_TYPE_SG:
            {
                ClAmsSGConfigT  *config = (ClAmsSGConfigT *)entityConfig;
                config->entity.type = CL_AMS_ENTITY_TYPE_SG;
                strcpy ((ClCharT *)config->entity.name.value, attr);
                config->entity.name.length = strlen ((const ClCharT *)config->entity.name.value)+1;

                break;
            }
        case CL_AMS_ENTITY_TYPE_SU:
            {
                ClAmsSUConfigT  *config = (ClAmsSUConfigT *)entityConfig;
                config->entity.type = CL_AMS_ENTITY_TYPE_SU;
                strcpy ((ClCharT *)config->entity.name.value, attr);
                config->entity.name.length = strlen ((const ClCharT *)config->entity.name.value)+1;

                break;
            }
        case CL_AMS_ENTITY_TYPE_SI:
            {
                ClAmsSIConfigT  *config = (ClAmsSIConfigT *)entityConfig;
                config->entity.type = CL_AMS_ENTITY_TYPE_SI;
                strcpy ((ClCharT *)config->entity.name.value, attr);
                config->entity.name.length = strlen ((const ClCharT *)config->entity.name.value)+1;

                break;
            }
        case CL_AMS_ENTITY_TYPE_COMP:
            {
                ClAmsCompConfigT    *config = (ClAmsCompConfigT *)entityConfig;
                config->entity.type = CL_AMS_ENTITY_TYPE_COMP;
                strcpy ((ClCharT *)config->entity.name.value, attr);
                config->entity.name.length = strlen ((const ClCharT *)config->entity.name.value)+1;

                break;
            }
        case CL_AMS_ENTITY_TYPE_CSI:
            {
                ClAmsCSIConfigT *config = (ClAmsCSIConfigT *)entityConfig;
                config->entity.type = CL_AMS_ENTITY_TYPE_CSI;
                strcpy ((ClCharT *)config->entity.name.value, attr);
                config->entity.name.length = strlen ((const ClCharT *)config->entity.name.value)+1;

                break;
            }
        default:
            { 
                AMS_LOG(CL_DEBUG_ERROR,
                        ("Function[%s]:invalid entity type [%d]\n",__FUNCTION__,entityType)); 
                return CL_AMS_ERR_INVALID_ENTITY;
            }
    }

    return CL_OK;
}
/******************************************************************************/

/*
 * Function to parse the CSI type values in the definitions XML File 
 */

ClRcT 
clAmsParserCSIDefParser(
        CL_IN ClParserPtrT csi )
{
    ClRcT rc = CL_OK;
    ClAmsCSIConfigT *csiConfig = NULL ;
    ClParserPtrT nameValueLists = NULL;
    ClParserPtrT nameValueList = NULL;

    AMS_CHECKPTR_SILENT ( !csi );

    /*
     *  For each csiConfig go through the list of the csis 
     */

    csiConfig = clHeapAllocate (sizeof (ClAmsCSIConfigT));

    AMS_CHECK_NO_MEMORY ( csiConfig );

    /*
     * Copy the default config
     */

    memcpy (csiConfig,&gClAmsCSIDefaultConfig,sizeof (ClAmsCSIConfigT));

    /*
     * Read the entity name and type
     */

    if ( ( rc = clAmsParserEntityAttrParser(
                    (ClAmsEntityConfigT *)csiConfig,
                    CL_AMS_ENTITY_TYPE_CSI,
                    csi,
                    "name" ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("<csiType>: missing name attribute \n")); 
        return rc;
    }

    if ( ( rc = clAmsParserSaNameParser (
                    &csiConfig->type,
                    csi,
                    "type" ))
            != CL_OK )
    {
        /*
         * In order to be backward compatible with multiple CSItype suppport
         * per compType, we make the name and the type the same
         * as its expected to have been in the first place.
         */
        memcpy(&csiConfig->type, &csiConfig->entity.name, sizeof(csiConfig->type));
        rc = CL_OK;
    }

    clAmsParserUint32Parser (
            &csiConfig->rank,
            csi,
            "rank");

     /* 
      * Add to list 
      */

     ClAmsParserConfigTypeT newConfigType = {0}; 
     ClAmsParserCSINVPListT *nvpHead = NULL;
     ClAmsParserCSINVPListT *ptr = nvpHead;

     /*
      * Parse the list of nvp's for this CSI
      */

     nameValueLists = clParserChild (csi,"nameValueLists");

     if ( !nameValueLists )
     {
         goto setCSIConfig;
     }

     nameValueList = clParserChild (nameValueLists,"nameValue");


     while (nameValueList)
     { 
         ClAmsParserCSINVPListT  *nvp = clHeapAllocate (sizeof (ClAmsParserCSINVPListT));

         AMS_CHECK_NO_MEMORY (nvp);

         if (( rc = clAmsParserStringParser (
                         &nvp->paramName,
                         nameValueList,
                         "name"))
                 != CL_OK )
         {
             AMS_LOG(CL_DEBUG_ERROR,
                     ("csiType[%s] : missing <name> attribute for tag <nameValue> \n", 
                      csiConfig->entity.name.value)); 
             return rc;
         }
         
         if (( rc = clAmsParserStringParser (
                         &nvp->paramValue,
                         nameValueList,
                         "value"))
                 != CL_OK )
         {
             AMS_LOG(CL_DEBUG_ERROR,
                     ("csiType[%s] : missing <value> attribute for tag <nameValue> \n", 
                      csiConfig->entity.name.value)); 
             return rc;
         } 
         
         nvp->csiName = clHeapAllocate(strlen((const ClCharT *)csiConfig->entity.name.value) + 1);
         AMS_CHECK_NO_MEMORY (nvp->csiName);
         memset (nvp->csiName,0,strlen((const ClCharT *)csiConfig->entity.name.value) + 1);
         strcpy (nvp->csiName,(const ClCharT *)csiConfig->entity.name.value);

         nvp->next = NULL;
         ptr = nvpHead;

         if ( !ptr )
         {
             /*
              * First node in the list
              */ 

             nvpHead = nvp; 
             nameValueList = nameValueList->next;
             continue;

         }

         while ( ptr->next != NULL)
         {
             ptr = ptr->next;
         } 
         
         ptr->next = nvp;
         nameValueList = nameValueList->next;
     }

setCSIConfig:

     newConfigType.entityConfig = (ClAmsEntityConfigT *)csiConfig;
     newConfigType.nvp = nvpHead;


     if ( ( rc = clAmsParserAddEntityTypeConfig (
                     &gAmfConfigTypeList,
                     newConfigType))
             != CL_OK )
     {
         AMS_LOG(CL_DEBUG_ERROR,
                 ("CSI Type [%s] : Error in adding to the configuration type list\n", 
                  csiConfig->entity.name.value)); 
         return rc;
     }

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the Component type values in the definitions XML File 
 */

ClRcT 
clAmsParserCompDefParser(
                         CL_IN ClParserPtrT comp )
{

    ClRcT rc = CL_OK;
    ClAmsCompConfigT *compConfig = NULL ;
    SaNameT *pSupportedCSITypes =  NULL;
    ClUint32T numSupportedCSITypes = 1;
    ClParserPtrT csiTypeInstances = NULL;
    ClParserPtrT csiTypeInstance = NULL;

    AMS_CHECKPTR ( !comp );

    /*
     *  For each compConfig go through the list of the comps 
     */
    
    compConfig = clHeapCalloc (1, sizeof (ClAmsCompConfigT));
    AMS_CHECK_NO_MEMORY (compConfig);

    /*
     * Copy the default config
     */

    memcpy (compConfig,&gClAmsCompDefaultConfig,sizeof (ClAmsCompConfigT));

    /*
     * Read the entity name and type
     */

    if ( ( rc = clAmsParserEntityAttrParser(
                                            (ClAmsEntityConfigT *)compConfig,
                                            CL_AMS_ENTITY_TYPE_COMP,
                                            comp,
                                            "name"))
         != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("<compType>: missing name attribute \n")); 
        return rc;
    }

    /*
     * capabilityModel
     */

    if ( ( rc = clAmsParserCompCapabilityModelParser (
                                                      compConfig,
                                                      comp ))
         != CL_OK )
    {
        if ( rc == CL_AMS_ERR_BAD_CONFIG )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("Component Type [%s] : bad <capabilityModel> tag\n",
                     compConfig->entity.name.value));
            return rc;
        }
    }

    if (( rc = clAmsParserCompPropertyParser(
                                             &compConfig->property,
                                             comp ))
        != CL_OK )
    {
        if ( rc == CL_AMS_ERR_BAD_CONFIG )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("Component Type [%s] : bad <property> tag\n",
                     compConfig->entity.name.value));
            return rc;
        }
    }

    if (( rc = clAmsParserCompInstantiateCommandParser(
                                                       compConfig->instantiateCommand,
                                                       comp ))
        != CL_OK )
    {
        if ( rc == CL_AMS_ERR_BAD_CONFIG )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("Component Type [%s] : bad <instantiateCommand> tag\n",
                     compConfig->entity.name.value));
            return rc;
        }
    }

    /*    cpmCompAppendInstantiateCommand(compConfig->entity.name.value, 
                                    compConfig->instantiateCommand,
                                    sizeof(compConfig->instantiateCommand) - 1);*/

    pSupportedCSITypes = clHeapCalloc(numSupportedCSITypes, 
                                      (ClUint32T)sizeof(SaNameT));
    if(!pSupportedCSITypes)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error allocating [%d] bytes", 
                                 (ClUint32T)sizeof(SaNameT)));
        return CL_AMS_RC(CL_ERR_NO_MEMORY);
    }

    /*
     * Get supportedCSIType
     */
    csiTypeInstances = clParserChild(comp, CSI_TYPES_TAG_NAME);
    if(!csiTypeInstances)
    {
        /*
         * Be backward compatible
         */
        if ( ( rc = clAmsParserSaNameParser (
                                             pSupportedCSITypes,
                                             comp,
                                             CSI_TYPE_TAG_NAME))
             != CL_OK )
        {
            if( (compConfig->property != CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE)
                &&
                (compConfig->property != CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE))
            {
               
                AMS_LOG(CL_DEBUG_ERROR,
                        ("Component Type [%s] : missing [%s] tag\n",
                         compConfig->entity.name.value, CSI_TYPE_TAG_NAME));
                clAmsFreeMemory(pSupportedCSITypes);
                return rc;
            }
        }
    }
    else
    {
        csiTypeInstance = clParserChild(csiTypeInstances, CSI_TYPE_TAG_NAME);
        if(!csiTypeInstance)
        {
            if( (compConfig->property == CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE)
                ||
                (compConfig->property == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE))
            {
                clAmsFreeMemory(pSupportedCSITypes);
                compConfig->pSupportedCSITypes = pSupportedCSITypes = NULL;
                compConfig->numSupportedCSITypes = numSupportedCSITypes = 0;
                goto proxy_csi_type;
            }

            AMS_LOG(CL_DEBUG_ERROR,
                    ("Component type [%.*s]: missing [%s] tags\n",
                     compConfig->entity.name.length, 
                     compConfig->entity.name.value,
                     CSI_TYPE_TAG_NAME));
            clAmsFreeMemory(pSupportedCSITypes);
            return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
        }
        numSupportedCSITypes = 0;
        while(csiTypeInstance)
        {
            const ClCharT *pData = clParserAttr(csiTypeInstance, "name");
            if(!pData)
            {
                AMS_LOG(CL_DEBUG_ERROR,
                        ("Component type [%.*s] missing csi name tag\n",
                         compConfig->entity.name.length, 
                         compConfig->entity.name.value));
                clAmsFreeMemory(pSupportedCSITypes);
                return CL_AMS_RC(CL_AMS_ERR_BAD_CONFIG);
            }
            /*
             * TODO - call saNameSet instead but we cannot be consistent
             * in one place and different in another.
             */
            pSupportedCSITypes[numSupportedCSITypes].length =
                CL_MIN(strlen(pData)+1, CL_MAX_NAME_LENGTH-1);
            strncpy((ClCharT *)pSupportedCSITypes[numSupportedCSITypes].value,
                    pData, CL_MAX_NAME_LENGTH-1);
            ++numSupportedCSITypes;
            csiTypeInstance = csiTypeInstance->next;
            if(csiTypeInstance)
            {
                pSupportedCSITypes = clHeapRealloc(pSupportedCSITypes,
                                                   (ClUint32T)sizeof(SaNameT)
                                                   * (numSupportedCSITypes+1));
                if(!pSupportedCSITypes)
                {
                    AMS_LOG(CL_DEBUG_ERROR,
                            ("Error allocating [%d] bytes while iterating over "\
                             "comp supported csitype instances\n", 
                             (ClUint32T)sizeof(SaNameT)*(numSupportedCSITypes+1)));
                    return CL_AMS_RC(CL_ERR_NO_MEMORY);
                }
            }
        }
    }
    
    compConfig->pSupportedCSITypes = pSupportedCSITypes;
    compConfig->numSupportedCSITypes = numSupportedCSITypes;

    /*
     * If the component is a proxied component read the proxyCSIType
     */

    if (( compConfig->property == CL_AMS_COMP_PROPERTY_PROXIED_PREINSTANTIABLE) ||
        (compConfig->property == CL_AMS_COMP_PROPERTY_PROXIED_NON_PREINSTANTIABLE ))
    {

    proxy_csi_type:

        if ( ( rc = clAmsParserSaNameParser (
                                             &compConfig->proxyCSIType,
                                             comp,
                                             "proxyCSIType" ))
             != CL_OK )
        {

            AMS_LOG(CL_DEBUG_ERROR,
                    ("Component Type [%s] : missing/bad <proxyCSIType> tag\n",
                     compConfig->entity.name.value));
            return CL_AMS_RC (rc);
        }
    } 
    
    clAmsParserBooleanParser(
                             &compConfig->isRestartable,
                             comp,
                             "isRestartable");

    clAmsParserBooleanParser(
                             &compConfig->nodeRebootCleanupFail,
                             comp,
                             "nodeRebootCleanupFail");

    clAmsParserUint32Parser(
                            &compConfig->instantiateLevel,
                            comp,
                            "instantiateLevel");
    /*
     * Zero is not a valid instantiate level.
     */
    if(!compConfig->instantiateLevel)
        compConfig->instantiateLevel = 1;

    // XXX Remove this after ClovisWorks change
    clAmsParserTimeoutParser(
                             &compConfig->timeouts.instantiateDelay,
                             comp,
                             "instantiateDelay",
                             (ClCharT *)compConfig->entity.name.value);

    clAmsParserUint32Parser(
                            &compConfig->numMaxInstantiate,
                            comp,
                            "numMaxInstantiate");

    clAmsParserUint32Parser(
                            &compConfig->numMaxInstantiateWithDelay,
                            comp,
                            "numMaxInstantiateWithDelay");

    clAmsParserUint32Parser(
                            &compConfig->numMaxTerminate,
                            comp,
                            "numMaxTerminate");

    clAmsParserUint32Parser(
                            &compConfig->numMaxAmStart,
                            comp,
                            "numMaxAmStart");

    clAmsParserUint32Parser(
                            &compConfig->numMaxAmStop,
                            comp,
                            "numMaxAmStop");

    clAmsParserUint32Parser(
                            &compConfig->numMaxActiveCSIs,
                            comp,
                            "numMaxActiveCSIs");

    clAmsParserUint32Parser(
                            &compConfig->numMaxStandbyCSIs,
                            comp,
                            "numMaxStandbyCSIs");

    /*
     * timeouts
     */ 
    
    clAmsParserCompTimeoutsParser(
                                  compConfig,
                                  comp);

    /*
     * recoveryOnTimeout
     */

    clAmsParserRecoveryOnTmOutParser(
                                     compConfig,
                                     comp );

    /* 
     * Add to list 
     */

    ClAmsParserConfigTypeT newConfigType = {0}; 

    newConfigType.nvp = NULL;
    newConfigType.entityConfig = (ClAmsEntityConfigT *)compConfig;

    if (( rc = clAmsParserAddEntityTypeConfig ( 
                                               &gAmfConfigTypeList,
                                               newConfigType))
        != CL_OK )
    { 
        AMS_LOG(CL_DEBUG_ERROR,
                ("Component Type [%s] : Error in adding to the configuration type list\n", 
                 compConfig->entity.name.value)); 
        return rc;
    }

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the SI type values in the definitions XML File 
 */

ClRcT 
clAmsParserSIDefParser(
        CL_IN ClParserPtrT si )
{
    ClRcT rc = CL_OK;
    ClAmsSIConfigT *siConfig = NULL ;

    AMS_CHECKPTR ( !si );

    /*
     *  For each siConfig go through the list of the sis 
     */

    siConfig = clHeapAllocate (sizeof (ClAmsSIConfigT));
    AMS_CHECK_NO_MEMORY ( siConfig );

    /*
     * Copy the default config
     */

    memcpy (siConfig,&gClAmsSIDefaultConfig,sizeof (ClAmsSIConfigT));

    /*
     * Read the entity name and type
     */

    if (( rc = clAmsParserEntityAttrParser(
                    (ClAmsEntityConfigT *)siConfig,
                    CL_AMS_ENTITY_TYPE_SI,
                    si,
                    "name"))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("<siType>: missing name attribute \n")); 
        return rc;
    }
    
    /*
     * Read the adminState
     */

    if ( ( rc = clAmsParserEntityAdminStateParser (
                    &siConfig->adminState,
                    si ))
            != CL_OK )
    {
        if ( rc == CL_AMS_ERR_BAD_CONFIG )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("SI Type [%s] : bad <adminState> tag\n",
                     siConfig->entity.name.value));
            return rc;
        }
    }

    /*
     * Read the rank 
     */

      clAmsParserUint32Parser (
              &siConfig->rank,
              si,
              "rank");

    /*
     * Read the numCSIs 
     */

      clAmsParserUint32Parser (
              &siConfig->numCSIs,
              si,
              "numCSIs");

    /*
     * Read the numStandbyAssignments 
     */

      clAmsParserUint32Parser (
              &siConfig->numStandbyAssignments,
              si,
              "numStandbyAssignments");


      clAmsParserUint32Parser (
              &siConfig->standbyAssignmentOrder,
              si,
              "standbyAssignmentOrder");

     /* 
      * Add to list 
      */

     ClAmsParserConfigTypeT newConfigType = {0}; 

     newConfigType.nvp = NULL;
     newConfigType.entityConfig = (ClAmsEntityConfigT *)siConfig;

     if (( rc = clAmsParserAddEntityTypeConfig (
                     &gAmfConfigTypeList,
                     newConfigType ))
             != CL_OK )
     {
         AMS_LOG(CL_DEBUG_ERROR,
                 ("SI Type [%s] : Error in adding to the configuration type list\n", 
                  siConfig->entity.name.value)); 
         return rc;
     }

    return CL_OK;

}

/******************************************************************************/

/*
 * Function to parse the SU type values in the definitions XML File 
 */

ClRcT 
clAmsParserSUDefParser( 
        CL_IN ClParserPtrT su )
{
    ClRcT rc = CL_OK;
    ClAmsSUConfigT *suConfig = NULL ;

    AMS_CHECKPTR ( !su );

    /*
     *  For each suConfig go through the list of the sus 
     */
    
    suConfig = clHeapAllocate (sizeof (ClAmsSUConfigT));
    AMS_CHECK_NO_MEMORY ( suConfig );

    /*
     * Copy the default config
     */

    memcpy (suConfig,&gClAmsSUDefaultConfig,sizeof (ClAmsSUConfigT));

    /*
     * Read the entity name and type
     */

    if ( ( rc = clAmsParserEntityAttrParser(
                    (ClAmsEntityConfigT *)suConfig,
                    CL_AMS_ENTITY_TYPE_SU,
                    su,
                    "name"))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("<suType>: missing name attribute \n")); 
        return rc;
    }


    /*
     * Read the adminState
     */

    if ( ( rc = clAmsParserEntityAdminStateParser (
                    &suConfig->adminState,
                    su ))
            != CL_OK )
    {
        if ( rc == CL_AMS_ERR_BAD_CONFIG )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("SU Type [%s] : bad <adminState> tag\n",
                     suConfig->entity.name.value));
            return rc;
        }
    }

    /*
     * Read the rank 
     */

      clAmsParserUint32Parser (
              &suConfig->rank,
              su,
              "rank");

    /*
     * Read the numComponents
     */

      clAmsParserUint32Parser (
              &suConfig->numComponents,
              su,
              "numComponents");

    /*
     * Read the isPreinstantiable
     */

     clAmsParserBooleanParser(
             &suConfig->isPreinstantiable,
             su,
             "isPreinstantiable");

    /*
     * Read the isRestartable
     */

    clAmsParserBooleanParser(
            &suConfig->isRestartable,
            su,
            "isRestartable");

    /*
     * Read the isContainerSU
     */

     clAmsParserBooleanParser(
             &suConfig->isContainerSU,
             su,
             "isContainerSU");

     /* 
      * Add to list 
      */

     ClAmsParserConfigTypeT newConfigType = {0}; 

     newConfigType.nvp = NULL;
     newConfigType.entityConfig = (ClAmsEntityConfigT *)suConfig;

     if ( ( rc = clAmsParserAddEntityTypeConfig (
                     &gAmfConfigTypeList,
                     newConfigType))
             != CL_OK )
     {
         AMS_LOG(CL_DEBUG_ERROR,
                 ("SU Type [%s] : Error in adding to the configuration type list\n", 
                  suConfig->entity.name.value)); 
         return rc;
     }

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the SG type values in the definitions XML File 
 */

ClRcT 
clAmsParserSGDefParser(
       CL_IN ClParserPtrT sg )
{

    ClRcT rc = CL_OK;
    ClAmsSGConfigT *sgConfig = NULL ;

    AMS_CHECKPTR ( !sg );
    /*
     *  For each sgConfig go through the list of the sgs 
     */

    sgConfig = clHeapAllocate (sizeof (ClAmsSGConfigT));
    AMS_CHECK_NO_MEMORY ( sgConfig );


    /*
     * Copy the default config
     */

    memcpy (sgConfig,&gClAmsSGDefaultConfig,sizeof (ClAmsSGConfigT));

    /*
     * Read the entity name and type
     */

    if ( ( rc = clAmsParserEntityAttrParser(
            (ClAmsEntityConfigT *)sgConfig,
            CL_AMS_ENTITY_TYPE_SG,
            sg,
            "name"))
            != CL_OK )
    { 
        AMS_LOG(CL_DEBUG_ERROR,
                ("<sgType>: missing name attribute \n")); 
        return rc;
    }

    /*
     * Read the adminState
     */

    if ( ( rc = clAmsParserEntityAdminStateParser (
                    &sgConfig->adminState,
                    sg ))
            != CL_OK )
    {
        if ( rc == CL_AMS_ERR_BAD_CONFIG )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("SG Type [%s] : bad <adminState> tag\n",
                     sgConfig->entity.name.value));
            return rc;
        }
    }

    /*
     * Read the redundancyModel 
     */

    if ( ( rc = clAmsParserSGRedundancyModelParser (
                    sgConfig,
                    sg ))
            != CL_OK )
    {
        if ( rc == CL_AMS_ERR_BAD_CONFIG )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("SG Type [%s] : bad <redundancyModel> tag\n",
                     sgConfig->entity.name.value));
            return rc;
        }
    }

    /*
     * Read the loading strategy
     */

    if ( ( rc = clAmsParserSGLoadingStrategyParser (
                    sgConfig,
                    sg ))
            != CL_OK )
    {
        if ( rc == CL_AMS_ERR_BAD_CONFIG )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("SG Type [%s] : bad <loadingStrategy> tag\n",
                     sgConfig->entity.name.value));
            return rc;
        }
    }

    /*
     * failbackOption 
     */

     clAmsParserBooleanParser(
             &sgConfig->failbackOption,
             sg,
             "failbackOption");

    /*
     * autoRepair 
     */

     clAmsParserBooleanParser(
             &sgConfig->autoRepair,
             sg,
             "autoRepair");

    /*
     * instantiateTimeout
     */

     clAmsParserTimeoutParser(
             &sgConfig->instantiateDuration,
             sg,
             "restartDuration",
             (ClCharT *)sgConfig->entity.name.value);

    /*
     * Read the numPrefActiveSUs 
     */

      clAmsParserUint32Parser(
              &sgConfig->numPrefActiveSUs,
              sg,
              "numPrefActiveSUs");

    /*
     * Read the numPrefStandbySUs 
     */

     clAmsParserUint32Parser (
                    &sgConfig->numPrefStandbySUs,
                    sg,
                    "numPrefStandbySUs");

    /*
     * Read the  numPrefInserviceSUs 
     */

     rc = clAmsParserUint32Parser (
             &sgConfig->numPrefInserviceSUs,
             sg,
             "numPrefInserviceSUs");

    /*
     * Read the numPrefAssignedSUs 
     */

      clAmsParserUint32Parser (
              &sgConfig->numPrefAssignedSUs,
              sg,
              "numPrefAssignedSUs" );

    /*
     * Read the  numPrefActiveSUsPerSI 
     */

     clAmsParserUint32Parser (
             &sgConfig->numPrefActiveSUsPerSI,
             sg,
             "numPrefActiveSUsPerSI");

    /*
     * Read the maxActiveSIsPerSU 
     */

     clAmsParserUint32Parser (
             &sgConfig->maxActiveSIsPerSU,
             sg,
             "maxActiveSIsPerSU");

    /*
     * Read the maxStandbySIsPerSU
     */

      clAmsParserUint32Parser (
              &sgConfig->maxStandbySIsPerSU,
              sg,
              "maxStandbySIsPerSU");

    /*
     * Read the compRestartDuration
     */

      clAmsParserTimeoutParser (
              &sgConfig->compRestartDuration,
              sg,
              "compRestartDuration",
              (ClCharT *)sgConfig->entity.name.value);

    /*
     * Read the compRestartCountMax
     */

      clAmsParserUint32Parser (
              &sgConfig->compRestartCountMax,
              sg,
              "compRestartCountMax");

    /*
     * Read the suRestartDuration
     */

     clAmsParserTimeoutParser (
             &sgConfig->suRestartDuration,
             sg,
             "suRestartDuration",
             (ClCharT *)sgConfig->entity.name.value);

    /*
     * Read the suRestartCountMax 
     */

     clAmsParserUint32Parser (
              &sgConfig->suRestartCountMax,
              sg,
              "suRestartCountMax");

    /*
     * isCollocationAllowed
     */

     clAmsParserBooleanParser(
             &sgConfig->isCollocationAllowed,
             sg,
             "isCollocationAllowed" );

    /*
     * alphaFactor
     */

      clAmsParserUint32Parser (
              &sgConfig->alpha,
              sg,
              "alphaFactor");


      /*
       * betaFactor for standbys
       */
      clAmsParserUint32Parser (
              &sgConfig->beta,
              sg,
              "betaFactor");

      /*
       * auto adjust 
       */
      clAmsParserBooleanParser(
                               &sgConfig->autoAdjust,
                               sg,
                               "autoAdjust");

      /*
       * auto adjust probation. 
       */
      
      clAmsParserTimeoutParser(&sgConfig->autoAdjustProbation,
                               sg,
                               "autoAdjustProbation",
                               (ClCharT *)sgConfig->entity.name.value);

      /*
       * Reduction procedure.
       */

      clAmsParserBooleanParser(
                               &sgConfig->reductionProcedure,
                               sg,
                               "reductionProcedure");

      clAmsParserUint32Parser (
                               &sgConfig->maxFailovers,
                               sg,
                               "maxFailovers" );

      clAmsParserTimeoutParser(&sgConfig->failoverDuration,
                               sg,
                               "failoverDuration",
                               (ClCharT *)sgConfig->entity.name.value);


     /* 
      * Add to list 
      */

     ClAmsParserConfigTypeT newConfigType = {0}; 

     newConfigType.nvp = NULL;
     newConfigType.entityConfig = (ClAmsEntityConfigT *)sgConfig;

     if ( ( rc = clAmsParserAddEntityTypeConfig (
                     &gAmfConfigTypeList,
                     newConfigType))
             != CL_OK )
     {
         AMS_LOG(CL_DEBUG_ERROR,
                 ("SG Type [%s] : Error in adding to the configuration type list\n", 
                  sgConfig->entity.name.value)); 
         return rc;
     } 
     
     return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the App type values in the definitions XML File 
 */

ClRcT 
clAmsParserAppDefParser(
        CL_IN ClParserPtrT app)
{
    ClRcT rc = CL_OK;
    ClAmsAppConfigT *appConfig = NULL ;

    /*
     *  For each appConfig go through the list of the apps 
     */
    
    appConfig = clHeapAllocate (sizeof (ClAmsAppConfigT));
    AMS_CHECK_NO_MEMORY ( appConfig );

    /*
     * Read the entity name and type
     */


    if ( ( rc = clAmsParserEntityAttrParser(
                    (ClAmsEntityConfigT *)appConfig,
                    CL_AMS_ENTITY_TYPE_APP,
                    app,
                    "name"))
            != CL_OK )
    {
        return rc;
    }

    return CL_OK;
}

/******************************************************************************/

/*
 * Function to parse the Node type values in the definitions XML File 
 */

ClRcT 
clAmsParserNodeDefParser(
        CL_IN ClParserPtrT node )
{

    ClRcT rc = CL_OK;
    ClAmsNodeConfigT *nodeConfig = NULL ;

    AMS_CHECKPTR ( !node );

    /*
     *  For each nodeConfig go through the list of the nodes
     */

    nodeConfig = clHeapAllocate (sizeof (ClAmsNodeConfigT));
    AMS_CHECK_NO_MEMORY ( nodeConfig );

    /*
     * Copy the default config
     */
    memcpy (nodeConfig,&gClAmsNodeDefaultConfig,sizeof (ClAmsNodeConfigT));

    /*
     * Read the entity name and type
     */

    if ( ( rc = clAmsParserEntityAttrParser (
                    (ClAmsEntityConfigT *)nodeConfig,
                    CL_AMS_ENTITY_TYPE_NODE,
                    node,
                    "name"))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR,
                ("<nodeType>: missing name attribute \n")); 
        return rc;
    }


    /*
     * Read the adminState
     */

    if ( ( rc = clAmsParserEntityAdminStateParser (
                    &nodeConfig->adminState,
                    node))
            != CL_OK )
    {
        if ( rc == CL_AMS_ERR_BAD_CONFIG )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("Node Type [%s] : bad <adminState> tag\n",
                     nodeConfig->entity.name.value));
            return rc;
        }
    }

    /*
     * Read the id
     */ 
    clAmsParserUint32Parser (
            &nodeConfig->id,
            node,
            "id" );

    /*
     * NodeClass
     */

     if ( ( rc = clAmsParserNodeClassTypeParser (
                     nodeConfig,
                     node ))
             != CL_OK )
     {
        if ( rc == CL_AMS_ERR_BAD_CONFIG )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("Node Type [%s] : bad <classType> tag\n",
                     nodeConfig->entity.name.value));
            return rc;
        }
     }

    /*
     * subClass
     */ 

     clAmsParserSaNameParser (
             &nodeConfig->subClassType,
             node,
             "subClassType");

    /*
     * isSwappable
     */

     clAmsParserBooleanParser(
             &nodeConfig->isSwappable,
             node,
             "isSwappable" );

    /*
     * isRestartable
     */ 
     
     clAmsParserBooleanParser(
             &nodeConfig->isRestartable,
             node,
             "isRestartable");

    /*
     * isASPAware
     */

     clAmsParserBooleanParser(
             &nodeConfig->isASPAware,
             node,
             "isASPAware" );

    /*
     * autoRepair 
     */

     clAmsParserBooleanParser(
            &nodeConfig->autoRepair,
            node,
            "autoRepair" );

    /*
     * suFailoverProbTimeMax
     */

     clAmsParserTimeoutParser (
             &nodeConfig->suFailoverDuration,
             node,
             "suFailoverDuration",
             (ClCharT *)nodeConfig->entity.name.value);


    /*
     * suFailoverCountMax
     */

      clAmsParserUint32Parser (
              &nodeConfig->suFailoverCountMax,
              node,
              "suFailoverCountMax" );

     /* 
      * Add to list 
      */

     ClAmsParserConfigTypeT newConfigType = {0}; 
     
     newConfigType.nvp = NULL;
     newConfigType.entityConfig = (ClAmsEntityConfigT *)nodeConfig;

     if ( ( rc = clAmsParserAddEntityTypeConfig (
                     &gAmfConfigTypeList,
                     newConfigType))
             != CL_OK )
     {
         AMS_LOG(CL_DEBUG_ERROR,
                 ("Node Type [%s] : Error in adding to the configuration type list\n", 
                  nodeConfig->entity.name.value)); 
         return rc;
     }

    return CL_OK;
}

/******************************************************************************/

/*
 * This function calls the appropriate parsing function for an entity
 * based on its entity type. This function parse all the definition types for
 * a given entity.
 */

ClRcT 
clAmsParserEntityTypeParser(
        CL_IN ClCharT *listName,
        CL_IN ClCharT *entityName,
        CL_IN ClAmsEntityTypeT entityType,
        CL_IN ClParserPtrT fileParserPtr )
{

    ClRcT rc = CL_OK;
    ClParserPtrT list = NULL;
    ClParserPtrT entity = NULL;

    list = clParserChild(fileParserPtr, listName);
    if ( !list )
    {
        return CL_OK;
    }

    entity  = clParserChild(list, entityName);
    if (!entity)
    {
        return CL_OK;
    }

    while ( entity )
    {
        switch (entityType)
        {
            case CL_AMS_ENTITY_TYPE_ENTITY:
                break;

            case CL_AMS_ENTITY_TYPE_NODE:
                {
                    if (( rc = clAmsParserNodeDefParser (entity) ) != CL_OK )
                    {
                        return rc;
                    }
                    break;
                }
            case CL_AMS_ENTITY_TYPE_APP:
                {
                    if (( rc = clAmsParserAppDefParser(entity) ) != CL_OK )
                    {
                        return rc;
                    }
                    break;
                }

            case CL_AMS_ENTITY_TYPE_SG:
                {
                    if (( rc = clAmsParserSGDefParser (entity) ) != CL_OK )
                    {
                        return rc;
                    }
                    break;
                }

            case CL_AMS_ENTITY_TYPE_SU:
                {
                    if (( rc = clAmsParserSUDefParser(entity) ) != CL_OK )
                    {
                        return rc;
                    }
                    break;
                }

            case CL_AMS_ENTITY_TYPE_SI:
                {
                    if (( rc = clAmsParserSIDefParser(entity) ) != CL_OK )
                    {
                        return rc;
                    }
                    break;
                }

            case CL_AMS_ENTITY_TYPE_COMP:
                {
                    if (( rc = clAmsParserCompDefParser(entity) ) != CL_OK )
                    {
                        return rc;
                    }
                    break;
                }

            case CL_AMS_ENTITY_TYPE_CSI:
                {
                    if (( rc = clAmsParserCSIDefParser(entity) ) != CL_OK )
                    {
                        return rc;
                    }
                    break;
                }

            default:
                {
                    AMS_LOG(CL_DEBUG_ERROR,
                            ("Function[%s]:invalid entity type [%d]\n",__FUNCTION__,entityType)); 
                    return CL_AMS_ERR_INVALID_ENTITY;
                }
        }

        entity = entity->next;
    }

    return CL_OK;
}

/******************************************************************************/
/*
 * This Function goes through the whole XML file and calls the appropriate 
 * functions to parse all the requires values
 */

ClRcT 
clAmsParserMain(
       CL_IN ClCharT *amfDefinitionFileName,
       CL_IN ClCharT *amfConfigFileName )
{

    ClRcT rc = CL_OK;
    ClParserPtrT defFilePtr = NULL;
    ClParserPtrT configFilePtr = NULL;
    ClCharT *filePath = NULL;

    filePath = getenv(CL_ASP_CONFIG_PATH);
    
    if ( !filePath )
    {
        AMS_LOG (CL_DEBUG_ERROR,
                ("Environment variable [CL_ASP_CONFIG_PATH] is not set\n"));
        return CL_AMS_RC (CL_AMS_ERR_BAD_CONFIG);
    }

    AMS_CHECKPTR ( !amfDefinitionFileName || !amfConfigFileName );

    AMS_LOG (CL_DEBUG_TRACE,
            ("Loading Config File [%s], Definitions File [%s]\n",
             amfConfigFileName,
             amfDefinitionFileName));

    if ( ( rc = clAmsMgmtInitialize(
                    &gHandle,
                    gAmsMgmtCallbacks,
                    &gVersion ))
            != CL_OK )
    {
        AMS_LOG (CL_DEBUG_ERROR,("Error in AMS Management library initialization\n"));
        return CL_AMS_RC (rc);
    }

    if ( (defFilePtr = clParserOpenFile(filePath,amfDefinitionFileName) ) == NULL )
    {
        AMS_LOG(CL_DEBUG_ERROR,( "Error in reading file [%s]\n",amfDefinitionFileName));
        return CL_AMS_RC (CL_AMS_ERR_BAD_CONFIG);
    }

    if (( rc = clAmsParserEntityTypeParser(
                    NODE_TYPES_TAG_NAME,
                    NODE_TYPE_TAG_NAME,
                    CL_AMS_ENTITY_TYPE_NODE,
                    defFilePtr ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    NODE_TYPES_TAG_NAME,amfDefinitionFileName )) ;
        goto exitfn;
    }

    if (( rc = clAmsParserEntityTypeParser(
                    APP_TYPES_TAG_NAME,
                    APP_TYPE_TAG_NAME,
                    CL_AMS_ENTITY_TYPE_APP,
                    defFilePtr ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    APP_TYPES_TAG_NAME,amfDefinitionFileName )) ;
        goto exitfn;
    }


    if (( rc = clAmsParserEntityTypeParser(
                    SG_TYPES_TAG_NAME,
                    SG_TYPE_TAG_NAME,
                    CL_AMS_ENTITY_TYPE_SG,
                    defFilePtr ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    SG_TYPES_TAG_NAME,amfDefinitionFileName )) ;
        goto exitfn;
    }


    if (( rc = clAmsParserEntityTypeParser(
                    SU_TYPES_TAG_NAME,
                    SU_TYPE_TAG_NAME,
                    CL_AMS_ENTITY_TYPE_SU,
                    defFilePtr ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    SU_TYPES_TAG_NAME,amfDefinitionFileName )) ;
        goto exitfn;
    }


    if (( rc = clAmsParserEntityTypeParser(
                    SI_TYPES_TAG_NAME,
                    SI_TYPE_TAG_NAME,
                    CL_AMS_ENTITY_TYPE_SI,
                    defFilePtr ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    SI_TYPES_TAG_NAME,amfDefinitionFileName )) ;
        goto exitfn;
    }


    if (( rc = clAmsParserEntityTypeParser(
                    COMP_TYPES_TAG_NAME,
                    COMP_TYPE_TAG_NAME,
                    CL_AMS_ENTITY_TYPE_COMP,
                    defFilePtr ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    COMP_TYPES_TAG_NAME,amfDefinitionFileName )) ;
        goto exitfn;
    }


    if (( rc = clAmsParserEntityTypeParser(
                    CSI_TYPES_TAG_NAME,
                    CSI_TYPE_TAG_NAME,
                    CL_AMS_ENTITY_TYPE_CSI,
                    defFilePtr ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    CSI_TYPES_TAG_NAME,amfDefinitionFileName )) ;
        goto exitfn;
    }

    /*
     * Second Pass for the list and References Part
     */ 


    if (( configFilePtr = clParserOpenFile(filePath,amfConfigFileName)) == NULL )
    {
        AMS_LOG(CL_DEBUG_ERROR,( "Error in reading file [%s]\n",amfConfigFileName));
        rc = CL_AMS_ERR_BAD_CONFIG;
        goto exitfn;
    }

#ifdef AMS_LOGGING_CONFIG 

    if (( rc = clAmsParserAmsConfigParser(configFilePtr )) != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    AMS_CONFIG,amfConfigFileName )) ;
        goto exitfn;
    }

#endif


    /*
     * build node containment hierarchy 
     */

    if (( rc = clAmsParserNodeCreation(configFilePtr) ) != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    NODE_INSTANCES_TAG_NAME,amfConfigFileName )) ;
        goto exitfn;
    }

    /*
     * build sg containment hierarchy 
     */

    if (( rc =  clAmsParserSGCreation(configFilePtr) ) != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    SG_INSTANCES_TAG_NAME, amfConfigFileName )) ;
        goto exitfn;
    }

    /*
     * build the lists 
     */

    if ( ( rc = clAmsParserCreateRelationship(
                    configFilePtr,
                    NODE_INSTANCES_TAG_NAME,
                    NODE_INSTANCE_TAG_NAME,
                    NODE_DEPENDENCIES_TAG_NAME,
                    NODE_TAG_NAME,
                    CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST,
                    CL_AMS_ENTITY_TYPE_NODE,
                    CL_AMS_ENTITY_TYPE_NODE ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    NODE_INSTANCES_TAG_NAME,amfConfigFileName )) ;
        goto exitfn;
    }
 
    if ( ( rc = clAmsParserCreateRelationship(
                    configFilePtr,
                    SG_INSTANCES_TAG_NAME,
                    SG_INSTANCE_TAG_NAME,
                    ASSOCIATED_SERVICE_UNITS,
                    ASSOCIATED_SERVICE_UNIT,
                    CL_AMS_SG_CONFIG_SU_LIST,
                    CL_AMS_ENTITY_TYPE_SG,
                    CL_AMS_ENTITY_TYPE_SU ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                    SG_INSTANCES_TAG_NAME, amfConfigFileName )) ;
        goto exitfn;
    }

    /*
     * Get SI lists
     */

    ClParserPtrT pInstances = NULL;
    ClParserPtrT pInstance = NULL;

    pInstances = clParserChild( configFilePtr, SG_INSTANCES_TAG_NAME);

    AMS_CHECKPTR_AND_EXIT ( !pInstances)

    pInstance = clParserChild (pInstances,SG_INSTANCE_TAG_NAME); 

    while ( pInstance )
    { 

        ClParserPtrT siInstances;
        ClParserPtrT siInstance;

        if ( ( rc = clAmsParserCreateRelationship(
                    pInstance,
                    SERVICE_INSTANCES_TAG_NAME,
                    SERVICE_INSTANCE_TAG_NAME,
                    SI_DEPENDENCIES_TAG_NAME,
                    SERVICE_INSTANCE_TAG_NAME,
                    CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST,
                    CL_AMS_ENTITY_TYPE_SI,
                    CL_AMS_ENTITY_TYPE_SI ))
            != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                        SERVICE_INSTANCES_TAG_NAME, amfConfigFileName )) ;
            goto exitfn;
        }

        if ( ( rc = clAmsParserCreateRelationship(
                    pInstance,
                    SERVICE_INSTANCES_TAG_NAME,
                    SERVICE_INSTANCE_TAG_NAME,
                    SI_PREFFERED_SERVICE_UNITS_TAG,
                    SI_PREFFERED_SERVICE_UNIT_TAG,
                    CL_AMS_SI_CONFIG_SU_RANK_LIST,
                    CL_AMS_ENTITY_TYPE_SI,
                    CL_AMS_ENTITY_TYPE_SU ))
            != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file [%s]\n",
                        SERVICE_INSTANCES_TAG_NAME, amfConfigFileName )) ;
            goto exitfn;
        }

        siInstances = clParserChild(pInstance, SERVICE_INSTANCES_TAG_NAME);
        if(siInstances)
        {
            siInstance = clParserChild(siInstances, SERVICE_INSTANCE_TAG_NAME);
            while(siInstance)
            {
                rc = clAmsParserCreateRelationship(siInstance, 
                                                   CSI_INSTANCES_TAG_NAME,
                                                   CSI_INSTANCE_TAG_NAME,
                                                   CSI_DEPENDENCIES_TAG_NAME,
                                                   CSI_INSTANCE_TAG_NAME,
                                                   CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST,
                                                   CL_AMS_ENTITY_TYPE_CSI,
                                                   CL_AMS_ENTITY_TYPE_CSI);
                if(rc != CL_OK)
                {
                    AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> in file <%s>\n",
                                             CSI_INSTANCES_TAG_NAME, amfConfigFileName));
                    goto exitfn;
                }
                siInstance = siInstance->next;
            }
        }

        pInstance = pInstance->next; 

    }

    if (( rc = clAmsMgmtFinalize(gHandle) )!= CL_OK )
    {
        AMS_LOG (CL_DEBUG_ERROR,("Error in AMS Management library finalization\n"));
        goto exitfn;
    }


    /*
     * free all the memory associated with  types
     */

     clAmsParserFreeConfigType(
             gAmfConfigTypeList );

    clParserFree(defFilePtr);
    clParserFree(configFilePtr);

    return CL_OK;

exitfn:

    clAmsParserFreeConfigType(
                    gAmfConfigTypeList );

    clParserFree(defFilePtr);
    clParserFree(configFilePtr);

    return CL_AMS_RC (rc);
}

/******************************************************************************/
/*
 * This function stores all the definition types configuration in the
 * definitions config file in a link list . These types are queried while
 * instantiating a particular type of entity .
 */

ClRcT   
clAmsParserAddEntityTypeConfig( 
        CL_INOUT ClAmsParserConfigTypeT **head,
        CL_IN ClAmsParserConfigTypeT newEntityType )
{

    ClAmsParserConfigTypeT *ptr = NULL;
    ClAmsParserConfigTypeT *newType = NULL;

    AMS_CHECKPTR ( !head || !newEntityType.entityConfig)

    newType = clHeapAllocate ( sizeof (ClAmsParserConfigTypeT));
    AMS_CHECK_NO_MEMORY ( newType );

    ptr = *head;
    newType->entityConfig = newEntityType.entityConfig;
    newType->nvp = newEntityType.nvp;
    newType->next = NULL;

    if ( !ptr )
    {
        *head = newType;
        return CL_OK;
    }

    while ( ptr->next != NULL)
    {
        ptr = ptr->next;
    }

    ptr->next = newType;

    return CL_OK;

}
/******************************************************************************/

/*
 * This function is called to get entity configuration based on a particular
 * type name and entity type value.
 */

ClRcT   
clAmsParserFindConfigType(
       CL_IN ClAmsParserConfigTypeT *head,
       CL_IN ClCharT *typeName,
       CL_IN ClAmsEntityTypeT entityType,
       CL_IN ClAmsParserConfigTypeT *entityConfigType )   
{

    ClAmsParserConfigTypeT *ptr = NULL;
    ClUint32T size = 0;

    AMS_CHECKPTR ( !head || !typeName || !entityConfigType)

    ptr = head;

    switch (entityType)
    {
        case CL_AMS_ENTITY_TYPE_ENTITY:
            break;

        case CL_AMS_ENTITY_TYPE_NODE:
            {
                size = sizeof (ClAmsNodeConfigT);
                break;
            }

        case CL_AMS_ENTITY_TYPE_APP:
            {
                size = sizeof (ClAmsAppConfigT);
                break;
            }

        case CL_AMS_ENTITY_TYPE_SG:
            {
                size = sizeof (ClAmsSGConfigT);
                break;
            }

        case CL_AMS_ENTITY_TYPE_SU:
            {
                size = sizeof (ClAmsSUConfigT);
                break;
            }

        case CL_AMS_ENTITY_TYPE_SI:
            {
                size = sizeof (ClAmsSIConfigT);
                break;
            }

        case CL_AMS_ENTITY_TYPE_COMP:
            {
                size = sizeof (ClAmsCompConfigT);
                break;
            }

        case CL_AMS_ENTITY_TYPE_CSI:
            {
                size = sizeof (ClAmsCSIConfigT);
                break;
            }

        default:
            {
                AMS_LOG(CL_DEBUG_ERROR,
                        ("Function[%s]:invalid entity type [%d]\n",__FUNCTION__,entityType)); 
                return CL_AMS_ERR_INVALID_ENTITY;
            }
    }

    while (ptr )
    {
        if ( !strcmp ((const ClCharT *)ptr->entityConfig->name.value,typeName) &&
                (ptr->entityConfig->type == entityType))
        {
            memcpy (entityConfigType->entityConfig, ptr->entityConfig,size) ;
            entityConfigType->nvp = ptr->nvp;
            return CL_OK;
        }

        ptr = ptr->next;
    }

    clDbgRootCauseError(CL_AMS_RC(CL_ERR_NOT_EXIST),("Config parameter [%s] does not exist in [%s]",typeName,head->entityConfig->name.value));
    return CL_AMS_RC (CL_ERR_NOT_EXIST);

}

/*
 * This function is called to delete all the memory used for types
 */

ClRcT   
clAmsParserFreeConfigType(
       CL_IN ClAmsParserConfigTypeT *head )
{ 
    ClAmsParserConfigTypeT *ptr = NULL;
    ClAmsParserCSINVPListT *nvpPtr = NULL;
    ClAmsParserCSINVPListT *tmpNVPPtr = NULL;

    while ( head)
    {
        ptr = head;
        head = head->next;
        nvpPtr = ptr->nvp;

        while ( nvpPtr )
        {
            tmpNVPPtr = nvpPtr;
            nvpPtr = nvpPtr->next;
            clAmsFreeMemory (tmpNVPPtr->csiName);
            clAmsFreeMemory (tmpNVPPtr->paramName);
            clAmsFreeMemory (tmpNVPPtr->paramValue);
            clAmsFreeMemory (tmpNVPPtr);
        }

        if(ptr->entityConfig->type == CL_AMS_ENTITY_TYPE_COMP)
        {
            ClAmsCompConfigT *compConfig = (ClAmsCompConfigT*)ptr->entityConfig;
            if(compConfig->pSupportedCSITypes)
            {
                clAmsFreeMemory(compConfig->pSupportedCSITypes);
            }
        }
        clAmsFreeMemory(ptr->entityConfig);
        clAmsFreeMemory (ptr);
    }

    return CL_OK;
}

/******************************************************************************/
/*
 * This function parses the node containment hierarchy 
 */

ClRcT 
clAmsParserNodeCreation(
        CL_IN ClParserPtrT fileParserPtr ) 
{

    ClRcT rc = CL_OK;
    ClParserPtrT nodeInstance = NULL;
    ClParserPtrT nodeInstances = NULL;
    ClAmsNodeConfigT *nodeConfig = NULL;

    AMS_CHECKPTR ( !fileParserPtr );

    nodeConfig = clHeapAllocate (sizeof (ClAmsNodeConfigT));
    AMS_CHECK_NO_MEMORY ( nodeConfig );

    nodeInstances  = clParserChild( fileParserPtr, NODE_INSTANCES_TAG_NAME);

    if ( !nodeInstances)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Tag <%s> is missing in file [clAmfConfig.xml \n",
                    NODE_INSTANCES_TAG_NAME));
        return CL_ERR_NULL_POINTER;
    }

    nodeInstance = clParserChild (nodeInstances, NODE_INSTANCE_TAG_NAME);

    if ( !nodeInstance)
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Tag <%s> is missing in file [clAmfConfig.xml] \n",
                    NODE_INSTANCE_TAG_NAME));
        return CL_ERR_NULL_POINTER;
    }

    while ( nodeInstance )
    {
        if (( rc = clAmsParserSetEntityConfig (
                        nodeInstance,
                        CL_AMS_ENTITY_TYPE_NODE,
                        (ClAmsEntityConfigT *)nodeConfig,
                        NULL ))
                != CL_OK )
        {
            return rc;
        }

        if ( ( rc =clAmsParserEntityCreate (
                        (ClAmsEntityConfigT *)nodeConfig))
                != CL_OK )
        {
            return rc;
        }

        /*
         * Recursively traverse SU in this node
         */
        if (( rc = clAmsParserSUCreation(
                        nodeInstance,
                        nodeConfig )) 
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> for Node [%s]\n",
                        SU_INSTANCES_TAG_NAME,nodeConfig->entity.name.value)) ;
            return rc;
        }

        nodeInstance = nodeInstance->next;
    }

    clAmsFreeMemory(nodeConfig);
        
    return CL_OK;
}

/******************************************************************************/

/*
 * This function parses the SU containment hierarchy 
 */

ClRcT 
clAmsParserSUCreation( 
        CL_IN ClParserPtrT nodePtr ,
        CL_IN ClAmsNodeConfigT *nodeConfig )
{

    ClRcT rc = CL_OK;
    ClParserPtrT suInstance = NULL;
    ClParserPtrT suInstances = NULL;
    ClCharT *parentNode = NULL;
    ClCharT *suTypeName = NULL;
    ClCharT *suRank = NULL;
    ClAmsSUConfigT *suConfig = NULL;
    ClAmsEntityT targetEntity = {0};

    AMS_CHECKPTR ( !nodePtr || !nodeConfig );

    suInstances  = clParserChild( nodePtr, SU_INSTANCES_TAG_NAME);
    if ( !suInstances)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing for Node [%s] \n",
                        SU_INSTANCES_TAG_NAME,nodeConfig->entity.name.value));
        return CL_OK;
    }

    suInstance  = clParserChild( suInstances, SU_INSTANCE_TAG_NAME);
    if ( !suInstance )
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing for Node [%s] \n",
                        SU_INSTANCE_TAG_NAME,nodeConfig->entity.name.value));
        return CL_OK;
    }

    suConfig = clHeapAllocate (sizeof (ClAmsSUConfigT));
    AMS_CHECK_NO_MEMORY ( suConfig );

    /*
     * Get the parent node name here
     */

    if ( ( rc =  clAmsParserStringParser (
                    &parentNode,
                    nodePtr,
                    "name"))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Attribute <name> is missing for Node [%s] \n",
                    nodeConfig->entity.name.value));
        goto exitfn;
    }

    while (suInstance)
    {
        /*
         * Instantiate the su 
         */ 
        
        AMS_CHECK_RC_ERROR( clAmsParserSetEntityConfig (
                    suInstance,
                    CL_AMS_ENTITY_TYPE_SU,
                    (ClAmsEntityConfigT *)suConfig,
                    parentNode) );

        /*
         * Read the suRank if provisioned at instance level
         */

        if ( clAmsParserStringParser (
                    &suRank,
                    suInstance,
                    "rank") 
                == CL_OK )
        {
            suConfig->rank = atoi (suRank);
        }

        AMS_CHECK_RC_ERROR( clAmsParserEntityCreate (
                    (ClAmsEntityConfigT *)suConfig) );

        AMS_CHECK_RC_ERROR( clAmsParserStringParser (
                    &suTypeName,
                    suInstance,
                    "type") );

        /*
         * Add parentNode for this SU here
         */

        memset (targetEntity.name.value,0,CL_MAX_NAME_LENGTH);
        strcpy ((ClCharT *)targetEntity.name.value, parentNode);
        targetEntity.name.length = strlen(parentNode) +1;
        targetEntity.type = CL_AMS_ENTITY_TYPE_NODE;

        if ( ( rc = clAmsMgmtEntitySetRef (
                        gHandle,
                        &suConfig->entity,
                        &targetEntity))
                != CL_OK )
        { 
            AMS_LOG(CL_DEBUG_ERROR, ("Error in setting Node [%s] reference for SU  [%s] \n",
                        parentNode,suConfig->entity.name.value));
            goto exitfn;
        }
        /*
         * Recursively traverse Components in this SU 
         */

        if ( ( rc = clAmsParserCompCreation(
                        suInstance,
                        suConfig,
                        parentNode)) 
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> for SU [%s]\n",
                        COMP_INSTANCES_TAG_NAME,suConfig->entity.name.value)) ;
            goto exitfn;
        }

        /*
         * Add this SU in the nodeList
         */

        if ( ( rc = clAmsMgmtEntityListEntityRefAdd(
                        gHandle,
                        &nodeConfig->entity,
                        &suConfig->entity,
                        CL_AMS_NODE_CONFIG_SU_LIST))
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in adding SU [%s] in the Node [%s] SU list \n",
                        suConfig->entity.name.value,nodeConfig->entity.name.value ));
            goto exitfn;
        }

        suInstance = suInstance->next;
        clAmsFreeMemory(suTypeName);
        clAmsFreeMemory(suRank);

    } 

exitfn:

    clAmsFreeMemory(suTypeName);
    clAmsFreeMemory(suConfig);
    clAmsFreeMemory (parentNode);
    clAmsFreeMemory(suRank);
    return CL_AMS_RC (rc);
    
}

/******************************************************************************/

/*
 * This function parses the component hierarchy 
 */

ClRcT 
clAmsParserCompCreation( 
        CL_IN ClParserPtrT suPtr,
        CL_IN ClAmsSUConfigT *suConfig,
        ClCharT *parentNode ) 
{

    ClRcT rc = CL_OK;
    ClParserPtrT compInstance = NULL;
    ClParserPtrT compInstances = NULL;
    ClAmsCompConfigT *compConfig = NULL;
    ClCharT *parentSU = NULL;
    ClAmsEntityT targetEntity = {0};

    AMS_CHECKPTR ( !suPtr || !suConfig || !parentNode );

    compInstances  = clParserChild( suPtr, COMP_INSTANCES_TAG_NAME);
    if ( !compInstances)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing for SU [%s] \n",
                    COMP_INSTANCES_TAG_NAME,suConfig->entity.name.value));
        return CL_OK;
    }

    compInstance  = clParserChild( compInstances, COMP_INSTANCE_TAG_NAME);
    if ( !compInstance)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing for SU [%s] \n",
                    COMP_INSTANCE_TAG_NAME,suConfig->entity.name.value));
        return CL_OK;
    }

    compConfig = clHeapAllocate (sizeof( ClAmsCompConfigT));
    AMS_CHECK_NO_MEMORY ( compConfig );

    /*
     * Get the parent SU name here
     */
    if (( rc =  clAmsParserStringParser (
                    &parentSU,
                    suPtr,
                    "name"))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Attribute <name> is missing for SU [%s] \n",
                    suConfig->entity.name.value));
        goto exitfn;
    }

    while (compInstance)
    {

        AMS_CHECK_RC_ERROR( clAmsParserSetEntityConfig (
                    compInstance,
                    CL_AMS_ENTITY_TYPE_COMP,
                    (ClAmsEntityConfigT *)compConfig,
                    parentNode) );

        AMS_CHECK_RC_ERROR( clAmsParserEntityCreate( 
                    (ClAmsEntityConfigT *)compConfig) );

        /*
         * Add parentSU for this comp here
         */

        memset (targetEntity.name.value,0,CL_MAX_NAME_LENGTH);
        strcpy ((ClCharT *)targetEntity.name.value, parentSU);
        targetEntity.name.length = strlen(parentSU) +1;
        targetEntity.type = CL_AMS_ENTITY_TYPE_SU;

        if ( ( rc = clAmsMgmtEntitySetRef (
                        gHandle,
                        &compConfig->entity,
                        &targetEntity))
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in setting SU [%s] reference for Component [%s] \n",
                        suConfig->entity.name.value,compConfig->entity.name.value));
            goto exitfn;
        }

        /*
         * Add this component in the suList
         */

        if ( ( rc = clAmsMgmtEntityListEntityRefAdd(
                        gHandle,
                        &suConfig->entity,
                        &compConfig->entity,
                        CL_AMS_SU_CONFIG_COMP_LIST))
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in adding Component [%s] in the SU [%s] Comp list \n",
                        compConfig->entity.name.value,suConfig->entity.name.value ));
            goto exitfn;
        }

        compInstance = compInstance->next;
    }

    clAmsFreeMemory(compConfig);
    clAmsFreeMemory (parentSU);

    return CL_OK;

exitfn:

    clAmsFreeMemory(compConfig);
    clAmsFreeMemory (parentSU);
    return CL_AMS_RC (rc);
}

/******************************************************************************/
/*
 * This function parses the SG containment hierarchy 
 */

ClRcT 
clAmsParserSGCreation(
        CL_IN ClParserPtrT fileParserPtr ) 
{

    ClRcT rc = CL_OK;
    ClParserPtrT sgInstance = NULL;
    ClParserPtrT sgInstances = NULL;
    ClAmsSGConfigT *sgConfig = NULL;

    AMS_CHECKPTR ( !fileParserPtr );

    sgInstances  = clParserChild( fileParserPtr, SG_INSTANCES_TAG_NAME);

    if ( !sgInstances)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing in the file [clAmfConfig.xml]\n",
                    SG_INSTANCES_TAG_NAME));
        return CL_OK;
    }

    sgInstance = clParserChild (sgInstances, SG_INSTANCE_TAG_NAME);
    if ( !sgInstance)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing in the file [clAmfConfig.xml]\n",
                    SG_INSTANCE_TAG_NAME));
        return CL_OK;
    }

    sgConfig = clHeapAllocate (sizeof (ClAmsSGConfigT));
    AMS_CHECK_NO_MEMORY ( sgConfig );

    while ( sgInstance )
    {

        AMS_CHECK_RC_ERROR( clAmsParserSetEntityConfig (
                    sgInstance,
                    CL_AMS_ENTITY_TYPE_SG,
                    (ClAmsEntityConfigT *)sgConfig, 
                    NULL) );

        AMS_CHECK_RC_ERROR( clAmsParserEntityCreate (
                    (ClAmsEntityConfigT *)sgConfig) );

        /*
         * Recursively traverse SI in this sg
         */

        if (( rc = clAmsParserSICreation(
                        sgInstance,
                        sgConfig )) 
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> for SG [%s]\n",
                        SERVICE_INSTANCES_TAG_NAME,sgConfig->entity.name.value)) ;
            goto exitfn;
        }

        sgInstance = sgInstance->next;
    }

    clAmsFreeMemory(sgConfig);

    return CL_OK;

exitfn:

    clAmsFreeMemory(sgConfig);
    return CL_AMS_RC (rc);

}

/******************************************************************************/

/*
 * This function parses the SI containment hierarchy 
 */

ClRcT 
clAmsParserSICreation( 
        CL_IN ClParserPtrT sgPtr ,
        CL_IN ClAmsSGConfigT *sgConfig ) 
{

    ClRcT rc = CL_OK;
    ClParserPtrT siInstance = NULL;
    ClParserPtrT siInstances = NULL;
    ClCharT *parentSG = NULL;
    ClAmsSIConfigT *siConfig = NULL;
    ClAmsEntityT targetEntity = {0};

    AMS_CHECKPTR ( !sgPtr || !sgConfig );

    siInstances  = clParserChild( sgPtr, SERVICE_INSTANCES_TAG_NAME);
    if ( !siInstances)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing in the file [clAmfConfig.xml]\n",
                    SERVICE_INSTANCES_TAG_NAME));
        return CL_OK;
    }

    siInstance  = clParserChild( siInstances, SERVICE_INSTANCE_TAG_NAME);
    if ( !siInstance)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing in the file [clAmfConfig.xml]\n",
                    SERVICE_INSTANCE_TAG_NAME));
        return CL_OK;
    }

    siConfig = clHeapAllocate (sizeof (ClAmsSIConfigT));
    AMS_CHECK_NO_MEMORY ( siConfig );

    /*
     * Get the parent sg name here
     */
       
     if ( ( rc =  clAmsParserStringParser (
                     &parentSG,
                     sgPtr,
                     "name"))
             != CL_OK )
     {
        AMS_LOG(CL_DEBUG_ERROR, ("Attribute <name> is missing for SG[%s] \n",
                    sgConfig->entity.name.value));
         goto exitfn;
     }


    while (siInstance)
    {
        AMS_CHECK_RC_ERROR( clAmsParserSetEntityConfig (
                    siInstance,
                    CL_AMS_ENTITY_TYPE_SI,
                    (ClAmsEntityConfigT *)siConfig,
                    NULL) );

        AMS_CHECK_RC_ERROR( clAmsParserEntityCreate (
                    (ClAmsEntityConfigT *)siConfig) );

        /*
         * ParentSG
         */

        memset (targetEntity.name.value,0,CL_MAX_NAME_LENGTH);
        strcpy ((ClCharT *)targetEntity.name.value, parentSG);
        targetEntity.name.length = strlen(parentSG) +1;
        targetEntity.type = CL_AMS_ENTITY_TYPE_SG;

        if ( ( rc = clAmsMgmtEntitySetRef (
                        gHandle,
                        &siConfig->entity,
                        &targetEntity))
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in setting SG [%s] reference for SI [%s]\n",
                        parentSG,siConfig->entity.name.value));
            goto exitfn;
        }

        /*
         * Recursively traverse Components in this SI 
         */

        if ( ( rc = clAmsParserCSICreation(
                        siInstance,
                        siConfig)) 
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in parsing XML tag <%s> for SI [%s]\n",
                        CSI_INSTANCES_TAG_NAME,siConfig->entity.name.value)) ;
            goto exitfn;
        }

        /*
         * Add this SI in the sgList
         */

        if ( ( rc = clAmsMgmtEntityListEntityRefAdd(
                        gHandle,
                        &sgConfig->entity,
                        &siConfig->entity,
                        CL_AMS_SG_CONFIG_SI_LIST))
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("error in setting sg config si list si = %s  sg = %s \n", 
                        siConfig->entity.name.value,sgConfig->entity.name.value)) ;
            goto exitfn;
        }

        siInstance = siInstance->next;
    } 
    
    clAmsFreeMemory(siConfig);
    clAmsFreeMemory (parentSG);
    return CL_OK;

exitfn:

    clAmsFreeMemory(siConfig);
    clAmsFreeMemory (parentSG);
    return CL_AMS_RC (rc);

}

/******************************************************************************/

/*
 * This function parses the CSI hierarchy 
 */

ClRcT clAmsParserCSICreation( 
        CL_IN ClParserPtrT siPtr,
        CL_IN ClAmsSIConfigT *siConfig ) 
{

    ClRcT rc = CL_OK;
    ClParserPtrT csiInstance;
    ClParserPtrT csiInstances;
    ClAmsCSIConfigT *csiConfig = NULL;
    ClCharT *parentSIName = NULL;
    ClCharT *csiType = NULL;

    AMS_CHECKPTR ( !siPtr || !siConfig );

    csiInstances  = clParserChild( siPtr, CSI_INSTANCES_TAG_NAME);
    if ( !csiInstances)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing in the file [clAmfConfig.xml]\n",
                    CSI_INSTANCES_TAG_NAME));
        return CL_OK;
    }

    csiInstance  = clParserChild( csiInstances, CSI_INSTANCE_TAG_NAME);
    if ( !csiInstance)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing in the file [clAmfConfig.xml]\n",
                    CSI_INSTANCE_TAG_NAME));
        return CL_OK;
    }

    csiConfig = clHeapCalloc (1, sizeof( ClAmsCSIConfigT));
    AMS_CHECK_NO_MEMORY (csiConfig);

    /*
     * Get the parent SI name here
     */
   
    if (( rc =  clAmsParserStringParser (
                    &parentSIName,
                    siPtr,
                    "name" ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, ("Attribute <name> is missing for SI [%s] \n",
                    siConfig->entity.name.value));
        goto exitfn;
    }

    while (csiInstance)
    {

        AMS_CHECK_RC_ERROR( clAmsParserSetEntityConfig (
                    csiInstance,
                    CL_AMS_ENTITY_TYPE_CSI,
                    (ClAmsEntityConfigT *)csiConfig,
                    NULL) );

        /*
         * Set the parentSI for this CSI
         */

        csiConfig->parentSI.entity.type = CL_AMS_ENTITY_TYPE_SI;
        memset ( &csiConfig->parentSI.entity.name,0,CL_MAX_NAME_LENGTH );
        strcpy ( (ClCharT *)csiConfig->parentSI.entity.name.value,parentSIName);
        csiConfig->parentSI.entity.name.length = strlen (parentSIName) + 1;

        AMS_CHECK_RC_ERROR( clAmsParserEntityCreate (
                    (ClAmsEntityConfigT *)csiConfig) );

        /*
         * Set the the parentSI reference
         */
        if (( rc = clAmsMgmtEntitySetRef (
                        gHandle,
                        &csiConfig->entity,
                        &csiConfig->parentSI.entity ))
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in setting SI [%s] reference for CSI [%s] \n",
                        parentSIName,csiConfig->entity.name.value));
            goto exitfn;
        }


        /*
         * Read the nvp list for this entity
         */ 
        if (( rc =  clAmsParserStringParser (
                        &csiType,
                        csiInstance,
                        "type" ))
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR,
                    ("CSI [%s] : missing <type> attribute in file [clAmfConfig.xml]\n", 
                     csiConfig->entity.name.value)); 
            goto exitfn;
        }

        if (( rc = clAmsParserCreateCSINVPList (
                        (ClCharT *)csiConfig->entity.name.value,
                        csiType ))
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("CSI [%s] : Error in creating Name Value Pair list\n",
                        csiConfig->entity.name.value)) ;
            goto exitfn;
        }


        /*
         * Add this csi in the siList
         */
        if (( rc = clAmsMgmtEntityListEntityRefAdd(
                        gHandle,
                        &siConfig->entity,
                        &csiConfig->entity,
                        CL_AMS_SI_CONFIG_CSI_LIST ))
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Error in adding CSI [%s] in the SI [%s] CSI list \n",
                        csiConfig->entity.name.value,siConfig->entity.name.value ));
            goto exitfn;
        }

        csiInstance = csiInstance->next;
        clAmsFreeMemory (csiType);
    }

    clAmsFreeMemory(csiConfig);
    clAmsFreeMemory(parentSIName);
    clAmsFreeMemory(csiType);

    return CL_OK;

exitfn:

    clAmsFreeMemory(csiConfig);
    clAmsFreeMemory(parentSIName);
    clAmsFreeMemory(csiType);
    return CL_AMS_RC (rc);

}

/******************************************************************************/

/*
 * This function parses the lists portions of the instances XML 
 */

ClRcT 
clAmsParserCreateRelationship( 
        CL_IN ClParserPtrT fileParserPtr,
        CL_IN ClCharT *instancesName,
        CL_IN ClCharT *instanceName,
        CL_IN ClCharT *listName,
        CL_IN ClCharT *entityName,
        CL_IN ClAmsEntityListTypeT entityListName,
        CL_IN ClAmsEntityTypeT sourceEntityType ,
        CL_IN ClAmsEntityTypeT targetEntityType ) 
{

    ClRcT rc = CL_OK;
    ClParserPtrT pInstances = NULL;
    ClParserPtrT pInstance = NULL;
    ClParserPtrT pList = NULL;
    ClParserPtrT pEntity = NULL;
    ClCharT *targetName = NULL;
    ClCharT *sourceEntityName = NULL;
    ClAmsEntityT sourceEntity = {0};
    ClAmsEntityT targetEntity = {0}; 

    AMS_CHECKPTR ( !fileParserPtr || !instancesName || !instanceName || !listName || !entityName );

    sourceEntity.type = sourceEntityType;
    targetEntity.type = targetEntityType;

    pInstances = clParserChild( fileParserPtr, instancesName);
    if ( !pInstances)
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing in the file [clAmfConfig.xml]\n",
                    instancesName ));
        return CL_OK;
    }

    pInstance = clParserChild (pInstances,instanceName); 
    if ( !pInstance )
    {
        AMS_LOG(CL_DEBUG_TRACE, ("Tag <%s> is missing in the file [clAmfConfig.xml]\n",
                    instanceName ));
        return CL_OK;
    }

    while ( pInstance )
    {

        /*
         * Set the name 
         */

        if (( rc =  clAmsParserStringParser (
                        &sourceEntityName,
                        pInstance,
                        "name" ))
                != CL_OK )
        {
            AMS_LOG(CL_DEBUG_ERROR, ("Tag <name> is missing for entity tag <%s> \n",
                    instanceName));
            goto exitfn;
        }

        memset (sourceEntity.name.value,0,CL_MAX_NAME_LENGTH);
        strcpy ((ClCharT *)sourceEntity.name.value, sourceEntityName);
        sourceEntity.name.length = strlen(sourceEntityName) +1;

        /*
         * Traverse the list 
         */ 

        pList = clParserChild( pInstance, listName);
        if ( !pList)
        {
            pInstance = pInstance->next;
            clAmsFreeMemory (sourceEntityName);
            continue;
        }

        pEntity = clParserChild( pList, entityName); 
        while (pEntity)
        {

            /*
             * Add this to list
             */ 
            targetName = pEntity->txt;

            memset (targetEntity.name.value,0,CL_MAX_NAME_LENGTH);
            strcpy ((ClCharT *)targetEntity.name.value, targetName);
            targetEntity.name.length = strlen(targetName) +1;

            
            if (( rc = clAmsMgmtEntityListEntityRefAdd(
                            gHandle,
                            &sourceEntity,
                            &targetEntity,
                            entityListName ))
                    != CL_OK )
            {
                AMS_LOG(CL_DEBUG_ERROR, 
                        ("EntityList [%d] : Error in adding entity [%s] in the entity[%s] list\n",
                         entityListName,targetEntity.name.value, sourceEntity.name.value ));
                AMS_LOG(CL_DEBUG_ERROR, 
                        ("Entity [%s] reference may not be valid," 
                         "Create the instance of the entity before adding it into a list\n",
                         targetEntity.name.value ));
                goto exitfn;
            }

            /*
             * check if its a dependency list
             */

            if ( entityListName == CL_AMS_NODE_CONFIG_NODE_DEPENDENCIES_LIST )
            {

                AMS_CHECK_RC_ERROR( clAmsMgmtEntityListEntityRefAdd(
                            gHandle,
                            &targetEntity,
                            &sourceEntity,
                            CL_AMS_NODE_CONFIG_NODE_DEPENDENT_LIST) );

            }
            else if ( entityListName == CL_AMS_SI_CONFIG_SI_DEPENDENCIES_LIST ) 
            {

                AMS_CHECK_RC_ERROR( clAmsMgmtEntityListEntityRefAdd(
                            gHandle,
                            &targetEntity,
                            &sourceEntity,
                            CL_AMS_SI_CONFIG_SI_DEPENDENTS_LIST) );
            }
            else if (entityListName == CL_AMS_CSI_CONFIG_CSI_DEPENDENCIES_LIST )
            {
                AMS_CHECK_RC_ERROR( clAmsMgmtEntityListEntityRefAdd(
                                                                    gHandle,
                                                                    &targetEntity,
                                                                    &sourceEntity,
                                                                    CL_AMS_CSI_CONFIG_CSI_DEPENDENTS_LIST) );
            }
            else if ( entityListName == CL_AMS_SG_CONFIG_SU_LIST) 
            {
                /*
                 * Create the parentSG references for SU 
                 */ 

                AMS_CHECK_RC_ERROR( clAmsMgmtEntitySetRef (
                            gHandle,
                            &targetEntity,
                            &sourceEntity) );
            }

            pEntity = pEntity->next;
        } 
        
        pInstance = pInstance->next;
        clAmsFreeMemory (sourceEntityName);
    }

    return CL_OK;

exitfn:

    clAmsFreeMemory (sourceEntityName);
    return CL_AMS_RC (rc);
}

/******************************************************************************/

/*
 * This function creates an entity structure which can be used  by
 * entity instantiate and set config mgmt API to create and set the 
 * configuration for the entity 
 */

ClRcT   
clAmsParserSetEntityConfig (
        CL_IN ClParserPtrT ptr,
        CL_IN ClAmsEntityTypeT entityType,
        CL_OUT ClAmsEntityConfigT *entityConfig,
        CL_IN ClCharT *parentNode)
        
{

    ClRcT rc = CL_OK;
    ClCharT *type = NULL;
    ClCharT *name = NULL; 
    ClCharT *entityName = NULL; 
    ClAmsParserConfigTypeT amfConfigType = {0};

    AMS_CHECKPTR ( !ptr || !entityConfig )
    
    
    if (( rc =  clAmsParserStringParser(
                    &type,
                    ptr,
                    "type" ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, 
                ("Attribute <type> is missing for XML Tag [%s] in the file [clAmfConfig.xml\n", 
                 ptr->name ));
        return CL_AMS_RC (CL_ERR_NULL_POINTER);
    }

    /*
     * Set the name 
     */ 
    
    if (( rc =  clAmsParserStringParser (
                    &name,
                    ptr,
                    "name" ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, 
                ("Attribute <name> is missing for XML Tag [%s] in the file [clAmfConfig.xml\n", 
                 ptr->name ));
        goto exitfn;
    }

    amfConfigType.entityConfig = entityConfig;

    if ( ( rc = clAmsParserFindConfigType(
                    gAmfConfigTypeList,
                    type,
                    entityType,
                    &amfConfigType ))
            != CL_OK )
    {
        AMS_LOG(CL_DEBUG_ERROR, 
                ("Error in creating entity instance [%s], entity type [%s] is not modelled in file " 
                 "[clAmfDefinitions.xml] \n", name, type ));
        goto exitfn;
    }

    entityConfig = amfConfigType.entityConfig;
    entityName = name;
   
    AMS_CHECK_RC_ERROR( clAmsParserSetEntityName(
                entityConfig,
                entityName) );

    clAmsFreeMemory (type);
    clAmsFreeMemory (name);

    return CL_OK;

exitfn:

    clAmsFreeMemory (type);
    clAmsFreeMemory (name);

    return CL_AMS_RC (rc);
}

/******************************************************************************/

/*
 * This function creates and sets the configuration for an entity by calling
 * appropriate Management API's
 */

ClRcT    
clAmsParserEntityCreate (
        CL_IN ClAmsEntityConfigT *entityConfig )
{

    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {0};

    AMS_CHECKPTR ( !entityConfig );
    
    memcpy (&entity.name,&entityConfig->name,sizeof (SaNameT));
    entity.type = entityConfig->type; 
    
    if ( ( rc = clAmsMgmtEntityCreate(
                    gHandle,
                    &entity ))
            != CL_OK )
    {
        AMS_LOG ( CL_DEBUG_ERROR,
                ("Error in AMS Management Entity [%s] creation\n",entity.name.value ));
        return CL_AMS_RC (rc);
    }
    
    /*
     * Set the configuration for the node 
     */ 
    if (( rc = clAmsMgmtEntitySetConfig(
                    gHandle,
                    &entity,
                    (ClAmsEntityConfigT *)entityConfig,
                    1 ))
            != CL_OK )
    {
        AMS_LOG ( CL_DEBUG_ERROR,
                ("Error in AMS Management Set Configuration API for Entity [%s]\n",entity.name.value)); 
        return CL_AMS_RC (rc); 
    }

    return CL_OK;
}

/******************************************************************************/

/*
 * This function sets the name for an entity 
 */

ClRcT   
clAmsParserSetEntityName(
        CL_IN ClAmsEntityConfigT *entityConfig,
        CL_IN ClCharT *name )
{

    AMS_CHECKPTR ( !entityConfig || !name );

    memset (&entityConfig->name,0,sizeof (SaNameT));

    switch (entityConfig->type )
    {

        case CL_AMS_ENTITY_TYPE_NODE:
            {
                ClAmsNodeConfigT    *config = (ClAmsNodeConfigT *)entityConfig;
                strcpy ((ClCharT *)config->entity.name.value, name);
                config->entity.name.length = strlen(name) +1;
                break;
            }

        case CL_AMS_ENTITY_TYPE_APP:
            {
                ClAmsAppConfigT    *config = (ClAmsAppConfigT *)entityConfig;
                strcpy ((ClCharT *)config->entity.name.value, name);
                config->entity.name.length = strlen(name) +1;
                break;
            }

        case CL_AMS_ENTITY_TYPE_SG:
            {

                ClAmsSGConfigT    *config = (ClAmsSGConfigT *)entityConfig;
                strcpy ((ClCharT *)config->entity.name.value, name);
                config->entity.name.length = strlen(name) +1;
                break;
            }

        case CL_AMS_ENTITY_TYPE_SI:
            {
                ClAmsSIConfigT    *config = (ClAmsSIConfigT *)entityConfig;
                strcpy ((ClCharT *)config->entity.name.value, name);
                config->entity.name.length = strlen(name) +1;
                break;
            }

        case CL_AMS_ENTITY_TYPE_SU:
            {
                ClAmsSUConfigT    *config = (ClAmsSUConfigT *)entityConfig;
                strcpy ((ClCharT *)config->entity.name.value, name);
                config->entity.name.length = strlen(name) +1;
                break;
            }

        case CL_AMS_ENTITY_TYPE_COMP:
            {
                ClAmsCompConfigT    *config = (ClAmsCompConfigT *)entityConfig;
                strcpy ((ClCharT *)config->entity.name.value, name);
                config->entity.name.length = strlen(name) +1;
                break;
            }

        case CL_AMS_ENTITY_TYPE_CSI:
            {
                ClAmsCSIConfigT    *config = (ClAmsCSIConfigT *)entityConfig;
                strcpy ((ClCharT *)config->entity.name.value, name);
                config->entity.name.length = strlen(name) +1;
                break;
            }

        default:
            { 
                AMS_LOG(CL_DEBUG_ERROR,
                        ("Function[%s]:invalid entity type [%d]\n",__FUNCTION__,entityConfig->type)); 
                return CL_AMS_ERR_INVALID_ENTITY;
            } 
    } 

    return CL_OK;
}

/******************************************************************************/

/*
 * This function sets the NVP list for a CSI by calling the appropriate
 * Management API function
 */

ClRcT 
clAmsParserCreateCSINVPList (
        CL_IN ClCharT *csiName,
        CL_IN ClCharT *csiType )
{

    ClRcT rc = CL_OK;
    ClAmsEntityT entity = {0};
    ClAmsParserConfigTypeT amfConfigType = {0};
    ClAmsCSINameValuePairT nvp = {{0},{0},{0}};
    ClAmsParserCSINVPListT *ptr = NULL;

    AMS_CHECKPTR ( !csiName || !csiType );

    amfConfigType.entityConfig = clHeapAllocate ( sizeof (ClAmsCSIConfigT));
    AMS_CHECK_NO_MEMORY ( amfConfigType.entityConfig );


    entity.type = CL_AMS_ENTITY_TYPE_CSI;
    strcpy ((ClCharT *)entity.name.value, csiName);
    entity.name.length = strlen (csiName)+1;

    memcpy ( &nvp.csiName, &entity.name, sizeof (SaNameT));

    /*
     * find this csi in the types list
     */
 
    AMS_CHECK_RC_ERROR( clAmsParserFindConfigType(
                gAmfConfigTypeList,
                csiType,
                CL_AMS_ENTITY_TYPE_CSI,
                &amfConfigType) );

    /*
     * for each nvp call the mgmt api nvp add function
     */

    ptr = amfConfigType.nvp;

    while ( ptr != NULL)
    {
        /*
         * Make csiName paramName and paramValue
         */

        memset ( &nvp,0,sizeof (ClAmsCSINameValuePairT));
        memcpy (&nvp.csiName,&entity.name,sizeof (nvp.csiName));

        strcpy ((ClCharT *)nvp.paramName.value,ptr->paramName);
        strcpy ((ClCharT *)nvp.paramValue.value,ptr->paramValue);

        nvp.paramName.length = strlen (ptr->paramName)+1;
        nvp.paramValue.length = strlen (ptr->paramValue)+1;

        AMS_CHECK_RC_ERROR( clAmsMgmtCSISetNVP (
                    gHandle,
                    &entity,
                    nvp) );

        ptr = ptr->next;
    }

    clAmsFreeMemory (amfConfigType.entityConfig);

    return CL_OK;

exitfn:

    clAmsFreeMemory (amfConfigType.entityConfig);

    return CL_AMS_RC (rc);

}

/*
 * This function is called to parse the ams specific configuration. Currently
 * this parses the AMS logging related value.
 */

ClRcT
clAmsParserAmsConfigParser( 
        CL_IN  ClParserPtrT  fileParserPtr )
{

    ClParserPtrT  amsConfig = NULL;
    ClParserPtrT  amsLogging = NULL;
    ClParserPtrT  amsLogMessageTypes = NULL;
    ClUint8T  debugFlags = 0;
    ClRcT  rc = CL_OK;
    ClBoolT  isTrue = CL_FALSE;

    AMS_CHECKPTR_SILENT (!fileParserPtr);

    amsConfig = clParserChild (fileParserPtr,AMS_CONFIG);

    if ( amsConfig == NULL )
    { 
        AMS_LOG ( CL_DEBUG_WARN, ("Missing tag [%s] in file "
                "amfConfiguration.xml, AMS will use the default values for "
                "console logging and log message types\n",AMS_CONFIG));
        return CL_OK;
    }

    amsLogging = clParserChild (amsConfig,AMS_LOGGING);

    if ( amsLogging == NULL )
    { 
        AMS_LOG ( CL_DEBUG_WARN, ("Missing tag [%s] in file "
                "amfConfiguration.xml, AMS will use the default values for "
                "console logging and log message types\n",AMS_LOGGING));
        return CL_OK;
    }

    rc = clAmsParserBooleanParser( 
            &isTrue, 
            amsLogging,
            ENABLE_AMS_CONSOLE_LOGGING ); 
    
    if ( rc == CL_OK )
    {

        if ( isTrue )
        {

            if ( ( rc = clAmsMgmtDebugEnableLogToConsole (gHandle) ) != CL_OK )
            { 
                AMS_LOG(CL_DEBUG_ERROR, ("Error in enabling console logging "
                            "for AMS \n"));
                return rc;
            }

        }

        else
        {

            if ( ( rc = clAmsMgmtDebugDisableLogToConsole (gHandle) ) != CL_OK )
            { 
                AMS_LOG(CL_DEBUG_ERROR, ("Error in disabling console logging "
                            "for AMS \n"));
                return rc;
            }

        }

    }

    amsLogMessageTypes = clParserChild (amsLogging,AMS_LOG_MESSAGE_TYPES);

    if ( amsLogMessageTypes == NULL )
    { 

        AMS_LOG ( CL_DEBUG_WARN, ("Missing tag [%s] in file "
                "amfConfiguration.xml, AMS will use the default values for "
                "log message types\n",AMS_LOG_MESSAGE_TYPES));
        return CL_OK;
    }

    isTrue = CL_FALSE;

    clAmsParserBooleanParser( 
            &isTrue, 
            amsLogMessageTypes,
            ENABLE_AMS_EVENTS_MESSAGES ); 

    if ( isTrue == CL_TRUE )
    {
        debugFlags |= CL_AMS_MGMT_SUB_AREA_MSG;
    }

    isTrue = CL_FALSE;

    clAmsParserBooleanParser( 
            &isTrue, 
            amsLogMessageTypes,
            ENABLE_AMS_STATE_CHANGE_MESSAGES ); 

    if ( isTrue == CL_TRUE )
    {
        debugFlags |= CL_AMS_MGMT_SUB_AREA_STATE_CHANGE;
    }

    isTrue = CL_FALSE;

    clAmsParserBooleanParser( 
            &isTrue, 
            amsLogMessageTypes,
            ENABLE_AMS_TIMER_MESSAGES ); 

    if ( isTrue == CL_TRUE )
    {
        debugFlags |= CL_AMS_MGMT_SUB_AREA_TIMER;
    }

    isTrue = CL_FALSE;

    clAmsParserBooleanParser( 
            &isTrue, 
            amsLogMessageTypes,
            ENABLE_AMS_FUNCTION_TRACE_MESSAGES ); 

    if ( isTrue == CL_TRUE )
    {
        debugFlags |= CL_AMS_MGMT_SUB_AREA_FN_CALL;
    }


    if ( ( rc = clAmsMgmtDebugEnable (gHandle, NULL, debugFlags) ) != CL_OK )
    { 
        AMS_LOG(CL_DEBUG_ERROR, ("Error in setting log message types for "
                    "AMS \n"));
        return rc;
    }

    return rc;
}

/******************************************************************************/
