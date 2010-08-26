/**
 * 
 */
package com.clovis.cw.workspace.migration;

import java.io.File;

import org.eclipse.core.resources.IProject;

/**
 * Migration Utils for file.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MigrationFileUtils {

	/**
	 * Removes file from the given project at the given path.
	 * 
	 * @param project
	 * @param path
	 */
	public static void removeFile(IProject project, String path) {

		File removeFile = project.getLocation().append(path).toFile();
		if (removeFile.exists()) {
			removeFile.delete();
		}
	}
}
