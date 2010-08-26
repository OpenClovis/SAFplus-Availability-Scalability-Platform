package com.clovis.cw.editor.ca.validator;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;

/**
 * Class for validate properties which are related to Redundancy Model
 * Type.
 * @author Pushparaj
 *
 */
public class RedundancyModelValidator extends ObjectValidator {
	
	/**
	 * Constructor
	 */
	public RedundancyModelValidator(Vector featureNames) {
		super(featureNames);
	}
	/**
	 * Creates Validator Instance
	 * @param featureNames Vector
	 * @return ObjectValidator
	 */
	public static ObjectValidator createValidator(Vector featureNames) {
		return new RedundancyModelValidator(featureNames);
	}
	/* (non-Javadoc)
	 * @see com.clovis.common.utils.ui.ObjectValidator#isValid(org.eclipse.emf.ecore.EObject, java.util.List, org.eclipse.emf.common.notify.Notification)
	 */
	public String isValid(EObject eObj, List eList, Notification n) {
		String message = null;
		if (eObj.eContainer() != null)
			return message;
		String redundancyModel = EcoreUtils.getValue(eObj,
				ComponentEditorConstants.SG_REDUNDANCY_MODEL).toString();
		if (redundancyModel
				.equals(ComponentEditorConstants.TWO_N_REDUNDANCY_MODEL)) {
			int prefInServSU = ((Integer) EcoreUtils.getValue(eObj,
					"numPrefInserviceSUs")).intValue();
			if (prefInServSU < 2) {
				return "For 2N Redundancy InService SUs should be >= 2";
			}
		} else if (redundancyModel
				.equals(ComponentEditorConstants.M_PLUS_N_REDUNDANCY_MODEL)) {
			int activeSUs = ((Integer) EcoreUtils.getValue(eObj,
					"numPrefActiveSUs")).intValue();
			int standBySUs = ((Integer) EcoreUtils.getValue(eObj,
					"numPrefStandbySUs")).intValue();
			int prefInServSU = ((Integer) EcoreUtils.getValue(eObj,
					"numPrefInserviceSUs")).intValue();
			if (activeSUs < 1) {
				return "For M+N Redundancy Active SUs should be > 0";
			}
			if (standBySUs < 1) {
				return "For M+N Redundancy Standby SUs should be > 0";
			}
			if (prefInServSU < (activeSUs + standBySUs)) {
				return "For M+N Redundancy InService SUs should be >= (Active SUs + Standby SUs)";
			}
		}
		return message;
	}
}
