/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/CommonMenuAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IActionDelegate;

import com.clovis.cw.editor.ca.manageability.ui.ManageabilityEditorInput;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.migration.MigrationUtils;
import com.clovis.cw.workspace.natures.SystemProjectNature;

/**
 * @author pushparaj
 * Contains Common functionalities for both workbench and view Actions.
 */
public abstract class CommonMenuAction implements IActionDelegate 
{
	protected static IProject _project = null;
	protected Shell _shell = null;
	protected int ACTION_CONTINUE = 0;
	protected int ACTION_SAVE_CONTINUE = 1;
	protected int ACTION_CANCEL = 2;
			
	public void selectionChanged(IAction action, ISelection selection) {
		//IProject project = null;
		if (selection instanceof IStructuredSelection) {
			IStructuredSelection sel = (IStructuredSelection) selection;
			if (sel.getFirstElement() instanceof IResource) {
				IResource resource = (IResource) sel.getFirstElement();
				_project = resource.getProject();
			}
		}
		try {
			if (_project != null
					&& _project.exists()
					&& _project.isOpen()
					&& _project
							.hasNature(SystemProjectNature.CLOVIS_SYSTEM_PROJECT_NATURE)
					&& !MigrationUtils.isMigrationRequired(_project)) {
				action.setEnabled(true);
			} else {
				action.setEnabled(false);
			}
		} catch (Exception e) {
			WorkspacePlugin.LOG.error("Failed to check project nature.", e);
		}
	}
	/**
	 * Checks the Information Model for Project
	 * @return boolean
	 */
	public int canUpdateIM() 
	{
		try {
			ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
			GenericEditorInput caInput = (GenericEditorInput)pdm.getCAEditorInput();
			GenericEditorInput compInput = (GenericEditorInput)pdm.getComponentEditorInput();
			ManageabilityEditorInput maInput = (ManageabilityEditorInput) pdm.getManageabilityEditorInput();
			return (caInput != null && caInput.isDirty()) 
			|| (compInput != null && compInput.isDirty()) || (maInput != null && maInput.isDirty()) ? getStatus() : ACTION_CONTINUE;
		} catch(Exception e) {
			return ACTION_CONTINUE;
		}
	}
	/**
	 * Gets status from user
	 * @return boolean
	 */
    private int getStatus() {
        return new MessageDialog(_shell, "Save and Launch",
                null, "Information Model has been modified.", MessageDialog.QUESTION,
                new String[]{"Continue", "Save and Continue", "Cancel"}, 0).open();
	}
    /**
     * Save all model editors
     */
    public void updateIM()
    {	
    	try {
    	ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
    	GenericEditorInput caInput = (GenericEditorInput)pdm.getCAEditorInput();
    	GenericEditorInput compInput = (GenericEditorInput)pdm.getComponentEditorInput();
    	ManageabilityEditorInput maInput = (ManageabilityEditorInput) pdm.getManageabilityEditorInput();
    	if (caInput != null && caInput.getEditor() != null
				&& caInput.getEditor().getEditorModel() != null
				&& caInput.getEditor().getEditorModel().isDirty())
			caInput.getEditor().doSave(null);
		if (compInput != null && compInput.getEditor() != null
				&& compInput.getEditor().getEditorModel() != null
				&& compInput.getEditor().getEditorModel().isDirty())
			compInput.getEditor().doSave(null);
		if(maInput != null && maInput.isDirty())
			maInput.getEditor().doSave(null);
		
    	} catch (Exception e) {
    		WorkspacePlugin.LOG.error("Error in Auto Save", e);
    	}
    }
    public static IProject getProject() {
    	return _project;
    }
}
