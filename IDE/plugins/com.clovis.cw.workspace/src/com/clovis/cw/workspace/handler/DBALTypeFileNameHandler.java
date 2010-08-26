/**
 * 
 */
package com.clovis.cw.workspace.handler;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Dependency Handler to handle the dependecy for the db type.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class DBALTypeFileNameHandler extends NotificationHandler {

	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());

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
			if (((EStructuralFeature) n.getFeature()).getName().equals("Type")) {
				String dbType = ((EObject) changedObj).eGet(
						(EStructuralFeature) n.getFeature()).toString();

				if (dbType.equals("SQLite")) {
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[0], "libClSQLiteDB.so");

				} else if (dbType.equals("GDBM")) {
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[0], "libClGDBM.so");

				} else if (dbType.equals("Berkeley")) {
					EcoreUtils.setValue((EObject) dependentObj,
							featureNames[0], "libClBerkeleyDB.so");
				}
			}

		} catch (Exception e) {
			LOG.error("Error while invoking handler", e);
		}
	}
}
