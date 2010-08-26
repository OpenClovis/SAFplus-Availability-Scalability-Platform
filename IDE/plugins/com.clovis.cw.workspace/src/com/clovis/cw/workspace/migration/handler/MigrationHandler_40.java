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
import org.w3c.dom.NodeList;

import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.migration.MigrationUtils;

/**
 * Migration handler for 4.0.
 * 
 * @author Suraj Rajyaguru
 *
 */
public class MigrationHandler_40 {

	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());

	/**
	 * Make msgSU default for all the nodes.
	 * 
	 * @param project
	 */
	public static void makeMsgSuDefault(IProject project) {

		String amfConfigFile = project.getLocation().append(
				ICWProject.CW_PROJECT_CONFIG_DIR_NAME).append(
				ICWProject.CPM_XML_DATA_FILENAME).toFile().getAbsolutePath();
		Document document = MigrationUtils.buildDocument(amfConfigFile);

		NodeList nodeList = document.getElementsByTagName("aspServiceUnits");
		for (int i=0 ; i<nodeList.getLength() ; i++) {
			addMsgSu((Element) nodeList.item(i));
		}

		MigrationUtils.saveDocument(document, amfConfigFile);
	}

	/**
	 * Add the msdSU if not present.
	 * 
	 * @param node
	 */
	private static void addMsgSu(Element node) {
		NodeList nodeList = node.getElementsByTagName("aspServiceUnit");
		boolean hasMsgSu = false;

		for(int i=0; i<nodeList.getLength() ; i++) {
			if(((Element) nodeList.item(i)).getAttribute("name").equals("msgSU")) {
				hasMsgSu = true;
				break;
			}
		}

		if(!hasMsgSu) {
			Element childNode = node.getOwnerDocument().createElement(
					"aspServiceUnit");
			childNode.setAttribute("name", "msgSU");
			node.appendChild(childNode);
		}
	}

	/**
	 * Add the Message Server in Eo Configuration.
	 * 
	 * @param project
	 */
	public static void addMsgServerEoConfig(IProject project) {
		String eoConfigFile = project.getLocation().append(
				ICWProject.CW_PROJECT_CONFIG_DIR_NAME).append(
				ICWProject.EOCONFIG_XML_DATA_FILENAME).toFile()
				.getAbsolutePath();
		Document document = MigrationUtils.buildDocument(eoConfigFile);

		Element eoConfig = document.createElement("eoConfig");
		eoConfig.setAttribute("name", "MSG");

		Element eoMemConfig = document.createElement("eoMemConfig");
		eoMemConfig.setAttribute("heapConfig", "Default");
		eoMemConfig.setAttribute("bufferConfig", "Default");
		eoMemConfig.setAttribute("memoryConfig", "Default");
		eoConfig.appendChild(eoMemConfig);

		Element eoIocConfig = document.createElement("eoIocConfig");
		eoConfig.appendChild(eoIocConfig);

		document.getElementsByTagName("EOList").item(0).appendChild(eoConfig);
		MigrationUtils.saveDocument(document, eoConfigFile);
	}

	/**
	 * Add the required tags for Non ASP node.
	 * 
	 * @param project
	 */
	public static void addNonASPNodeParams(IProject project) {
		File amfConfigFile = project.getLocation().append(
				ICWProject.CW_PROJECT_CONFIG_DIR_NAME).append(
				ICWProject.CPM_XML_DATA_FILENAME).toFile();
		File compDataFile = project.getLocation().append(
				ICWProject.CW_PROJECT_MODEL_DIR_NAME).append(
				ICWProject.COMPONENT_XMI_DATA_FILENAME).toFile();

		Document amfConfigDoc = MigrationUtils.buildDocument(amfConfigFile
				.getAbsolutePath());
		Document compDataDoc = MigrationUtils.buildDocument(compDataFile
				.getAbsolutePath());

		NodeList nodeInstList = amfConfigDoc.getElementsByTagName("nodeInstance");
		NodeList nodeTypeList = compDataDoc.getElementsByTagName("node");
		Element node, nodeType = null;

		for (int i = 0; i < nodeInstList.getLength(); i++) {
			node = (Element) nodeInstList.item(i);

			for (int j = 0; j < nodeTypeList.getLength(); j++) {
				nodeType = (Element) nodeTypeList.item(j);
				if (nodeType.getAttribute("name").equals(
						node.getAttribute("type"))) {
					break;
				}
			}

			if (nodeType.getAttribute("classType")
					.equals("CL_AMS_NODE_CLASS_D")) {
				node.setAttribute("chassisId", "0");
				node.setAttribute("slotId", "0");
			}
		}

		MigrationUtils.saveDocument(amfConfigDoc, amfConfigFile.getAbsolutePath());
	}

	/**
	 * Set isASPAware false for Non ASP nodes.
	 * 
	 * @param project
	 */
	public static void setIsASPAware(IProject project) {

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

			Element node;
			Node classType, isASPAware;

			List<Node> nodeList = MigrationUtils.getNodesForPath(amfDefDoc
					.getDocumentElement(),
					"openClovisAsp,version,amfDefinitions:amfTypes,nodeTypes,nodeType");
			Iterator<Node> itr = nodeList.iterator();
			while (itr.hasNext()) {
				node = (Element) itr.next();
				classType = node.getElementsByTagName("classType").item(0);
				isASPAware = node.getElementsByTagName("isASPAware").item(0);

				if (classType.getTextContent().equals("CL_AMS_NODE_CLASS_D")) {
					isASPAware.setTextContent("CL_FALSE");
				} else {
					isASPAware.setTextContent("CL_TRUE");
					if(classType.getTextContent().equals("CL_AMS_NODE_CLASS_A")) {
						classType.setTextContent("CL_AMS_NODE_CLASS_B");
					}
				}
			}

			nodeList = MigrationUtils.getNodesForPath(compDataDoc
					.getDocumentElement(),
					"component:componentInformation,node");
			itr = nodeList.iterator();
			while (itr.hasNext()) {
				node = (Element) itr.next();

				if (node.getAttribute("classType")
						.equals("CL_AMS_NODE_CLASS_D")) {
					node.setAttribute("isASPAware", "CL_FALSE");
				} else {
					node.setAttribute("isASPAware", "CL_TRUE");
					if(node.getAttribute("classType").equals("CL_AMS_NODE_CLASS_A")) {
						node.setAttribute("classType", "CL_AMS_NODE_CLASS_B");
					}
				}
			}

			MigrationUtils
					.saveDocument(amfDefDoc, amfDefFile.getAbsolutePath());
			MigrationUtils.saveDocument(compDataDoc, compDataFile
					.getAbsolutePath());

		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating isASPAware settings for the project : "
							+ project.getName() + ".", e);
		}
	}
	/**
	 * Replace 'toBeCreatedByComponent' to 'autoCreate' in rt.xml
	 * @param project
	 */
	public static void migrateAutoCreate(IProject project) {
		File configFolder = project.getLocation().append(
				ICWProject.CW_PROJECT_CONFIG_DIR_NAME).toFile();
		File files[] = configFolder.listFiles();
		for (int i = 0; i < files.length; i++) {
			File file = files[i];
			if(file.getName().endsWith("_rt.xml")) {
				Document rtDoc = MigrationUtils.buildDocument(file.getAbsolutePath());
				NodeList resourceInstList = rtDoc.getElementsByTagName("resource");
				for (int j = 0; j < resourceInstList.getLength(); j++) {
					Element node = (Element) resourceInstList.item(j);
					String value = node.getAttribute("toBeCreatedByComponent");
					node.setAttribute("autoCreate", value);
					node.removeAttribute("toBeCreatedByComponent");
				}
				MigrationUtils.saveDocument(rtDoc, file.getAbsolutePath());
			}
		}
	}
	/**
	 * Adds config interval for each application and components
	 * @param project
	 */
	public static void migratePMConfigInterval(IProject project) {
		File compModelFile = project.getLocation().append(
				ICWProject.CW_PROJECT_MODEL_DIR_NAME).append(ICWProject.COMPONENT_XMI_DATA_FILENAME).toFile();
		Document compModelDoc = MigrationUtils.buildDocument(compModelFile.getAbsolutePath());
		List<Node> applicationList = MigrationUtils.getNodesForPath(compModelDoc.getDocumentElement(), "component:componentInformation,safComponent");
		
		File amfConfigFile = project.getLocation().append(
				ICWProject.CW_PROJECT_CONFIG_DIR_NAME).append(ICWProject.CPM_XML_DATA_FILENAME).toFile();
		Document amfConfigDoc = MigrationUtils.buildDocument(amfConfigFile.getAbsolutePath());
		NodeList componentList = amfConfigDoc.getElementsByTagName("componentInstance");
		
		File compileConfigFile = project.getLocation().append(ICWProject.CW_PROJECT_CONFIG_DIR_NAME)
		.append("compileconfigs.xml").toFile();
		Document compileConfigDoc = MigrationUtils.buildDocument(compileConfigFile.getAbsolutePath());
		Element pmNode = (Element) compileConfigDoc.getElementsByTagName("PM").item(0);
		pmNode.removeAttribute("pmConfigInterval");
		MigrationUtils.saveDocument(compileConfigDoc, compileConfigFile.getAbsolutePath());
		for (int i = 0; i < applicationList.size(); i++) {
			Element node = (Element) applicationList.get(i);
			String name = node.getAttribute("name");
			Element newNode = compileConfigDoc.createElement("appConfigInterval");
			newNode.setAttribute("applicationName", name);
			newNode.setAttribute("value", "3000");
			pmNode.appendChild(newNode);
		}
		for (int i = 0; i < componentList.getLength(); i++) {
			Element node = (Element) componentList.item(i);
			String name = node.getAttribute("name");
			String type = node.getAttribute("type");
			Element newNode = compileConfigDoc.createElement("compConfigInterval");
			newNode.setAttribute("componentName", name);
			newNode.setAttribute("value", "0");
			newNode.setAttribute("type", type);
			pmNode.appendChild(newNode);
		}
		MigrationUtils.saveDocument(compileConfigDoc, compileConfigFile.getAbsolutePath());
	}
}
