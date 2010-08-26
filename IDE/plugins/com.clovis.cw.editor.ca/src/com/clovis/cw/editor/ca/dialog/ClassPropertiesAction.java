/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ClassPropertiesAction.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Shell;

import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.cw.editor.ca.editpart.ClassEditPart;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.project.data.DependencyListener;

/**
 * @author shubhada
 * 
 * Action Class for Class Properties. Opens main propertyu dialog for Class.
 */
public class ClassPropertiesAction extends IActionClassAdapter {
	/**
	 * Visible only for single selection
	 * 
	 * @param environment
	 *            Environment
	 * @return true for single selection.
	 */
	public boolean isVisible(Environment environment) {
		boolean retValue = false;
		StructuredSelection selection = (StructuredSelection) environment
				.getValue("selection");

		if (selection.size() == 1
				&& selection.getFirstElement() instanceof ClassEditPart) {
			NodeModel nodeModel = (NodeModel) ((ClassEditPart) selection
					.getFirstElement()).getModel();
			if (!ClassEditorConstants.CHASSIS_RESOURCE_NAME.equals(nodeModel.getEObject().eClass()
					.getName())) {
				retValue = true;
			}
		}
		return retValue;
	}

	/**
	 * Method to open properties dialog.
	 * 
	 * @param args
	 *            0 - EObject for Method from Selection
	 * @return whether action is successfull.
	 */
	public boolean run(Object[] args) {
		StructuredSelection selection = (StructuredSelection) args[1];
		if (selection.getFirstElement() instanceof ClassEditPart) {
			ClassEditPart cep = (ClassEditPart) selection.getFirstElement();
			NodeModel nodeModel = (NodeModel) cep.getModel();
			IProject project = nodeModel.getRootModel().getProject();
			EObject classObj = nodeModel.getEObject();
			if (!ClassEditorConstants.CHASSIS_RESOURCE_NAME.equals(classObj
					.eClass().getName())) {
				PreferenceManager pmanager = new PreferenceManager();
				ClassPropertiesDialog pDialog = new ClassPropertiesDialog(
						(Shell) args[0], pmanager, classObj, project);

				Model viewModel = pDialog.getViewModel();
				DependencyListener listener = new DependencyListener(
						DependencyListener.VIEWMODEL_OBJECT);
				EcoreUtils.addListener(viewModel.getEObject(), listener, -1);

				pDialog.open();
				EcoreUtils.removeListener(viewModel.getEObject(), listener, -1);
			}
		}
		return true;
	}
}
