/**
 * 
 */
package com.clovis.cw.editor.ca.handler;

import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Handler to handle change in EO name. The same will be updated in the
 * clEoConfig.xml file.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class EOConfigEONameHandler extends NotificationHandler {

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
			if (n.getEventType() == Notification.SET) {

				EObject rootObj = (EObject) ((List) dependentObj).get(0);
				EStructuralFeature feature = (EStructuralFeature) rootObj
						.eClass().getEAllReferences().get(0);
				List dependentList = (List) rootObj.eGet(feature);

				for (int i = 0; i < dependentList.size(); i++) {
					EObject eoObj = (EObject) dependentList.get(i);
					String eoConfigEOName = EcoreUtils.getName(eoObj);
					if (n.getOldStringValue().equals(eoConfigEOName)) {
						EcoreUtils.setValue(eoObj, "name", n
								.getNewStringValue());
					}
				}
			}

		} catch (Exception e) {
			LOG.error("Error while invoking handler", e);
		}
	}

}
