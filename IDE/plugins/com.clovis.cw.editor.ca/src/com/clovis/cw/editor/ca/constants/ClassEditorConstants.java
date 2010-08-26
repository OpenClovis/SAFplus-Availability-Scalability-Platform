/*******************************************************************************
 * ModuleName  : com
 * $File$
 * $Author$
 * $Date$
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.constants;

import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.SWT;
import org.eclipse.jface.resource.JFaceResources;
/**
 * @author ashish
 * Provides constants for Resource Editor
 */
public interface ClassEditorConstants
{
    static final Font CLASS_NAME_FONT =
        new Font(null, JFaceResources.TEXT_FONT, 10, SWT.NORMAL);
    static final Font NAME_ITALICS_FONT =
        new Font(null, JFaceResources.TEXT_FONT, 10, SWT.NORMAL | SWT.ITALIC);
    static final Font EXTENSIONS_FONT =
        new Font(null, JFaceResources.TEXT_FONT, 7, SWT.NORMAL);
    static final Font ATTRIBUTE_NAME_FONT =
        new Font(null, JFaceResources.TEXT_FONT, 9, SWT.NORMAL | SWT.NORMAL);
    static final Font METHOD_NAME_ITALICS_FONT =
        new Font(null, JFaceResources.TEXT_FONT, 9, SWT.NORMAL | SWT.ITALIC);
    static final Font METHOD_NAME_FONT =
        new Font(null, JFaceResources.TEXT_FONT, 9, SWT.NORMAL | SWT.NORMAL);
    public static final String EDITOR_TYPE = "Resource Editor";
    public static final String CHASSIS_NUM_SLOTS = "numSlots";
    // Various settable and gettable properties of a class
    public static final String CLASS_NAME = "name";
    public static final String CLASS_MAX_INSTANCES = "maxInstances";
    public static final String CLASS_ATTRIBUTES = "attribute";
    public static final String CLASS_METHODS = "Methods";
    public static final String CLASS_SERVICE_TYPE = "ServiceType";
    public static final String CLASS_DOCUMENTATION = "documentation";

    public static final String ATTRIBUTE_CLASS_NAME = "Attribute";
    public static final String PROV_ATTRIBUTE_CLASS_NAME = "ProvAttribute";
    public static final String PM_ATTRIBUTE_CLASS_NAME = "PMAttribute";
    public static final String ATTRIBUTE_NAME = "name";
    public static final String ATTRIBUTE_TYPE = "dataType";
    public static final String ATTRIBUTE_MULTIPLICITY = "multiplicity";
    public static final String ATTRIBUTE_IS_IMPORTED = "isImported";
    public static final String ATTRIBUTE_MIN_VALUE = "minValue";
    public static final String ATTRIBUTE_MAX_VALUE = "maxValue";
    public static final String ATTRIBUTE_DEFAULT_VALUE = "defaultValue";
    public static final String ATTRIBUTE_OID = "OID";
    public static final String ATTRIBUTE_ATTRIBUTETYPE = "type";
    public static final String ATTRIBUTE_ACCESS = "access";
    public static final String ATTRIBUTE_ACCESS_READWRITE = "READWRITE";
    public static final String ATTRIBUTE_PERSISTENCY = "persistency";
    public static final String ATTRIBUTE_CACHING = "caching";
    public static final String ATTRIBUTE_WRITABLE = "writable";
    public static final String ATTRIBUTE_INITIALIZED = "initialized";

    public static final String METHOD_NAME = "Name";
    public static final String METHOD_RETURN_TYPE = "Type";
    public static final String METHOD_IS_ABSTRACT = "IsAbstract";
    public static final String METHOD_IS_STATIC = "IsStatic";
    public static final String METHOD_IS_CONST = "IsConst";
    public static final String METHOD_MODIFIER = "Modifier";
    public static final String METHOD_PARAMS = "Params";
    public static final String METHOD_DOCUMENTATION = "Documentation";

    public static final String PARAM_NAME = "Name";
    public static final String PARAM_TYPE = "Type";
    public static final String PARAM_MULTIPLICITY = "Multiplicity";

    public static final String RESOURCE_PROVISIONING = "provisioning";
    public static final String RESOURCE_PM = "pm";
    public static final String RESOURCE_CHASSIS = "ChassisManagement";
    public static final String RESOURCE_ALARM = "alarmManagement";
    public static final String RESOURCE_FAULT = "FaultManagement";
    public static final String DEVICE_OBJECT = "deviceObject";
    public static final String ASSOCIATED_DO = "associatedDO";
    public static final String DEVICE_ID = "deviceId";
    public static final String DO_BOOT_PRIORITY = "bootPriority";
    public static final String ASSOCIATED_ALARM_IDS = "alarmIDs";
    public static final String ASSOCIATED_ALARM_LINK = "associatedAlarm";
    public static final String ALARM_PROBABLE_CAUSE = "ProbableCause";
    public static final String ALARM_ID = "alarmID";
    public static final String ALARM_ALARMPROFILE = "AlarmProfile";
    public static final String ALARM_POLLING_INTERVAL = "pollingInterval";
    public static final String ALARM_GENERATIONRULE = "generationRule";
    public static final String ALARM_SUPPRESSIONRULE = "suppressionRule";
    public static final String ALARM_RELATIONTYPE = "relationType";
    public static final String ALARM_MAXALARM = "maxAlarm";
    public static final String ALARM_ALARMOBJ = "alarm";
    public static final String ALARM_ALARMRULE = "alarmRule";
    public static final String ALARM_RESOURCE = "resource";
    public static final String ALARM_RESOURCE_NAME = "name";
    public static final String ALARM_SPECIFIC_PROBLEM = "SpecificProblem";
    public static final String MIB_NAME_FEATURE = "mibName";
    public static final String MIB_RESOURCE_SCALAR_TYPE = "isScalar";
    
    public static final String DATA_STRUCTURE_NAME = "DataStructure";
    public static final String RESOURCE_NAME = "Resource";
    public static final String HARDWARE_RESOURCE_NAME = "HardwareResource";
    public static final String SOFTWARE_RESOURCE_NAME = "SoftwareResource";
    public static final String MIB_RESOURCE_NAME = "MibResource";
    public static final String MIB_CLASS_NAME = "Mib";
    public static final String NODE_HARDWARE_RESOURCE_NAME
    = "NodeHardwareResource";
    public static final String SYSTEM_CONTROLLER_NAME = "SystemController";
    public static final String CHASSIS_RESOURCE_NAME = "ChassisResource";
    
    public static final String CHASSIS_RESOURCE_REF_NAME = "chassisResource";
    public static final String DATA_STRUCTURE_REF_NAME = "dataStructure";
    public static final String HARDWARE_RESOURCE_REF_NAME = "hardwareResource";
    public static final String SOFTWARE_RESOURCE_REF_NAME = "softwareResource";
    public static final String MIB_RESOURCE_REF_NAME = "mibResource";
    public static final String MIB_REF_NAME = "mib";
    public static final String NODE_HARDWARE_RESOURCE_REF_NAME
    = "nodeHardwareResource";
    public static final String SYSTEM_CONTROLLER_REF_NAME = "systemController";
    
    public static final String NODES_REF_TYPES[] = {
    	CHASSIS_RESOURCE_REF_NAME,
    	DATA_STRUCTURE_REF_NAME,
    	HARDWARE_RESOURCE_REF_NAME,
    	SOFTWARE_RESOURCE_REF_NAME,
    	NODE_HARDWARE_RESOURCE_REF_NAME,
    	SYSTEM_CONTROLLER_REF_NAME,
    	MIB_RESOURCE_REF_NAME,
    	MIB_REF_NAME
    };
                                               
    public static final String CONNECTION_NAME = "Connection";
    public static final String ASSOCIATION_NAME = "Association";
    public static final String COMPOSITION_NAME = "Composition";
    public static final String INHERITENCE_NAME = "Inheritence";
    public static final String ASSOCIATION_REF_NAME = "association";
    public static final String COMPOSITION_REF_NAME = "composition";
    public static final String INHERITENCE_REF_NAME = "inheritance";
    
    public static final String EDGES_REF_TYPES[] = {
    	ASSOCIATION_REF_NAME,
    	COMPOSITION_REF_NAME,
    	INHERITENCE_REF_NAME
    	};
    
    public static final String CONNECTION_START = "source";
    public static final String CONNECTION_END = "target";
    public static final String PARENT_FEATURE = "parent";
    public static final String CONTAINMENT_FEATURE = "contains";
    public static final String ASSOCIATION_FEATURE = "associatedTo";
    public static final String INHERITANCE_FEATURE = "inherits";
    public static final String RESOURCE_INFO_CLASS = "resourceInformation";
    
    static final Color DATA_STRUCTURE_COLOR
        = new Color(null, 229, 229, 229);
    static final Color HARDWARE_RESOURCE_COLOR
        = new Color(null, 221, 255, 221);
    static final Color SOFTWARE_RESOURCE_COLOR
        = new Color(null, 255, 217, 255);
    static final Color NODE_HARDWARE_RESOURCE_COLOR
        = new Color(null, 255, 228, 202);
    static final Color SYSTEM_CONROLLER_COLOR
        = new Color(null, 219, 219, 255);
    static final Color IS_ABSTRACT_COLOR
        = new Color(null, 215, 230, 170);

    static final String METHOD_NAME_LABEL         = "Name:";
    static final String METHOD_TYPE_LABEL         = "Return Type:";
    static final String VISIBILITY_LABLE_NAME     = "Visibility:";
    static final String VISIBILITY_PUBLIC_NAME    = "public";
    static final String VISIBILITY_DATA           = "VISIBILITY_BUTTON";
    static final String VISIBILITY_PRIVATE_NAME   = "private";
    static final String MODIFIER_LABLE_NAME       = "Modifier:";
    static final String PARAMETERS_GROUP_NAME     = "Parameters";
    static final String METHOD_PROPERTIES_MESSAGE = "Edit Operation Properties";

    static final int META_CLASSES_ONLY = 1;
    static final int META_OBJECTS_ONLY = 2;
    static final int META_CLASSES_AND_META_OBJECTS = 3;
}
