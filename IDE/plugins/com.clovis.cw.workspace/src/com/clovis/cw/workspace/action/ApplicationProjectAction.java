package com.clovis.cw.workspace.action;

import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.workspace.project.CwProjectPropertyPage;
import com.clovis.cw.workspace.project.app.NewApplicationProjectCreation;

public class ApplicationProjectAction extends CommonMenuAction implements
IViewActionDelegate, IWorkbenchWindowActionDelegate{
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.ui.IViewActionDelegate#init(org.eclipse.ui.IViewPart)
	 */
	public void init(IViewPart view) {
		_shell = view.getViewSite().getShell();
	}
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	public void run(IAction action) {
		try {
			String srcLocation = CwProjectPropertyPage.getSourceLocation(_project);
			String sdkLocation = CwProjectPropertyPage.getSDKLocation(_project);
			
			String errMsg = "";
			if (sdkLocation == null || sdkLocation.equals("")) {
				errMsg += "\n SDK location is not set on Project ["
						+ _project.getName()
						+ "]. Use Project->Right Click->properties->Clovis System Project to set its value.\n";
			}
			if (srcLocation == null || srcLocation.equals("")) {
				errMsg += "\n Project Area location is not set on Project ["
						+ _project.getName()
						+ "]. Use Project->Right Click->properties->Clovis System Project to set its value.\n";
			}
			if (!errMsg.equals("")) {
				MessageDialog.openError(_shell, "Project settings errors for " + _project.getName(), errMsg);
				return;
			}
			NewApplicationProjectCreation creation = new NewApplicationProjectCreation(_project, _shell, srcLocation, sdkLocation);
			creation.createCDTProjectsForApplication();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#dispose()
	 */
	public void dispose() {
		
	}
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchWindowActionDelegate#init(org.eclipse.ui.IWorkbenchWindow)
	 */
	public void init(IWorkbenchWindow window) {
		_shell = window.getShell();
	}

}
