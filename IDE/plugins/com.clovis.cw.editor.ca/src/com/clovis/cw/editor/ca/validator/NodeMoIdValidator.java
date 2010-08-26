package com.clovis.cw.editor.ca.validator;

import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Validates Node MoId and SC Node Instances
 * @author ravi
 *
 */
public class NodeMoIdValidator extends AMFListUniquenessValidator{
	
	/**
	 * Constructor
	 * @param featureNames
	 */
	public NodeMoIdValidator(Vector featureNames) {
		super(featureNames);
	}
	
	/**
	 * Creates Validator Instance
	 * @param featureNames Vector
	 * @return ObjectValidator
	 */
	public static ObjectValidator createValidator(Vector featureNames) {
		return new NodeMoIdValidator(featureNames);
	}
	
	/* (non-Javadoc)
	 * @see com.clovis.common.utils.ui.ObjectValidator#isValid(org.eclipse.emf.ecore.EObject, java.util.List, org.eclipse.emf.common.notify.Notification)
	 */
	public String isValid(EObject eObj, List eList, Notification n) {
		
		String message = super.isValid(eObj, eList, n);
		
		/*if (message == null) {
			message = MoInstanceIDValidator.validateConfiguredInst(eObj, "nodeMoId");
		}*/

		if(message == null) {
			message = validateNonASPNodeParams(eObj);
		}

		if (message == null) {
			IProject project = (IProject) NodeProfileDialog.getInstance()
			.getProject();
			ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
			EObject object = eObj.eContainer();
			EList list = (EList) EcoreUtils.getValue(object, "nodeInstance");
			String scName = getSystemControllerNodeName(pdm);
			int count = 0;
			for (int i = 0; i < list.size(); i++) {
				EObject node = (EObject) list.get(i);
				if(String.valueOf(EcoreUtils.getValue(node, "type")).equals(scName))
					count++;
				if(count > 2) {
					return "More than two node instances cannot be created for System Controller node";
				}
			}
		}
		return message;
	}
	
	/**
	 * Find System Controller Nodeif(_systemCtrNodeName == null) { Name 
	 * @return SC name
	 */
	private String getSystemControllerNodeName(ProjectDataModel pdm) {
		List nodeTypeList = ComponentDataUtils.getNodesList(pdm.getComponentModel().getEList());
		for (int i = 0; i < nodeTypeList.size(); i++) {
			EObject eobj = (EObject) nodeTypeList.get(i);
	        String classType = String.valueOf(EcoreUtils.getValue(eobj, "classType"));
	        if(classType.equals("CL_AMS_NODE_CLASS_B") || classType.equals("CL_AMS_NODE_CLASS_A")){
	            	return EcoreUtils.getName(eobj);
	        }
		}
		return null;
	}

	/**
	 * Validates Non ASP node fields.
	 * 
	 * @param nodeObj
	 * @return
	 */
	private String validateNonASPNodeParams(EObject nodeObj) {
		IProject project = (IProject) NodeProfileDialog.getInstance()
				.getProject();
		ComponentDataUtils cdu = new ComponentDataUtils(ProjectDataModel
				.getProjectDataModel(project).getComponentModel().getEList());
		EObject nodeType = cdu.getObjectFrmName(EcoreUtils.getValue(nodeObj,
				"type").toString());

		if (EcoreUtils.getValue(nodeType, "classType").toString().equals(
				"CL_AMS_NODE_CLASS_D")) {

			if (EcoreUtils.getValue(nodeObj, "chassisId") == null) {
				return "Chassis Id cannot be blank";
			}
			if (EcoreUtils.getValue(nodeObj, "slotId") == null) {
				return "Slot Id cannot be blank";
			}
		}
		return null;
	}
}
