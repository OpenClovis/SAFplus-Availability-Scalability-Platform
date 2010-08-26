/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ComponentPropertiesAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.menu.IActionClassAdapter;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
import com.clovis.cw.editor.ca.ComponentEditor;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.editparts.AbstractComponentNodeEditPart;
import com.clovis.cw.genericeditor.editparts.BaseEditPart;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.project.data.DependencyListener;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.data.SubModelMapReader;

/**
 * @author pushparaj
 * 
 * This is the action class for Component property
 */
public class ComponentPropertiesAction extends IActionClassAdapter {
	private Environment _environment;
	/**
	 * Visible only for single selection
	 * 
	 * @param environment
	 *            Environment for action
	 * @return true for single selection.
	 */
	public boolean isVisible(Environment environment) {
		_environment = environment;
		StructuredSelection selection = (StructuredSelection) environment
				.getValue("selection");
		return (selection.size() == 1)
				&& (selection.getFirstElement() instanceof AbstractComponentNodeEditPart);
	}

	/**
	 * Method to open properties dialog.
	 * 
	 * @param args
	 *            array of EObjects
	 * @return boolean
	 */
	public boolean run(Object[] args) {
		EObject eObj;
		StructuredSelection selection = (StructuredSelection) args[1];
		BaseEditPart cep = (BaseEditPart) selection.getFirstElement();

		NodeModel model = (NodeModel) cep.getModel();
		eObj = model.getEObject();
		Model viewModel = null;
		TitleAreaDialog dialog = null;
		if(eObj.eClass().getName().equals(ComponentEditorConstants.NONSAFCOMPONENT_NAME)){
			dialog = new NonSAFComponentPropertiesDialog((Shell) args[0], eObj, isProxied(model));
			viewModel = ((NonSAFComponentPropertiesDialog)dialog).getViewModel();
		} else if(eObj.eClass().getName().equals(ComponentEditorConstants.SAFCOMPONENT_NAME)){
			dialog = new SAFComponentPropertiesPushButtonDialog((Shell) args[0], eObj.eClass(), eObj);
			viewModel = ((PushButtonDialog)dialog).getViewModel();
		} else {
			dialog = new PushButtonDialog((Shell) args[0], eObj.eClass(), eObj);
			viewModel = ((PushButtonDialog)dialog).getViewModel();
		}
        DependencyListener listener = new DependencyListener(
        		DependencyListener.VIEWMODEL_OBJECT);
        EcoreUtils.addListener(viewModel.getEObject(), listener, 1);
		dialog.open();
        EcoreUtils.removeListener(viewModel.getEObject(), listener, 1);
		return true;
	}
	private boolean isProxied(NodeModel node)
	{
		Vector conns = node.getTargetConnections();
		for (int i = 0; i < conns.size(); i++)
		{
			EdgeModel edge = (EdgeModel) conns.get(i);
			EObject obj = edge.getEObject();
			String type = (String)EcoreUtils.getValue(obj, ComponentEditorConstants.CONNECTION_TYPE);
			if(type.equals(ComponentEditorConstants.PROXY_PROXIED_NAME)) {
				return true;
			}
		}	
		return false;
	}
	
	class SAFComponentPropertiesPushButtonDialog extends PushButtonDialog {

		private List<String> _componentNames;

		public SAFComponentPropertiesPushButtonDialog(Shell shell, EClass class1,
				Object value) {
			super(shell, class1, value);

			List<EObject> componentList = (List<EObject>) EcoreUtils.getValue(
					((EObject) value).eContainer(),
					ComponentEditorConstants.SAFCOMPONENT_REF_NAME);
			_componentNames = ClovisUtils.getFeatureValueList(componentList,
					"name");
			_componentNames
					.remove(EcoreUtils.getValue((EObject) value, "name"));
		}
		/**
		 * @see com.clovis.common.utils.ui.dialog.PushButtonDialog#okPressed()
		 */
		protected void okPressed()
	    {
			EObject obj = getViewModel().getEObject();
			boolean isSNMP = Boolean.parseBoolean(EcoreUtils.getValue(obj, "isSNMPSubAgent").toString());
			if (isSNMP) {
				IProject project = (IProject) _environment.getValue("project");
				ProjectDataModel pdm = ProjectDataModel
						.getProjectDataModel(project);
				GenericEditorInput geInput = (GenericEditorInput) pdm
						.getComponentEditorInput();
				ComponentEditor editor = (ComponentEditor) geInput.getEditor();
				EObject mapObj = editor.getLinkViewModel().getEObject();
				EObject linkObj = SubModelMapReader.getLinkObject(mapObj,
						ComponentEditorConstants.ASSOCIATE_RESOURCES_NAME);
				Model linkModel = new Model(editor.getLinkViewModel()
						.getResource(), linkObj);
				String rdn = (String) EcoreUtils.getValue(obj,
						ModelConstants.RDN_FEATURE_NAME);
				NotifyingList associatedResourcesList = (NotifyingList) SubModelMapReader
						.getLinkTargetObjects(linkModel.getEObject(), rdn);
				if (associatedResourcesList != null
						&& associatedResourcesList.size() != 0) {
					MessageDialog.openError(getShell(), "Invalid SNMP Sub Agent", "Component cannot be a sub agent if the component manage resources.");
					return;
				}
			}

			if (_componentNames.contains(EcoreUtils.getValue(obj, "name")
					.toString())) {

				MessageDialog.openError(getShell(),
						"Duplicate SAF Component Name",
						"Two or more components can not have same name '"
								+ EcoreUtils.getValue(obj, "name").toString()
								+ "'. Try using a different name.");
				return;
			}

			super.okPressed();
	    }
	}
}
