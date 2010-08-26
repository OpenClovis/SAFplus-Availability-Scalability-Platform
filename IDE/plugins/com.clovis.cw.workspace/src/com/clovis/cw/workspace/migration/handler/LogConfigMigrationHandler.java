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
import org.w3c.dom.Document;

import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.migration.MigrationUtils;

/**
 * Handler for the log configuration changes.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class LogConfigMigrationHandler extends AbstractMigrationHandler {

	/**
	 * Constructor.
	 * 
	 * @param project
	 */
	public LogConfigMigrationHandler(IProject project) {
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
			File logConfigFile = _project.getLocation().append(
					ICWProject.CW_PROJECT_CONFIG_DIR_NAME).append(
							"log.xml").toFile();

			if (logConfigFile.exists()) {
				Document logDoc = MigrationUtils.buildDocument(logConfigFile
						.getAbsolutePath());

				if (logDoc.getElementsByTagName("perennialStreamsData")
						.getLength() == 0) {
					logConfigFile.delete();

					URL xmiURL = FileLocator.find(DataPlugin.getDefault()
							.getBundle(), new Path(ICWProject.PLUGIN_XML_FOLDER
							+ File.separator
							+ ICWProject.LOG_DEFAULT_CONFIGS_XML_FILENAME),
							null);
					Path xmiPath = new Path(FileLocator.resolve(xmiURL)
							.getPath());

					String dataFilePath = ICWProject.CW_PROJECT_CONFIG_DIR_NAME
							+ File.separator
							+ "log.xml";
					IFile dst = _project.getFile(new Path(dataFilePath));
					dst.getParent().refreshLocal(1, null);
					dst.create(new FileInputStream(xmiPath.toOSString()), true,
							null);
					dst.getParent().refreshLocal(1, null);
				}
			}

		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating 'log.xml' for the project : "
							+ _project.getName() + ".", e);
		}
	}
}
