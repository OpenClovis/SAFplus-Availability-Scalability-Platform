package com.clovis.cw.editor.ca.validator;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;

public class SAFHealthCheckValidator extends ObjectValidator {

	public SAFHealthCheckValidator(Vector featureNames) {
		super(featureNames);
	}
	/**
	 * @param featureNames Vector
	 * @return ObjectValidator
	 */
	public static ObjectValidator createValidator(Vector featureNames) {
		return new SAFHealthCheckValidator(featureNames);
	}
	/**
	 * @see com.clovis.common.utils.ui.ObjectValidator#isValid(org.eclipse.emf.ecore.EObject, java.util.List, org.eclipse.emf.common.notify.Notification)
	 */
	public String isValid(EObject obj, List eList, Notification n) {
		long period = Long.parseLong(EcoreUtils.getValue(obj, "period").toString());
		long duration = Long.parseLong(EcoreUtils.getValue(obj, "maxDuration").toString());
		if(duration < (2 * period)) {
			return "Maximum duration should be >= 2 * Period";
		}
		
		return null;
	}
}
