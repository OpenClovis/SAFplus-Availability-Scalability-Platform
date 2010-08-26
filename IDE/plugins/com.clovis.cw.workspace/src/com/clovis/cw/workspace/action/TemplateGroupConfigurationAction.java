package com.clovis.cw.workspace.action;

import org.eclipse.jface.action.IAction;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.dialog.TemplateGroupConfigurationDialog;

/**
 * Action to configure template group
 * 
 * @author Suraj Rajyaguru
 *
 */
public class TemplateGroupConfigurationAction extends CommonMenuAction
		implements IViewActionDelegate, IWorkbenchWindowActionDelegate,
		ICWProject {

	/**
	 * Initializes the View.
	 * @param view ViewPart
	 */
	public void init(IViewPart view) {
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#dispose()
	 */
	public void dispose() {
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#init(org.eclipse.ui.IWorkbenchWindow)
	 */
	public void init(IWorkbenchWindow window) {
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	public void run(IAction action) {
		if (_project != null) {
			new TemplateGroupConfigurationDialog(_shell, _project).open();
		}
	}
}
