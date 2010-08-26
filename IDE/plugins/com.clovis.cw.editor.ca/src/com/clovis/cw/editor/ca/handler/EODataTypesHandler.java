package com.clovis.cw.editor.ca.handler;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Handler to handle the include path dependency.
 * 
 * @author Suraj Rajyaguru
 *
 */
public class EODataTypesHandler extends NotificationHandler {

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
			Object dependentObj, String[] featureNames,
			String[] referencePaths, ProjectDataModel pdm) {

    	if (dependentObj == null || !(dependentObj instanceof EObject)) {
			return;
		}

    	try {
			boolean defNeed = Boolean.parseBoolean(EcoreUtils.getValue(
					(EObject) changedObj, "defNeed").toString());

			if (defNeed) {
				EcoreUtils
						.setValue((EObject) dependentObj, featureNames[0], "");
			}

    	} catch (Exception e) {
			LOG.error("Error while invoking handler", e);
		}
	}
}
