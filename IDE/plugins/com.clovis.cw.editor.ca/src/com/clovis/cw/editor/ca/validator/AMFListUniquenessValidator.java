package com.clovis.cw.editor.ca.validator;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Validator for Uniqueness of feature across the list.
 * @author Suraj Rajyaguru
 *
 */
public class AMFListUniquenessValidator extends ObjectValidator {
	
	/**
	 * Constructor
	 * @param featureNames
	 */
	public AMFListUniquenessValidator(Vector featureNames) {
		super(featureNames);
	}
	
	/**
	 * Creates Validator Instance
	 * @param featureNames Vector
	 * @return ObjectValidator
	 */
	public static ObjectValidator createValidator(Vector featureNames) {
		return new AMFListUniquenessValidator(featureNames);
	}
	
	/* (non-Javadoc)
	 * @see com.clovis.common.utils.ui.ObjectValidator#isValid(org.eclipse.emf.ecore.EObject, java.util.List, org.eclipse.emf.common.notify.Notification)
	 */
	public String isValid(EObject eObj, List eList, Notification n) {
		String message = null;
		message = checkPatternAndBlankValue(eObj);
		
		List amfList = NodeProfileDialog.getInstance().getViewModel().getEList();
		List amfConfigObjList = getAllAMFObjects(amfList);
		if (message == null) {
			message = isDuplicate(eObj, amfConfigObjList, n);
		}
		if (message == null) {
			message = isNonBlankFeatureBlank(eObj);
		}
		return message;
	}
	/**
	 * 
	 * @param amfList - Amf Resource list
	 * @return the List of all AMF objects
	 */
	public static List getAllAMFObjects(List amfList)
	{
		
		List nodeInsts = ProjectDataModel.getNodeInstListFrmNodeProfile(amfList);
		List sgInsts = ProjectDataModel.getSGInstListFrmNodeProfile(amfList);
		List amfConfigObjList = new ArrayList();
		try {
			for (int i = 0; i < nodeInsts.size(); i++) {
				EObject node = (EObject) nodeInsts.get(i);
				amfConfigObjList.add(node);
				EReference serviceUnitInstsRef = (EReference) node.eClass()
						.getEStructuralFeature(
								SafConstants.SERVICEUNIT_INSTANCES_NAME);
				EObject serviceUnitInstsObj = (EObject) node
						.eGet(serviceUnitInstsRef);
				if (serviceUnitInstsObj != null) {
					EReference serviceUnitInstRef = (EReference) serviceUnitInstsObj
							.eClass().getEStructuralFeature(
									SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
					List suObjs = (List) serviceUnitInstsObj
							.eGet(serviceUnitInstRef);

					for (int j = 0; j < suObjs.size(); j++) {
						EObject su = (EObject) suObjs.get(j);
						amfConfigObjList.add(su);
						
						EReference componentInstsRef = (EReference) su.eClass()
								.getEStructuralFeature(
										SafConstants.COMPONENT_INSTANCES_NAME);
						EObject componentInstsObj = (EObject) su
								.eGet(componentInstsRef);
						if (componentInstsObj != null) {
							EReference componentInstRef = (EReference) componentInstsObj
									.eClass()
									.getEStructuralFeature(
											SafConstants.COMPONENT_INSTANCELIST_NAME);
							List compObjs = (List) componentInstsObj
									.eGet(componentInstRef);
							for (int k = 0; k < compObjs.size(); k++) {
								EObject comp = (EObject) compObjs.get(k);
								amfConfigObjList.add(comp);
							}
						}
					}
				}
			}
		} catch (Exception e) {
			System.out.println(e.getMessage());
		}

		try {
			for (int i = 0; i < sgInsts.size(); i++) {
				EObject sg = (EObject) sgInsts.get(i);
				amfConfigObjList.add(sg);
				EReference siInstsRef = (EReference) sg.eClass()
						.getEStructuralFeature(
								SafConstants.SERVICE_INSTANCES_NAME);
				EObject serviceInstanceInstsObj = (EObject) sg.eGet(siInstsRef);

				if (serviceInstanceInstsObj != null) {
					EReference siInstRef = (EReference) serviceInstanceInstsObj
							.eClass().getEStructuralFeature(
									SafConstants.SERVICE_INSTANCELIST_NAME);
					List siObjs = (List) serviceInstanceInstsObj
							.eGet(siInstRef);

					for (int j = 0; j < siObjs.size(); j++) {
						EObject si = (EObject) siObjs.get(j);
						amfConfigObjList.add(si);
						EReference csiInstsRef = (EReference) si.eClass()
								.getEStructuralFeature(
										SafConstants.CSI_INSTANCES_NAME);
						EObject csiInstsObj = (EObject) si.eGet(csiInstsRef);

						if (csiInstsObj != null) {

							EReference csiInstRef = (EReference) csiInstsObj
									.eClass().getEStructuralFeature(
											SafConstants.CSI_INSTANCELIST_NAME);
							List csiObjs = (List) csiInstsObj.eGet(csiInstRef);
							for (int k = 0; k < csiObjs.size(); k++) {
								EObject csi = (EObject) csiObjs.get(k);
								amfConfigObjList.add(csi);
							}
						}
					}
				}
			}
		} catch (Exception e) {
			System.out.println(e.getMessage());
		}
		return amfConfigObjList;
	}
}
