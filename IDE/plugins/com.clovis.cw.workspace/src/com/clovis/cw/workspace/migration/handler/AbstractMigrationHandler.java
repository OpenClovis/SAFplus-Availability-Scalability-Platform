/**
 * 
 */
package com.clovis.cw.workspace.migration.handler;

import org.eclipse.core.resources.IProject;

import com.clovis.common.utils.log.Log;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Abstract implementation for the single task based migration handler.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public abstract class AbstractMigrationHandler {

	protected IProject _project;

	protected static Log LOG = Log.getLog(WorkspacePlugin.getDefault());

	/**
	 * Constructor.
	 * 
	 * @param _project
	 */
	public AbstractMigrationHandler(IProject project) {
		_project = project;
	}

	/**
	 * Performs the migration.
	 */
	public abstract void migrate();
}
