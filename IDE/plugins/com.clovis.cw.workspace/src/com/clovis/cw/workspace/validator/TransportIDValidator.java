/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/validator/TransportIDValidator.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.validator;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.workspace.dialog.BootTime;

/**
 * 
 * @author pushparaj
 * Validator for Transport ID
 */
public class TransportIDValidator extends ObjectValidator{

	/**
	    *constructor
	    * @param featureNames - Feature Names
	    */
	   public TransportIDValidator(Vector featureNames)
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
	        return new TransportIDValidator(featureNames);
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
	           message = checkPortIDValidity(eobj);
	       }
	       if (message == null) {
	           message = checkPortIDUniqueness(eobj);
	       }
	       return message;
	   }
	   /**
	    * method to validate the Transport ID
	    * @param eobj - EObject
	    * @return the error message if any
	    */
	    private String checkPortIDValidity(EObject eobj)
	    {
	        long portid = ((Long)EcoreUtils.getValue(eobj, "transportId")).longValue();
	        if (portid <= 1023 || portid > 65535) {
	        	return "Transport ID cannot be <=1023 or > 65535";
	        }
	        return null;
	    }
		/**
		 * Validates the port id.
		 * 
		 * @param eObj
		 * @return
		 */
		private String checkPortIDUniqueness(EObject eObj) {
			Vector<Model> bootConfigVMList = (Vector<Model>) BootTime.getInstance().getBootConfigList().get(0);

			for(int i=0 ; i<bootConfigVMList.size() ; i++) {
				EObject obj = bootConfigVMList.get(i).getEObject();

				if(obj.eClass().getName().equals("GMS")) {
					 long transportId = Long.valueOf(EcoreUtils.getValue(eObj, "transportId").toString());
					 long multicastPort = Long.valueOf(EcoreUtils.getValue(obj, "MulticastPort").toString());

					 if(transportId == multicastPort) {
						 return "IOC transport Id can not have the smae value as GMS multicast port.";
					 }
				 }
			}
			return null;
		}
}
