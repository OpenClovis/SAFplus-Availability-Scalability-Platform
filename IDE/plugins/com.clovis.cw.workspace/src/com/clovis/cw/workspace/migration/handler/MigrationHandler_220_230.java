/**
 * 
 */
package com.clovis.cw.workspace.migration.handler;

import java.io.File;

import org.eclipse.core.resources.IProject;

import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Migration handler for 2.2.0 to 2.3.0 migration.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MigrationHandler_220_230 implements ICWProject {

	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());

	private static IProject _project = null;

	/**
	 * Performs the migration.
	 * 
	 * @param proj
	 */
	public static void migrate(IProject proj) {
		_project = proj;
		migrateRMD();
	}

	/**
	 * Migrates the RMD file.
	 */
	private static void migrateRMD() {
		String fileName = _project.getLocation().toOSString() + File.separator
				+ ICWProject.CW_PROJECT_IDL_DIR_NAME + File.separator
				+ IDL_XML_DATA_FILENAME;

		try {
			if (new File(fileName).exists())
				new File(fileName).delete();

		} catch (Exception e) {
			LOG.error("Migration : Error migrating " + fileName + ".", e);
		}
	}
}
