/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/handler/SGRedundancyHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.handler;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * Specific Handler for handling change in SG redundancy model.
 */
public class SGRedundancyHandler extends NotificationHandler
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
        if (dependentObj == null || !(dependentObj instanceof EObject)) {
            return;
        }
        try {
	        String redundancyModel = EcoreUtils.getValue((EObject) changedObj,
	                ComponentEditorConstants.SG_REDUNDANCY_MODEL).toString();
	        if (redundancyModel.equals(ComponentEditorConstants.
	                NO_REDUNDANCY_MODEL)) {
	            EcoreUtils.setValue((EObject) dependentObj, featureNames[0], "1");
	            
	            
	            EcoreUtils.setValue((EObject) dependentObj, featureNames[1], "0");
	            
	            
	            EcoreUtils.setValue((EObject) dependentObj, featureNames[2], "1");
	            
	            EcoreUtils.setValue((EObject) dependentObj, featureNames[3], "1");
	            
	            EcoreUtils.setValue((EObject) dependentObj, featureNames[4], "1");
	            
	        } else if (redundancyModel.equals(ComponentEditorConstants.
	                TWO_N_REDUNDANCY_MODEL)) {
	            
	            EcoreUtils.setValue((EObject) dependentObj, featureNames[0], "1");
	            
	            EcoreUtils.setValue((EObject) dependentObj, featureNames[1], "1");
	            
	            EcoreUtils.setValue((EObject) dependentObj, featureNames[2], "2");
	            
	            EcoreUtils.setValue((EObject) dependentObj, featureNames[3], "2");
	            
	            EcoreUtils.setValue((EObject) dependentObj, featureNames[4], "1");
	            
	        } else if (redundancyModel.equals(ComponentEditorConstants.
	        		M_PLUS_N_REDUNDANCY_MODEL)) {
	            //Active >1       	
	        	EcoreUtils.setValue((EObject) dependentObj, featureNames[0], "1");
	            //Standby >= 1
	        	EcoreUtils.setValue((EObject) dependentObj, featureNames[1], "1");
	            //Inservice >= Assigned
	        	EcoreUtils.setValue((EObject) dependentObj, featureNames[2], "2");
	            //Assigned = Active + Standby
	        	EcoreUtils.setValue((EObject) dependentObj, featureNames[3], "1");
	           
	        	EcoreUtils.setValue((EObject) dependentObj, featureNames[4], "1");
	           
	        }	
	    } catch (Exception e) {
            LOG.error("Error while invoking handler", e);
        }
        
    }
}
