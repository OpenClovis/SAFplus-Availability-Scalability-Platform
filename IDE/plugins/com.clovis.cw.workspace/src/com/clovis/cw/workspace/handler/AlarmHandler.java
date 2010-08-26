/*
 * @(#) $RCSfile: AlarmHandler.java,v $
 * $Revision: #3 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.cw.workspace.handler;

import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.project.data.DependencyListener;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.dialog.AlarmProfileDialog;

public class AlarmHandler extends NotificationHandler
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
        String [] refPaths = referencePaths[0].split("\\$");
        for (int i = 0; i < refPaths.length; i++) {
            String refPath = refPaths[i];
            String [] references = refPath.split(",");
            Model viewModel = null;
        	
            try {
                List dependentList;
                if (references[0].equals(ClassEditorConstants.
                        ALARM_GENERATIONRULE)
                        || referencePaths[0].equals(ClassEditorConstants.
                        ALARM_SUPPRESSIONRULE)) {
                    viewModel = AlarmProfileDialog.getInstance().getViewModel();
                    List alarmProfiles = viewModel.getEList();
                    dependentList = new Vector();
                    for (int j = 0; j < alarmProfiles.size(); j++) {
                        EObject eobj = (EObject) alarmProfiles.get(j);
                        DependencyListener.getObject(dependentList, references, eobj, 0);
                    }
                } else {
                    dependentList = (List) dependentObj;
                }
            	
            	EStructuralFeature eAttr = (EStructuralFeature) n.getFeature();
                String changedFeature = null;
                if( eAttr != null )
                    changedFeature = eAttr.getName();
                for (int j = 0; j < dependentList.size(); j++) {
                     EObject dependent = (EObject) dependentList.get(j);  
    	         	 List associatedIDs = (List) EcoreUtils.getValue(
                            dependent, featureNames[0]);
                     if (n.getEventType() == Notification.SET)
                     {      
                         if (changedFeature != null && changedFeature.equals(
                                 ClassEditorConstants.ALARM_ID)) {
                             String oldAlarmID = n.getOldStringValue();
                             if (associatedIDs.contains(oldAlarmID)) {
                                 int index = associatedIDs.indexOf(oldAlarmID);
                                 associatedIDs.set(index, n.getNewStringValue());
                                 
                             }
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
                            String oldAlarmID = (String) EcoreUtils.getValue(oldEObj,
                            		ClassEditorConstants.ALARM_ID);
                            if (associatedIDs.contains(oldAlarmID)) {
                                associatedIDs.remove(oldAlarmID);
                            }
                        }
                        
                     }
                     
                }
                if ((references[0].equals(ClassEditorConstants.
                        ALARM_GENERATIONRULE)
                        || referencePaths[0].equals(ClassEditorConstants.
                        ALARM_SUPPRESSIONRULE))) {
                   // viewModel.save(false);
                    
                }  
                
            } catch (Exception e) {
                LOG.error("Error while invoking handler", e);
            }
        }
    }
}
