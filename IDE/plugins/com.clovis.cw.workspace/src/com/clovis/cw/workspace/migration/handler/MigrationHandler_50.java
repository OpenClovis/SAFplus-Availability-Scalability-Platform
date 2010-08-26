package com.clovis.cw.workspace.migration.handler;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Properties;

import org.eclipse.core.resources.IProject;

import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Migration Handler for 5.0
 * @author pushparaj
 *
 */
public class MigrationHandler_50 {
	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());
	
	public static void updateProjectPropertiesFile(IProject project) {
		Properties properties = new Properties();
		try {
			properties.load(new FileInputStream(project.getLocation()
					.toOSString()
					+ File.separator + ICWProject.CW_PROJECT_PROPERTIES_FILE_NAME));
			if (!properties.containsKey("codegenmode")) {
				properties.clear();
				properties.setProperty("codegenmode", ICWProject.PROJECT_DEFAULT_CODEGEN_OPTION);
				properties.store(new FileOutputStream(project.getLocation()
						.toOSString()
						+ File.separator + ICWProject.CW_PROJECT_PROPERTIES_FILE_NAME),
						ICWProject.CW_PROJECT_PROPERTIES_FILE_DESCRIPTION);
			}
		} catch (Exception e) {
			LOG.error(
					"Migration : Error in updating code gen mode in property file : "
							+ project.getName() + ".", e);
		}
	}
}
