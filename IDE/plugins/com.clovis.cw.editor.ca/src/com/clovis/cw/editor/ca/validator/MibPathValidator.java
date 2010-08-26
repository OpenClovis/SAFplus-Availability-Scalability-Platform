/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/validator/MibPathValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.validator;

import java.io.File;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.editor.ca.constants.SnmpConstants;

/**
*
* @author shubhada
*Validator to check validity of mib path.
*/
public class MibPathValidator extends ObjectValidator
{
    /**
    *
    * @param featureNames Feature Names
    */
   public MibPathValidator(Vector featureNames)
   {
       super(featureNames);
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
          message = checkMibPathValidity(eobj);
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
	   message = checkMibPathValidity(eobj);
	   if (message != null) {
	       errorList.add(message);
	   }
	   return errorList;
  }
  /**
   * 
   * @param eobj - EObject
   * @return the Message if any else null
   */
  private String checkMibPathValidity(EObject eobj)
  {
      String mibPath = (String) EcoreUtils.getValue(eobj,
              SnmpConstants.MIB_LOCATION_FIELD);
      File pythonFile = new File(mibPath);
      if (!pythonFile.isDirectory()) {
          return "Specified mib file location is not a valid path";
      }
      return null;
  }
  /**
   * Create Validator Instance.
   * @param featureNames Vector
   * @return Validator
   */
  public static ObjectValidator createValidator(Vector featureNames)
  {
      return new MibPathValidator(featureNames);
  }
}
