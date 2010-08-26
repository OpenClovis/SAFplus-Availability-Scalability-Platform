/**
 * 
 */
package com.clovis.cw.editor.ca.handler;

import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
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
 * Dependency Handler to handle the deletion of SU references if the
 * corresponding Node Instance is deleted.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class AMFNodeSUHandler extends NotificationHandler {

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
			if (n.getEventType() == Notification.REMOVE
					&& n.getOldValue() instanceof EObject) {

				EObject SUInstsRef = (EObject) EcoreUtils.getValue(
						(EObject) changedObj,
						SafConstants.SERVICEUNIT_INSTANCES_NAME);
				List SUList = (List) EcoreUtils.getValue(SUInstsRef,
						SafConstants.SERVICEUNIT_INSTANCELIST_NAME);

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
				
				for (int k = 0; k < dependentList.size(); k++) {
					EObject dependent = (EObject) dependentList.get(k);
					List dependentObjList = (List) EcoreUtils.getValue(dependent,
							featureNames[0]);
					Iterator iterator = dependentObjList.iterator();

					while (iterator.hasNext()) {
						String dependentName = (String) iterator.next();

						for (int i = 0; i < SUList.size(); i++) {
							if (dependentName.equals(EcoreUtils
									.getName((EObject) SUList.get(i)))) {

								MessageDialog.openWarning(null,
										"Updation warning",
										"Configuration done in AMF for field "
												+ dependent
														.eContainingFeature()
														.getName()
												+ " in '"
												+ EcoreUtils.getName(dependent
														.eContainer())
												+ "' referring '"
												+ dependentName
												+ "' will be deleted");
								iterator.remove();
							}
						}
					}
				}
			}
		} catch (Exception e) {
			LOG.error("Error while invoking handler", e);
		}
	}
}
