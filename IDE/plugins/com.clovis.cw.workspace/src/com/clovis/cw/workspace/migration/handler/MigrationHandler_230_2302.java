/**
 * 
 */
package com.clovis.cw.workspace.migration.handler;

import java.io.File;
import java.io.FileInputStream;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

import javax.xml.parsers.DocumentBuilderFactory;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.migration.MigrationUtils;

/**
 * Migration handler for 2.3.0 to 2.3.0.2 migration.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MigrationHandler_230_2302 implements ICWProject {

	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());

	/**
	 * Migrates the alarm model.
	 * 
	 * @param project
	 */
	public static void migrateAlarmModel(IProject project) {

		try {
			File alarmDataFile = project.getLocation().append(
					CW_PROJECT_MODEL_DIR_NAME).append(
					ALARM_PROFILES_XMI_DATA_FILENAME).toFile();
			File resDataFile = project.getLocation().append(
					CW_PROJECT_MODEL_DIR_NAME).append(
					RESOURCE_XML_DATA_FILENAME).toFile();
			File resAlarmMapFile = project.getLocation().append(
					CW_PROJECT_MODEL_DIR_NAME).append("resource_alarm_map.xml")
					.toFile();
			File alarmRuleFile = project.getLocation().append(
					CW_PROJECT_MODEL_DIR_NAME).append(
					ALARM_RULES_XML_DATA_FILENAME).toFile();

			if (alarmDataFile.exists() && resDataFile.exists()
					&& resAlarmMapFile.exists()) {

				Document alarmDataDoc = MigrationUtils
						.buildDocument(alarmDataFile.getAbsolutePath());
				Document resDataDoc = MigrationUtils.buildDocument(resDataFile
						.getAbsolutePath());
				Document resAlarmMapDoc = MigrationUtils
						.buildDocument(resAlarmMapFile.getAbsolutePath());
				Document alarmRuleDoc = null;

				alarmRuleDoc = DocumentBuilderFactory.newInstance()
						.newDocumentBuilder().newDocument();

				HashMap<String, Element> genRuleMap = new HashMap<String, Element>();
				HashMap<String, Element> supRuleMap = new HashMap<String, Element>();
				HashMap<String, String> rdnAlarmIDMap = new HashMap<String, String>();

				NodeList alarmProfileList = alarmDataDoc
						.getElementsByTagName("AlarmProfile");
				Element alarmProfileElement, genRuleElement, supRuleElement;
				String rdn, alarmID;

				for (int i = 0; i < alarmProfileList.getLength(); i++) {
					alarmProfileElement = (Element) alarmProfileList.item(i);
					rdn = alarmProfileElement.getAttribute("rdn");
					alarmID = alarmProfileElement.getAttribute("AlarmID");
					rdnAlarmIDMap.put(rdn, alarmID);

					if (alarmProfileElement.getElementsByTagName(
							"GenerationRule").getLength() > 0) {
						genRuleElement = (Element) alarmProfileElement
								.getElementsByTagName("GenerationRule").item(0);
						alarmProfileElement.removeChild(genRuleElement);
						genRuleMap.put(alarmID, genRuleElement);
					}

					if (alarmProfileElement.getElementsByTagName(
							"SuppressionRule").getLength() > 0) {
						supRuleElement = (Element) alarmProfileElement
								.getElementsByTagName("SuppressionRule")
								.item(0);
						alarmProfileElement.removeChild(supRuleElement);
						supRuleMap.put(alarmID, supRuleElement);
					}

					alarmProfileElement.normalize();
				}

				MigrationUtils.renameAttribute(alarmDataDoc
						.getDocumentElement(), "AlarmProfile", "AlarmID",
						"alarmID");
				MigrationUtils.saveDocument(alarmDataDoc, alarmDataFile
						.getAbsolutePath());

				Element alarmRuleElement = alarmRuleDoc.createElementNS(
						"alarmRule.ecore", "alarmRule:alarmRuleInformation");
				alarmRuleDoc.appendChild(alarmRuleElement);

				Element resDataRootElement = resDataDoc.getDocumentElement();
				ArrayList<Element> resList = new ArrayList<Element>();
				MigrationUtils.getElementListByTagName(resDataRootElement,
						"hardwareResource", resList);
				MigrationUtils.getElementListByTagName(resDataRootElement,
						"softwareResource", resList);
				MigrationUtils.getElementListByTagName(resDataRootElement,
						"nodeHardwareResource", resList);
				MigrationUtils.getElementListByTagName(resDataRootElement,
						"systemController", resList);
				MigrationUtils.getElementListByTagName(resDataRootElement,
						"mibResource", resList);

				HashMap<String, Element> resAlarmMap = new HashMap<String, Element>();
				NodeList resAlarmMapList = resAlarmMapDoc
						.getElementsByTagName("linkDetail");
				Element resAlarmMapElement;
				for (int i = 0; i < resAlarmMapList.getLength(); i++) {
					resAlarmMapElement = (Element) resAlarmMapList.item(i);
					if (resAlarmMapElement.getElementsByTagName("linkTarget")
							.getLength() > 0) {
						resAlarmMap
								.put(resAlarmMapElement
										.getAttribute("linkSource"),
										resAlarmMapElement);
					}
				}

				Element resElement, alarmElement;
				String resName;

				Iterator<Element> resIterator = resList.iterator();
				while (resIterator.hasNext()) {

					resName = resIterator.next().getAttribute("name");
					resElement = alarmRuleDoc.createElement("resource");
					resElement.setAttribute("name", resName);

					NodeList linkTargetList, alarmIdList;
					resAlarmMapElement = resAlarmMap.get(resName);

					if (resAlarmMapElement != null) {
						alarmRuleElement.appendChild(resElement);
						linkTargetList = resAlarmMapElement
								.getElementsByTagName("linkTarget");

						for (int i = 0; i < linkTargetList.getLength(); i++) {
							alarmID = linkTargetList.item(i).getChildNodes()
									.item(0).getNodeValue();

							alarmElement = alarmRuleDoc.createElement("alarm");
							alarmElement.setAttribute("alarmID", alarmID);
							resElement.appendChild(alarmElement);

							genRuleElement = (Element) alarmRuleDoc.importNode(
									genRuleMap.get(alarmID), true);
							alarmIdList = genRuleElement
									.getElementsByTagName("AlarmIDs");
							for (int j = 0; j < alarmIdList.getLength(); j++) {
								alarmIdList.item(j).getFirstChild().setNodeValue(
										rdnAlarmIDMap.get(alarmIdList.item(j)
												.getFirstChild().getNodeValue()));
							}
							alarmElement.appendChild(genRuleElement);

							supRuleElement = (Element) alarmRuleDoc.importNode(
									supRuleMap.get(alarmID), true);
							alarmIdList = supRuleElement
									.getElementsByTagName("AlarmIDs");
							for (int j = 0; j < alarmIdList.getLength(); j++) {
								alarmIdList.item(j).getFirstChild().setNodeValue(
										rdnAlarmIDMap.get(alarmIdList.item(j)
												.getFirstChild().getNodeValue()));
							}
							alarmElement.appendChild(supRuleElement);
						}
					}
				}

				MigrationUtils.renameNode(alarmRuleElement, "GenerationRule",
						"generationRule");
				MigrationUtils.renameNode(alarmRuleElement, "SuppressionRule",
						"suppressionRule");

				MigrationUtils.renameAttribute(alarmRuleElement,
						"generationRule", "Relation", "relationType");
				MigrationUtils.renameAttribute(alarmRuleElement,
						"generationRule", "MaxAlarm", "maxAlarm");
				MigrationUtils.renameAttribute(alarmRuleElement,
						"suppressionRule", "Relation", "relationType");
				MigrationUtils.renameAttribute(alarmRuleElement,
						"suppressionRule", "MaxAlarm", "maxAlarm");

				MigrationUtils.renameNode(alarmRuleElement, "AlarmIDs",
						"alarmIDs");
				Node attrNode;
				NodeList nodeList = alarmRuleElement
						.getElementsByTagName("generationRule");
				for (int i = 0; i < nodeList.getLength(); i++) {
					attrNode = nodeList.item(i).getAttributes().getNamedItem(
							"AlarmIDs");
					if (attrNode != null) {
						MigrationUtils.renameNode(attrNode, "alarmIDs");
					}
				}
				nodeList = alarmRuleElement
						.getElementsByTagName("suppressionRule");
				for (int i = 0; i < nodeList.getLength(); i++) {
					attrNode = nodeList.item(i).getAttributes().getNamedItem(
							"AlarmIDs");
					if (attrNode != null) {
						MigrationUtils.renameNode(attrNode, "alarmIDs");
					}
				}

				MigrationUtils.saveDocument(alarmRuleDoc, alarmRuleFile
						.getAbsolutePath());
			}
		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating Alarm Model for the project : "
							+ project.getName() + ".", e);
		}
	}

	/**
	 * Migrates the eoConfig file.
	 * 
	 * @param project
	 */
	public static void migrateEoConfig(IProject project) {

		try {
			File clEoConfigFile = project.getLocation().append(
					CW_PROJECT_CONFIG_DIR_NAME).append(
					EOCONFIG_XML_DATA_FILENAME).toFile();

			if (!clEoConfigFile.exists()) {
				URL xmiURL = FileLocator.find(DataPlugin.getDefault()
						.getBundle(), new Path(PLUGIN_XML_FOLDER
						+ File.separator + EOCONFIG_DEFAULT_XML_DATA_FILENAME),
						null);

				Path xmiPath = new Path(FileLocator.resolve(xmiURL).getPath());
				String eoConfigPath = CW_PROJECT_CONFIG_DIR_NAME
						+ File.separator + EOCONFIG_XML_DATA_FILENAME;

				IFile dst = project.getFile(new Path(eoConfigPath));
				dst.getParent().refreshLocal(1, null);
				dst.create(new FileInputStream(xmiPath.toOSString()), true,
						null);

				eoConfigPath = project.getLocation().toOSString()
						+ File.separator + CW_PROJECT_CONFIG_DIR_NAME
						+ File.separator + EOCONFIG_XML_DATA_FILENAME;
				Document eoConfigDoc = MigrationUtils
						.buildDocument(eoConfigPath);
				Node eoListNode = eoConfigDoc.getDocumentElement();

				String componentDataPath = project.getLocation().toOSString()
						+ File.separator + CW_PROJECT_MODEL_DIR_NAME
						+ File.separator + COMPONENT_XMI_DATA_FILENAME;
				Document componentDataDoc = MigrationUtils
						.buildDocument(componentDataPath);
				NodeList compList = componentDataDoc
						.getElementsByTagName("safComponent");

				Element eoConfigElement, eoMemConfigElement, eoIocConfigElement;
				for (int i = 0; i < compList.getLength(); i++) {

					eoConfigElement = eoConfigDoc.createElement("eoConfig");
					eoListNode.appendChild(eoConfigElement);
					eoConfigElement.setAttribute("name", ((Element) compList
							.item(i)).getAttribute("name"));

					eoMemConfigElement = eoConfigDoc
							.createElement("eoMemConfig");
					eoConfigElement.appendChild(eoMemConfigElement);
					eoMemConfigElement.setAttribute("heapConfig", "Default");
					eoMemConfigElement.setAttribute("bufferConfig", "Default");
					eoMemConfigElement.setAttribute("memoryConfig", "Default");

					eoIocConfigElement = eoConfigDoc
							.createElement("eoIocConfig");
					eoConfigElement.appendChild(eoIocConfigElement);
				}

				MigrationUtils.saveDocument(eoConfigDoc, eoConfigPath);
			}
		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating 'clEoConfig.xml' for the project : "
							+ project.getName() + ".", e);
		}
	}

	/**
	 * Migrates the EoDefinitons file.
	 * 
	 * @param project
	 */
	public static void migrateEoDefinitons(IProject project) {

		try {
			File clEoDefinitionsFile = project.getLocation().append(
					CW_PROJECT_CONFIG_DIR_NAME).append(
					MEMORYCONFIG_XML_DATA_FILENAME).toFile();

			if (!clEoDefinitionsFile.exists()) {
				URL xmiURL = FileLocator.find(DataPlugin.getDefault()
						.getBundle(), new Path(PLUGIN_XML_FOLDER
						+ File.separator
						+ MEMORYCONFIG_DEFAULT_XML_DATA_FILENAME), null);

				Path xmiPath = new Path(FileLocator.resolve(xmiURL).getPath());
				String eoDefinitionsPath = CW_PROJECT_CONFIG_DIR_NAME
						+ File.separator + MEMORYCONFIG_XML_DATA_FILENAME;

				IFile dst = project.getFile(new Path(eoDefinitionsPath));
				dst.getParent().refreshLocal(1, null);
				dst.create(new FileInputStream(xmiPath.toOSString()), true,
						null);
			}
		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating 'clEoDefinitions.xml' for the project : "
							+ project.getName() + ".", e);
		}
	}

	/**
	 * Migrates the IocConfig file.
	 * 
	 * @param project
	 */
	public static void migrateIocConfig(IProject project) {

		try {
			File clIocConfigFile = project.getLocation().append(
					CW_PROJECT_CONFIG_DIR_NAME).append(
					IOCBOOT_XML_DATA_FILENAME).toFile();

			if (!clIocConfigFile.exists()) {
				URL xmiURL = FileLocator.find(DataPlugin.getDefault()
						.getBundle(), new Path(PLUGIN_XML_FOLDER
						+ File.separator + IOCBOOT_DEFAULT_XML_DATA_FILENAME),
						null);

				Path xmiPath = new Path(FileLocator.resolve(xmiURL).getPath());
				String iocConfigPath = CW_PROJECT_CONFIG_DIR_NAME
						+ File.separator + IOCBOOT_XML_DATA_FILENAME;

				IFile dst = project.getFile(new Path(iocConfigPath));
				dst.getParent().refreshLocal(1, null);
				dst.create(new FileInputStream(xmiPath.toOSString()), true,
						null);
			}

			String iocConfigPath = clIocConfigFile.getAbsoluteFile().toString();
			Document iocDoc = MigrationUtils.buildDocument(iocConfigPath);

			NodeList linkList = iocDoc.getElementsByTagName("link");
			for (int i = 0; i < linkList.getLength(); i++) {
				((Element) linkList.item(i))
						.removeAttribute("linkSupportsMulticast");
			}

			Element iocElement = (Element) iocDoc.getElementsByTagName("ioc")
					.item(0);
			iocElement.setAttribute("heartBeatInterval", "1000");
			iocElement.setAttribute("reassemblyTimeout", "1000");

			Element waterMarkElement = iocDoc.createElement("waterMark");
			waterMarkElement.setAttribute("lowLimit", "35");
			waterMarkElement.setAttribute("highLimit", "90");

			Element actionElement = iocDoc.createElement("action");
			waterMarkElement.appendChild(actionElement);

			Element eventElement = iocDoc.createElement("event");
			actionElement.appendChild(eventElement);
			eventElement.setAttribute("enable", "false");

			Element notificationElement = iocDoc.createElement("notification");
			actionElement.appendChild(notificationElement);
			notificationElement.setAttribute("enable", "false");

			Element logElement = iocDoc.createElement("log");
			actionElement.appendChild(logElement);
			logElement.setAttribute("enable", "false");

			Element customElement = iocDoc.createElement("custom");
			actionElement.appendChild(customElement);
			customElement.setAttribute("enable", "false");

			Element sendQueueElement = iocDoc.createElement("sendQueue");
			sendQueueElement.setAttribute("size", "0");
			sendQueueElement.appendChild(waterMarkElement.cloneNode(true));

			Element receiveQueueElement = iocDoc.createElement("receiveQueue");
			receiveQueueElement.setAttribute("size", "0");
			receiveQueueElement.appendChild(waterMarkElement.cloneNode(true));

			if (iocDoc.getElementsByTagName("sendQueue").getLength() == 0) {
				Element transportElement = (Element) iocDoc
						.getElementsByTagName("transport").item(0);

				iocElement.insertBefore(sendQueueElement.cloneNode(true),
						transportElement);
				iocElement.insertBefore(receiveQueueElement.cloneNode(true),
						transportElement);
			}

			Element nodeInstancesElement;
			if (iocDoc.getElementsByTagName("nodeInstances").getLength() == 0) {
				nodeInstancesElement = iocDoc.createElement("nodeInstances");
				iocElement.appendChild(nodeInstancesElement);
			} else {
				nodeInstancesElement = (Element) iocDoc.getElementsByTagName(
						"nodeInstances").item(0);
			}

			File amfConfigFile = project.getLocation().append(
					CW_PROJECT_CONFIG_DIR_NAME).append("amfConfig.xml")
					.toFile();
			if (amfConfigFile.exists()) {
				Document amfDoc = MigrationUtils.buildDocument(amfConfigFile
						.getAbsolutePath());
				if (iocDoc.getElementsByTagName("nodeInstance").getLength() == 0) {
					Element nodeInstElement = iocDoc
							.createElement("nodeInstance");
					nodeInstElement.appendChild(sendQueueElement
							.cloneNode(true));
					nodeInstElement.appendChild(receiveQueueElement
							.cloneNode(true));

					NodeList amfNodeInstList = amfDoc
							.getElementsByTagName("nodeInstance");
					Element nodeElement;
					String nodeInstName;
					for (int i = 0; i < amfNodeInstList.getLength(); i++) {
						nodeInstName = ((Element) amfNodeInstList.item(i))
								.getAttribute("name");
						nodeElement = (Element) nodeInstElement.cloneNode(true);
						nodeElement.setAttribute("name", nodeInstName);
						nodeInstancesElement.appendChild(nodeElement);
					}
				}
			}

			MigrationUtils.saveDocument(iocDoc, iocConfigPath);
		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating 'clIocConfig.xml' for the project : "
							+ project.getName() + ".", e);
		}
	}

	/**
	 * Prepends nodeMoId path to moId in <nodeInstanceName>_rt.xml corresponding
	 * to each nodeInstance
	 */
	public static void migrateMoID(IProject project) {

		try {
			File amfConfigFile = project.getLocation().append(
					CW_PROJECT_CONFIG_DIR_NAME).append("amfConfig.xml")
					.toFile();
			if (amfConfigFile.exists()) {
				Document amfConfigDoc = MigrationUtils
						.buildDocument(amfConfigFile.getAbsolutePath());
				NodeList nodeInstList = amfConfigDoc
						.getElementsByTagName(SafConstants.NODE_INSTANCELIST_NAME);

				Element nodeInst, resInst;
				String nodeInstName, nodeMoID, rtPath, moID, isAbsolutePath;
				Document rtDoc;
				NodeList resList;

				for (int i = 0; i < nodeInstList.getLength(); i++) {
					nodeInst = (Element) nodeInstList.item(i);
					nodeInstName = nodeInst.getAttribute("name");
					nodeMoID = nodeInst.getAttribute("nodeMoId");

					rtPath = project.getLocation().toOSString()
							+ File.separator + CW_PROJECT_CONFIG_DIR_NAME
							+ File.separator + nodeInstName + "_"
							+ SafConstants.RT_SUFFIX_NAME;
					rtDoc = MigrationUtils.buildDocument(rtPath);
					resList = rtDoc
							.getElementsByTagName(SafConstants.RESOURCELIST_NAME);

					for (int j = 0; j < resList.getLength(); j++) {
						resInst = (Element) resList.item(j);
						resInst.setAttribute("primaryOI", "false");
						moID = resInst.getAttribute("moID");
						isAbsolutePath = resInst.getAttribute("isAbsolutePath");

						if (!moID.equals("\\")) {

							if (isAbsolutePath.equals("true")) {
								moID = "\\" + nodeMoID.split("\\\\")[1] + moID;

							} else if (!moID.equals("")
									&& !moID.startsWith(nodeMoID)) {
								moID = nodeMoID + moID;
							}
						}

						resInst.setAttribute("moID", moID);
						resInst.removeAttribute("isAbsolutePath");
					}

					MigrationUtils.saveDocument(rtDoc, rtPath);
				}
			}
		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating 'moID path for resources' for the project : "
							+ project.getName() + ".", e);
		}
	}

	/**
	 * Migrates the Log config xml file.
	 */
	public static void migrateLogConfig(IProject project) {

		try {
			File logConfigFile = project.getLocation().append(
					CW_PROJECT_CONFIG_DIR_NAME)
					.append("log.xml").toFile();

			if (logConfigFile.exists()) {
				logConfigFile.delete();
			}

			URL xmiURL = FileLocator.find(DataPlugin.getDefault().getBundle(),
					new Path(PLUGIN_XML_FOLDER + File.separator
							+ LOG_DEFAULT_CONFIGS_XML_FILENAME), null);

			Path xmiPath = new Path(FileLocator.resolve(xmiURL).getPath());
			String dataFilePath = CW_PROJECT_CONFIG_DIR_NAME + File.separator
					+ "log.xml";

			IFile dst = project.getFile(new Path(dataFilePath));
			dst.getParent().refreshLocal(1, null);
			dst.create(new FileInputStream(xmiPath.toOSString()), true, null);
			dst.getParent().refreshLocal(1, null);

		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating 'log.xml' for the project : "
							+ project.getName() + ".", e);
		}
	}
}
