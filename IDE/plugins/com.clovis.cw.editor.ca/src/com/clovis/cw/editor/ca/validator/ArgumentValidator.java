/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/validator/ArgumentValidator.java $
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
import com.clovis.cw.editor.ca.LengthVariableComboBoxCellEditor;

/**
 * 
 * @author shubhada
 *
 * Specific Argument Validator class
 */
public class ArgumentValidator extends ObjectValidator
{
    /**
     *constructor
     * @param featureNames - Feature Names
     */
    public ArgumentValidator(Vector featureNames)
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
         return new ArgumentValidator(featureNames);
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
            message = checkPointerTypeForOutArg(eList);
        }
        if (message == null) {
        	message = validateArgument(eList);
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
 	   message = checkPointerTypeForOutArg(eList);
 	   if (message != null) {
 	       errorList.add(message);
 	   }
 	   message = null;
 	   message = validateArgument(eList);
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
    private String checkPointerTypeForOutArg(List eList)
    {
        String msg = null;
        for (int i = 0; i < eList.size(); i++) {
            EObject eobj = (EObject) eList.get(i);
            String parameterType = EcoreUtils.getValue(eobj, "ParameterType").
                toString();
            String pointerType = ((EEnumLiteral) EcoreUtils.getValue(eobj, "pointer")).getName();
            if ((parameterType.equals("CL_OUT") || parameterType.equals("CL_INOUT"))
            		&& pointerType.equals(LengthVariableComboBoxCellEditor.NONE)) {
                return 
                "Argument should be a pointer when Parameter Type is 'CL_OUT' or 'CL_INOUT'";
            }
        }
        return msg;
    }
    /**
     * 
     * @param eList
     * @return
     */
    private String validateArgument(List eList)
    {
    	String msg = null;

        for (int i = 0; i < eList.size(); i++) 
        {
            EObject eobj = (EObject) eList.get(i);
            String paramType = EcoreUtils.getValue(eobj, "ParameterType").toString();
            String pointerType = EcoreUtils.getValue(eobj, "pointer").toString();
            String lengthVar = EcoreUtils.getValue(eobj, "lengthVar").toString();

            // Checks if length variable is CL_OUT then argument should be a double pointer
            EObject lengthVarObj = getObjectFrmName(eList, lengthVar);
            if (lengthVarObj != null)
            {
            	String objParamType = EcoreUtils.getValue(lengthVarObj, "ParameterType").toString();
            	String objLengthVar = EcoreUtils.getValue(lengthVarObj, "lengthVar").toString();
            	String objPointerType = EcoreUtils.getValue(lengthVarObj, "pointer").toString();

            	if (!objLengthVar.equals(LengthVariableComboBoxCellEditor.NONE)) {
            		return EcoreUtils.getName(eobj) + ": Length variable argument cannot have it's length variable defined";
            	}
            	if (objParamType.equals("CL_OUT")
            			&& (pointerType.equals("Single") || pointerType.equals(LengthVariableComboBoxCellEditor.NONE))) {
            		return EcoreUtils.getName(eobj) + ": If length variable is of type CL_OUT, then argument must be a double pointer";
            	} 
            	// CL_OUT lenth variable cannot be used for arguments of type CL_IN
            	if (paramType.equals("CL_IN")) {
            		if (objParamType.equals("CL_OUT")) {
                		return EcoreUtils.getName(eobj) + ": CL_OUT argument cannot be used as length variable for CL_IN argument";
            		} 
            		if (pointerType.equals("Single") &&
            				(objParamType.equals("CL_OUT")
            						|| objParamType.equals("CL_INOUT")) ) {
            			
            			return EcoreUtils.getName(eobj)
            			+ ": Single pointer argument of type CL_IN cannot have length variable of type CL_OUT or CL_INOUT";
            		}
            			
            	} 
            	//If argument type is CL_OUT and length variable is CL_INOUT,
            	//then argument must be a double pointer
            	if (paramType.equals("CL_OUT") 
            			&& objParamType.equals("CL_INOUT")
            			&& (pointerType.equals(LengthVariableComboBoxCellEditor.NONE) || pointerType.equals("Single"))) {
            		return EcoreUtils.getName(eobj) + ": If argument type is CL_OUT and length variable is CL_INOUT, then argument must be a double pointer";
            		
            	}
            	// if argument is a double pointer, its length variable should be single pointer
            	if (pointerType.equals("Double") && !objPointerType.equals("Single")) {
            		return EcoreUtils.getName(eobj) + ": If the argument is a double pointer, then length variable should be single pointer";
            	}
            } else {
            	// CL_OUT type Argument with double pointer should have length variable defined 
            	if (paramType.equals("CL_OUT") && pointerType.equals("Double") 
            			&& lengthVar.equals(LengthVariableComboBoxCellEditor.NONE)) {
            		return EcoreUtils.getName(eobj) + ": Double pointer, CL_OUT type argument should have length variable defined";
            	}

                // check that length variable is valid
            	if (!lengthVar.equals(LengthVariableComboBoxCellEditor.NONE))
    			{
    				return EcoreUtils.getName(eobj) + ": Length Variable is invalid.";
    			}
            }
            // CL_INOUT length var should not be mixed for CL_IN and CL_OUT buffers
            if (paramType.equals("CL_INOUT")) {
            	if (!isValidInOutParameter(eList, eobj)) {
            		return EcoreUtils.getName(eobj) + ": CL_INOUT type argument should not be used as length variable both of CL_IN and CL_OUT arguments";
            	}
            	//If the Argument is a CL_INOUT type pointer with length variable defined,
                // then flag error
            	if (pointerType.equals("Single")
            			&& !lengthVar.equals(LengthVariableComboBoxCellEditor.NONE)) {
            		return EcoreUtils.getName(eobj)
            		+ ": CL_INOUT type pointer argument should not have length variable defined";
            	}
            } // If argument is a double pointer of type CL_IN, then flag an error
            if ((paramType.equals("CL_IN") || paramType.equals("CL_INOUT"))
            		&& pointerType.equals("Double")) {
            	  return EcoreUtils.getName(eobj)
          		+ ": CL_IN or CL_INOUT type argument cannot be double pointer";
            }
        }
        return msg;
    }
    /**
     * Goes thru all the Editor Objects to find out the Object with the
     * given name and returns the Object
     * @param name - Name of the Object to be fetched
     * @param list Editor Object List
     * @return the Object corresponding to Name
     */
    public EObject getObjectFrmName(List list, String name)
    {
        for (int i = 0; i < list.size(); i++) {
            EObject obj = (EObject) list.get(i);
            String objName = EcoreUtils.getName(obj);
            if (objName != null && objName.equals(name)) {
                return obj;
            }
        }
        return null;
    }
    /**
     * 
     * @param modelList - List of arguments in the model
     * 
     * @param inOutLengthVar - The Argument whose parameter type
     * is CL_INOUT and which need to checked whether it can be a
     * valid length variable of any other argument
     * 
     * @return true, if inOutLengthVar is valid length variable
     * else return false
     */
    public static boolean isValidInOutParameter(List modelList,
    		EObject inOutLengthVar)
    {
    	boolean inUsed = false, outUsed = false;
    	String argName = EcoreUtils.getName(inOutLengthVar);
    	for (int i = 0; i < modelList.size(); i++) {
    		EObject modelObj = (EObject) modelList.get(i);
    		if (!modelObj.equals(inOutLengthVar)) {
    			String paramType = EcoreUtils.getValue(modelObj,
    					"ParameterType").toString();
    			String lengthVar = EcoreUtils.getValue(modelObj,
					"lengthVar").toString();
    			if (paramType.equals("CL_IN")
    					&& lengthVar.equals(argName)) {
    				inUsed = true;
    				
    			} else if (paramType.equals("CL_OUT")
    					&& lengthVar.equals(argName)) {
    				outUsed = true;
    			}
    			if (inUsed && outUsed) {
    				return false;
    			}
    		}
    	}
    	return true;
    }
}
