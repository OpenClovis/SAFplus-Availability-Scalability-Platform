/**
 * 
 */
package com.clovis.cw.workspace.migration;

import java.lang.reflect.InvocationTargetException;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.swt.widgets.Display;

/**
 * Migration thread.
 * 
 * @author Suraj Rajyaguru
 */
public class MigrationThread implements Runnable {

	private IProject _project;

	/**
	 * Constructor.
	 * 
	 * @param project
	 */
	public MigrationThread(IProject project) {
		_project = project;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see java.lang.Runnable#run()
	 */
	public void run() {
		ProgressMonitorDialog pmd = new ProgressMonitorDialog(Display
				.getDefault().getActiveShell());

		try {
			pmd.run(true, false, new MigrationRunnableCode());
		} catch (InvocationTargetException e1) {
			e1.printStackTrace();
		} catch (InterruptedException e1) {
			e1.printStackTrace();
		}
	}

	/**
	 * Runnable code for migration.
	 * 
	 * @author Suraj Rajyaguru
	 */
	class MigrationRunnableCode implements IRunnableWithProgress {

		/*
		 * (non-Javadoc)
		 * 
		 * @see org.eclipse.jface.operation.IRunnableWithProgress#run(org.eclipse.core.runtime.IProgressMonitor)
		 */
		public void run(IProgressMonitor monitor)
				throws InvocationTargetException, InterruptedException {

			monitor.beginTask("Migrating project '" + _project.getName()
					+ "'...", IProgressMonitor.UNKNOWN);
			new MigrationManager(_project).migrateProject();
			monitor.done();
		}
	}
}
