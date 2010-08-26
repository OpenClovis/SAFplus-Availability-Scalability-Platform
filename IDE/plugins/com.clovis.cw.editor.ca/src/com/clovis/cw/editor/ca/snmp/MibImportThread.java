/**
 * 
 */
package com.clovis.cw.editor.ca.snmp;

import java.lang.reflect.InvocationTargetException;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.ProgressMonitorDialog;
import org.eclipse.jface.operation.IRunnableWithProgress;
import org.eclipse.swt.widgets.Display;

/**
 * Mib import thread.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MibImportThread implements Runnable {

	private IProject _project;

	private String _mibName;

	private List _selObjs;

	/**
	 * @param _project
	 * @param mibName
	 * @param objs
	 */
	public MibImportThread(IProject project, String mibName, List selObjs) {
		_project = project;
		_mibName = mibName;
		_selObjs = selObjs;
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
			pmd.run(true, false, new MibImportRunnableCode());
		} catch (InvocationTargetException e1) {
			e1.printStackTrace();
		} catch (InterruptedException e1) {
			e1.printStackTrace();
		}
	}

	/**
	 * Runnable code for Mib import.
	 * 
	 * @author Suraj Rajyaguru
	 * 
	 */
	class MibImportRunnableCode implements IRunnableWithProgress {

		public void run(IProgressMonitor monitor)
				throws InvocationTargetException, InterruptedException {

			monitor.beginTask("Importing from mib", IProgressMonitor.UNKNOWN);
			MibImportManager manager = new MibImportManager(_project, monitor);
			manager.convertMibObjToClovisObj(_mibName, _selObjs);
			monitor.done();
		}
	}
}
