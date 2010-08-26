/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/validator/SlotNumberValidator.java $
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
 * @author pushparaj
 * Validator for Manual Configuration
 */
public class SlotNumberValidator extends ObjectValidator{

	/**
	    *constructor
	    * @param featureNames - Feature Names
	    */
	   public SlotNumberValidator(Vector featureNames)
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
	        return new SlotNumberValidator(featureNames);
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
	           message = checkSlotNumber(eobj, eList);
	       }
	       return message;
	   }
	   /**
		 * Validate Slot Number, Interface Address
		 * 
		 * @param eobj
		 *            EObject
		 * @param eList
		 *            EList
		 * @return message
		 */
	private String checkSlotNumber(EObject eobj, List list) {
		ArrayList slotList = new ArrayList();
		ArrayList addressList = new ArrayList();
		slotList.add(String.valueOf(EcoreUtils.getValue(eobj, "slot")));
		String interfaceAddress = String.valueOf(EcoreUtils.getValue(eobj, "interfaceAddress"));
		if(interfaceAddress.equals("")) {
			   return "Interface Address cannot be blank";
		}
		addressList.add(interfaceAddress);
		for (int i = 0; i < list.size(); i++) {
			EObject tableEntry = (EObject) list.get(i);
			if (!tableEntry.equals(eobj)) {
				String slot = String.valueOf(EcoreUtils.getValue(tableEntry, "slot"));
				if (slotList.contains(slot)) {
					return "Duplicate Entry for Slot Number :  '" + slot + "'";
				}
				interfaceAddress = (String) EcoreUtils.getValue(tableEntry, "interfaceAddress");
				if (interfaceAddress.equals("")) {
					return "Interface Address cannot be blank";
				}
				if (addressList.contains(interfaceAddress)) {
					return "Duplicate Entry for Interface Address :  '" + interfaceAddress + "'";
				}
				slotList.add(slot);
				addressList.add(interfaceAddress);
			}
		}
		return null;
	}
}
