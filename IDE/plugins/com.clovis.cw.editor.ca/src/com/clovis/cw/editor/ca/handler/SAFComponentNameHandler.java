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
 * Specific Handler for handling change in component details.
 */
public class SAFComponentNameHandler extends NotificationHandler
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
        	String componentName = EcoreUtils.getValue((EObject) changedObj,
	                ComponentEditorConstants.NAME).toString();

        	for (int i=0; i<featureNames.length; i++)
        	{
        		String featureName = featureNames[i];
        		if (featureName.equals("instantiateCommand"))
        		{
                	EcoreUtils.setValue((EObject) dependentObj, featureName, componentName);
        		}
        		else if (featureName.equals("eoProperties"))
        		{
                	EObject eoPropertiesObj = (EObject) EcoreUtils.getValue((EObject) dependentObj, featureName);
                	EcoreUtils.setValue(eoPropertiesObj, "eoName", componentName + "_EO");
        		}
        	}
	    } catch (Exception e) {
            LOG.error("Error while invoking handler", e);
        }
        
    }
}
