package com.clovis.cw.workspace.action;


import org.eclipse.jface.action.IAction;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.manageability.ui.ManageabilityEditorInput;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;


/**
 * 
 * @author Pushparaj
 * 
 * Associate Resource Action Class
 *
 */
public class ManageabilityConfigurationAction extends CommonMenuAction  implements
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
        	IWorkbenchPage page =
                WorkspacePlugin
                    .getDefault()
                    .getWorkbench()
                    .getActiveWorkbenchWindow()
                    .getActivePage();
            if (page != null) {
            	try {
            		IEditorInput caInput = OpenResourceEditorAction.getResourceEditorInput(_project);
            		IEditorInput compInput = OpenComponentEditorAction.getComponentEditorInput(_project);
            		ManageabilityEditorInput input = null;
            		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
            		input = (ManageabilityEditorInput)pdm.getManageabilityEditorInput();
            		if(input == null) {
            			input = new ManageabilityEditorInput(pdm); 
            			pdm.setManageabilityEditorInput(input);
            		}
            		if(caInput != null)
            			input.setCaInput((GenericEditorInput)caInput);
            		if(compInput != null)
            			input.setCompInput((GenericEditorInput)compInput);
                    page.openEditor(input, ICWProject.MANAGEABILITY_EDITOR_ID);
                } catch (Exception e) {
                    WorkspacePlugin.LOG.error("Open Manageability Editor Failed.", e);
                }
            }
		}
	}

}
