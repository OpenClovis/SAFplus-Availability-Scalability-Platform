/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/UpdateProjectAction.java $
 * $Author: bkpavan $
 * $Date: 2007/01/25 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;

import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IWorkspace;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.Path;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IViewActionDelegate;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
//import com.clovis.cw.editor.ca.dialog.TemplateGroupConfigurationDialog;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.dialog.TemplateGroupConfigurationDialog;
import com.clovis.cw.workspace.project.FolderCreator;

/**
 * @author nadeem
 *
 * Updates sctits and build xml on users request.
 */
public class UpdateProjectAction extends CommonMenuAction implements
		IViewActionDelegate, IWorkbenchWindowActionDelegate, ICWProject {
   
		
	//List of folders to be updated in Project.
    private static final String[] FOLDERS = new String[] {
    	CW_PROJECT_CONFIG_DIR_NAME,
        CW_PROJECT_SCRIPT_DIR_NAME,
        CW_PROJECT_IDL_DIR_NAME,
        CW_PROJECT_TEMPLATE_DIR_NAME
    };
    /**
     * Initializes the View.
     * @param view ViewPart
     */
    public void init(IViewPart view){
    }

	public void dispose() {
		
	}

	public void init(IWorkbenchWindow window) {
	
	}
	public void run(IAction action)
	{
		if (_project != null) {
			try {
				FolderCreator fd = new FolderCreator(_project);
				createFoldersForUpdate();
				fd.copyScript();
				fd.copyTemplates();
				fd.createDefaultConfigFiles(true);
				migrateProject();
				fd.copyCodeGenFiles();
			} catch (Exception e) {
				WorkspacePlugin.LOG.error("Failed to update project.", e);
			}
		}
	}

	/**
	 * Migrates the project.
	 */
	private void migrateProject() {
		prependFullPathToMoid();
		migrateResourceData();
		migrateComponentData();
		migrateEOConfig();
		migrateEODefinitions();
		migrateIOCConfig();
		migrateCompileConfigs();
		updateProjectVersion();
	}

	/**
	 * Updates the project version.
	 */
	private void updateProjectVersion() {
		IWorkspace workspace = ResourcesPlugin.getWorkspace();
        final IProjectDescription description = workspace
                .newProjectDescription(_project.getName());

        String version = DataPlugin.getDefault().getProductVersion();
        if (version != null) {
            description.setComment("Project Version:" + version);
        }
	}

	/**
	 * Migrates the Resource Data xml file.
	 */
	private void migrateResourceData() {
		String fileName = _project.getLocation().toOSString() + File.separator
				+ ICWProject.CW_PROJECT_MODEL_DIR_NAME + File.separator
				+ RESOURCE_XML_DATA_FILENAME;

		if (!isUpdateRequired(fileName)) {
			return;
		}

		String strToBeReplaced[] = { " isTransient=\"true\"",
				" isTransient=\"false\"", "type" };
		String strReplacement[] = { "", "", "dataType" };

		String attrAppendTag[] = { "attribute" };
		String attrAppendStr[] = { " type=\"CONFIG\"" };

		migrateFile(fileName, strToBeReplaced, strReplacement, attrAppendTag,
				attrAppendStr, new String[] {}, new String[] {});

		ProjectDataModel.getProjectDataModel(_project).createCAModel();
	}

	/**
	 * Migrates the Component Data xml file.
	 */
	private void migrateComponentData() {
		String fileName = _project.getLocation().toOSString() + File.separator
				+ ICWProject.CW_PROJECT_MODEL_DIR_NAME + File.separator
				+ COMPONENT_XMI_DATA_FILENAME;

		if (!isUpdateRequired(fileName)) {
			return;
		}

		String attrAppendTag[] = { "serviceGroup" };
		String attrAppendStr[] = { " isCollocationAllowed=\"true\" alphaFactor=\"100\"" };

		migrateFile(fileName, new String[] {}, new String[] {}, attrAppendTag,
				attrAppendStr, new String[] {}, new String[] {});

		ProjectDataModel.getProjectDataModel(_project).createComponentModel();
	}

	/**
	 * Migrates the EOConfig xml file.
	 */
	private void migrateEOConfig() {
		String fileName = _project.getLocation().toOSString() + File.separator
				+ ICWProject.CW_PROJECT_CONFIG_DIR_NAME + File.separator
				+ EOCONFIG_XML_DATA_FILENAME;

		if (!isUpdateRequired(fileName)) {
			return;
		}

		String strToBeReplaced[] = { "memConfig" };
		String strReplacement[] = { "memoryConfig" };

		String elmAppendTag[] = { "eoConfig" };
		String elmAppendStr[] = { "    <eoIocConfig/>\n" };

		migrateFile(fileName, strToBeReplaced, strReplacement,
				new String[] {}, new String[] {}, elmAppendTag, elmAppendStr);

		ProjectDataModel.getProjectDataModel(_project).createEOConfiguration();
	}

	/**
	 * Migrates the EODefinitions xml file.
	 */
	private void migrateEODefinitions() {
		String fileName = _project.getLocation().toOSString() + File.separator
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

		ProjectDataModel.getProjectDataModel(_project).createEODefinitions();
	}

	/**
	 * Migrates the IOCConfig xml file.
	 */
	private void migrateIOCConfig() {
		String fileName = _project.getLocation().toOSString() + File.separator
		+ ICWProject.CW_PROJECT_CONFIG_DIR_NAME + File.separator
		+ IOCBOOT_XML_DATA_FILENAME;

		if (!isUpdateRequired(fileName)) {
			return;
		}

		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
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
			WorkspacePlugin.LOG.error("Failed to update project.", e);
		}

		NodeProfileDialog.storeIOCConfiguration(pdm);
	}

	/**
	 * Migrates the CompileCinfigs xml file.
	 */
	private void migrateCompileConfigs() {
		String fileName = _project.getLocation().toOSString() + File.separator
				+ ICWProject.CW_PROJECT_CONFIG_DIR_NAME + File.separator
				+ COMPILE_CONFIGS_XMI_FILENAME;

		if (!isUpdateRequired(fileName)) {
			return;
		}

		String strToBeReplaced[] = {
				"^([\\s]*)(<IOC)([ \"=a-zA-Z0-9]*)(/>)$" };
		String strReplacement[] = { "" };

		migrateFile(fileName, strToBeReplaced, strReplacement, new String[] {},
				new String[] {}, new String[] {}, new String[] {});
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
	private void migrateFile(String fileName, String strToBeReplaced[],
			String strReplacement[], String attrAppendTag[],
			String attrAppendStr[], String elmAppendTag[],
			String elmAppendStr[]) {

		String tempFile = _project.getLocation().toOSString() + File.separator + "temp.tmp";

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
			WorkspacePlugin.LOG.error("Failed to update project.", e);

		} catch (IOException e) {
			WorkspacePlugin.LOG.error("Failed to update project.", e);
		}
	}

	/**
	 * Checks whether the file needs to be updated for the migration or not.
	 * 
	 * @param fileName
	 *            the Name of the File.
	 * @return true if updatation required, false otherwise
	 */
	private boolean isUpdateRequired(String fileName) {

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
	private boolean fileContainsString(String fileName, String checkStr) {
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
			WorkspacePlugin.LOG.error("Failed to update project.", e);

		} catch (IOException e) {
			WorkspacePlugin.LOG.error("Failed to update project.", e);
		}
		return false;
	}

	/**
	 * Creates the folders to update scripts and build files (if needed)
	 * 
	 * @throws CoreException
	 * @throws IOException
	 */
	public void createFoldersForUpdate()
    throws CoreException, IOException
    {
	
		for (int i = 0; i < FOLDERS.length; i++) {
			
			if(!(_project.getFolder(new Path(FOLDERS[i])).exists())){
				_project.getFolder(new Path(FOLDERS[i])).create(true, true, null);
			}
		}
		/*if(!(_project.getFolder(
				new Path(CW_PROJECT_TEMPLATE_DIR_NAME)
						.append(CW_PROJECT_DEFAULT_TEMPLATE_GROUP_DIR_NAME)).exists())){
		_project.getFolder(
				new Path(CW_PROJECT_TEMPLATE_DIR_NAME)
						.append(CW_PROJECT_DEFAULT_TEMPLATE_GROUP_DIR_NAME))
				.create(true, true, null);
		}*/
	}
	/**
	 * Prepends nodeMoId path to moId in <nodeInstanceName>_rt.xml 
	 * corresponding to each nodeInstance
	 */
	public void prependFullPathToMoid(){
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
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
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
}
