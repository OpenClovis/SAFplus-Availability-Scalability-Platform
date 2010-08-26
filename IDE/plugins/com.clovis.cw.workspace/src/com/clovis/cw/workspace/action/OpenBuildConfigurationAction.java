/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/OpenBuildConfigurationAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.action.CommonMenuAction;
import com.clovis.cw.workspace.dialog.BuildTime;

/**
 * @author shanth
 *
 *Action class for Build Configuration
 */
public class OpenBuildConfigurationAction extends CommonMenuAction implements
		IViewActionDelegate, IWorkbenchWindowActionDelegate {

	/**
	 * @see org.eclipse.ui.IViewActionDelegate#init(org.eclipse.ui.IViewPart)
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
            PreferenceManager pmanager = new PreferenceManager();
            try {
                new BuildTime(_shell, pmanager, _project).open();
            } catch (Exception ex) {
                WorkspacePlugin.LOG.error("Failed to open Build Config.", ex);
            }
        }
	}
}
