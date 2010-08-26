/**
 * 
 */
package com.clovis.cw.editor.ca.validator;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;

/**
 * Validator for Water Mark.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MemoryWaterMarkValidator extends ObjectValidator {

	/**
	 * Consturctor.
	 * 
	 * @param featureNames
	 */
	public MemoryWaterMarkValidator(Vector featureNames) {
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
			message = checkForLimitValidity(eObj);
		}
		return message;
	}

	/**
	 * Validates the water mark limits.
	 * 
	 * @param eObj
	 * @return
	 */
	private String checkForLimitValidity(EObject eObj) {
		int lowLimit = ((Integer) EcoreUtils.getValue(eObj, "lowLimit"))
				.intValue();
		int highLimit = ((Integer) EcoreUtils.getValue(eObj, "highLimit"))
				.intValue();

		if (lowLimit > 100 || highLimit > 100) {
			return "Invalid Entry. WaterMark Limit value should be less or equal 100.";
		}

		if (lowLimit >= highLimit) {
			return "Invalid Entry. Low Limit should be less than High Limit.";
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
		return new MemoryWaterMarkValidator(featureNames);
	}
}
