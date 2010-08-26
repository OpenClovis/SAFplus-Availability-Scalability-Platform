/**
 * 
 */
package com.clovis.cw.workspace.migration.handler;

import java.io.File;
import java.io.FileInputStream;
import java.net.URL;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.QualifiedName;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.migration.MigrationUtils;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * Migration handler for 3.0 to 3.0 migration.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class MigrationHandler_30_30 implements ICWProject {

	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());

	/**
	 * Renames old SISP files.
	 * 
	 * @param project
	 */
	public static void renameSispFiles(IProject project) {

		try {
			File sispEnvFile = project.getLocation().append(
					PROJECT_SCRIPT_FOLDER).append(
					"sispenv.py").toFile();
			File clSispCfgFile = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"clSISPCfg.c").toFile();
			File clSispDefFile = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"sispDefinitions.xml").toFile();
			File clSispInsFile = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"sispInstances.xml").toFile();

			if (sispEnvFile.exists())
			{
				sispEnvFile.renameTo(project.getLocation().append(
						PROJECT_SCRIPT_FOLDER).append(
						"sispenv.py").toFile());
				sispEnvFile.delete();
			}

			if (clSispCfgFile.exists())
			{
				clSispCfgFile.renameTo(project.getLocation().append(
						CW_PROJECT_SRC_DIR_NAME).append(
						"config").append(
						"clSISPCfg.c").toFile());
				clSispCfgFile.delete();
			}

			if (clSispDefFile.exists())
			{
				clSispDefFile.renameTo(project.getLocation().append(
						"config").append(
						"sispDefinitions.xml").toFile());
				clSispDefFile.delete();
			}

			if (clSispInsFile.exists())
			{
				clSispInsFile.renameTo(project.getLocation().append(
						CW_PROJECT_SRC_DIR_NAME).append(
						"config").append(
						"sispInstances.xml").toFile());
				clSispInsFile.delete();
			}
			
		} catch (Exception e) {
			LOG.error(
					"Migration : Error renaming SISP legacy files : "
							+ project.getName() + ".", e);
		}
	}

	/**
	 * Performs the migration required for multilple CSI.
	 * 
	 * @param project
	 */
	public static void multipleCSIMigration(IProject project) {

		try {
			File amfDefFile = project.getLocation().append(
					CW_PROJECT_CONFIG_DIR_NAME).append("amfDefinitions.xml")
					.toFile();
			File compDataFile = project.getLocation().append(
					CW_PROJECT_MODEL_DIR_NAME).append(COMPONENT_XMI_DATA_FILENAME)
					.toFile();

			Document amfDefDoc = MigrationUtils.buildDocument(amfDefFile
					.getAbsolutePath());
			Document compDataDoc = MigrationUtils.buildDocument(compDataFile
					.getAbsolutePath());

			migrateAMFDefForMultipleCSI(amfDefDoc.getDocumentElement(),
					"amfDefinitions:amfTypes,csiTypes,csiType",
					"amfDefinitions:amfTypes,compTypes,compType");
			migrateCompDataForMultipleCSI(
					compDataDoc.getDocumentElement(),
					"component:componentInformation,componentServiceInstance",
					"component:componentInformation,safComponent#component:componentInformation,nonSAFComponent");

			MigrationUtils.saveDocument(amfDefDoc, amfDefFile.getAbsolutePath());
			MigrationUtils.saveDocument(compDataDoc, compDataFile.getAbsolutePath());

		} catch (Exception e) {
			LOG.error(
					"Migration : Error migrating ComponentData and AMF Definitions for the project : "
							+ project.getName() + ".", e);
		}
	}

	/**
	 * Migrates the amf definitions for the multiple CSI support.
	 * 
	 * @param rootNode
	 * @param csiPath
	 * @param compPath
	 */
	private static void migrateAMFDefForMultipleCSI(Node rootNode,
			String csiPath, String compPath) {

		Node node, childNode;
		String name = null, type = null;
		HashMap<String, String> typeNameMap = new HashMap<String, String>();

		List<Node> nodeList = MigrationUtils.getNodesForPath(rootNode, csiPath);
		Iterator<Node> itr = nodeList.iterator();

		while (itr.hasNext()) {
			node = itr.next();
			name = ((Element) node).getAttribute("name");
			NodeList childList = node.getChildNodes();

			for (int i = 0; i < childList.getLength(); i++) {
				childNode = childList.item(i);

				if (childNode.getNodeName().equals("type")) {
					type = childNode.getTextContent();
					break;
				}
			}

			typeNameMap.put(type, name);
		}

		itr = MigrationUtils.getNodesForPath(rootNode, compPath).iterator();
		while (itr.hasNext()) {

			node = itr.next();
			NodeList childList = node.getChildNodes();

			for (int i = 0; i < childList.getLength(); i++) {
				childNode = childList.item(i);

				if (childNode.getNodeName().equals("csiType")) {
					childNode.setTextContent(typeNameMap.get(childNode
							.getTextContent()));
					break;
				}
			}
		}
	}

	/**
	 * Migrates the component data for the multiple CSI support.
	 * 
	 * @param rootNode
	 * @param csiPath
	 * @param compPath
	 */
	private static void migrateCompDataForMultipleCSI(Node rootNode,
			String csiPath, String compPath) {

		Element element;
		String name = null, type = null;
		HashMap<String, String> typeNameMap = new HashMap<String, String>();

		List<Node> nodeList = MigrationUtils.getNodesForPath(rootNode, csiPath);
		Iterator<Node> itr = nodeList.iterator();

		while (itr.hasNext()) {
			element = (Element) itr.next();

			name = (element).getAttribute("name");
			type = (element).getAttribute("type");
			typeNameMap.put(type, name);
		}

		String path = compPath.split("#")[0];
		nodeList = MigrationUtils.getNodesForPath(rootNode, path);
		path = compPath.split("#")[1];
		nodeList.addAll(MigrationUtils.getNodesForPath(rootNode, path));
		itr = nodeList.iterator();

		while (itr.hasNext()) {
			element = (Element) itr.next();
			(element).setAttribute("csiType", typeNameMap.get(element.getAttribute("csiType")));
		}
	}

	/**
	 * Migrates the name attribute for the CSI Type.
	 * 
	 * @param project
	 */
	public static void migrateCSIName(IProject project) {

		try {
			File amfDefFile = project.getLocation().append(
					CW_PROJECT_CONFIG_DIR_NAME).append("amfDefinitions.xml")
					.toFile();
			File compDataFile = project.getLocation().append(
					CW_PROJECT_MODEL_DIR_NAME).append(COMPONENT_XMI_DATA_FILENAME)
					.toFile();

			Document amfDefDoc = MigrationUtils.buildDocument(amfDefFile
					.getAbsolutePath());
			Document compDataDoc = MigrationUtils.buildDocument(compDataFile
					.getAbsolutePath());

			createCSITypeName(amfDefDoc.getDocumentElement(),
						"amfDefinitions:amfTypes,compTypes,compType,csiTypes,csiType");
			createCSITypeName(compDataDoc.getDocumentElement(),
			"component:componentInformation,safComponent,csiTypes,csiType#component:componentInformation,nonSAFComponent,csiTypes,csiType");

			MigrationUtils.saveDocument(amfDefDoc, amfDefFile.getAbsolutePath());
			MigrationUtils.saveDocument(compDataDoc, compDataFile.getAbsolutePath());

		} catch(Exception e) {
			LOG.error(
					"Migration : Error migrating ComponentData and AMF Definitions for the project : "
							+ project.getName() + ".", e);
		}
	}

	/**
	 * Removes the csi name from the element and adds it as attribute.
	 * 
	 * @param rootNode
	 * @param csiPath
	 */
	private static void createCSITypeName(Node rootNode, String csiPath) {
		List<Node> nodeList;

		if(csiPath.contains("#")) {
			String path = csiPath.split("#")[0];
			nodeList = MigrationUtils.getNodesForPath(rootNode, path);
			path = csiPath.split("#")[1];
			nodeList.addAll(MigrationUtils.getNodesForPath(rootNode, path));

		} else {
			nodeList = MigrationUtils.getNodesForPath(rootNode, csiPath);
		}

		Iterator<Node> itr = nodeList.iterator();
		while(itr.hasNext()) {
			Element element = (Element) itr.next();
			element.setAttribute("name", element.getTextContent());
			element.setTextContent(null);
		}
	}


	/**
	 * Renames old SISP files.
	 * 
	 * @param project
	 */
	public static void renameConfFiles(IProject project) {

		try {
			File amfModelConfigFile = project.getLocation().append(
					"configs").append(
					"amfConfig.xml").toFile();
			File amfModelDefinitionsFile = project.getLocation().append(
					"configs").append(
					"amfDefinitions.xml").toFile();
			File gmsModelConfigFile = project.getLocation().append(
					"configs").append(
					"gmsconfig.xml").toFile();
			File logModelFile = project.getLocation().append(
					"configs").append(
					"log.xml").toFile();
			
			if (amfModelConfigFile.exists())
			{
				amfModelConfigFile.renameTo(project.getLocation().append(
						"configs").append(
						"clAmfConfig.xml").toFile());
				amfModelConfigFile.delete();
			}
		
			if (amfModelDefinitionsFile.exists())
			{
				amfModelDefinitionsFile.renameTo(project.getLocation().append(
						"configs").append(
						"clAmfDefinitions.xml").toFile());
				amfModelDefinitionsFile.delete();
			}
		
			if (gmsModelConfigFile.exists())
			{
				gmsModelConfigFile.renameTo(project.getLocation().append(
						"configs").append(
						"clGmsConfig.xml").toFile());
				gmsModelConfigFile.delete();
			}
		
			if (logModelFile.exists())
			{
				logModelFile.renameTo(project.getLocation().append(
						"configs").append(
						"clLog.xml").toFile());
				logModelFile.delete();
			}
		
			File amfSrcConfigFile = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"amfConfig.xml").toFile();
			File amfSrcDefinitionsFile = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"amfDefinitions.xml").toFile();
			File gmsSrcConfigFile = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"gmsconfig.xml").toFile();
			File logSrcFile = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"log.xml").toFile();
			File compileSrcConfigFilee = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"compileconfigs.xml").toFile();
			File aspSrcDefinitionsFile = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"aspDefinitions.xml").toFile();
			File aspSrcInstancesFile = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"aspInstances.xml").toFile();
			File moidSrcFile = project.getLocation().append(
					CW_PROJECT_SRC_DIR_NAME).append(
					"config").append(
					"MOID.h").toFile();
			
			if (amfSrcConfigFile.exists())
			{
				amfSrcConfigFile.renameTo(project.getLocation().append(
						CW_PROJECT_SRC_DIR_NAME).append(
						"config").append(
						"clAmfConfig.xml").toFile());
				amfSrcConfigFile.delete();
			}

			if (amfSrcDefinitionsFile.exists())
			{
				amfSrcDefinitionsFile.renameTo(project.getLocation().append(
						CW_PROJECT_SRC_DIR_NAME).append(
						"config").append(
						"clAmfDefinitions.xml").toFile());
				amfSrcDefinitionsFile.delete();
			}

			if (gmsSrcConfigFile.exists())
			{
				gmsSrcConfigFile.renameTo(project.getLocation().append(
						CW_PROJECT_SRC_DIR_NAME).append(
						"config").append(
						"clGmsConfig.xml").toFile());
				gmsSrcConfigFile.delete();
			}

			if (logSrcFile.exists())
			{
				logSrcFile.renameTo(project.getLocation().append(
						CW_PROJECT_SRC_DIR_NAME).append(
						"config").append(
						"clLog.xml").toFile());
				logSrcFile.delete();
			}

			if (compileSrcConfigFilee.exists())
			{
				logSrcFile.delete();
			}

			if (aspSrcDefinitionsFile.exists())
			{
				aspSrcDefinitionsFile.renameTo(project.getLocation().append(
						CW_PROJECT_SRC_DIR_NAME).append(
						"config").append(
						"clAspDefinitions.xml").toFile());
				aspSrcDefinitionsFile.delete();
			}

			if (aspSrcInstancesFile.exists())
			{
				aspSrcInstancesFile.renameTo(project.getLocation().append(
						CW_PROJECT_SRC_DIR_NAME).append(
						"config").append(
						"clAspInstances.xml").toFile());
				aspSrcInstancesFile.delete();
			}

			if (moidSrcFile.exists())
			{
				moidSrcFile.renameTo(project.getLocation().append(
						CW_PROJECT_SRC_DIR_NAME).append(
						"config").append(
						"clMOID.h").toFile());
				moidSrcFile.delete();
			}
		} catch (Exception e) {
			LOG.error(
					"Migration : Error renaming config files : "
							+ project.getName() + ".", e);
		}
	}
	/**
	 * Updates the Template directories with new directory structure
	 * @param project
	 */
	public static void updateUserDefinedTemplates(IProject project) {
		try {
			
			IFolder prtemplatesFolder = project
					.getFolder(ICWProject.CW_PROJECT_TEMPLATE_DIR_NAME);
			IResource[] folders = prtemplatesFolder.members();
			for (int i = 0; i < folders.length; i++) {
				if (folders[i] instanceof IFolder) {
					IFolder folder = (IFolder) folders[i];
					if (folder
							.getName()
							.equals(
									ICWProject.CW_PROJECT_DEFAULT_TEMPLATE_GROUP_DIR_NAME)) {
						IResource files[] = folder.members();
						for (int j = 0; j < files.length; j++) {
							if (files[j] instanceof IFile
									&& !files[j]
											.getName()
											.equals(
													ICWProject.CW_PROJECT_TEMPLATE_GROUP_MARKER)) {
								files[j].delete(true, null);
							}
						}
					} else {
						IFolder saffolder = project.getFolder(
								new Path(CW_PROJECT_TEMPLATE_DIR_NAME)
										.append(folder.getName()).append(PROJECT_DEFAULT_SAF_TEMPLATE_GROUP_FOLDER));
						if (!saffolder.exists()) {
							saffolder.create(true, true, null);
						}
				        IFolder nsaffolder = project.getFolder(
								new Path(CW_PROJECT_TEMPLATE_DIR_NAME)
										.append(folder.getName()).append(PROJECT_DEFAULT_NONSAF_TEMPLATE_GROUP_FOLDER));
						if (!nsaffolder.exists()) {
							nsaffolder.create(true, true, null);
						}
						URL templatesURL = FileLocator.find(WorkspacePlugin.getDefault().getBundle(), new Path(PROJECT_TEMPLATE_FOLDER)
						.append(PROJECT_DEFAULT_TEMPLATE_GROUP_FOLDER).append(PROJECT_DEFAULT_SAF_TEMPLATE_GROUP_FOLDER), null);
						Path templatesPath = new Path(FileLocator.resolve(templatesURL)
								.getPath());
						File templatesDir = new File(templatesPath.toOSString());
						File[] templates = templatesDir.listFiles();
						IPath dstPrefix = new Path(PROJECT_TEMPLATE_FOLDER)
						.append(folder.getName())
						.append(
								PROJECT_DEFAULT_SAF_TEMPLATE_GROUP_FOLDER);
						for (int k = 0; k < templates.length; k++) {
							if (templates[k].isDirectory()) {
								continue;
							}
							IFile dst = project.getFile(dstPrefix
									.append(templates[k].getName()));
							if (dst.exists()) {
								dst.delete(true, true, null);
							}
							dst.create(new FileInputStream(templates[k]), true,
									null);
						}
						templatesURL = FileLocator.find(WorkspacePlugin.getDefault().getBundle(), new Path(PROJECT_TEMPLATE_FOLDER)
						.append(PROJECT_DEFAULT_TEMPLATE_GROUP_FOLDER).append(PROJECT_DEFAULT_NONSAF_TEMPLATE_GROUP_FOLDER), null);
						templatesPath = new Path(FileLocator.resolve(templatesURL)
								.getPath());
						templatesDir = new File(templatesPath.toOSString());
						templates = templatesDir.listFiles();
						dstPrefix = new Path(PROJECT_TEMPLATE_FOLDER).append(
								folder.getName()).append(
								PROJECT_DEFAULT_NONSAF_TEMPLATE_GROUP_FOLDER);
						for (int k = 0; k < templates.length; k++) {
							if (templates[k].isDirectory()) {
								continue;
							}
							IFile dst = project.getFile(dstPrefix
									.append(templates[k].getName()));
							if (dst.exists()) {
								dst.delete(true, true, null);
							}
							dst.create(new FileInputStream(templates[k]), true,
									null);
						}
					}
				}
			}
		} catch (Exception e) {
			LOG.error("Migration : Error in updating templates : "
					+ project.getName() + ".", e);
		}
	}
	
	/**
	 * Migrate the symbolic link to the src directory from an absolute link
	 *  to a relative link.
	 * @param project
	 */
	public static void makeSrcLinkRelative(IProject project)
	{
		String projPath = project.getLocation().toOSString();
		String srcLinkPath = projPath + File.separator + ICWProject.CW_PROJECT_SRC_DIR_NAME;
		File srcLinkFile = new File(srcLinkPath);

		try {
			// see if the absolute and canonical paths are the different
			//  if they are then the file is a symbolic link
			String absolutePath = srcLinkFile.getAbsolutePath();
			String canonicalPath = srcLinkFile.getCanonicalPath();
			if (!absolutePath.equals(canonicalPath))
			{
				// remove the old absolute symbolic link
				Process proc = Runtime.getRuntime().exec("rm " + absolutePath);
				proc.waitFor();
				
				// create the new relative symbolic link
				if (!ClovisFileUtils.createRelativeSourceLink(canonicalPath, absolutePath))
				{
					LOG.error("Migration : Error converting absolute source link to relative source link"
							+ " for project " + project.getName());
				}
			}
		} catch (Exception e) {
			LOG.error("Migration : Error converting absolute source link to relative source link : "
					+ project.getName() + ".", e);
		}
	}
	
	/**
	 * Migrate the AUTOBACKUP_MODE property from true/false value to always/never/prompt
	 *  value. If AUTOBACKUP_MODE was set to 'true' then new value will be 'always'.
	 *  Otherwise the property will be set to 'prompt'.
	 * @param project
	 */
	public static void migrateAutoBackupMode(IProject project)
	{
		String newBackupMode = "prompt";
		String oldBackupMode = CwProjectPropertyPage.getAutoBackupMode(project);
		if (oldBackupMode.toLowerCase().equals("true"))
		{
			newBackupMode = "always";
		}
		try {
			project.setPersistentProperty(
	                new QualifiedName("", CwProjectPropertyPage.AUTOBACKUP_MODE), newBackupMode);
		} catch (CoreException ce) {
			LOG.error("Migration : Error converting auto backup mode of [" + oldBackupMode + "]: "
					+ project.getName() + ".", ce);
		}
	}

	/**
	 * Removes alarm sequence table entries for alarms with severity value of:
	 *  CR : critical
	 *  MJ : major
	 *  IN : indeterminate
	 *  CL : clear
	 * Reason for this is that alarms with severity CR and MJ are service impacting
	 * and fault reporting is only done for non-service impacting alarms. As for
	 * severities IN and CL, the user cannot create alarm profiles with these
	 * severities so there is no reason/use to have them available in the sequence
	 * table.
	 * 
	 * @param project
	 */
	public static void removeInvalidAlarmSequence(IProject project) {

		try {
			// get the document representing compile configurations
			File compileConfigFile = project.getLocation().append(CW_PROJECT_CONFIG_DIR_NAME)
					.append("compileconfigs.xml").toFile();
			Document compileConfigDoc = MigrationUtils.buildDocument(compileConfigFile.getAbsolutePath());

			// get an iterator of all alarm sequences
			Node rootNode = compileConfigDoc.getDocumentElement();
			String alarmSeqPath = "CompileConfigs:ComponentsInfo,FM,localSeqTableEntry";
			List<Node> alarmSeqList = MigrationUtils.getNodesForPath(rootNode, alarmSeqPath);
			Iterator<Node> itr = alarmSeqList.iterator();

			// if the alarm sequence has an 'invalid' severity then remove it
			String severity;
			Node node;
			while (itr.hasNext()) {
				node = itr.next();
				severity = ((Element) node).getAttribute("Severity").toUpperCase();
				if ("CRMJINCL".contains(severity))
				{
					node.getParentNode().removeChild(node);
				}
			}

			// save the compile configuation
			MigrationUtils.saveDocument(compileConfigDoc, compileConfigFile.getAbsolutePath());

		} catch (Exception e) {
			LOG.error(
					"Migration : Error removing alarms with invalid severity values from : "
							+ project.getName() + ".", e);
		}
	}
}
