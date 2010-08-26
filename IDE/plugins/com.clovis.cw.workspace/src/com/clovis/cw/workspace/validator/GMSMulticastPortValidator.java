/**
 * 
 */
package com.clovis.cw.workspace.validator;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.workspace.dialog.BootTime;

/**
 * Validator for Port Id.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class GMSMulticastPortValidator extends ObjectValidator {

	/**
	 * Consturctor.
	 * 
	 * @param featureNames
	 */
	public GMSMulticastPortValidator(Vector featureNames) {
		super(featureNames);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see com.clovis.common.utils.ui.ObjectValidator#isValid(org.eclipse.emf.ecore.EObject,
	 *      java.util.List, org.eclipse.emf.common.notify.Notification)
	 */
	public String isValid(EObject eObj, List eList, Notification n) {
		String message = super.isValid(eObj, eList, n);
		if (message == null) {
			message = checkPortUniqueness(eObj);
		}
		return message;
	}

	/**
	 * Validates the port id.
	 * 
	 * @param eObj
	 * @return
	 */
	private String checkPortUniqueness(EObject eObj) {
		Vector<Model> bootConfigVMList = (Vector<Model>) BootTime.getInstance().getBootConfigList().get(0);

		for(int i=0 ; i<bootConfigVMList.size() ; i++) {
			EObject obj = bootConfigVMList.get(i).getEObject();

			if(obj.eClass().getName().equals("IOC")) {
				 EObject transportObj = (EObject) EcoreUtils.getValue(obj, "transport");

				 long transportId = Long.valueOf(EcoreUtils.getValue(transportObj, "transportId").toString());
				 long multicastPort = Long.valueOf(EcoreUtils.getValue(eObj, "MulticastPort").toString());

				 if(transportId == multicastPort) {
					 return "GMS multicast port can not have the smae value as IOC transport Id.";
				 }
			 }
		}
		return null;
	}

	/**
	 * Create Validator Instance.
	 * 
	 * @param featureNames
	 *            Vector
	 * @return Validator
	 */
	public static ObjectValidator createValidator(Vector featureNames) {
		return new GMSMulticastPortValidator(featureNames);
	}
}
