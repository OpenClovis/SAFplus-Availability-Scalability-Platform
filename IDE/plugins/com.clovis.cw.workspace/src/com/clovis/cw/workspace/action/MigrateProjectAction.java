/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/MigrateProjectAction.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.migration.MigrationDialog;
import com.clovis.cw.workspace.project.migration.MigrateProjectDialog;
/**
 * 
 * @author shubhada
 *
 * 
 */
public class MigrateProjectAction
implements IWorkbenchWindowActionDelegate, IViewActionDelegate
{
	private Shell _shell = null;
    /**
     * @param action - IAction
     */
    public void run(IAction action)
    {
        updateIM();

        IProject selectedProject = MigrateProjectDialog.getSelectedProject();
		if (selectedProject != null) {
			MigrationDialog dialog = new MigrationDialog(_shell,  selectedProject);
			dialog.open();
		}
    }
    /**
	 * Does nothing
	 */
    public void dispose()
    {
    }
    /**
     * initializes the shell variable
     * @param window - IWorkbenchWindow
     * 
     */
    public void init(IWorkbenchWindow window)
    {
    	_shell = window.getShell();
    }
    /**
     * initializes the shell variable
     * @param view - IViewPart
     */
    public void init(IViewPart view)
    {
        _shell = view.getViewSite().getShell();
        
    }
    /**
     * Save all model editors
     */
    public void updateIM()
    {   
        try {
            IWorkspaceRoot workspaceRoot = ResourcesPlugin.getWorkspace().getRoot();
            IProject [] projects = workspaceRoot.getProjects();
            
            for (int i = 0; i < projects.length; i++) {
                IProject project = projects[i];
                ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
                GenericEditorInput caInput = (GenericEditorInput)pdm.getCAEditorInput();
                GenericEditorInput compInput = (GenericEditorInput) pdm.getComponentEditorInput();
               
                if (caInput != null && caInput.getEditor() != null
                        && caInput.getEditor().getEditorModel() != null
                        && caInput.getEditor().getEditorModel().isDirty())
                    caInput.getEditor().doSave(null);
                
                if (compInput != null && compInput.getEditor() != null
                        && compInput.getEditor().getEditorModel() != null
                        && compInput.getEditor().getEditorModel().isDirty())
                    compInput.getEditor().doSave(null);
            }
        } catch (Exception e) {
            WorkspacePlugin.LOG.error("Error in Auto Save", e);
        }
    }
    /**
     * Does nothing
     * @param action - IAction
     * @param selection - ISelection
     */
	public void selectionChanged(IAction action, ISelection selection)
	{
		
		
	}
}
