package com.clovis.cw.workspace.project;

import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.swt.widgets.Display;

import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.migration.MigrationUtils;

/**
 * Separate Thread Class for Project Validation. This is synchronized
 * @author Pushparaj
 *
 */
public class ProjectValidatorThread implements Runnable{
	private IProject _project;
	private List _problems = new ArrayList();
	public ProjectValidatorThread(IProject project) {
		_project = project;
	}
	public List getProblems() {
		return _problems;
	}
	public void run() {

		if(MigrationUtils.isMigrationRequired(_project)) {
			return;
		}

		ProgressMonitorDialog pmDialog = null;
		pmDialog =
			new ProgressMonitorDialog(
				Display.getDefault().getActiveShell());
		try {
			pmDialog.run(
				true,
				true,
				new ValidationRunnableCode());
		} catch (InvocationTargetException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}

	}
	
	class ValidationRunnableCode implements IRunnableWithProgress,Runnable {
		public void run(IProgressMonitor monitor) throws InvocationTargetException, InterruptedException {
			monitor.beginTask(
					"Validating : "
						+ _project.getName(),
					IProgressMonitor.UNKNOWN);
			Display.getDefault().syncExec(this);
		    monitor.done();
		    if (monitor.isCanceled())
		        throw new InterruptedException("Project Validation for " + _project.getName() + " is failed");
		}
		public void run() {
			ProjectValidator validator = new ProjectValidator();
			_problems = validator.validate(ProjectDataModel.getProjectDataModel(_project));
		}
	}
	
}
