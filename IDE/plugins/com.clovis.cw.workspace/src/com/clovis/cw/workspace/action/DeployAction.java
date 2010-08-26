/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/DeployAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import java.io.File;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.workspace.dialog.DeploymentDialog;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * @author Pushparaj
 * 
 * Action Class for Deployment
 */
public class DeployAction extends CommonMenuAction implements
IViewActionDelegate, IWorkbenchWindowActionDelegate{
	
	public void init(IViewPart view) {
		_shell = view.getViewSite().getShell();
	}
	
	public void run(IAction action) {
		String imagesPath = CwProjectPropertyPage.getProjectAreaLocation(_project)
		+ File.separator + "target" + File.separator + _project.getName() + File.separator + "images";
		if(!new File(imagesPath).exists()) {
			String message = "Please create images for this project using 'Make Image(s)' option and try again.";
			MessageDialog.openError(_shell, "Deploy images error for " + _project.getName(), message);
			return;
		}
		DeploymentDialog dialog = new DeploymentDialog(_project, _shell);
		dialog.open();
	}
	public void dispose() {
			
	}

	public void init(IWorkbenchWindow window) {
		_shell = window.getShell();
	}
}
