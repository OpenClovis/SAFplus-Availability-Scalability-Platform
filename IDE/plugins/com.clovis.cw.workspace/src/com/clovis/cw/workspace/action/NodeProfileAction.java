/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/NodeProfileAction.java $
 * $Author: srajyaguru $
 * $Date: 2006/10/17 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;


import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;

/**
 * @author shubhada
 *
 * Node Profile Action Class
 */
public class NodeProfileAction extends CommonMenuAction implements
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
		if(_project != null) {
        	int actionStatus = canUpdateIM();
        	if(actionStatus == ACTION_CANCEL) {
        		return;
            } else if(actionStatus == ACTION_SAVE_CONTINUE) {
            	updateIM();
            }
            openNodeProfileDialog(_shell, _project);
		}
	}
	/**
     * Opens the NodeProfileDialog 
     * @param shell - Shell
     * @param project - IProject
	 */
    public static void openNodeProfileDialog(Shell shell, IProject project) {
		new NodeProfileDialog(shell, new PreferenceManager(), project).open();
	}

	public static void openNodeProfileDialog(Shell shell, IProject project,
			EObject source) {
		new NodeProfileDialog(shell, new PreferenceManager(), project, source)
				.open();
	}
}
