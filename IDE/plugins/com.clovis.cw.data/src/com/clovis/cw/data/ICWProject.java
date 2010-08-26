/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/data/ICWProject.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 * Project's Constants
 */
package com.clovis.cw.data;
public interface ICWProject
{
    final String RESOURCE_ECORE_FILENAME       = "resource.ecore";
    final String RESOURCE_METACLASS_FILENAME   = "resource.xml";
    final String RESOURCE_XML_DATA_FILENAME    = "resourcedata.xml";
    final String RESOURCE_UI_PROPRTY_FILENAME  = "resourceui.xml";
    final String COMPONENT_ECORE_FILENAME      = "component.ecore";
    final String COMPONENT_METACLASS_FILENAME  = "component.xml";
    final String COMPONENT_XMI_DATA_FILENAME   = "componentdata.xml";
    final String COMPONENT_UI_PROPRTY_FILENAME = "componentui.xml";
    final String AMFDEF_XML_DATA_FILENAME   = "clAmfDefinitions.xml";
    final String CPM_XML_DATA_FILENAME   = "clAmfConfig.xml";
    final String CPM_ECORE_FILENAME      = "cpmconfig.ecore";
    final String IOCBOOT_XML_DATA_FILENAME   = "clIocConfig.xml";
    final String IOCBOOT_DEFAULT_XML_DATA_FILENAME   = "defaultClIocConfig.xml";
    final String IOCBOOT_ECORE_FILENAME      = "ioc.ecore";
    final String SLOT_INFORMATION_ECORE   = "map.ecore";
    final String SLOT_INFORMATION_XML_FILENAME   = "clSlotInfo.xml";
    final String SLOT_INFO_DEFAULT_CONFIGS_XML_FILENAME = "defaultClSlotInfo.xml";
    final String GMS_INFO_DEFAULT_CONFIGS_XML_FILENAME = "defaultgmsconfig.xml";
    final String GMS_CONFIGURATION_XML_FILENAME   = "clGmsConfig.xml";
    final String GMS_CONFIGURATION_ECORE_FILENAME      = "gms.ecore";
    final String IDL_XML_DATA_FILENAME   = "server_idl_interface.xml";
    final String IDL_ECORE_FILENAME      = "rmd.ecore";
    final String DBALCONFIG_ECORE_FILENAME = "dbal.ecore";
    final String DBALCONFIG_XML_FILENAME = "clDbalConfig.xml";
    final String DBALCONFIG_DEFAULT_XML_FILENAME = "defaultClDbalConfig.xml";
    final String PROBLEM_ECORE_FILENAME  = "problem.ecore";
    final String SUBMODEL_MAPPING_ECORE_FILENAME  = "modelMapping.ecore";
    final String TEMPLATEGROUP_MAPPING_ECORE_FILENAME = "templateGroupMapping.ecore";
    final String TEMPLATEGROUP_MAPPING_XML_FILENAME = "component_templateGroup_map.xml";
    final String MODEL_TEMPLATE_ECORE_FILENAME = "modelTemplate.ecore";
    final String MEMORYCONFIG_ECORE_FILENAME = "memoryConfig.ecore";
    final String MEMORYCONFIG_XML_DATA_FILENAME = "clEoDefinitions.xml";
    final String MEMORYCONFIG_DEFAULT_XML_DATA_FILENAME = "defaultClEoDefinitions.xml";
    final String EOCONFIG_ECORE_FILENAME = "eoConfig.ecore";
    final String EOCONFIG_XML_DATA_FILENAME = "clEoConfig.xml";
    final String EOCONFIG_DEFAULT_XML_DATA_FILENAME = "defaultClEoConfig.xml";
    final String CW_PROJECT_ENV_FILENAME = "clModelAspEnv.sh";
    final String CW_PROJECT_SCRIPT_FILENAME = "cl_build.sh";
    final String CW_PROJECT_MAKE_FILENAME = "Makefile";
    final String CW_PROJECT_MODEL_DIR_NAME     = "models";
    final String CW_PROJECT_SRC_DIR_NAME       = "src";
    final String CW_PROJECT_SCRIPT_DIR_NAME    = "scripts";
    final String CW_PROJECT_TEMPLATE_DIR_NAME     = "templates";
    final String CW_PROJECT_PROPERTIES_FILE_NAME     = "project.properties";
    final String CW_PROJECT_PROPERTIES_FILE_DESCRIPTION     = "Project Properties";
    final String CW_PROJECT_DEFAULT_TEMPLATE_GROUP_DIR_NAME  = "default";
    final String CW_PROJECT_TEMPLATE_GROUP_MARKER = ".templates";
    final String CW_PROJECT_IDL_DIR_NAME    = "idl";
    final String RESOURCE_EDITOR_ID         = "com.clovis.cw.editor.resource";
    final String COMPONENT_EDITOR_ID        = "com.clovis.cw.editor.component";
    final String MANAGEABILITY_EDITOR_ID        = "com.clovis.cw.editor.manageability";
    final String PM_EDITOR_ID               = "com.clovis.cw.editor.pm";

    final String BACKUP_RESOURCE_XML_DATA_FILENAME  = "resourcedata_bak.xml";
    final String BACKUP_COMPONENT_XMI_DATA_FILENAME = "componentdata_bak.xml";
    final String ALARM_PROFILES_ECORE_FILENAME      = "alarm.ecore";
    final String ALARM_PROFILES_XMI_DATA_FILENAME   = "alarmdata.xml";
    final String ALARM_RULES_ECORE_FILENAME      	= "alarmRule.ecore";
    final String ALARM_RULES_XML_DATA_FILENAME   	= "alarmrule.xml";
    final String ALARM_ASSOCIATION_ECORE_FILENAME   = "alarmAssociation.ecore";
    final String ALARM_ASSOCIATION_XML_FILENAME     = "alarmAssociation.xml";
    final String RESOURCE_ASSOCIATION_ECORE_FILENAME   = "resourceAssociation.ecore";
    final String RESOURCE_ASSOCIATION_XML_FILENAME     = "resourceAssociation.xml";
    
    final String MIGRATION_XMI_DATA_FILENAME   = "migration";
    final String PLUGIN_XML_FOLDER                  = "xml";
    final String PLUGIN_MODELS_FOLDER               = "model";
    final String PLUGIN_MIGRATION_FOLDER               = "migration";
    
    final String PROJECT_SCRIPT_FOLDER              = "scripts";
    final String PROJECT_TEMPLATE_FOLDER              = "templates";
    final String PROJECT_DEFAULT_TEMPLATE_GROUP_FOLDER = "default";
    final String PROJECT_DEFAULT_SAF_TEMPLATE_GROUP_FOLDER = "saf";
    final String PROJECT_DEFAULT_NONSAF_TEMPLATE_GROUP_FOLDER = "nonsaf";
    final String PLUGIN_DEFAULT_XMIS_FOLDER         = "defaultxmis";
    final String CW_PROFILES_FOLDER_NAME            = "profiles";
    final String CW_ASP_COMPONENTS_FOLDER_NAME      = "aspcomponents";
    final String CW_SAF_COMPONENTS_FOLDER_NAME      = "safcomponents";
    final String CW_ASP_COMPONENTS_CONFIGS_FOLDER_NAME  =
                                                        "aspcompconfigs";
    final String CW_BOOT_TIME_COMPONENTS_FOLDER_NAME  = "boottime";
    final String CW_BUILD_TIME_COMPONENTS_FOLDER_NAME  = "buildtime";
    final String CW_COMPILE_TIME_COMPONENTS_FOLDER_NAME = "compiletime";
    final String CW_SYSTEM_COMPONENTS_FOLDER_NAME  = "system";
    final String CW_PROJECT_CONFIG_DIR_NAME = "configs";
    final String RESOURCE_DEFAULT_XMI_FILENAME  = "defresaourcedata.xmi";
    final String COMPONENT_DEFAULT_XMI_FILENAME = "defcomponentdata.xmi";
    final String COMPILE_CONFIGS_XMI_FILENAME = "compileconfigs.xml";
    final String LOG_DEFAULT_CONFIGS_XML_FILENAME = "defaultlog.xml";
    final String LOG_CONFIGS_XML_FILENAME = "clLog.xml";
    final String RESOURCE_TEMPLATE_FOLDER = "resource_templates";
    final String COMPONENT_TEMPLATE_FOLDER = "component_templates";
    final String PROJECT_TEMP_DIR = ".temp_dir";
    
    //project xml file names
    final String PROJECT_RESOURCE_DATA_FILE_NAME = "resourceData.xml";
    final String PROJECT_COMPONENT_DATA_FILE_NAME = "componentData.xml";
    final String PROJECT_ALARM_DATA_FILE_NAME = "alarmData.xmi";
    final String PROJECT_AMF_CONFIGURATION_FILE_NAME = "clAmfConfig.xml";
    final String PROJECT_IOC_CONFIGURATION_FILE_NAME = "iocConfig.xml";
    final String PROJECT_SLOT_CONFIGURATION_FILE_NAME = "slotInfo.xml";
    final String PROJECT_GMS_CONFIGURATION_FILE_NAME = "clGmsConfig.xml";
    final String PROJECT_LOG_CONFIGURATION_FILE_NAME = "clLog.xml";
    final String PROJECT_COMPILE_CONFIGURATION_FILE_NAME = "compileConfig.xmi";
    final String PROJECT_IDL_FILE_NAME = "idlInterface.xml";
    final String PROJECT_CODEGEN_FOLDER = "codegen";
    final String PROJECT_DEFAULT_CODEGEN_OPTION = "openclovis";
    final String CLOVIS_CODEGEN_OPTION = "openclovis";
    
    
}
