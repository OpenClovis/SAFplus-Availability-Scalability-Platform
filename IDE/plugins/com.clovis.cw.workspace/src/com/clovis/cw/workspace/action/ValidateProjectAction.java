/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/ValidateProjectAction.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import java.util.List;

import org.eclipse.jface.action.IAction;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;
import org.eclipse.ui.PartInitException;

import com.clovis.cw.workspace.ProblemsView;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.project.ProjectValidatorThread;

/**
 * Action class for Validate Project
 * @author pushparaj
 *
 */
public class ValidateProjectAction extends CommonMenuAction implements
IViewActionDelegate, IWorkbenchWindowActionDelegate {

	/**
	 * @param view - IViewPart
	 */
	public void init(IViewPart view) {
		_shell = view.getViewSite().getShell();
	}

	public void dispose() {
	}

	public void init(IWorkbenchWindow window) {
		_shell = window.getShell();
	}
	
	public void run(IAction action)
	{
		if (_project != null) {
        	int actionStatus = canUpdateIM();
        	if(actionStatus == ACTION_CANCEL) {
        		return;
            } else if(actionStatus == ACTION_SAVE_CONTINUE) {
            	updateIM();
            }
			ProblemsView view = ProblemsView.getInstance();
			if(view == null) {
				IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
		    	.getActiveWorkbenchWindow().getActivePage();
				if (page != null) {
					try {
						view = (ProblemsView) page.showView("com.clovis.cw.workspace.problemsView");
					} catch (PartInitException e) {
						e.printStackTrace();
					}
					
				}
			}
			if(view != null) {
				if(view.getProject() == _project) {
					_project = view.findAndUpdateSelectedProject();
				}
				ProjectValidatorThread validatorThread = new ProjectValidatorThread(_project);
				Display.getDefault().syncExec(validatorThread);
				List problems = validatorThread.getProblems();
				view.addProblems(problems);
				view.updateColorsForProblems();
			}
		}
	}

}
