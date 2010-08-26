/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/GenerateSourceAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.action.IAction;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.workspace.codegen.GenerateSource;

/**
 * @author Pushparaj
 * 
 * Action Class for Code Generation
 */
public class GenerateSourceAction extends CommonMenuAction implements
		IViewActionDelegate, IWorkbenchWindowActionDelegate {

	public void init(IViewPart view) {
		_shell = view.getViewSite().getShell();
	}

	public void run(IAction action) {
		int actionStatus = canUpdateIM();
		if (actionStatus == ACTION_CANCEL) {
			return;
		} else if (actionStatus == ACTION_SAVE_CONTINUE) {
			updateIM();
		}
		IProject[] projects = { _project };
		GenerateSource genSource = new GenerateSource(_shell);
		try {
			genSource.generateSource(projects);
			/*String srcLocation = CwProjectPropertyPage.getSourceLocation(_project);
			if(srcLocation == null || srcLocation.equals("")) {
				return;
			}
			
			String sdkLocation = CwProjectPropertyPage.getSDKLocation(_project);
			if(sdkLocation == null || sdkLocation.equals("")) {
				return;
			}
			if (MessageDialog
					.openConfirm(_shell, "Confirmation for CDT support",
							"Do you want to use CDT support for IDE generated applications?")) {
				NewApplicationProjectCreation creation = new NewApplicationProjectCreation(_project, _shell);
				creation.createCDTProjectsForApplication(srcLocation, sdkLocation);
			}*/
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public void dispose() {
	}

	public void init(IWorkbenchWindow window) {
		_shell = window.getShell();
	}
    
}
