/*
 * @(#) $RCSfile: AlarmProfileAdapter.java,v $
 * $Revision: #3 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/AlarmProfileAdapter.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * @(#) $RCSfile: AlarmProfileAdapter.java,v $
 * $Revision: #3 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.cw.workspace.dialog;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * AlarmProfileAdapter class to listen to the changes
 * in alarm configuration
 */
public class AlarmProfileAdapter extends AdapterImpl
{
    private ArrayList _deletedAlarmProfiles;
    private Shell _shell = null;
    private ProjectDataModel _pdm = null;
    /**
     * Constructor
     * @param shell - Shell
     * @param deletedList - Deleted Alarm Profiles
     */
    public AlarmProfileAdapter(Shell shell, ArrayList deletedList,
            ProjectDataModel pdm)
    {
        _shell = shell;
        _deletedAlarmProfiles = deletedList;
        _pdm = pdm;
    }
    
    /**
     * @param msg Notification
     */
    public void notifyChanged(Notification msg) {
        switch (msg.getEventType()) {
        case Notification.REMOVING_ADAPTER:
            break;
        case Notification.SET:
            break;
        case Notification.ADD:
            Object newVal = msg.getNewValue();
            if (newVal instanceof EObject) {
                EObject obj = (EObject) newVal;
                EcoreUtils.addListener(obj, this, 1);
            }
            break;

        case Notification.ADD_MANY:
            List objs = (List) msg.getNewValue();
            for (int i = 0; i < objs.size(); i++) {
                if (objs.get(i) instanceof EObject) {
                    EObject eObj = (EObject) objs.get(i);
                    EcoreUtils.addListener(eObj, this, 1);
                }
            }
            break;
        case Notification.REMOVE:
            Object obj = msg.getOldValue();
            if (obj instanceof EObject) {
                EObject ob = (EObject) obj;
                EcoreUtils.removeListener(ob, this, 1);
                String name = (String) EcoreUtils.getName(ob);
                if (name != null) {
                    String updateResource = checkForUpdates(name);
                    if (updateResource != null) {
                        _deletedAlarmProfiles.add(name);
                        MessageDialog.openWarning(_shell, "Warning",
                                "This alarm profile is associated to resource "
                                        + "'" + updateResource + "'");
                    }
                }
            }
            break;
        case Notification.REMOVE_MANY:
            objs = (List) msg.getOldValue();
            for (int i = 0; i < objs.size(); i++) {
                if (objs.get(i) instanceof EObject) {
                    EObject o = (EObject) objs.get(i);
                    EcoreUtils.removeListener(o, this, 1);
                    String name = (String) EcoreUtils.getName(o);
                    if (name != null) {
                        String updateResource = checkForUpdates(name);
                        if (updateResource != null) {
                            _deletedAlarmProfiles.add(name);
                            MessageDialog.openWarning(_shell, "Warning",
                                    "This alarm profile is associated to resource "
                                            + "'" + updateResource + "'");
                        }
                    }
                }
            }
            break;
        }
    }
    /**
    *
    * @param oldVal if the Changed values is associated with a Resource.
    * @return true if alarm profile is associated with resource else false.
    */
   private String checkForUpdates(String oldVal)
   {
       String returnVal = null;
       List resList = ResourceDataUtils.getMoList(_pdm.getCAModel().getEList());
       for (int i = 0; i < resList.size(); i++) {
           EObject obj = (EObject) resList.get(i);
           List associatedAlarmList = (List) ResourceDataUtils.
               getAssociatedAlarms(_pdm.getProject(), obj);
           if (associatedAlarmList != null) {
	           for (int j = 0; j < associatedAlarmList.size(); j++) {
	               String alarm = (String) associatedAlarmList.get(j);
	               if (oldVal.equals(alarm)) {
	                   returnVal = EcoreUtils.getName(obj);
	               }
	           }
           }
           
       }
       return returnVal;
   }
    
}

