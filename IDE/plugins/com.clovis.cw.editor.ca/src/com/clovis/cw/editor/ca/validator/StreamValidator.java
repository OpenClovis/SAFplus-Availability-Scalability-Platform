/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/validator/StreamValidator.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.validator;

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
 * Stream Validator to validate stream object related
 * validations
 */
public class StreamValidator extends ObjectValidator
{
    /**
     *constructor
     * @param featureNames - Feature Names
     */
    public StreamValidator(Vector featureNames)
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
         return new StreamValidator(featureNames);
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
            message = validateFileSize(eList);
        }
        if (message == null) {
            message = validateFlushData(eobj);
        }
        return message;
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
  	   message = validateFileSize(eList);
  	   if (message != null) {
  	       errorList.add(message);
  	   }
  	   message = validateFlushData(eobj);
  	   if (message != null) {
  	       errorList.add(message);
  	   }
  	   return errorList;
    }
    /**
     * 
     * @param eList - list of eobjects
     * @return the Error message if any
     */
    private String validateFileSize(List eList)
    {
        for (int i = 0; i < eList.size(); i++) {
            EObject eobj = (EObject) eList.get(i);
            long logFileSize = ((Long) EcoreUtils.getValue(eobj,
                    "fileUnitSize")).longValue();
            long logFileRecordSize = ((Long) EcoreUtils.getValue(eobj,
            "recordSize")).longValue();
            if (logFileSize < logFileRecordSize) {
                return 
                    "'File Unit Size' should not be less than 'Record Size'"; 
            }
        }
        return null;
    }

    /**
	 * Validates the flush freq and flush interval.
	 * 
	 * @param eobj
	 * @return
	 */
    private String validateFlushData(EObject eobj) {
        long flushFreq = ((Long) EcoreUtils.getValue(eobj, "flushFreq"))
				.longValue();
		long flushInterval = ((Long) EcoreUtils.getValue(eobj, "flushInterval"))
				.longValue();

		if (flushFreq == 0 && flushInterval == 0) {
			return "Both 'Flush Frequency' and 'Flush Interval' should not be 0";
		}
		return null;
	}
}
