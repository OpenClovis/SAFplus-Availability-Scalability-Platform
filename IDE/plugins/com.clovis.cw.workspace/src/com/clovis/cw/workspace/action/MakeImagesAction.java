/*******************************************************************************
 * ModuleName  : com
 * $File: $
 * $Author: matt $
 * $Date: 2007/10/15 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import java.util.HashMap;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.workspace.builders.MakeImages;
import com.clovis.cw.workspace.dialog.MakeImagesDialog;

/**
 * @author matt
 * 
 * Action Class for Make Images
 */
public class MakeImagesAction extends CommonMenuAction implements
	IViewActionDelegate, IWorkbenchWindowActionDelegate {

	
	public void init(IViewPart view) {
		_shell = view.getViewSite().getShell();
	}

	/**
	 * First calls the MakeImagesDialog to collect settings for the target.conf
	 * file. Then calls MakeImages to run the actual command.
	 */
	public void run(IAction action) {
		HashMap targetConfSettings = MakeImages.readTargetConf(_project);
		
		if (targetConfSettings != null)
		{
			MakeImagesDialog dialog = new MakeImagesDialog(_shell, _project, targetConfSettings);
			dialog.open();
			if (dialog.getReturnCode() == Window.CANCEL) return;
			
			IProject [] projects = {_project};
			MakeImages makeImages = new MakeImages(_shell);
			try {
				makeImages.makeImages(projects);
			} catch (Exception e) {
				e.printStackTrace();
			}
		} else {
			String message = "Project has not been built. Please build the project and try again.";
			MessageDialog.openError(new Shell(), "Build images errors for " + _project.getName(), message);
		}
	}
	
	public void dispose() {
			
	}

	public void init(IWorkbenchWindow window) {
		_shell = window.getShell();
	}
}
