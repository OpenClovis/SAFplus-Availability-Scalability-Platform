/**
 * 
 */
package com.clovis.cw.workspace.migration.handler;

import java.io.File;
import java.io.FileInputStream;
import java.net.URL;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;

import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.migration.MigrationConstants;

/**
 * Handler for DBAL Config file.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class DBALConfigMigrationHandler extends AbstractMigrationHandler {

	/**
	 * Constructor.
	 * 
	 * @param project
	 */
	public DBALConfigMigrationHandler(IProject project) {
		super(project);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.migration.handler.AbstractMigrationHandler#migrate()
	 */
	@Override
	public void migrate() {

		try {
			IFile dbalConfigFile = _project.getFile(new Path(
					ICWProject.CW_PROJECT_CONFIG_DIR_NAME)
					+ File.separator + ICWProject.DBALCONFIG_XML_FILENAME);

			if (!dbalConfigFile.exists()) {

				URL xmiURL = FileLocator.find(DataPlugin.getDefault()
						.getBundle(), new Path(
						MigrationConstants.BACKUP_FOLDER_31
								+ File.separator + ICWProject.DBALCONFIG_XML_FILENAME), null);
				Path xmiPath = new Path(FileLocator.resolve(xmiURL).getPath());

				dbalConfigFile.getParent().refreshLocal(1, null);
				dbalConfigFile.create(new FileInputStream(xmiPath.toOSString()),
						true, null);
				dbalConfigFile.getParent().refreshLocal(1, null);
			}

		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating 'clDbalConfig.xml' for the project : "
							+ _project.getName() + ".", e);
		}
	}
}
