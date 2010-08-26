/**
 * 
 */
package com.clovis.cw.workspace.action;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.editor.ca.dialog.MemoryConfigurationDialog;

/**
 * Action class for Memory Configuration.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MemoryConfigurationAction extends CommonMenuAction implements
		IViewActionDelegate, IWorkbenchWindowActionDelegate {

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.IViewActionDelegate#init(org.eclipse.ui.IViewPart)
	 */
	public void init(IViewPart view) {
		_shell = view.getViewSite().getShell();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#dispose()
	 */
	public void dispose() {
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#init(org.eclipse.ui.IWorkbenchWindow)
	 */
	public void init(IWorkbenchWindow window) {
		_shell = window.getShell();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	public void run(IAction action) {
		if (_project != null) {
        	int actionStatus = canUpdateIM();
        	if(actionStatus == ACTION_CANCEL) {
        		return;
            } else if(actionStatus == ACTION_SAVE_CONTINUE) {
            	updateIM();
            }
			openMemoryConfigurationDialog(_shell, _project);
		}
	}

	/**
	 * Opens the MemoryConfigurationDialog
	 * 
	 * @param shell -
	 *            Shell
	 * @param project -
	 *            IProject
	 */
	public static void openMemoryConfigurationDialog(Shell shell,
			IProject project) {
		new MemoryConfigurationDialog(shell, new PreferenceManager(), project)
				.open();
	}
}
