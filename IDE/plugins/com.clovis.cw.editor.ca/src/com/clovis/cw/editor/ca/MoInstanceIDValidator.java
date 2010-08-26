/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/MoInstanceIDValidator.java $
 * $Author: srajyaguru $
 * $Date: 2007/05/18 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.MessageDialog;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 *
 * @author shubhada
 *Validator to check for single CPM node.
 */
public class MoInstanceIDValidator extends ObjectValidator
{
    /**
     *
     * @param featureNames Feature Names
     */
    public MoInstanceIDValidator(Vector featureNames)
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
           message = checkForToBeCreatedByComponentValidity(eobj);
       }
       if (message == null) {
           message = checkForPrimaryOIValidity(eobj);
       }
       /*if (message == null) {
           message = validateConfiguredInst(eobj, "moID");
       }*/
      return message;
   }

   /**
	 * Checks for the validity for the primary OI.
	 * 
	 * @param eobj
	 * @return
	 */
	private String checkForPrimaryOIValidity(EObject eobj) {
		boolean primaryOI = ((Boolean) EcoreUtils.getValue(eobj, "primaryOI"))
				.booleanValue();

		if (primaryOI) {
			String moId = EcoreUtils.getValue(eobj, "moID").toString();
			int startIndex = moId.lastIndexOf("\\");
			int endIndex = moId.lastIndexOf(":");

			if (startIndex != -1 && endIndex != -1) {
				String resName = moId.substring(startIndex + 1, endIndex);

				IProject project = (IProject) NodeProfileDialog.getInstance()
						.getProject();
				ProjectDataModel pdm = ProjectDataModel
						.getProjectDataModel(project);
				EObject resObj = ResourceDataUtils.getObjectFrmName(pdm
						.getCAModel().getEList(), resName);

				if(ResourceDataUtils.hasTransientAttr(resObj)) {
					return null;
				}
			}

			return "Primary OI can not be true for the resources which dont have any transient attribute.";
		}
		return null;
	}

	/**
	 * Checks for the validity for 'to be created by component'.
	 * 
	 * @param eobj
	 * @return
	 */
	private String checkForToBeCreatedByComponentValidity(EObject eobj) {
		boolean autoCreate = ((Boolean) EcoreUtils.getValue(eobj,
				"autoCreate")).booleanValue();

		if (autoCreate) {

			String moId = EcoreUtils.getValue(eobj, "moID").toString();
			if (moId.indexOf('*') != -1) {
				return "'Auto Create' can not be true "
						+ "if wildcard exists in the instance ID";
			}

			int startIndex = moId.lastIndexOf("\\");
			int endIndex = moId.lastIndexOf(":");

			if (startIndex != -1 && endIndex != -1) {
				String resName = moId.substring(startIndex + 1, endIndex);

				IProject project = (IProject) NodeProfileDialog.getInstance()
						.getProject();
				ProjectDataModel pdm = ProjectDataModel
						.getProjectDataModel(project);
				EObject resObj = ResourceDataUtils.getObjectFrmName(pdm
						.getCAModel().getEList(), resName);

				if (ResourceDataUtils.hasInitializedAttr(resObj)) {
					return "'Auto Create' can not be true for the "
					+ "resource which have intialized attribute.";
				}
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
	   message = checkForToBeCreatedByComponentValidity(eobj);
	   if (message != null) {
	       errorList.add(message);
	   }
	   message = checkForPrimaryOIValidity(eobj);
	   if (message != null) {
	       errorList.add(message);
	   }
	   return errorList;
   }
    /**
     * Create Validator Instance.
     * @param featureNames Vector
     * @return Validator
     */
    public static ObjectValidator createValidator(Vector featureNames)
    {
        return new MoInstanceIDValidator(featureNames);
    }
    
    
     /**
      * Validates that for any resource, the number of configured instances
      * should not exceed the maximum instances configured for that resource
      * @param eObj EObject
      * @param featureName name of MoId field
      * @return Message
      */
    
    public static String validateConfiguredInst(EObject eObj, String featureName)
    {
    	String 	moId = EcoreUtils.getValue(eObj, featureName).toString();
    	   	    	   	
    	IProject project = (IProject) NodeProfileDialog.getInstance()
				.getProject();

    	ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
    	
    	int startIndex 	= 	moId. indexOf("\\");
		int endIndex 	= 	moId. indexOf(":");
		int confInst 	=	-1;
		int maxInst		=	-1;
		
    	while( startIndex != -1 && endIndex != -1 )
    	{
        		
			String resName = moId.substring(startIndex + 1, endIndex);
			
			startIndex 	=	endIndex + 1;
			endIndex 	= 	moId. indexOf("\\",	startIndex);
			
			String confInstStr = "";
			
			if(-1 == endIndex){
				
				confInstStr = moId.substring(startIndex, moId.length());
			}
			else
			{
				confInstStr =  moId.substring(startIndex, endIndex);
			}
			
			if(-1 != endIndex)
			{
				startIndex 	= 	moId. indexOf("\\", startIndex);
				endIndex 	= 	moId. indexOf(":", endIndex);
			}
			
			
			
			if(confInstStr.equals("*")){
				continue;
			}
			
			confInst = Integer.parseInt(confInstStr);
			
			EObject resObj = ResourceDataUtils.getObjectFrmName(pdm
					.getCAModel().getEList(), resName);
			
			maxInst = ((Integer)EcoreUtils.getValue(resObj, 
					ClassEditorConstants.CLASS_MAX_INSTANCES)).intValue();
			
			if(confInst >= maxInst ){
				String message = "Invalid instance number '"+ confInst + 
								 "' for resource : " +
								 EcoreUtils.getValue(resObj, "name") +
								 ". It should be less than the maximum " +
								 "instances configured ("+ maxInst + ").";
				return message;
			}
			
		}
    	return null;
    }
    /**
    *
    * @param eobj
    *            EObject which is modified or newly added to list
    * @param eList Elist
    * @param n - Notification Object
    * @return errorMessage if the feature value is duplicated in the list.
    * else return null
    */
   protected String isDuplicate(EObject eobj, List eList, Notification n)
   {
       
       for (int j = 0; j < _featureNames.size(); j++) {
           String featureName = (String) _featureNames.get(j);
           String name = null;
           if (eobj.eClass().getEStructuralFeature(featureName) != null) {
               name = EcoreUtils.getValue(eobj, featureName).toString();
           }
           EStructuralFeature feature = eobj.eClass().
               getEStructuralFeature(featureName);
           String featureLabel = EcoreUtils.getAnnotationVal(feature, null, "label");
           String label = featureLabel != null ? featureLabel: featureName;
           for (int i = 0; i < eList.size(); i++) {
               EObject obj = (EObject) eList.get(i);
               if (featureName != null && !eobj.equals(obj)) {
                   String objName = null;
                   if (obj.eClass().getEStructuralFeature(
                           featureName) != null) {
                   objName = EcoreUtils.getValue(obj, featureName).toString();
                   }
                   if (name != null && objName != null) {
                	   if(name.trim().equals("") || objName.trim().equals(""))
                		   continue;
                       if(name.equals(objName)) {
                       if (n != null && n.getEventType() == Notification.SET) {
                           String oldVal = n.getOldStringValue();
                           if( oldVal != null && !oldVal.equals(""))
                           {
 		                      EcoreUtils.setValue(eobj, featureName, oldVal);
                               MessageDialog.openError(null, "Validations", "Duplicate entry for " + label + " : '" + name + "'. " +
                                   label + " reverted");
 		                      return null;
                           }
                       }
                       return "Duplicate entry for - " + label + " : " + name +
                       ".\n This has to be corrected in order to be able to save changes.";
                   }
                   }
               }
            }
       }
       return null;
   }
}
