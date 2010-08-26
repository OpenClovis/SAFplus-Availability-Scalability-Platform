/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/ReLoadProjectAction.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.action;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorReference;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;
import org.eclipse.ui.PlatformUI;

import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.ClassAssociationEditor;
import com.clovis.cw.editor.ca.ComponentEditor;
import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Action class for Loading all models in the project
 * @author pushparaj
 *
 */
public class ReLoadProjectAction extends CommonMenuAction implements
IViewActionDelegate, IWorkbenchWindowActionDelegate, ICWProject {
	/**
     * Initializes the View.
     * @param view ViewPart
     */
    public void init(IViewPart view){
    }

	public void dispose() {
		
	}

	public void init(IWorkbenchWindow window) {
	
	}
	public void run(IAction action)
	{
		try {
			List <GenericEditor>editorsList = new ArrayList<GenericEditor>(); 
			closeClovisEditors(_project, editorsList);
			ProjectDataModel.getProjectDataModel(_project)
					.removeDependencyListeners();
			ProjectDataModel
					.removeProjectDataModel(_project);
			for(int i = 0; i < editorsList.size(); i++) {
				GenericEditor input = editorsList.get(i);
				if(input instanceof ClassAssociationEditor) {
					OpenResourceEditorAction.openResourceEditor(_project);
				}else if(input instanceof ComponentEditor) {
					OpenComponentEditorAction.openComponentEditor(_project);
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	/**
	 * This will close all clovis editors for
	 * respective projects
	 * @project Project obj
	 */
	private void closeClovisEditors(IProject project, List<GenericEditor> list) {
		try {
			Display.getDefault().syncExec(new CloseEditorThread(project, list));
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
	/**
	 * Thead to close editors.
	 * @author pushparaj
	 *
	 */
	class CloseEditorThread implements Runnable {
		IProject project;
		List<GenericEditor> list;
		public CloseEditorThread(IProject project, List<GenericEditor> list) {
			this.project = project;
			this.list = list;
		}
		public void run() {
			IWorkbenchPage page = PlatformUI.getWorkbench()
					.getActiveWorkbenchWindow().getActivePage();
			IEditorReference refs[] = page.getEditorReferences();

			for (int j = 0; j < refs.length; j++) {
				try {
					IEditorInput input = refs[j].getEditorInput();
					if (input instanceof GenericEditorInput) {
						if (project == ((GenericEditorInput) input)
								.getResource().getProject()) {
							GenericEditor editor = ((GenericEditorInput) input)
									.getEditor();
							if (editor.getEditorModel().isDirty()) {
								if (!MessageDialog
										.openConfirm(_shell, editor.getTitle(),
												"Do you want to load the file system file and overrite all the unsaved datas.")) {
									continue;
								}
							}
							list.add(editor);
							IEditorReference ref[] = { refs[j] };
							page.closeEditors(ref, false);
						}
					}
				} catch (Exception e) {
					e.printStackTrace();
				}
			}
		}
		
	}
}
