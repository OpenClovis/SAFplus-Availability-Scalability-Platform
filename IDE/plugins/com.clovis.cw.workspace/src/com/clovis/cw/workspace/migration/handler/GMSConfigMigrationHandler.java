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
 * Handler for GMS Config file.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class GMSConfigMigrationHandler extends AbstractMigrationHandler {

	/**
	 * Constructor.
	 * 
	 * @param project
	 */
	public GMSConfigMigrationHandler(IProject project) {
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
			IFile gmsConfigFile = _project.getFile(new Path(
					ICWProject.CW_PROJECT_CONFIG_DIR_NAME)
					+ File.separator + "gmsconfig.xml");

			if (!gmsConfigFile.exists()) {

				URL xmiURL = FileLocator.find(DataPlugin.getDefault()
						.getBundle(), new Path(
						MigrationConstants.BACKUP_FOLDER_2302_30
								+ File.separator + "gmsconfig.xml"), null);
				Path xmiPath = new Path(FileLocator.resolve(xmiURL).getPath());

				gmsConfigFile.getParent().refreshLocal(1, null);
				gmsConfigFile.create(new FileInputStream(xmiPath.toOSString()),
						true, null);
				gmsConfigFile.getParent().refreshLocal(1, null);
			}

		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating 'gmsconfig.xml' for the project : "
							+ _project.getName() + ".", e);
		}
	}
}
