/*
 * @(#) $RCSfile: AssociateResourceHandler.java,v $
 * $Revision: #5 $ $Date: 2007/03/26 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.cw.editor.ca.handler;

import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.MessageDialog;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;
/**
 * 
 * @author shubhada
 * 
 * Handler class to update the change in name of the resources to the
 * component_resource_map.xml
 *
 */
public class AssociateResourceHandler extends NotificationHandler
{
    private static final Log LOG = Log.getLog(CaPlugin.getDefault());
    /**
     * 
     * @param n - Notification Object
     * @param changedObj - Object which is changed
     * @param dependentObj - The dependent object(s) which has to be 
     * updated
     * @param the features to updated in dependent object(s)
     */
    public static void processNotifications(Notification n, Object changedObj,
            Object dependentObj, String [] featureNames,
            String [] referencePaths, ProjectDataModel pdm)
    {
    	if (n.getEventType() == Notification.ADD
    			|| n.getEventType() == Notification.ADD_MANY)
        {
    		return;
        }
    	try {
    		if (changedObj instanceof EObject) {
    			EObject changedEObj = (EObject) changedObj;
    			if (changedEObj.eClass().getName().equals(
    	                ClassEditorConstants.HARDWARE_RESOURCE_NAME)
    	                || changedEObj.eClass().getName().equals(
    	                    ClassEditorConstants.SOFTWARE_RESOURCE_NAME)
    	                || changedEObj.eClass().getName().equals(
    	                    ClassEditorConstants.MIB_RESOURCE_NAME)
    	                || changedEObj.eClass().getName().equals(
    	                    ClassEditorConstants.NODE_HARDWARE_RESOURCE_NAME)
    	                 || changedEObj.eClass().getName().equals(
    	    	                    ClassEditorConstants.SYSTEM_CONTROLLER_NAME)) {
    				GenericEditorInput compInput = (GenericEditorInput) pdm
    						.getComponentEditorInput();
    				if (compInput != null && compInput.getEditor() != null
							&& compInput.getEditor().getEditorModel() != null
							&& compInput.isDirty()) {
    					boolean status = MessageDialog.openQuestion(
    							null, "Save component_resource_map model",
    							"Component_Resource_Map Model has been modified. Save changes?");
    					if (status) {
    						if (compInput != null && compInput.getEditor() != null
    								&& compInput.getEditor().getEditorModel() != null
    								&& compInput.getEditor().getEditorModel().isDirty()) {
    							compInput.getEditor().doSave(null);
    						}
    					}
    				}
    				
    			}
    		}
        	 EStructuralFeature eAttr = (EStructuralFeature) n.getFeature();
             String changedFeature = null;
             if( eAttr != null )
                 changedFeature = eAttr.getName();
            List dependentObjs = (List) dependentObj;
            for (int i = 0; i < dependentObjs.size(); i++) {
	         	 EObject dependent = (EObject) dependentObjs.get(i);
	         	 List associatedResList = (List) EcoreUtils.getValue(
                        dependent, featureNames[0]);
                if (associatedResList != null) {
                     if (n.getEventType() == Notification.SET
                    		 && changedFeature.equals(
                    				 ClassEditorConstants.CLASS_NAME))
                     {      
                         String oldResName = n.getOldStringValue();
                         if (associatedResList.contains(oldResName)) {
                        	 associatedResList.set(associatedResList.
                        			 indexOf(oldResName), n.getNewStringValue());
                             
                         }    
                     }
                     // check if notification is for delete, then warn
                     //the user that all the objects which
                     // refer to deleted object will be deleted.
                     else if (n.getEventType() == Notification.REMOVE
                             || n.getEventType() == Notification.REMOVE_MANY)
                     {
                        Object oldVal = n.getOldValue();
                        if (oldVal instanceof EObject) {
                            EObject oldEObj = (EObject) oldVal;
                            String oldResName = EcoreUtils.getName(oldEObj);
                            if (associatedResList.contains(oldResName)) {
                            	associatedResList.remove(oldResName);
                                /*MessageDialog.openWarning(null, "Updation Warning", "Deleted resource '"
                                        + EcoreUtils.getName(oldEObj) + "'" + " will be dissociated from component '"
                                        + EcoreUtils.getValue(dependent, "linkSource") + "'");*/
                                GenericEditorInput compInput = (GenericEditorInput) pdm
        						.getComponentEditorInput();
                                if (compInput != null && compInput.getEditor() != null
            							&& compInput.getEditor().getEditorModel() != null) {
                                	boolean isDirty = compInput.getEditor().isDirty();
                                	compInput.getEditor().getViewer().getContents().refresh();
                                	if(!isDirty) {
                                		compInput.getEditor().doSave(null);
                                	}
                				}
                            }
                        }   
                     }
                     
                }
                 
            }
            
            } catch (Exception e) {
                LOG.error("Error while invoking handler", e);
            }
    }

}
