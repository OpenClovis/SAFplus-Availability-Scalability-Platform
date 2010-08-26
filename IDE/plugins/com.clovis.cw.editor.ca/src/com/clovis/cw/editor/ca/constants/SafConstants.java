/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/constants/SafConstants.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.constants;

public interface SafConstants
{
    public static final String RT_SUFFIX_NAME = "rt.xml";
    public static final String CPM_CONFIGS_NAME = "cpmConfigs";
    public static final String CPM_CONFIGLIST_NAME = "cpmConfig";
    public static final String BOOT_CONFIGS_NAME = "bootConfigs";
    public static final String BOOT_CONFIGLIST_NAME = "bootConfig";
    public static final String NODE_INSTANCES_NAME = "nodeInstances";
    public static final String NODE_INSTANCELIST_NAME = "nodeInstance";
    public static final String SERVICEUNIT_INSTANCES_NAME = "serviceUnitInstances";
    public static final String SERVICEUNIT_INSTANCELIST_NAME = "serviceUnitInstance";
    public static final String COMPONENT_INSTANCES_NAME = "componentInstances";
    public static final String COMPONENT_INSTANCELIST_NAME = "componentInstance";
    public static final String RESOURCES_NAME = "resources";
    public static final String RESOURCELIST_NAME = "resource";
    public static final String SERVICEGROUP_INSTANCES_NAME = "serviceGroups";
    public static final String SERVICEGROUP_INSTANCELIST_NAME = "serviceGroup";
    public static final String SERVICE_INSTANCES_NAME = "serviceInstances";
    public static final String SERVICE_INSTANCELIST_NAME = "serviceInstance";
    public static final String CSI_INSTANCELIST_NAME = "componentServiceInstance";
    public static final String CSI_INSTANCES_NAME = "componentServiceInstances";
    public static final String NODE_INSTANCELIST_ECLASS = "NodeInstance";
    public static final String NODE_INSTANCES_ECLASS = "NodeInstances";
    public static final String SERVICEUNIT_INSTANCELIST_ECLASS = "SUInstance";
    public static final String SERVICEUNIT_INSTANCES_ECLASS = "SUInstances";
    public static final String COMPONENT_INSTANCELIST_ECLASS = "ComponentInstance";
    public static final String COMPONENT_INSTANCES_ECLASS = "ComponentInstances";
    public static final String SG_INSTANCELIST_ECLASS = "ServiceGroupInstance";
    public static final String SG_INSTANCES_ECLASS = "ServiceGroupInstances";
    public static final String SI_INSTANCELIST_ECLASS = "ServiceInstance";
    public static final String SI_INSTANCES_ECLASS = "ServiceInstances";
    public static final String CSI_INSTANCELIST_ECLASS = "ComponentServiceInstance";
    public static final String CSI_INSTANCES_ECLASS = "ComponentServiceInstances";
    
    public static final String COMPONENT_MAX_ACTIVE_CSI = "numMaxActiveCSIs";
    public static final String COMPONENT_MAX_STANDBY_CSI = "numMaxStandbyCSIs";
    
    public static final String SG_ACTIVE_SU_COUNT = "numPrefActiveSUs";
    public static final String SG_STANDBY_SU_COUNT = "numPrefStandbySUs";
    public static final String SG_INSERVICE_SU_COUNT = "numPrefInserviceSUs";
    public static final String SG_ASSIGNED_SU_COUNT = "numPrefAssignedSUs";
    public static final String SG_ACTIVE_SUS_PER_SI = "numPrefActiveSUsPerSI";
    
    public static final String ASP_SERVICE_UNITS = "aspServiceUnits";
    public static final String ASP_SERVICE_UNITS_LIST = "aspServiceUnit";
    public static final String ASSOCIATED_SERVICEUNITS_NAME = "associatedServiceUnits";
    public static final String ASSOCIATED_SERVICEUNIT_LIST = "associatedServiceUnit";
    
    public static final String IOC_CONFIGURATION = "iocConfiguration";
    public static final String IOC_CONFIGURATION_ECLASS = "IOCConfiguration";
    public static final String IOC = "ioc";
    public static final String SEND_QUEUE = "sendQueue";
    public static final String RECEIVE_QUEUE = "receiveQueue";
    public static final String QUEUE_ECLASS = "Queue";
}
