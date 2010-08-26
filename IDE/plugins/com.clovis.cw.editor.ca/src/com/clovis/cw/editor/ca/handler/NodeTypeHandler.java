/**
 * 
 */
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
 * Handler to for the Node classType.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class NodeTypeHandler extends NotificationHandler {

	private static final Log LOG = Log.getLog(CaPlugin.getDefault());

	/**
	 * 
	 * @param n -
	 *            Notification Object
	 * @param changedObj -
	 *            Object which is changed
	 * @param dependentObj -
	 *            The dependent object(s) which has to be updated
	 * @param the
	 *            features to updated in dependent object(s)
	 */
	public static void processNotifications(Notification n, Object changedObj,
			Object dependentObj, String[] featureNames,
			String[] referencePaths, ProjectDataModel pdm) {

		if (dependentObj == null || !(dependentObj instanceof EObject)) {
			return;
		}

		try {
			String classType = EcoreUtils.getValue((EObject) changedObj,
					ComponentEditorConstants.NODE_CLASS_TYPE).toString();

			if (classType.equals(ComponentEditorConstants.NODE_CLASS_D)) {
				EcoreUtils.setValue((EObject) dependentObj, featureNames[0],
						"CL_FALSE");

			} else {
				EcoreUtils.setValue((EObject) dependentObj, featureNames[0],
						"CL_TRUE");
			}

		} catch (Exception e) {
			LOG.error("Error while invoking handler", e);
		}
	}
}
