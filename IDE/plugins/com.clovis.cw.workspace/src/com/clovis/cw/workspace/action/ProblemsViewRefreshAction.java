/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/ProblemsViewRefreshAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Display;

import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.ProblemsView;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.migration.MigrationUtils;
import com.clovis.cw.workspace.project.ProjectValidatorThread;

/**
 * 
 * @author shubhada
 * Action Class to refresh the Problems View 
 */
public class ProblemsViewRefreshAction extends Action
{
	private ProblemsView _view = null;
	/**
	 * 
	 * @param view - ProblemsView
	 */
	public ProblemsViewRefreshAction(
			ProblemsView view)
	{
		_view = view;
	}
	/**
	 * Implementation of run method
	 */
	public void run()
	{
		IProject project = _view.getProject();
		if(project == null) {
			project = _view.findAndUpdateSelectedProject();
		}
		if(project != null) {
			if(canUpdateIM(project)) {
            	updateIM(project);
            }

			if(MigrationUtils.isMigrationRequired(project)) {
				_view.setLabel("Project is not in the latest version. Migrate the Project.");
				return;
			}

			ProjectValidatorThread validatorThread = new ProjectValidatorThread(
					project);
			Display.getDefault().syncExec(validatorThread);
			List problems = validatorThread.getProblems();
			_view.addProblems(problems);
			_view.updateColorsForProblems();
		}
	}
	/**
     * Save all model editors
     */
	public void updateIM(IProject project)
    {	
    	try {
    	ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
    	GenericEditorInput caInput = (GenericEditorInput)pdm.getCAEditorInput();
    	GenericEditorInput compInput = (GenericEditorInput)pdm.getComponentEditorInput();
    	if (caInput != null && caInput.getEditor() != null
				&& caInput.getEditor().getEditorModel() != null
				&& caInput.getEditor().getEditorModel().isDirty())
			caInput.getEditor().doSave(null);
		if (compInput != null && compInput.getEditor() != null
				&& compInput.getEditor().getEditorModel() != null
				&& compInput.getEditor().getEditorModel().isDirty())
			compInput.getEditor().doSave(null);
    	} catch (Exception e) {
    		WorkspacePlugin.LOG.error("Error in Auto Save", e);
    	}
    }
	/**
	 * Checks the Information Model for Project
	 * @return boolean
	 */
	public boolean canUpdateIM(IProject project) 
	{
		try {
			ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
			GenericEditorInput caInput = (GenericEditorInput)pdm.getCAEditorInput();
			GenericEditorInput compInput = (GenericEditorInput)pdm.getComponentEditorInput();
			return (caInput != null && caInput.isDirty()) 
			|| (compInput != null && compInput.isDirty()) ? getStatus() : false;
		} catch(Exception e) {
			return false;
		}
	}
	/**
	 * Gets status from user
	 * @return boolean
	 */
    private boolean getStatus() {
		return MessageDialog.openQuestion(_view.getSite().getShell(), "Save Information Model",
				"Information Model has been modified. Save changes?");
	}
}
