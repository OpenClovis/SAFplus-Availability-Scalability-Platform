/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/validator/SequenceTableValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.validator;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;

/**
 * 
 * @author shubhada
 *
 * Validator class to validate the FM local sequence table.
 */
public class SequenceTableValidator extends ObjectValidator
{
    /**
    *constructor
    * @param featureNames - Feature Names
    */
   public SequenceTableValidator(Vector featureNames)
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
        return new SequenceTableValidator(featureNames);
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
           message = checkSequenceTableValidity(eobj, eList);
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
 	   message = checkSequenceTableValidity(eobj, eList);
 	   if (message != null) {
 	       errorList.add(message);
 	   }
 	   return errorList;
   }
   /**
    * method to validate the FM local sequence table
    * for unique combination of category, severity, escalation level.
    * @param eobj - EObject
    * @param list - EList
    * @return the error message if any
    */
    private String checkSequenceTableValidity(EObject eobj, List list)
    {
        ArrayList tableList = new ArrayList();
		String str = EcoreUtils.getValue(eobj, "Category").toString()
				+ EcoreUtils.getValue(eobj, "Severity").toString()
				+ EcoreUtils.getValue(eobj, "EscalationLevel").toString();
		tableList.add(str);
		for (int i = 0; i < list.size(); i++) {
			EObject sequenceTableEntry = (EObject) list.get(i);
			if (!sequenceTableEntry.equals(eobj)) {
				str = EcoreUtils.getValue(sequenceTableEntry, "Category")
						.toString()
						+ EcoreUtils.getValue(sequenceTableEntry, "Severity")
								.toString()
						+ EcoreUtils.getValue(sequenceTableEntry,
								"EscalationLevel").toString();
				if(tableList.contains(str)) {
					return "Two sequence table entries cannot have same "
	                   + "category, severity, escalationLevel";
				} else {
					tableList.add(str);
				}
				/*if(!tableList.add(str)) {
					return "Two sequence table entries cannot have same "
	                   + "category, severity, escalationLevel";
				}*/
			}
		}
        return null;
    }
}
