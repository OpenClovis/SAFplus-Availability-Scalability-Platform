/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/GenerateAllAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.jface.action.IAction;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.workspace.codegen.GenerateSource;

/**
 * @author Pushparaj
 * 
 * Action Class for Code Generation.
 */

public class GenerateAllAction extends CommonMenuAction implements
IViewActionDelegate, IWorkbenchWindowActionDelegate {

	public void init(IViewPart view) {
		_shell = view.getViewSite().getShell();
	}

	public void run(IAction action) {
		IProject [] projects = ResourcesPlugin.getWorkspace().getRoot().getProjects();
		GenerateSource genSource = new GenerateSource(_shell);
		try {
			genSource.generateSource(projects);
		} catch (Exception e) {

		}
	}
	public void dispose() {
			
	}

	public void init(IWorkbenchWindow window) {
		_shell = window.getShell();
	}

}
