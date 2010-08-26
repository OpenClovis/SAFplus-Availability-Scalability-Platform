package com.clovis.cw.editor.ca.validator;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.editor.ca.dialog.PreferenceUtils;

/**
 * Validator for Uniqueness of feature across the list.
 * @author Suraj Rajyaguru
 *
 */
public class ListUniquenessValidator extends ObjectValidator {
	
	/**
	 * Constructor
	 * @param featureNames
	 */
	public ListUniquenessValidator(Vector featureNames) {
		super(featureNames);
	}
	
	/**
	 * Creates Validator Instance
	 * @param featureNames Vector
	 * @return ObjectValidator
	 */
	public static ObjectValidator createValidator(Vector featureNames) {
		return new ListUniquenessValidator(featureNames);
	}
	
	/* (non-Javadoc)
	 * @see com.clovis.common.utils.ui.ObjectValidator#isValid(org.eclipse.emf.ecore.EObject, java.util.List, org.eclipse.emf.common.notify.Notification)
	 */
	public String isValid(EObject eObj, List eList, Notification n) {
		String message = null;
		message = checkPatternAndBlankValue(eObj);
		if (message == null) {
			message = isDuplicate(eObj, PreferenceUtils.getContainerEList(eObj), n);
		}
		if (message == null) {
			message = isNonBlankFeatureBlank(eObj);
		}
		return message;
	}
}
