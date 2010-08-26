/**
 * 
 */
package com.clovis.cw.editor.ca.validator;

import java.math.BigInteger;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;

/**
 * Validator for Memory Pool.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MemoryPoolValidator extends ObjectValidator {

	/**
	 * Consturctor.
	 * 
	 * @param featureNames
	 */
	public MemoryPoolValidator(Vector featureNames) {
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
			message = checkForSizeValidity(eObj);
		}
		return message;
	}

	/**
	 * Validates the Pool sizes.
	 * 
	 * @param eObj
	 * @return
	 */
	private String checkForSizeValidity(EObject eObj) {
		long maxSize = ((BigInteger) EcoreUtils.getValue(eObj, "maxSize"))
				.longValue();
		if(maxSize == 0) {
			return null;
		}

		long initialSize = ((BigInteger) EcoreUtils.getValue(eObj, "initialSize"))
				.longValue();

		if (initialSize >= maxSize) {
			return "Invalid Entry. Initial Size should be less than Maximum Size.";
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
		return new MemoryPoolValidator(featureNames);
	}
}
