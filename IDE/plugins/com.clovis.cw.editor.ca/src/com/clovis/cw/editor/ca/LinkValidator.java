/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/LinkValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.List;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;

/**
 *
 * @author shubhada
 *Validator to check for single CPM node.
 */
public class LinkValidator extends ObjectValidator
{
    /**
     *
     * @param featureNames Feature Names
     */
    public LinkValidator(Vector featureNames)
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
           message = checkForLinkValidity(eobj, eList);
       }
       if (message == null) {
           message = checkForPortNoValidity(eobj);
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
	   message = checkForLinkValidity(eobj, eList);
	   if (message != null) {
	       errorList.add(message);
	   }
	   message = null;
	   message = checkForPortNoValidity(eobj);
	   if (message != null) {
	       errorList.add(message);
	   }
	   return errorList;
   }
/**
    *
    * @param eobj EObject
    * @return the Message if Port No is not Valid else return null
    */
   private String checkForPortNoValidity(EObject eobj)
   {
       String address = EcoreUtils.getValue(eobj, "linkAddress").toString();
       String broadcastaddr = EcoreUtils.getValue(eobj, "linkBroadcastAddress").
           toString();
       int index = address.indexOf(':');
       String addrPortNo = address.substring(index + 1);
       index = broadcastaddr.indexOf(':');
       String broadcastPortNo = broadcastaddr.substring(index + 1);
       if (!broadcastPortNo.equals(addrPortNo)) {
           return "UDP Port No of Link Address and BroadCast"
           + "Address should be same";
       }
       return null;
    }
/**
    *
    * @param eobj EObject
    * @param eList List of Model Objects
    * @return Message
    */
   private String checkForLinkValidity(EObject eobj, List eList)
   {
       String address = EcoreUtils.getValue(eobj,
				"linkAddress").toString();
       if (address.trim().equals("") || address.indexOf(":") == -1) {
		   return null;
	   }
       StringTokenizer tokenizer = new StringTokenizer(address, ":");
       address = tokenizer.nextToken();
       String portNo = tokenizer.nextToken();
       for (int i = 0; i < eList.size(); i++) {
           EObject modelObj = (EObject) eList.get(i);
           if (modelObj != eobj) {
           String maddress = EcoreUtils.getValue(
						modelObj, "linkAddress").toString();
		   if (maddress.trim().equals("") || maddress.indexOf(":") == -1) {
			   continue;
		   }
           StringTokenizer tokeniser = new StringTokenizer(maddress, ":");
		   maddress = tokeniser.nextToken();
           String mportNo = tokeniser.nextToken();
           if (maddress != null && maddress.equals(address)
                   && mportNo.equals(portNo)) {
               
                   return "Two Links cannot have same port number when "
                   + "link Addresses are same";
               
           }
           }

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
        return new LinkValidator(featureNames);
    }
}
