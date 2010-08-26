/**
 * 
 */
package com.clovis.cw.workspace.migration.handler;

import java.io.File;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.migration.MigrationUtils;

/**
 * Migration Handler for 3.1.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MigrationHandler_31 {

	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());

	/**
	 * Converts the HealthCheck params to millis.
	 * 
	 * @param project
	 */
	public static void convertHealthCheckToMillis(IProject project) {

		try {
			File amfDefFile = project.getLocation().append(
					ICWProject.CW_PROJECT_CONFIG_DIR_NAME).append(
					ICWProject.AMFDEF_XML_DATA_FILENAME).toFile();
			File compDataFile = project.getLocation().append(
					ICWProject.CW_PROJECT_MODEL_DIR_NAME).append(
					ICWProject.COMPONENT_XMI_DATA_FILENAME).toFile();

			Document amfDefDoc = MigrationUtils.buildDocument(amfDefFile
					.getAbsolutePath());
			Document compDataDoc = MigrationUtils.buildDocument(compDataFile
					.getAbsolutePath());

			Element healthCheck;
			Node period, maxDuration;

			List<Node> nodeList = MigrationUtils.getNodesForPath(amfDefDoc
					.getDocumentElement(),
					"amfDefinitions:amfTypes,compTypes,compType,healthCheck");
			Iterator<Node> itr = nodeList.iterator();
			while (itr.hasNext()) {
				healthCheck = (Element) itr.next();
				period = healthCheck.getElementsByTagName("period").item(0);
				period.setTextContent(period.getTextContent() + "000");
				maxDuration = healthCheck.getElementsByTagName("maxDuration")
						.item(0);
				maxDuration.setTextContent(maxDuration.getTextContent() + "000");
			}

			nodeList = MigrationUtils.getNodesForPath(compDataDoc
					.getDocumentElement(),
					"component:componentInformation,safComponent,healthCheck");
			itr = nodeList.iterator();
			while (itr.hasNext()) {
				healthCheck = (Element) itr.next();
				healthCheck.setAttribute("period", healthCheck
						.getAttribute("period")
						+ "000");
				healthCheck.setAttribute("maxDuration", healthCheck
						.getAttribute("maxDuration")
						+ "000");
			}

			MigrationUtils
					.saveDocument(amfDefDoc, amfDefFile.getAbsolutePath());
			MigrationUtils.saveDocument(compDataDoc, compDataFile
					.getAbsolutePath());

		} catch (Exception e) {
			LOG
					.error(
							"Migration : Error migrating Health Check parameters to millis for the project : "
									+ project.getName() + ".", e);
		}
	}
}
