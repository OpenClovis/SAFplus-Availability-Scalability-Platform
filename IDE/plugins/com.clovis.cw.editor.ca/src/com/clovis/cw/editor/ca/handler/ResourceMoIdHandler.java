/**
 * 
 */
package com.clovis.cw.editor.ca.handler;

import java.io.File;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.dialogs.MessageDialog;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Specific handler to handle change in resources for moId.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ResourceMoIdHandler extends NotificationHandler {

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
			for (int i = 0; i < dependentObjs.size(); i++) {
				EObject dependent = (EObject) dependentObjs.get(i);
				String moId = EcoreUtils.getValue(dependent, featureNames[0])
						.toString();

				if (n.getEventType() == Notification.SET) {

					if (((EStructuralFeature) n.getFeature()).getName().equals(
							"name")) {

						String oldValue = "\\" + n.getOldStringValue() + ":";
						String newValue = "\\" + n.getNewStringValue() + ":";

						if (moId.contains(oldValue)) {
							EcoreUtils.setValue(dependent,
									featureNames[0], moId.replace(oldValue,
											newValue));
						}

						Resource rtResource = NodeProfileDialog
								.getCompResResource(EcoreUtils
										.getName(dependent), false, pdm);
						List<EObject> resList = NodeProfileDialog
								.getCompResListFromRTRes(rtResource);
						EObject resObj;
						for (int j = 0; j < resList.size(); j++) {
							resObj = resList.get(j);
							moId = EcoreUtils.getValue(resObj, "moID")
									.toString();

							if (moId.contains(oldValue)) {
								EcoreUtils.setValue(resObj, "moID",
										moId.replace(oldValue, newValue));
							}
						}
						EcoreModels.saveResource(rtResource);

					} /*else if (((EStructuralFeature) n.getFeature()).getName()
							.equals("maxInstances")) {

						String resName = "\\"
								+ EcoreUtils.getValue(
										(EObject) n.getNotifier(), "name")
										.toString() + ":";
						int maxInstances = Integer.parseInt(n
								.getNewStringValue());
						int instanceId;
						int index = moId.indexOf(resName);

						if (index != -1) {
							index += resName.length();
							instanceId = Integer.parseInt(moId.substring(index,
									moId.indexOf("\\", index) != -1 ? moId.indexOf("\\", index) : moId.length()));

							if (instanceId >= maxInstances) {
								removeNodeInstance(dependent, resName, pdm);
								continue;
							}
						}

						Resource rtResource = NodeProfileDialog
								.getCompResResource(EcoreUtils
										.getName(dependent), false, pdm);
						List<EObject> resList = NodeProfileDialog
								.getCompResListFromRTRes(rtResource);
						EObject resObj;

						for (int j = 0; j < resList.size(); j++) {
							resObj = resList.get(j);
							moId = EcoreUtils.getValue(resObj, "moID")
									.toString();
							index = moId.indexOf(resName);

							if (index != -1) {
								index += resName.length();

								try {
									instanceId = Integer.parseInt(moId.substring(index,
											moId.indexOf("\\", index) != -1 ? moId.indexOf("\\", index) : moId.length()));
									if (instanceId >= maxInstances) {
										removeFromParent(resObj);
									}
								} catch (NumberFormatException e) {
								}
							}
						}

						EcoreModels.saveResource(rtResource);
					}*/

				} else if (n.getEventType() == Notification.REMOVE
						|| n.getEventType() == Notification.REMOVE_MANY) {

					String oldValue = "\\"
							+ EcoreUtils.getName((EObject) n.getOldValue())
							+ ":";

					if (moId.contains(oldValue)) {
						removeNodeInstance(dependent, oldValue, pdm);

					} else {
						Resource rtResource = NodeProfileDialog
								.getCompResResource(EcoreUtils
										.getName(dependent), false, pdm);
						List<EObject> resList = NodeProfileDialog.getCompResListFromRTRes(rtResource);
						EObject resObj;

						for (int j = 0; j < resList.size(); j++) {
							resObj = resList.get(j);
							moId = EcoreUtils.getValue(resObj, "moID")
									.toString();

							if (moId.contains(oldValue)) {
								removeFromParent(resObj);
							}
						}

						EcoreModels.saveResource(rtResource);
					}
				}
			}

		} catch (Exception e) {
			LOG.error("Error while invoking handler", e);
		}
	}

	/**
	 * Removes the node instance.
	 * 
	 * @param nodeInstance
	 * @param n
	 * @param pdm
	 */
	private static void removeNodeInstance(EObject nodeInstance, String resName, ProjectDataModel pdm) {
		String dependentName = EcoreUtils.getName(nodeInstance);

		MessageDialog.openWarning(null, "Updation warning",
				"Configuration done for '" + dependentName
						+ "' referring object '" + resName
						+ "' will be deleted");

		removeFromParent(nodeInstance);

		String dataFilePath = pdm.getProject().getLocation()
				.toOSString()
				+ File.separator
				+ ICWProject.CW_PROJECT_CONFIG_DIR_NAME
				+ File.separator
				+ EcoreUtils.getName(nodeInstance)
				+ "_"
				+ SafConstants.RT_SUFFIX_NAME;
		File file = new File(dataFilePath);
		file.delete();
	}

	/**
	 * Removes the object from its parent.
	 * 
	 * @param eObj
	 */
	private static void removeFromParent(EObject eObj) {
		EObject container = eObj.eContainer();
		Object val = container.eGet(eObj
				.eContainingFeature());

		if (val instanceof List) {
			((List) val).remove(eObj);
		} else if (val instanceof EObject) {
			((EObject) val).eUnset(eObj
					.eContainingFeature());
		}
	}
}
