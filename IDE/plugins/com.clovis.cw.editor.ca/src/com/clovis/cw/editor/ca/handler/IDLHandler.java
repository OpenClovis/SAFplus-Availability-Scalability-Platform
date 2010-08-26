/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/handler/IDLHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.handler;

import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * Handler to handle the change in EO names. It will also update the 
 * references to EONames.
 */
public class IDLHandler extends NotificationHandler
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
        try {
        List dependentList = (List) dependentObj;
        for (int i = 0; i < dependentList.size(); i++) {
            EObject eoObj = (EObject) dependentList.get(i);
            String idlEOName = EcoreUtils.getValue(eoObj, featureNames[0]).toString();
            if (n.getEventType() == Notification.SET && n.getOldValue().equals(idlEOName)) {
                String eoName = EcoreUtils.getName((EObject) changedObj);
                EcoreUtils.setValue(eoObj, featureNames[0], eoName);
            } 
        }
        } catch (Exception e) {
            LOG.error("Error while invoking handler", e);
        }
    }

}
