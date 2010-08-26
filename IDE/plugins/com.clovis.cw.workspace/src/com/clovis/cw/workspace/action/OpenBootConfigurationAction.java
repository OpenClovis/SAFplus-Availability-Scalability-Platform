/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/OpenBootConfigurationAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.common.utils.ClovisProjectUtils;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.action.CommonMenuAction;
import com.clovis.cw.workspace.dialog.BootTime;

/**
 * @author shanth
 *
 * Action class for Boot configuration
 */
public class OpenBootConfigurationAction extends CommonMenuAction implements
		IViewActionDelegate, IWorkbenchWindowActionDelegate {

	/**
     * @see org.eclipse.ui.IViewActionDelegate#init(org.eclipse.ui.IViewPart)
     */
    public void init(IViewPart view)
    {
    	_shell = view.getViewSite().getShell();
    }

    /**
     * @see org.eclipse.ui.IActionDelegate#run
     *      (org.eclipse.jface.action.IAction)
     */
    public void run(IAction action)
	{
		if (_project != null) {
        	int actionStatus = canUpdateIM();
        	if(actionStatus == ACTION_CANCEL) {
        		return;
            } else if(actionStatus == ACTION_SAVE_CONTINUE) {
            	updateIM();
            }
			openBootConfigurationDialog(_shell, _project);
        }

	}

    /**
	 * Opens the Boot Configuration Dialog.
	 * 
	 * @param shell -
	 *            Shell
	 * @param project -
	 *            IProject
	 */
	public static void openBootConfigurationDialog(Shell shell, IProject project) {
		PreferenceManager pmanager = new PreferenceManager();
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
		List nodes = ComponentDataUtils.getNodesList(pdm.getComponentModel()
				.getEList());
		BootTime pDialog = new BootTime(shell, pmanager, nodes, project);

		pDialog.open();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.action.CommonMenuAction#selectionChanged(org.eclipse.jface.action.IAction,
	 *      org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection) {
		super.selectionChanged(action, selection);

		if (_project != null
				&& _project.isOpen()
				&& !ClovisProjectUtils.getCodeGenMode(_project).equals(
						"openclovis")) {

			action.setEnabled(false);
		}
	}

	public void dispose() {
		
	}
	public void init(IWorkbenchWindow window) {
		_shell = window.getShell();
	}
	
}
