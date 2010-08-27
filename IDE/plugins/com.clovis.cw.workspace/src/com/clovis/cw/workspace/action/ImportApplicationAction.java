/**
 * 
 */
package com.clovis.cw.workspace.action;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.action.IAction;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.workspace.modelTemplate.ImportApplicationDialog;

/**
 * Action class for importing application.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ImportApplicationAction extends CommonMenuAction implements
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
			if (actionStatus == ACTION_CANCEL) {
				return;
			} else if (actionStatus == ACTION_SAVE_CONTINUE) {
				updateIM();
			}
			openImportApplicationDialog(_shell, _project);
		}
	}

	/**
	 * Opens the Import Application Dialog.
	 * 
	 * @param shell -
	 *            Shell
	 * @param project -
	 *            IProject
	 */
	public static void openImportApplicationDialog(Shell shell, IProject project) {
		new ImportApplicationDialog(shell).open();
	}
}
