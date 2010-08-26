/**
 * 
 */
package com.clovis.cw.workspace.migration.handler;

import java.io.File;
import java.io.FileFilter;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.Path;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.migration.MigrationUtils;

/**
 * Migration Handler to add the versioning info to config files.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ConfigVersioningMigrationHandler extends AbstractMigrationHandler {

	/**
	 * Constructor.
	 * 
	 * @param project
	 */
	public ConfigVersioningMigrationHandler(IProject project) {
		super(project);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.migration.handler.AbstractMigrationHandler#migrate()
	 */
	@Override
	public void migrate() {
		File configDir = _project.getFolder(
				new Path(ICWProject.CW_PROJECT_CONFIG_DIR_NAME)).getLocation()
				.toFile();
		File[] configFiles = configDir.listFiles(new FileFilter() {
			public boolean accept(File pathname) {
				if (pathname.toString().endsWith(".xml")) {
					return true;
				}
				return false;
			}
		});

		for (File file : configFiles) {
			addVersioningToFile(file.getAbsolutePath());
		}
	}

	/**
	 * Adds the versioning info to the path.
	 * 
	 * @param path
	 */
	private static void addVersioningToFile(String path) {
		Document document = MigrationUtils.buildDocument(path);
		Element rootElement = document.createElement("openClovisAsp");

		Element versionElement = document.createElement("version");
		rootElement.appendChild(versionElement);

		versionElement.setAttribute("v0", "4.0.0");
		versionElement.appendChild(document.getDocumentElement());

		document.appendChild(rootElement);
		MigrationUtils.saveDocument(document, path);
	}
}
