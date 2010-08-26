/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/constants/ComponentEditorConstants.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.constants;

import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;

/**
 * @author pushparaj
 *
 * Provides Constants for Component Editor
 */
public interface ComponentEditorConstants
{
	public static final Font COMPONENT_NAME_FONT =
        new Font(null, JFaceResources.TEXT_FONT, 9, SWT.NORMAL);
    public static final Font COMPONENT_PROPERTY_FONT =
        new Font(null, JFaceResources.TEXT_FONT, 7, SWT.NORMAL | SWT.ITALIC);

    public static final String NAME = "name";
    public static final String TYPE = "Type";

    public static final Color COMPONENT_COLOR = new Color(
            null, 229, 229, 229);
    public static final String EDITOR_TYPE = "Component Editor";
    public static final String SAFCOMPONENT_NAME = "SAFComponent";
    public static final String NONSAFCOMPONENT_NAME = "NonSAFComponent";
    //public static final String PROXYCOMPONENT_NAME = "ProxyComponent";
    //public static final String PROXIED_PREINSTANTIABLE_NAME = "ProxiedPreinstantiable";
    public static final String COMPONENT_NAME = "Component";
    public static final String NODE_NAME = "Node";
    public static final String CLUSTER_NAME = "Cluster";
    public static final String SERVICEUNIT_NAME = "ServiceUnit";
    public static final String SERVICEGROUP_NAME = "ServiceGroup";
    public static final String SERVICEINSTANCE_NAME = "ServiceInstance";
    public static final String COMPONENTSERVICEINSTANCE_NAME =
        "ComponentServiceInstance";
    
    public static final String SAFCOMPONENT_REF_NAME = "safComponent";
    public static final String NONSAFCOMPONENT_REF_NAME = "nonSAFComponent";
    public static final String NODE_REF_NAME = "node";
    public static final String CLUSTER_REF_NAME = "cluster";
    public static final String SERVICEUNIT_REF_NAME = "serviceUnit";
    public static final String SERVICEGROUP_REF_NAME = "serviceGroup";
    public static final String SERVICEINSTANCE_REF_NAME = "serviceInstance";
    public static final String COMPONENTSERVICEINSTANCE_REF_NAME =
        "componentServiceInstance";
    public static final String NODES_REF_TYPES[] = {
    	SAFCOMPONENT_REF_NAME,
    	NONSAFCOMPONENT_REF_NAME,
    	NODE_REF_NAME,
    	CLUSTER_REF_NAME,
    	SERVICEUNIT_REF_NAME,
    	SERVICEGROUP_REF_NAME,
    	SERVICEINSTANCE_REF_NAME,
    	COMPONENTSERVICEINSTANCE_REF_NAME
    	};
    
    public static final String SAF_AWARE_COMPONENT_NAME
    = "CL_AMS_SA_AWARE";
    public static final String INSTANTIATION_COMMAND = "instantiateCommand";
    public static final String EO_PROPERTIES_NAME = "eoProperties";
    public static final String EO_NAME = "eoName";
    public static final String EO_OSALLIB_NAME = "OSAL";
    public static final String EO_BUFFERLIB_NAME = "Buffer";
    public static final String EO_IOCLIB_NAME = "IOC";
    public static final String EO_RMDLIB_NAME = "RMD";
    public static final String EO_EOLIB_NAME = "EO";
    public static final String EO_OMLIB_NAME = "OM";
    public static final String EO_HALLIB_NAME = "HAL";
    public static final String EO_TRANSACTIONLIB_NAME = "Transaction";
    public static final String EO_ASPLIB_NAME = "aspLib";
    public static final String EO_PROVLIB_NAME = "Prov";
    public static final String EO_MAX_CLIENTS = "maxNoClients";
    public static final String EO_ALARMLIB_NAME = "Alarm";
    public static final String ASSOCIATE_RESOURCES_NAME = "associatedResource";
    public static final String CONNECTION_START = "source";
    public static final String CONNECTION_END = "target";
    public static final String PROXIED_PREINSTANTIABLE
    = "CL_AMS_PROXIED_PREINSTANTIABLE";
    public static final String PROXIED_NON_PREINSTANTIABLE
    = "CL_AMS_PROXIED_NON_PREINSTANTIABLE";
    public static final String NON_PROXIED_NON_PREINSTANTIABLE
    = "CL_AMS_NON_PROXIED_NON_PREINSTANTIABLE";
    public static final String AUTO_NAME = "Relation";
    public static final String AUTO_REF_NAME = "relation";
    
    public static final String EDGES_REF_TYPES[] = {AUTO_REF_NAME};
                                               
    public static final String CONTAINMENT_NAME = "Containment";
    public static final String ASSOCIATION_NAME = "Association";
    //public static final String ASSIGNMENT_NAME = "CSI_Component";
    //public static final String SGSICONNECTION_NAME = "SG_SI";
    public static final String PROXY_PROXIED_NAME = "Proxy_Proxied";
    public static final String COMPONENT_CSI_TYPE = "csiType";
    public static final String COMPONENT_CSI_TYPES = "csiTypes";
    public static final String COMPONENTPROXY_CSI_TYPE = "proxyCSIType";
    public static final String COMPONENT_PROPERTY = "property";
    public static final String COMPONENT_CAPABILITY_MODEL = "capabilityModel";
    
    /***** Capability Model constants *****/
    public static final String COMPONENT_CAP_X_ACTIVE_AND_Y_STANDBY = "CL_AMS_COMP_CAP_X_ACTIVE_AND_Y_STANDBY";
    public static final String COMPONENT_CAP_X_ACTIVE_OR_Y_STANDBY = "CL_AMS_COMP_CAP_X_ACTIVE_OR_Y_STANDBY";
    public static final String COMPONENT_CAP_ONE_ACTIVE_OR_X_STANDBY = "CL_AMS_COMP_CAP_ONE_ACTIVE_OR_X_STANDBY";
    public static final String COMPONENT_CAP_ONE_ACTIVE_OR_ONE_STANDBY = "CL_AMS_COMP_CAP_ONE_ACTIVE_OR_ONE_STANDBY";
    public static final String COMPONENT_CAP_X_ACTIVE = "CL_AMS_COMP_CAP_X_ACTIVE";
    public static final String COMPONENT_CAP_ONE_ACTIVE = "CL_AMS_COMP_CAP_ONE_ACTIVE";
    public static final String COMPONENT_CAP_NON_PREINSTANTIABLE = "CL_AMS_COMP_CAP_NON_PREINSTANTIABLE";
    
    public static final String IS_SNMP_SUBAGENT = "isSNMPSubAgent";
    
    public static final String SG_REDUNDANCY_MODEL = "redundancyModel";
    public static final String TWO_N_REDUNDANCY_MODEL = 
        "CL_AMS_SG_REDUNDANCY_MODEL_TWO_N";
    public static final String M_PLUS_N_REDUNDANCY_MODEL = 
        "CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N";
    public static final String NO_REDUNDANCY_MODEL = 
        "CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY";
    public static final String NODE_CLASS_TYPE = "classType";
    public static final String NODE_CLASS_A = "CL_AMS_NODE_CLASS_A";
    public static final String NODE_CLASS_B = "CL_AMS_NODE_CLASS_B";
    public static final String NODE_CLASS_C = "CL_AMS_NODE_CLASS_C";
    public static final String NODE_CLASS_D = "CL_AMS_NODE_CLASS_D";
    public static final String CONNECTION_TYPE = "type";
    public static final String CONTAINMENT_FEATURE = "contains";
    public static final String ASSOCIATION_FEATURE = "associatedTo";
    public static final String COMPONENT_PROXIES_FEATURE = "proxies";
    public static final String COMPONENT_INFO_CLASS = "componentInformation";
    static final Color NODE_COLOR
    = new Color(null, 255, 255, 206);
    static final Color CLUSTER_COLOR
    = new Color(null, 255, 255, 255);
    static final Color SERVICEUNIT_COLOR
    = new Color(null, 242, 255, 242);
    static final Color SAFCOMPONENT_COLOR
    = new Color(null, 240, 248, 255);
    static final Color PROXY_COLOR
    = new Color(null, 240, 240, 255);
    static final Color PROXIED_PREINSTANTIABLE_COLOR
    = new Color(null, 240, 240, 255);
    static final Color PROXIED_NON_PREINSTANTIABLE_COLOR
    = new Color(null, 248, 248, 239);
    static final Color NON_PROXIED_NON_PREINSTANTIABLE_COLOR
    = new Color(null, 255, 240, 225);
    static final Color SERVICEGROUP_COLOR
    = new Color(null, 255, 233, 210);
    static final Color SERVICEINSTANCE_COLOR
    = new Color(null, 240, 225, 225);
    static final Color COMPONENTSERVICEINSTANCE_COLOR
    = new Color(null, 255, 221, 221);
}
