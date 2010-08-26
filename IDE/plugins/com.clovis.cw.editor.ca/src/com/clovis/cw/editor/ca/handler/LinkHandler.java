/*
 * @(#) $RCSfile: AssociateResourceHandler.java,v $
 * $Revision: #4 $ $Date: 2007/03/26 $
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

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;
/**
 * 
 * @author shubhada
 * Link Handler is used to update the change in linkSourceName in both
 * component_resource_map.xml and alarm_resource_map.xml 
 */
public class LinkHandler extends NotificationHandler
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
    		
        	 EStructuralFeature eAttr = (EStructuralFeature) n.getFeature();
             String changedFeature = null;
             if( eAttr != null )
                 changedFeature = eAttr.getName();
            List dependentObjs = (List) dependentObj;
            for (int i = 0; i < dependentObjs.size(); i++) {
	         	 EObject dependent = (EObject) dependentObjs.get(i);
	         	 String linkSource = (String) EcoreUtils.getValue(
                        dependent, featureNames[0]);
                 if (n.getEventType() == Notification.SET
                		 && changedFeature.equals(
                				 ClassEditorConstants.CLASS_NAME))
                 {      
                     String oldObjName = n.getOldStringValue();
                     String newObjName = n.getNewStringValue();
                     if (linkSource.equals(oldObjName)) {
                    	 EcoreUtils.setValue(dependent, featureNames[0], newObjName);
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
                        String oldObjName = EcoreUtils.getName(oldEObj);
                        if (linkSource.equals(oldObjName)) {
                        	EObject container = dependent.eContainer();
                            Object val = container.eGet(dependent.eContainingFeature());
                            if (val instanceof List) {
                                ((List) val).remove(dependent);
                            } else if (val instanceof EObject) {
                                ((EObject) val).eUnset(dependent.eContainingFeature());
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
