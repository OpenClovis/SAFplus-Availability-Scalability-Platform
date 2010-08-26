/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.util.Iterator;

import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.menu.Environment;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Action class for creating resource model template.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class CreateResourceModelTemplateAction extends
		CreateModelTemplateAction {

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.cw.workspace.modelTemplate.CreateModelTemplateAction#isEnabled(com.clovis.common.utils.menu.Environment)
	 */
	@Override
	public boolean isEnabled(Environment environment) {
		EObject selObj;
		Object obj;

		Iterator itr = _selection.iterator();
		while (itr.hasNext()) {

			obj = itr.next();
			if (obj instanceof BaseEditPart) {

				BaseEditPart ep = (BaseEditPart) obj;
				selObj = ((NodeModel) ep.getModel()).getEObject();

				if (selObj.eClass().getName().equals(
						ClassEditorConstants.CHASSIS_RESOURCE_NAME)
						|| selObj.eClass().getName().equals(
								ClassEditorConstants.MIB_CLASS_NAME)
						|| selObj.eClass().getName().equals(
								ClassEditorConstants.MIB_RESOURCE_NAME)) {
					return false;
				}
			} else {
				return false;
			}
		}

		return true;
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
				.getCAEditorInput();

		if (geInput.getEditor().isDirty()) {
			MessageDialog
					.openWarning(shell, "Editor Dirty",
							"Save the editor to avoid conflicts in model template creation.");

		} else {
			CreateModelTemplateDialog dialog = new CreateResourceModelTemplateDialog(
					shell, _project, _selection, MODEL_TYPE_RESOURCE);
			dialog.open();
		}

		return true;
	}
}
