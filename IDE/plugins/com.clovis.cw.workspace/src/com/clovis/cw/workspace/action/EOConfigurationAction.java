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

import com.clovis.cw.editor.ca.dialog.EOConfigurationDialog;

/**
 * Action class for EO Configuration.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class EOConfigurationAction extends CommonMenuAction implements
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
   			openEOConfigurationDialog(_shell, _project);
		}
	}

	/**
	 * Opens the EOConfigurationDialog
	 * 
	 * @param shell -
	 *            Shell
	 * @param project -
	 *            IProject
	 */
	public static void openEOConfigurationDialog(Shell shell,
			IProject project) {
		new EOConfigurationDialog(shell, new PreferenceManager(), project)
				.open();
	}
}
