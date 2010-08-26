/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/NodeCpmTypeValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ui.ObjectValidator;

/**
 *
 * @author shubhada
 *Validator to check for single CPM node.
 */
public class NodeCpmTypeValidator extends ObjectValidator
{
    /**
     *
     * @param featureNames Feature Names
     */
    public NodeCpmTypeValidator(Vector featureNames)
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
           message = checkCpmTypeDuplicateAcrossNodes(eobj);
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
	   message = checkCpmTypeDuplicateAcrossNodes(eobj);
	   if (message != null) {
	       errorList.add(message);
	   }
	   return errorList;
   }
   /**
    *
    * @param eobj EObject
    * @return Message
    */
   private String checkCpmTypeDuplicateAcrossNodes(EObject eobj)
   {
        /*int global = 0;
        List nodeList = NodeProfileDialog.getInstance().getNodeList();
        for (int i = 0; i < nodeList.size(); i++) {
            EObject obj = (EObject) nodeList.get(i);
            String nodeName = EcoreUtils.getName(obj);
        }
        if (global > 1) {
            return "Cannot have 2 Global Node Instances";
        }*/
        return null;
    }
    /**
     * Create Validator Instance.
     * @param featureNames Vector
     * @return Validator
     */
    public static ObjectValidator createValidator(Vector featureNames)
    {
        return new NodeCpmTypeValidator(featureNames);
    }
}
