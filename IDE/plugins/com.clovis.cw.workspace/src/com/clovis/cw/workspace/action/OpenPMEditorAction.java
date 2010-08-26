/**
 * 
 */
package com.clovis.cw.workspace.action;

import org.eclipse.jface.action.IAction;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.pm.PMEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Action to open PM Editor.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class OpenPMEditorAction extends CommonMenuAction implements
		IViewActionDelegate, IWorkbenchWindowActionDelegate {

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.ui.IViewActionDelegate#init(org.eclipse.ui.IViewPart)
	 */
	public void init(IViewPart view) {
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

			IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
					.getActiveWorkbenchWindow().getActivePage();

			if (page != null) {
				try {
					PMEditorInput input = new PMEditorInput(ProjectDataModel
							.getProjectDataModel(_project));
					page.openEditor(input, ICWProject.PM_EDITOR_ID);

				} catch (Exception e) {
					WorkspacePlugin.LOG.error("Open PM Editor Failed.", e);
				}
			}
		}
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
	}
}
