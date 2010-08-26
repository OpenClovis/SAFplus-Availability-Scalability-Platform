package com.clovis.cw.workspace.handler;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.net.URL;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 * Specific Resource Editor data migration handler to handle the changes between
 * FCS1 to FCS2
 */
public class FCS1MigrationHandler implements ICWProject
{
	private static IProject project = null;
	private static final Log LOG = Log.getLog(DataPlugin.getDefault());
	public static void migrate(IProject proj)
	{
		project = proj;
		try {
			createDefaultConfigFiles(true);
		} catch (CoreException e) {
			LOG.error("Migration : " + e.getMessage() + ".", e);
		} catch (IOException e) {
			LOG.error("Migration : " + e.getMessage() + ".", e);
		}
		migrateProject();
	}
	
	/**
	 * Migrates the project.
	 */
	private static void migrateProject() {
		prependFullPathToMoid();
		migrateResourceData();
		migrateComponentData();
		migrateEOConfig();
		migrateEODefinitions();
		migrateIOCConfig();
		migrateCompileConfigs();
		migrateLogConfig();
	}

	/**
	 * Migrates the Resource Data xml file.
	 */
	private static void migrateResourceData() {
		String fileName = project.getLocation().toOSString() + File.separator
				+ ICWProject.CW_PROJECT_MODEL_DIR_NAME + File.separator
				+ RESOURCE_XML_DATA_FILENAME;

		if (!isUpdateRequired(fileName)) {
			return;
		}
		
		if(fileContainsString(fileName, "dataType")) {
			String strToBeReplaced[] = { " isTransient=\"true\"",
					" isTransient=\"false\""};
				String strReplacement[] = { "", "" };
				String attrAppendTag[] = { };
				String attrAppendStr[] = { };

				migrateFile(fileName, strToBeReplaced, strReplacement, attrAppendTag,
						attrAppendStr, new String[] {}, new String[] {});
		} else {
			String strToBeReplaced[] = { " isTransient=\"true\"",
				" isTransient=\"false\"", "type" };
			String strReplacement[] = { "", "", "dataType" };
			String attrAppendTag[] = { "attribute" };
			String attrAppendStr[] = { " type=\"CONFIG\"" };

			migrateFile(fileName, strToBeReplaced, strReplacement, attrAppendTag,
					attrAppendStr, new String[] {}, new String[] {});
		}

		

		ProjectDataModel.getProjectDataModel(project).createCAModel();
	}

	/**
	 * Migrates the Component Data xml file.
	 */
	private static void migrateComponentData() {
		String fileName = project.getLocation().toOSString() + File.separator
				+ ICWProject.CW_PROJECT_MODEL_DIR_NAME + File.separator
				+ COMPONENT_XMI_DATA_FILENAME;

		if (!isUpdateRequired(fileName)) {
			return;
		}

		String attrAppendTag[] = { "serviceGroup" };
		String attrAppendStr[] = { " isCollocationAllowed=\"true\" alphaFactor=\"100\"" };

		migrateFile(fileName, new String[] {}, new String[] {}, attrAppendTag,
				attrAppendStr, new String[] {}, new String[] {});

//		ProjectDataModel.getProjectDataModel(project).createComponentModel();
	}

	/**
	 * Migrates the EOConfig xml file.
	 */
	private static void migrateEOConfig() {
		String fileName = project.getLocation().toOSString() + File.separator
				+ ICWProject.CW_PROJECT_CONFIG_DIR_NAME + File.separator
				+ EOCONFIG_XML_DATA_FILENAME;
		
		if(fileContainsString(fileName, "AlarmServer")) {
			String strToBeReplaced[] = { "AlarmServer", "CpmServer", "CkptServer", "CmServer", "CorServer", "DebugServer", "EventServer", "FaultServer", "GmsServer", "LogServer", "NameServer", "SnmpServer", "TxnServer"};
			String strReplacement[] = { "ALM", "AMF", "CKP", "CHM", "COR", "DBG", "EVT", "FLT", "GMS", "LOG", "NAM", "SNS", "TXN"};
			migrateFile(fileName, strToBeReplaced, strReplacement, new String[] {},
					new String[] {}, new String[] {}, new String[] {});
//			ProjectDataModel.getProjectDataModel(project).createEOConfiguration();
		}
		
		if (!isUpdateRequired(fileName)) {
			return;
		}

		String strToBeReplaced[] = { "memConfig"};
		String strReplacement[] = { "memoryConfig"};

		String elmAppendTag[] = { "eoConfig" };
		String elmAppendStr[] = { "    <eoIocConfig/>\n" };

		migrateFile(fileName, strToBeReplaced, strReplacement, new String[] {},
				new String[] {}, elmAppendTag, elmAppendStr);

//		ProjectDataModel.getProjectDataModel(project).createEOConfiguration();
	}

	/**
	 * Migrates the EODefinitions xml file.
	 */
	private static void migrateEODefinitions() {
		String fileName = project.getLocation().toOSString() + File.separator
				+ ICWProject.CW_PROJECT_CONFIG_DIR_NAME + File.separator
				+ MEMORYCONFIG_XML_DATA_FILENAME;

		if (!isUpdateRequired(fileName)) {
			return;
		}

		String strToBeReplaced[] = {
				"<MemoryConfiguration xmlns=\"memoryConfigurations.ecore\">",
				"</MemoryConfiguration>", "memConfigPool" };
		String strReplacement[] = {
				"<eoDefinitions xmlns=\"eoDefinitions.ecore\">\n<memoryConfiguration>",
				"</memoryConfiguration>\n  <iocConfiguration>\n    <iocConfigPool/>\n  </iocConfiguration>\n</eoDefinitions>",
				"memoryConfigPool" };

		migrateFile(fileName, strToBeReplaced, strReplacement, new String[] {},
				new String[] {}, new String[] {}, new String[] {});

//		ProjectDataModel.getProjectDataModel(project).createEODefinitions();
	}

	/**
	 * Migrates the IOCConfig xml file.
	 */
	private static void migrateIOCConfig() {
		String fileName = project.getLocation().toOSString() + File.separator
		+ ICWProject.CW_PROJECT_CONFIG_DIR_NAME + File.separator
		+ IOCBOOT_XML_DATA_FILENAME;

		if (!isUpdateRequired(fileName)) {
			return;
		}

		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
		EObject bootConfigObject = (EObject) pdm.getIOCConfigList().get(0);
		EObject iocObject = (EObject) EcoreUtils.getValue(bootConfigObject,
				SafConstants.IOC);

		EClass queueClass = (EClass) iocObject.eClass().getEPackage()
				.getEClassifier(SafConstants.QUEUE_ECLASS);
		EObject sendQueueObj = EcoreUtils.createEObject(queueClass, true);
		EObject receiveQueueObj = EcoreUtils.createEObject(queueClass, true);

		EStructuralFeature sendQueueRef = iocObject.eClass()
				.getEStructuralFeature(SafConstants.SEND_QUEUE);
		iocObject.eSet(sendQueueRef, sendQueueObj);
		EStructuralFeature receiveQueueRef = iocObject.eClass()
				.getEStructuralFeature(SafConstants.RECEIVE_QUEUE);
		iocObject.eSet(receiveQueueRef, receiveQueueObj);

		EClass nodeInstancesClass = (EClass) iocObject.eClass().getEPackage()
				.getEClassifier(SafConstants.NODE_INSTANCES_ECLASS);
		EObject nodeInstancesObj = EcoreUtils.createEObject(nodeInstancesClass,
				true);
		EStructuralFeature nodeInstancesRef = iocObject.eClass()
				.getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
		iocObject.eSet(nodeInstancesRef, nodeInstancesObj);

		try {
			EcoreModels.save(iocObject.eResource());
		} catch (IOException e) {
			LOG.error("Migration : Error migrating " + fileName + ".", e);
		}

		NodeProfileDialog.storeIOCConfiguration(pdm);
	}

	/**
	 * Migrates the CompileCinfigs xml file.
	 */
	private static void migrateCompileConfigs() {
		String fileName = project.getLocation().toOSString() + File.separator
				+ ICWProject.CW_PROJECT_CONFIG_DIR_NAME + File.separator
				+ COMPILE_CONFIGS_XMI_FILENAME;

		if (!isUpdateRequired(fileName)) {
			return;
		}

		String strToBeReplaced[] = { "^([\\s]*)(<IOC)([ \"=a-zA-Z0-9]*)(/>)$" };
		String strReplacement[] = { "" };

		migrateFile(fileName, strToBeReplaced, strReplacement, new String[] {},
				new String[] {}, new String[] {}, new String[] {});
	}
	
	/**
	 * Migrates the Log config xml file.
	 */
	private static void migrateLogConfig() {
		File logConfigFile = project.getLocation().append(
				CW_PROJECT_CONFIG_DIR_NAME).append("log.xml")
				.toFile();

		if (logConfigFile.exists()) {
			logConfigFile.delete();
		}

		URL xmiURL = FileLocator.find(DataPlugin.getDefault().getBundle(),
				new Path(PLUGIN_XML_FOLDER + File.separator
						+ LOG_DEFAULT_CONFIGS_XML_FILENAME), null);

		try {
			Path xmiPath = new Path(FileLocator.resolve(xmiURL).getPath());
			String dataFilePath = CW_PROJECT_CONFIG_DIR_NAME + File.separator
					+ "log.xml";

			IFile dst = project.getFile(new Path(dataFilePath));
			dst.getParent().refreshLocal(1, null);
			dst.create(new FileInputStream(xmiPath.toOSString()), true, null);

		} catch (IOException e) {
			LOG.error("Migration : Error migrating " + logConfigFile.getName() + ".", e);

		} catch (CoreException e) {
			LOG.error("Migration : Error migrating " + logConfigFile.getName() + ".", e);
		}
	}

	/**
	 * Migrates the file.
	 * 
	 * @param fileName
	 * @param strToBeReplaced
	 * @param strReplacement
	 * @param attrAppendTag
	 * @param attrAppendStr
	 * @param elmAppendTag
	 * @param elmAppendStr
	 */
	private static void migrateFile(String fileName, String strToBeReplaced[],
			String strReplacement[], String attrAppendTag[],
			String attrAppendStr[], String elmAppendTag[],
			String elmAppendStr[]) {

		String tempFile = project.getLocation().toOSString() + File.separator + "temp.tmp";

		try {
			BufferedReader br = new BufferedReader(new FileReader(fileName));
			BufferedWriter bw = new BufferedWriter(new FileWriter(tempFile));

			String str = null;
			while ((str = br.readLine()) != null) {

				for (int i = 0; i < strToBeReplaced.length; i++) {
					str = str.replaceAll(strToBeReplaced[i], strReplacement[i]);
				}

				for (int i = 0; i < attrAppendTag.length; i++) {
					if (str.contains("<" + attrAppendTag[i])) {
						if(str.endsWith("/>")) {
							str = str.replace("/>", attrAppendStr[i] + "/>");
						} else {
							str = str.replace(">", attrAppendStr[i] + ">");
						}
						break;
					}
				}

				for (int i = 0; i < elmAppendTag.length; i++) {
					if (str.contains("</" + elmAppendTag[i] + ">")) {
						str = elmAppendStr[i] + str;
						break;
					}
				}

				bw.write(str);
				bw.newLine();
			}

			bw.close();
			new File(tempFile).renameTo(new File(fileName));

		} catch (FileNotFoundException e) {
			LOG.warn("Migration : File " + fileName + " not found.", e);

		} catch (IOException e) {
			LOG.error("Migration : Error migrating " + fileName + ".", e);
		}
	}

	/**
	 * Checks whether the file needs to be updated for the migration or not.
	 * 
	 * @param fileName
	 *            the Name of the File.
	 * @return true if updatation required, false otherwise
	 */
	private static boolean isUpdateRequired(String fileName) {

		if (fileName.endsWith(RESOURCE_XML_DATA_FILENAME)) {
			return fileContainsString(fileName, "isTransient");

		} else if (fileName.endsWith(COMPONENT_XMI_DATA_FILENAME)) {
			return !fileContainsString(fileName, "alphaFactor");

		} else if (fileName.endsWith(EOCONFIG_XML_DATA_FILENAME)) {
			return !fileContainsString(fileName, "eoIocConfig");

		} else if (fileName.endsWith(MEMORYCONFIG_XML_DATA_FILENAME)) {
			return fileContainsString(fileName, "MemoryConfiguration");

		} else if (fileName.endsWith(IOCBOOT_XML_DATA_FILENAME)) {
			return !fileContainsString(fileName, "sendQueue");

		} else if (fileName.endsWith(COMPILE_CONFIGS_XMI_FILENAME)) {
			return fileContainsString(fileName, "IOC");
		}

		return false;
	}

	/**
	 * Checks whether the file is having the given string.
	 * 
	 * @param fileName
	 * @param checkStr
	 * @return
	 */
	private static boolean fileContainsString(String fileName, String checkStr) {
		try {
			BufferedReader br = new BufferedReader(new FileReader(fileName));
			String str = null;

			while ((str = br.readLine()) != null) {
				if (str.contains(checkStr)) {
					return true;
				}
			}

			return false;

		} catch (FileNotFoundException e) {
			LOG.warn("Migration : File " + fileName + " not found.", e);

		} catch (IOException e) {
			LOG.error("Migration : Error migrating " + fileName + ".", e);
		}
		return false;
	}
	
	/**
	 * Prepends nodeMoId path to moId in <nodeInstanceName>_rt.xml 
	 * corresponding to each nodeInstance
	 */
	public static void prependFullPathToMoid(){
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
		EObject amfConfigObj = (EObject) pdm.getNodeProfiles().getEList().get(0);
		EObject nodeInstancesObj = (EObject) EcoreUtils.getValue(amfConfigObj, SafConstants.NODE_INSTANCES_NAME);
		List nodeInstObjList = (List) EcoreUtils.getValue(nodeInstancesObj, SafConstants.NODE_INSTANCELIST_NAME);
		
		HashMap nodeInstMoidMap = new HashMap();
		for (int i = 0; i < nodeInstObjList.size(); i++) {
			EObject nodeInstObj = (EObject) nodeInstObjList.get(i);
			String nodeInstName = EcoreUtils.getName(nodeInstObj);
			String moId = (String)EcoreUtils.getValue(nodeInstObj, "nodeMoId");
			nodeInstMoidMap.put(nodeInstName, moId);
		}
		
		for (int i = 0; i < nodeInstObjList.size(); i++) {
			
			EObject nodeInstObj = (EObject) nodeInstObjList.get(i);
			String nodeInstName = EcoreUtils.getName(nodeInstObj);
			Resource rtResource = NodeProfileDialog.getCompResResource(nodeInstName, false, pdm);
			
			if (rtResource != null && rtResource.getContents().size() > 0) {
				EObject compInstancesObj = (EObject) rtResource.getContents().get(0);
				List compInstObjList = (List) EcoreUtils.getValue(compInstancesObj,"compInst");
				
				for (int j = 0; compInstObjList != null && j < compInstObjList.size(); j++) {
					EObject compInstObj = (EObject) compInstObjList.get(j);
					List resObjList = (List) EcoreUtils.
							getValue(compInstObj, SafConstants.RESOURCELIST_NAME);
					
					for(int k = 0; resObjList != null && k < resObjList.size(); k++){
						EObject resObj = (EObject) resObjList.get(k);
						EcoreUtils.setValue(resObj, "primaryOI", "true");
						String moId = (String) EcoreUtils.getValue(resObj, "moID");
						String newMoId = new String();
						newMoId = (String) nodeInstMoidMap.get(nodeInstName);
						
						if( moId != null && !moId.equals("") && !moId.startsWith(newMoId)){
							if(!moId.equals("\\")) {
								newMoId += moId;
							}
							EcoreUtils.setValue(resObj, "moID", newMoId);
							
						}
					}
					
				}
			}
			try {
				EcoreModels.save(rtResource);
			} catch (IOException e) {
				LOG.error("Migration : Error migrating " + rtResource.getURI().path() + ".", e);
			}
		}
	}
	
	/**
     * Create Default Config Files.
     * @param updateFlag 
     *
     * @throws CoreException
     * @throws IOException
     */
    public static void createDefaultConfigFiles(boolean updateFlag)
        throws CoreException, IOException
    {
        
        File clEoConfigFile = project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                		EOCONFIG_XML_DATA_FILENAME).toFile();
        if (!clEoConfigFile.exists()) {
            URL xmiURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + EOCONFIG_DEFAULT_XML_DATA_FILENAME));
            
            Path xmiPath = new Path(Platform.resolve(xmiURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + EOCONFIG_XML_DATA_FILENAME;
            IFile dst = project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmiPath.toOSString()), true, null);

            if(updateFlag) {
                ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
        		EObject compModel = (EObject) pdm.getComponentModel().getEList().get(0);
        		List compList = (List) compModel.eGet(compModel.eClass()
        				.getEStructuralFeature("safComponent"));

                pdm.createEOConfiguration();
        		Model eoConfiguration = pdm.getEOConfiguration();
        		EObject eoList = (EObject) eoConfiguration.getEList().get(0);

        		Iterator itr = compList.iterator();
        		while (itr.hasNext()) {
        			EObject newComp = EcoreUtils.createEObject(
        					(EClass) eoList.eClass()
        							.getEStructuralFeature("eoConfig").getEType(),
        					true);

        			EcoreUtils.setValue(newComp, "name", EcoreUtils
							.getName((EObject) EcoreUtils.getValue(
									(EObject) itr.next(), "eoProperties")));
        			((List) EcoreUtils.getValue(eoList, "eoConfig")).add(newComp);
        		}
        		EcoreModels.save(eoConfiguration.getResource());
            }
        }
        
        File clEoDefinitionsFile = project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                		MEMORYCONFIG_XML_DATA_FILENAME).toFile();
        if (!clEoDefinitionsFile.exists()) {
            URL xmiURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + MEMORYCONFIG_DEFAULT_XML_DATA_FILENAME));
            
            Path xmiPath = new Path(Platform.resolve(xmiURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + MEMORYCONFIG_XML_DATA_FILENAME;
            IFile dst = project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmiPath.toOSString()), true, null);

            if(updateFlag) {
                ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
                pdm.createEODefinitions();
    		}
        }
        
        File clIocConfigFile = project.getLocation().append(
                CW_PROJECT_CONFIG_DIR_NAME).append(
                        IOCBOOT_XML_DATA_FILENAME).toFile();
        if (!clIocConfigFile.exists()) {
            URL xmiURL = DataPlugin.getDefault().find(
                new Path(PLUGIN_XML_FOLDER + File.separator
                    + IOCBOOT_DEFAULT_XML_DATA_FILENAME));
            
            Path xmiPath = new Path(Platform.resolve(xmiURL).getPath());
            String dataFilePath =
                CW_PROJECT_CONFIG_DIR_NAME
                + File.separator + IOCBOOT_XML_DATA_FILENAME;
            IFile dst = project.getFile(new Path(dataFilePath));
            dst.create(new FileInputStream(xmiPath.toOSString()), true, null);
        }
    }
}
