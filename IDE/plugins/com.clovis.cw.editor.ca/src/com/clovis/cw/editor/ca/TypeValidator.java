/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/TypeValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


package com.clovis.cw.editor.ca;

import java.math.BigInteger;
import java.util.List;
import java.util.Vector;
import java.util.regex.Pattern;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
/**
 *
 * @author shubhada
 * Validator class to check the validity of Min, Max, Default
 * Values of an attribute.
 */
public class TypeValidator extends ObjectValidator
{
    /**
     *
     * @param featureNames Validation Features
     */
    public TypeValidator(Vector featureNames)
    {
        super(featureNames);
    }
    /**
     * @param eobj EObject
     * @param eList EList
     * @return Message
     */
    public String isValid(EObject eobj, List eList, Notification n)
    {
        String msg = super.isValid(eobj, eList, n);
        if (msg == null) {
            msg = checkForValidity(eobj);
        }
        if(msg == null) {
        	msg = checkForDependency(eobj);
        }
        return msg;
    }

    /**
	 * Checks for the internal dependency for the fields.
	 * 
	 * @param eobj
	 * @return
	 */
    private String checkForDependency(EObject eobj) {

        String attrType = EcoreUtils.getValue(eobj,
				ClassEditorConstants.ATTRIBUTE_ATTRIBUTETYPE).toString();
		if (attrType.equals("RUNTIME")) {
			EcoreUtils.setValue(eobj, ClassEditorConstants.ATTRIBUTE_INITIALIZED, "false");
			EcoreUtils.setValue(eobj, ClassEditorConstants.ATTRIBUTE_WRITABLE, "false");
			boolean isCached = Boolean.parseBoolean(EcoreUtils.getValue(eobj,
					ClassEditorConstants.ATTRIBUTE_CACHING).toString());
			if (!isCached) {

				boolean isPersistent = Boolean.parseBoolean(EcoreUtils
						.getValue(eobj,
								ClassEditorConstants.ATTRIBUTE_PERSISTENCY)
						.toString());
				if (isPersistent) {
					return "Caching should be enabled to enable persistency "
							+ "when Attribute type is RUNTIME.";
				}
			}

		} else if (attrType.equals("CONFIG")) {
			
			boolean isWritable = Boolean.parseBoolean(EcoreUtils.getValue(eobj,
					ClassEditorConstants.ATTRIBUTE_WRITABLE).toString());
			boolean isInitialized = Boolean.parseBoolean(EcoreUtils.getValue(eobj,
					ClassEditorConstants.ATTRIBUTE_INITIALIZED).toString());

			if(!isWritable && !isInitialized) {
				return "Non-Initialized and Non-Writable is not supported "
				+ "when Attribute type is CONFIG.";
			}
			
			boolean isCached = Boolean.parseBoolean(EcoreUtils.getValue(eobj,
					ClassEditorConstants.ATTRIBUTE_CACHING).toString());
			boolean isPersistent = Boolean.parseBoolean(EcoreUtils
					.getValue(eobj,
							ClassEditorConstants.ATTRIBUTE_PERSISTENCY)
					.toString());
			if(!isCached || !isPersistent){
				return "Caching and Persistency should be enabled "
				+ "when Attribute type is CONFIG.";
			}
		}
		return null;
	}
	/**
	 * @param eobj -
	 *            EObject to be validated
	 * @param eList -
	 *            List of EObjects
	 */
    public List getAllErrors(EObject eobj, List eList)
    {
 		
 	   List errorList =  super.getAllErrors(eobj, eList);
 	   String message = null;
 	   message = checkForValidity(eobj);
 	   if (message != null) {
 	       errorList.add(message);
 	   }
 	   message = checkForDependency(eobj);
 	   if (message != null) {
 	       errorList.add(message);
 	   }
 	   return errorList;
    }
    /**
     *
     * @param eobj EObject
     * @return the Message
     */
    private String checkForValidity(EObject eobj)
    {
        String msg = null;
        if (eobj.eClass().getName().equals("Attribute")
				|| eobj.eClass().getName().equals("ProvAttribute")
				|| eobj.eClass().getName().equals("PMAttribute")) {
            EEnumLiteral attrTypeLiteral = (EEnumLiteral) EcoreUtils.
            getValue(eobj, ClassEditorConstants.ATTRIBUTE_TYPE);
            BigInteger minVal = (BigInteger) EcoreUtils.getValue(eobj,
                    ClassEditorConstants.ATTRIBUTE_MIN_VALUE);
            BigInteger maxVal = (BigInteger) EcoreUtils.getValue(eobj,
                    ClassEditorConstants.ATTRIBUTE_MAX_VALUE);            
            
            BigInteger defaultVal = (BigInteger) EcoreUtils.getValue(eobj,
                    ClassEditorConstants.ATTRIBUTE_DEFAULT_VALUE);
            String pattern =
                EcoreUtils.getAnnotationVal(attrTypeLiteral, null, "pattern");
            msg =
                EcoreUtils.getAnnotationVal(attrTypeLiteral, null, "message");
            if (pattern != null) {
                if (!Pattern.compile(pattern).
                          matcher(String.valueOf(minVal)).matches()) {
                    msg = "Minimum " + msg;
                    return msg;
                } else if (!Pattern.compile(pattern).
                          matcher(String.valueOf(maxVal)).matches()) {
                    msg = "Maximum " + msg;
                    return msg;
                } else if (!Pattern.compile(pattern).
                          matcher(String.valueOf(defaultVal)).matches()) {
                    msg = "Default " + msg;
                    return msg;
                } else {
                    msg = null;
                }
            }
            if (minVal.compareTo(maxVal) > 0) {
                msg = "MinValue should be less than Max value";
                return msg;
            } else if (defaultVal.compareTo(minVal) < 0){
                msg = "DefaultValue should not be less than Min value";
                return msg;
            } else if (defaultVal.compareTo(maxVal) > 0) {
                msg = "DefaultValue should be less than Max value";
                return msg;
            }
        }
        return msg;
    }
    /**
     * Create Validator Instance.
     * @param featureNames Vector
     * @return Validator
     */
    public static ObjectValidator createValidator(Vector featureNames)
    {
        return new TypeValidator(featureNames);
    }
}
