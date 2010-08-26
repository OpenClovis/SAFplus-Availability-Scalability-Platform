package com.clovis.cw.editor.ca.validator;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;

public class ComponentPropertiesValidator extends ObjectValidator {

	public ComponentPropertiesValidator(Vector featureNames) {
		super(featureNames);
	}
	/**
	 * @param featureNames Vector
	 * @return ObjectValidator
	 */
	public static ObjectValidator createValidator(Vector featureNames) {
		return new ComponentPropertiesValidator(featureNames);
	}
	/**
	 * @see com.clovis.common.utils.ui.ObjectValidator#isValid(org.eclipse.emf.ecore.EObject, java.util.List, org.eclipse.emf.common.notify.Notification)
	 */
	public String isValid(EObject eObj, List eList, Notification n) {
		String restartable = String.valueOf(EcoreUtils.getValue(eObj, "isRestartable"));
		String recovery = String.valueOf(EcoreUtils.getValue(eObj, "recoveryOnTimeout"));
		if(restartable.equals("CL_FALSE") && recovery.equals("CL_AMS_RECOVERY_COMP_RESTART")) {
			return "Recovery Action on error should not be Component restart if is restartable is false";
		}
		boolean is3rdparty = Boolean.parseBoolean(EcoreUtils.getValue(eObj, "is3rdpartyComponent").toString());
		boolean isSNMP = Boolean.parseBoolean(EcoreUtils.getValue(eObj, "isSNMPSubAgent").toString());
		if(is3rdparty && isSNMP) {
			return "Third party component cannot be SNMP sub-agent";
		}

		String message = null;
		message = checkPatternAndBlankValue(eObj);

		if (message == null) {
			_featureNames
					.remove(ComponentEditorConstants.INSTANTIATION_COMMAND);
			message = isDuplicate(eObj, eList, n);
			_featureNames.add(ComponentEditorConstants.INSTANTIATION_COMMAND);
		}

		if (message == null) {
			message = isFieldCombinationDuplicate(eObj, eList, n);
		}

		if (message == null) {
			message = isNonBlankFeatureBlank(eObj);
		}

		return message;
	}
}
