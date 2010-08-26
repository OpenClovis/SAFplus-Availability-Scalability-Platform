/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/validator/DataMemberValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.validator;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;


public class DataMemberValidator  extends ObjectValidator {

	/**
     *constructor
     * @param featureNames - Feature Names
     */
    public DataMemberValidator(Vector featureNames)
    {
        super(featureNames);
    }
     /**
      * Create Validator Instance.
      * @param featureNames Vector
      * @return Validator
      */
     public static ObjectValidator createValidator(Vector featureNames)
     {
         return new DataMemberValidator(featureNames);
     }
     /**
     *
     * @param eobj EObject
     * @param eList EList
     * @return message
     */
    public String isValid(EObject eobj, List eList, Notification n)
    {
        String message = super.isValid(eobj, eList, n);
        if (message == null) {
            message = checkPointerTypeForArray(eList);
        }
        return message;
    }
    /**
     * @param eobj - EObject to be validated
     * @param eList - List of EObjects
     */
    public List getAllErrors(EObject eobj, List eList)
    {
 		
 	   List errorList =  super.getAllErrors(eobj, eList);
 	   String message = null;
 	   message = checkPointerTypeForArray(eList);
 	   if (message != null) {
 	       errorList.add(message);
 	   }
 	   return errorList;
    }
    /**
     * Method to check whether the argument is a pointer if
     * argument type is "CL_OUT"
     * @param eobj - Object to be checked
     * @return the error message if any
     */
    private String checkPointerTypeForArray(List eList)
    {
        String msg = null;
        for (int i = 0; i < eList.size(); i++) {
            EObject eobj = (EObject) eList.get(i);            
            int multiplicity = ((Integer) EcoreUtils.getValue(eobj, "multiplicity")).intValue();
            String pointer = ((EEnumLiteral) EcoreUtils.getValue(eobj, "pointer")).getName();
            if ( multiplicity > 0 && pointer.equals("Single")) {
                return 
                "Pointer type cannot have multiplicity greater than 0";
            }
        }
        return msg;
    }
}
