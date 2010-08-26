/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/handler/NodeAdmissionHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.handler;

import java.util.Iterator;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * Handler to update the allowed NodeTypes
 * in a slot when the Nodes change in component
 * editor 
 */
public class NodeAdmissionHandler extends NotificationHandler
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
        //dependenObj is always a list.
        if (((List) dependentObj).isEmpty()) {
            return;
        }

        try {
			List dependentList = (List) dependentObj;
			EClass classTypeClass = (EClass) ((EObject) dependentList.get(0))
					.eClass().getEPackage().getEClassifier(
							ModelConstants.SLOT_CLASSTYPE_CLASS);
			String nameFeature = ModelConstants.NAME_FEATURE_NAME;

			for (int i = 0; i < dependentList.size(); i++) {

				if (n.getEventType() == Notification.ADD) {
					EObject obj = EcoreUtils
							.createEObject(classTypeClass, true);

					EcoreUtils.setValue(obj, nameFeature, EcoreUtils
							.getName((EObject) changedObj));
					((List) EcoreUtils.getValue((EObject) dependentList.get(i),
							ModelConstants.SLOT_CLASSTYPE_LIST_REF)).add(obj);

				} else if (n.getEventType() == Notification.SET) {
					List<EObject> classTypeList = (List<EObject>) EcoreUtils
							.getValue((EObject) dependentList.get(i),
									ModelConstants.SLOT_CLASSTYPE_LIST_REF);
					Iterator<EObject> classTypeItr = classTypeList.iterator();

					while (classTypeItr.hasNext()) {
						EObject obj = classTypeItr.next();
						String name = (String) EcoreUtils.getValue(obj,
								nameFeature);

						if (name.equals(n.getOldValue())) {
							EcoreUtils.setValue(obj, nameFeature, n
									.getNewStringValue());
						}
					}

				} else if (n.getEventType() == Notification.REMOVE
						&& n.getOldValue() instanceof EObject) {

					List<EObject> classTypeList = (List<EObject>) EcoreUtils
							.getValue((EObject) dependentList.get(i),
									ModelConstants.SLOT_CLASSTYPE_LIST_REF);
					Iterator<EObject> classTypeItr = classTypeList.iterator();

					while (classTypeItr.hasNext()) {
						EObject obj = classTypeItr.next();
						String name = (String) EcoreUtils.getValue(obj,
								nameFeature);

						EObject oldObj = (EObject) n.getOldValue();
						String oldObjName = EcoreUtils.getName(oldObj);

						if (name.equals(oldObjName)) {
							classTypeItr.remove();
						}
					}
				}
			}
		} catch (Exception e) {
            LOG.error("Error while invoking handler", e);
        }
    }
}
