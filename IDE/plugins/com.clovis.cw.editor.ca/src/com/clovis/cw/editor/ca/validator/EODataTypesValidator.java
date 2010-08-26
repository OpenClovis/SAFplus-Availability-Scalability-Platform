package com.clovis.cw.editor.ca.validator;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;

/**
 * Validator for EO Data Types.
 * @author Suraj Rajyaguru
 *
 */
public class EODataTypesValidator extends ListUniquenessValidator {

	/**
	 * Constructor
	 * @param featureNames
	 */
	public EODataTypesValidator(Vector featureNames) {
		super(featureNames);
	}

	/**
	 * Creates Validator Instance
	 * @param featureNames Vector
	 * @return ObjectValidator
	 */
	public static ObjectValidator createValidator(Vector featureNames) {
		return new EODataTypesValidator(featureNames);
	}

	/* (non-Javadoc)
	 * @see com.clovis.common.utils.ui.ObjectValidator#isValid(org.eclipse.emf.ecore.EObject, java.util.List, org.eclipse.emf.common.notify.Notification)
	 */
	public String isValid(EObject eObj, List eList, Notification n) {
		String message =  super.isValid(eObj, eList, n);
		if(message == null) {
			message = validateIncludePath(eObj);
		}
		return message;
	}
	/**
	 * 
	 * @param eObj - EObject to be validated
	 * @return the Error Message if Error, else return null
	 */
	private String validateIncludePath(EObject eObj)
	{
		boolean defNeed = Boolean.parseBoolean(EcoreUtils.getValue(eObj,
			"defNeed").toString());
		if(!defNeed) {
			String include = EcoreUtils.getValue(eObj, "include").toString();
			if (include.equalsIgnoreCase("")
					|| include.equalsIgnoreCase(" ")) {
				return "Please specify header file path which contains the definition.";
				
			}
		}
		return null;
	}
	 /**
     * @param eobj - EObject to be validated
     * @param eList - List of EObjects
     */
    public List getAllErrors(EObject eobj, List eList)
    {
 		
 	   List errorList =  super.getAllErrors(eobj, eList);
 	   String message = null;
 	   message = validateIncludePath(eobj);
 	   if (message != null) {
 	       errorList.add(message);
 	   }
 	   return errorList;
    }
}
