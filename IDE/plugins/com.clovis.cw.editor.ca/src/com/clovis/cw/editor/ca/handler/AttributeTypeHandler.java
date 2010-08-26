/**
 * 
 */
package com.clovis.cw.editor.ca.handler;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Dependency Handler to handle the dependecy for the attribute type.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class AttributeTypeHandler extends NotificationHandler {

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
			if (((EStructuralFeature) n.getFeature()).getName().equals("type")) {
				String attrType = ((EObject) changedObj).eGet(
						(EStructuralFeature) n.getFeature()).toString();

				if (attrType.equals("CONFIG")) {
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[0], "true");
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[1], "true");
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[2], "true");

				} else if (attrType.equals("RUNTIME")) {
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[0], "false");
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[1], "false");
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[2], "false");
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[3], "false");

				}

			} else if (((EStructuralFeature) n.getFeature()).getName().equals(
					"dataType")) {
				String dataType = EcoreUtils.getValue((EObject) changedObj,
						"dataType").toString();

				if (dataType.equals("TruthValue")) {
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[0], "0");
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[1], "1");
				}
			}

		} catch (Exception e) {
			LOG.error("Error while invoking handler", e);
		}
	}
}
