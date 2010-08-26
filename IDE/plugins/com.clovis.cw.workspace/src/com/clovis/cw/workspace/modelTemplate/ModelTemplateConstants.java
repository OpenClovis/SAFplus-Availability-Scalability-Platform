/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.io.File;

import org.eclipse.core.resources.ResourcesPlugin;

/**
 * Constants for the Model Templates.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public interface ModelTemplateConstants {

	static final String MODEL_TYPE_RESOURCE = "Resource";

	static final String MODEL_TYPE_COMPONENT = "Component";


	static final String EDITOR_TYPE_RESOURCE = "Resource Editor";

	static final String EDITOR_TYPE_COMPONENT = "Component Editor";


	static final String FEATURE_MODEL_TEMPLATE_NAME = "name";

	static final String FEATURE_MODEL_TYPE = "modelType";

	static final String FEATURE_INCLUDE_CHILD_HIERARCHY = "includeChildHierarchy";

	static final String FEATURE_INCLUDE_RELATED_ENTITIES = "includeRelatedEntities";

	static final String FEATURE_ENTITIES = "entities";

	static final String FEATURE_RESOURCE_INFORMATION = "resourceInformation";

	static final String FEATURE_COMPONENT_INFORMATION = "componentInformation";

	static final String FEATURE_ALARM_INFORMATION = "alarmInformation";

	static final String FEATURE_ALARM_RULE_INFORMATION = "alarmRuleInformation";

	static final String FEATURE_RESOURCE_ALARM_MAP_INFORMATION = "resourceAlarmMapInformation";

	static final String FEATURE_COMPONENT_RESOURCE_MAP_INFORMATION = "componentResourceMapInformation";

	static final String FEATURE_RELEASE_VERSION = "releaseVersion";

	static final String FEATURE_UPDATE_VERSION = "updateVersion";


	static final String MODEL_TEMPLATE_FILE_EXT = ".xml";

	static final String MODEL_TEMPLATE_ARCHIEVE_EXT = ".tgz";

	static final String SUFFIX_RESOURCE_MODEL_TEMPLATE_FILE = "_"
			+ MODEL_TYPE_RESOURCE + MODEL_TEMPLATE_FILE_EXT;

	static final String SUFFIX_COMPONENT_MODEL_TEMPLATE_FILE = "_"
			+ MODEL_TYPE_COMPONENT + MODEL_TEMPLATE_FILE_EXT;


	static final String MODEL_TEMPLATE_FOLDER_PATH = ResourcesPlugin
			.getWorkspace().getRoot().getLocation().toOSString()
			+ File.separator + "modelTemplate";


	static final int DIALOG_TYPE_IMPORT = 0;

	static final int DIALOG_TYPE_EXPORT = 1;
}
