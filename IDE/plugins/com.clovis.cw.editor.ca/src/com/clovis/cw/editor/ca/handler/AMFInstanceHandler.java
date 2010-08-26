/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/handler/AMFInstanceHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.handler;

import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.MessageDialog;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.project.data.DependencyListener;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * AMFInstanceHandler which handles the instance dependendency
 * updation.
 */
public class AMFInstanceHandler extends NotificationHandler
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
            Object dependentObj, String [] featureNames, String [] referencePaths, ProjectDataModel pdm)
    {
        try {
        List dependentList = null;

        if (NodeProfileDialog.getInstance() != null) {
			Model viewModel = NodeProfileDialog.getInstance()
					.getViewModel();
			EObject amfObj = viewModel.getEObject();

			dependentList = new Vector();
			DependencyListener.getObject(dependentList, referencePaths[0]
					.split(","), amfObj, 0);
		} else {
			dependentList = (List) dependentObj;
		}
        
        if (n.getEventType() == Notification.SET) {
            EObject changedEobj = (EObject) changedObj;
            if (changedEobj.eClass().getName().equals(
                    SafConstants.NODE_INSTANCELIST_ECLASS)
                    || changedEobj.eClass().getName().equals(
                            SafConstants.SI_INSTANCELIST_ECLASS)
                    || changedEobj.eClass().getName().equals(
                            SafConstants.CSI_INSTANCELIST_ECLASS)
                    || changedEobj.eClass().getName().equals(
                            SafConstants.SERVICEUNIT_INSTANCELIST_ECLASS)) {
                for (int i = 0; i < dependentList.size(); i++) {
                    EObject dependent = (EObject) dependentList.get(i);
                    List objDependencyList = (List) EcoreUtils.getValue(dependent,
                            featureNames[0]);
                    for (int j = 0; j < objDependencyList.size(); j++) {
                        String dependentObjName = (String) objDependencyList.
                            get(j);
                        if (n.getOldValue() != null && n.getOldValue().equals(dependentObjName)) {
                            objDependencyList.set(j, n.getNewValue());
                        }
                    }
                }
                
            } 
        } else if (n.getEventType() == Notification.REMOVE
                && n.getOldValue() instanceof EObject) {
            EObject changedEobj = (EObject) n.getOldValue();
            String oldInstanceName = EcoreUtils.getName(changedEobj);
            if (changedEobj.eClass().getName().equals(
                    SafConstants.NODE_INSTANCELIST_ECLASS)
                    || changedEobj.eClass().getName().equals(
                            SafConstants.SI_INSTANCELIST_ECLASS)
                    || changedEobj.eClass().getName().equals(
                            SafConstants.CSI_INSTANCELIST_ECLASS)
                    || changedEobj.eClass().getName().equals(
                            SafConstants.SERVICEUNIT_INSTANCELIST_ECLASS)
                    || changedEobj.eClass().getName().equals(
                            SafConstants.SG_INSTANCELIST_ECLASS)) {
            	if (changedEobj.eClass().getName().equals(
                            SafConstants.SG_INSTANCELIST_ECLASS)) {
            		EObject siInstObj = (EObject) EcoreUtils.getValue(changedEobj,
            				SafConstants.SERVICE_INSTANCES_NAME);
            		if (siInstObj != null) {
            			List siInstList = (List) EcoreUtils.getValue(siInstObj,
            					SafConstants.SERVICE_INSTANCELIST_NAME);
            			for (int i = 0; i < siInstList.size(); i++) {
            				EObject siInst = (EObject) siInstList.get(i);
            				oldInstanceName = EcoreUtils.getName(siInst);
            				removeDependency(dependentList, featureNames, oldInstanceName);
            			}
            		}
            	} else {
            		removeDependency(dependentList, featureNames, oldInstanceName);
            	}
                
            }
        }
        } catch (Exception e) {
            LOG.error("Error while invoking handler", e);
        }
    }
    /**
     * 
     * @param dependentList - Dependent Objects list
     * @param featureNames - FeatureNames specified in annotation
     * @param oldInstanceName - Old instance name which was deleted
     */
    private static void removeDependency(List dependentList, String[] featureNames, String oldInstanceName)
    {
    	for (int i = 0; i < dependentList.size(); i++) {
            EObject dependent = (EObject) dependentList.get(i);
            List objDependencyList = (List) EcoreUtils.getValue(dependent,
                    featureNames[0]);
            Iterator iterator = objDependencyList.iterator();
            while (iterator.hasNext()) {
                String dependentObjName = (String) iterator.next();
                if (oldInstanceName.equals(dependentObjName)) {
                    iterator.remove();
                    EObject container = dependent.eContainer();
                    EStructuralFeature containFeature = dependent.
                        eContainingFeature();
                    if (container != null) {
                        MessageDialog.openWarning(null, "Updation warning",
                        "Configuration done in AMF for field " + containFeature.getName() 
                        + " in '" + EcoreUtils.getName(container) + "' referring '"+ oldInstanceName
                        + "' will be deleted");
                        
                    }
                }
            }
        }
    }
}
