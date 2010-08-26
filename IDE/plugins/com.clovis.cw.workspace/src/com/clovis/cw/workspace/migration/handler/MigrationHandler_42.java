package com.clovis.cw.workspace.migration.handler;

import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;

import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Migration Handler for 4.2
 * @author pushparaj
 *
 */
public class MigrationHandler_42 {
	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());
	
	public static void removeOldCodeGenFiles(IProject project) {
		try {
			IFolder scriptFolder = project.getFolder(ICWProject.CW_PROJECT_SCRIPT_DIR_NAME);
			if(scriptFolder.exists())
				scriptFolder.delete(true, null);
			IFolder templateFolder = project.getFolder(ICWProject.CW_PROJECT_TEMPLATE_DIR_NAME);
			if(templateFolder.exists())
				templateFolder.delete(true, null);
		} catch (CoreException e) {
			LOG.error(
					"Migration : Error in removing old code generation files : "
							+ project.getName() + ".", e);
		}
	}
}
