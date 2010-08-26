/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/WebEMSConfigurationAction.java $
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

import com.clovis.cw.editor.ca.dialog.WebEMSDialog;

/**
 * @author shubhada
 *
 * Node Profile Action Class
 */
public class WebEMSConfigurationAction extends CommonMenuAction implements
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
			PreferenceManager pmanager = new PreferenceManager();
	        WebEMSDialog pDialog =
	            new WebEMSDialog(_shell,  pmanager, _project);
	        pDialog.open();
		}
	}
}
