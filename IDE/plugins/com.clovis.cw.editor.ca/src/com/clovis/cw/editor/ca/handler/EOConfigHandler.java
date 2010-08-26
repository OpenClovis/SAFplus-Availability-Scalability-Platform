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
 * Handler to update the clEoConfig.xml based on the changes in CE.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class EOConfigHandler extends NotificationHandler {

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
			EObject rootObj = (EObject) ((List) dependentObj).get(0);
			EStructuralFeature feature = (EStructuralFeature) rootObj.eClass()
					.getEAllReferences().get(0);
			List dependentObjs = (List) rootObj.eGet(feature);

			if (n.getEventType() == Notification.ADD) {
				EObject newComp = EcoreUtils.createEObject(
						((EObject) dependentObjs.get(0)).eClass(), true);

				EcoreUtils.setValue(newComp, "name", EcoreUtils
						.getName((EObject) EcoreUtils.getValue((EObject) n
								.getNewValue(), "eoProperties")));
				dependentObjs.add(newComp);

			} else if (n.getEventType() == Notification.REMOVE) {
				String oldCompName = EcoreUtils.getName((EObject) EcoreUtils
						.getValue((EObject) n.getOldValue(), "eoProperties"));

				for (int i = 0; i < dependentObjs.size(); i++) {
					EObject dependent = (EObject) dependentObjs.get(i);
					String dependentCompName = EcoreUtils.getValue(dependent,
							"name").toString();

					if (dependentCompName.equals(oldCompName)) {

						MessageDialog.openWarning(null, "Updation warning",
								"Configuration done for '" + dependentCompName
										+ "' will be deleted.");

						dependentObjs.remove(dependent);
						break;
					}
				}
			}
		} catch (Exception e) {
			LOG.error("Error while invoking handler", e);
		}
	}
}
