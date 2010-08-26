/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.util.Iterator;

import org.eclipse.core.resources.IProject;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.menu.Environment;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.editparts.BaseDiagramEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Action class for creating component model template.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class CreateComponentModelTemplateAction extends
		CreateModelTemplateAction {

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.CreateModelTemplateAction#isVisible(com.clovis.common.utils.menu.Environment)
	 */
	@Override
	public boolean isVisible(Environment environment) {
		_project = (IProject) environment.getValue("project");
		_selection = (StructuredSelection) environment.getValue("selection");

		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.CreateModelTemplateAction#isEnabled(com.clovis.common.utils.menu.Environment)
	 */
	@SuppressWarnings("unchecked")
	@Override
	public boolean isEnabled(Environment environment) {
		if (_selection.size() > 0) {
			Iterator<Object> itr = _selection.iterator();

			while (itr.hasNext()) {
				Object obj = itr.next();

				if (obj instanceof BaseDiagramEditPart) {
					return false;
				} else if (obj instanceof BaseEditPart) {
					return true;
				}
			}
		}

		return false;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.CreateModelTemplateAction#run(java.lang.Object[])
	 */
	@Override
	public boolean run(Object[] args) {
		Shell shell = (Shell) args[0];
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		GenericEditorInput geInput = (GenericEditorInput) pdm
				.getComponentEditorInput();

		if (geInput.getEditor().isDirty()) {
			MessageDialog
					.openWarning(shell, "Editor Dirty",
							"Save the editor to avoid conflicts in model template creation.");

		} else {
			CreateModelTemplateDialog dialog = new CreateComponentModelTemplateDialog(
					(Shell) args[0], _project, _selection, MODEL_TYPE_COMPONENT);
			dialog.open();
		}

		return true;
	}
}
