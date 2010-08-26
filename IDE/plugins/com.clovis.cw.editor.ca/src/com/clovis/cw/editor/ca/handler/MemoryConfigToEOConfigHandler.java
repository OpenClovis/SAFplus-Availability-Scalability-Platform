/**
 * 
 */
package com.clovis.cw.editor.ca.handler;

import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.MessageDialog;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Handler to handle the dependency between memory config to eo config.
 * 
 * @author Suraj Rajyaguru
 *
 */
public class MemoryConfigToEOConfigHandler extends NotificationHandler {

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

		try {
			List dependentObjs = (List) dependentObj;

			if (n.getEventType() == Notification.SET) {

				if (((EStructuralFeature) n.getFeature()).getName().equals(
						"name")) {
					String oldVal = n.getOldStringValue();

					for (int i = 0; i < dependentObjs.size(); i++) {
						EObject dependent = (EObject) dependentObjs.get(i);
						String dependentVal = EcoreUtils.getValue(
								dependent, featureNames[0]).toString();

						if (dependentVal.equals(oldVal)) {

							EcoreUtils.setValue(dependent, featureNames[0], n
									.getNewStringValue());
						}
					}
				}

			} else if (n.getEventType() == Notification.REMOVE) {
				String oldVal = EcoreUtils.getName((EObject) n
						.getOldValue());
				boolean deleteFlag = false;
				String val = "Default";

				for (int i = 0; i < dependentObjs.size(); i++) {
					EObject dependent = (EObject) dependentObjs.get(i);
					String dependentVal = EcoreUtils.getValue(dependent,
							featureNames[0]).toString();

					if (dependentVal.equals(oldVal)) {

						deleteFlag = true;
						if(featureNames[0].equals("iocConfig")) {
							val = "";
						}
						EcoreUtils.setValue(dependent, featureNames[0], val);
					}
				}

				if(deleteFlag) {
					MessageDialog.openWarning(null, "Updation warning",
							"Configuration done for '" + oldVal
									+ "' will be set to Default for it.");
				}
			}

		} catch (Exception e) {
			LOG.error("Error while invoking handler", e);
		}
	}
}
