package com.clovis.cw.editor.ca.handler;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

public class AutoSetAssignedSUHandler extends NotificationHandler {

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
        if (dependentObj == null || !(dependentObj instanceof EObject)) {
            return;
        }
        try {
	        String redundancyModel = EcoreUtils.getValue((EObject) changedObj,
	                ComponentEditorConstants.SG_REDUNDANCY_MODEL).toString();
	        if (redundancyModel.equals(ComponentEditorConstants.
	                M_PLUS_N_REDUNDANCY_MODEL)) {
	        	int activeSUs = ((Integer)EcoreUtils.getValue((EObject)dependentObj, SafConstants.SG_ACTIVE_SU_COUNT)).intValue();
	        	int standbySUs = ((Integer)EcoreUtils.getValue((EObject)dependentObj, SafConstants.SG_STANDBY_SU_COUNT)).intValue();
	        	int assignedSUs = activeSUs + standbySUs;
	        		        	
	        	EcoreUtils.setValue((EObject)dependentObj, featureNames[0], String.valueOf(assignedSUs));
	            
	        	
	        	
	        }
        }catch (Exception e) {
            LOG.error("Error while invoking handler", e);
        }    
    }
	
}
