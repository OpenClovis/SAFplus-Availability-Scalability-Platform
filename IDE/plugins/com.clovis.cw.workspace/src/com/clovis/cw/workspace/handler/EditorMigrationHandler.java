package com.clovis.cw.workspace.handler;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.URL;
import java.util.HashMap;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IPath;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.data.MigrationConstants;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.project.data.SubModelMapReader;
import com.clovis.cw.project.utils.FormatConversionUtils;
import com.clovis.cw.workspace.project.migration.MigrationManager;
import com.clovis.cw.workspace.project.migration.ResourceCopyHandler;

/**
 * 
 * @author shubhada
 * Specific Resource Editor data migration handler to handle the change in heirarchy
 * (i.e all the flat objects in R2.2 have be made to be contained under a toplevel
 * object now)
 */
public class EditorMigrationHandler
{
	private static final Log LOG = Log.getLog(DataPlugin.getDefault());
	public static void migrateResource(IProject project, Resource newResource, Resource oldResource,
			EObject changeInfoObj, EPackage ePack, String oldVersion)
	{
		String infoClassName = "";
		String editorType = "";
		String oldEcorePath = (String) EcoreUtils.getValue(changeInfoObj,
				MigrationConstants.MIGRATION_OLD_ECOREPATH);
		if (oldEcorePath.indexOf(ICWProject.
				RESOURCE_ECORE_FILENAME) != -1) {
			infoClassName = ClassEditorConstants.RESOURCE_INFO_CLASS;
			editorType = FormatConversionUtils.RESOURCE_EDITOR;
			
		} else if (oldEcorePath.indexOf(ICWProject.
				COMPONENT_ECORE_FILENAME) != -1) {
			infoClassName = ComponentEditorConstants.COMPONENT_INFO_CLASS;
			editorType = FormatConversionUtils.COMPONENT_EDITOR;
			
		}
		List retList = new Vector();
		if (!infoClassName.equals("")) {
			EClass objInfoClass = (EClass) ePack.getEClassifier(
					infoClassName);
			EObject infoObj = EcoreUtils.createEObject(
					objInfoClass, true);
			newResource.getContents().add(infoObj);
			HashMap destSrcMap = addObjectsToModel(oldResource.getContents(), infoObj);
			destSrcMap.put(infoObj, null);
			ResourceCopyHandler copyHandler = new ResourceCopyHandler(changeInfoObj);
	        copyHandler.copyResourceInfo(destSrcMap, retList);
	        FormatConversionUtils.convertToResourceFormat(infoObj, editorType);
	        MigrationManager.saveResource(newResource);
	        if (oldEcorePath.indexOf(ICWProject.
					RESOURCE_ECORE_FILENAME) != -1) {
				
				// create the resource and alarm map file
				createResourceAlarmMapResource(project, oldResource, oldVersion);
			} else if (oldEcorePath.indexOf(ICWProject.
					COMPONENT_ECORE_FILENAME) != -1) {
				// create the component and resource map file
				createComponentResourceMapResource(project, oldResource, oldVersion);
			}
	        // Delete the files componentui.xml and resourceui.xml
	        IPath modelsFolder  = new Path(ICWProject.CW_PROJECT_MODEL_DIR_NAME);
	       IFile componentUiXml = project.getFile(
	        		modelsFolder.append(ICWProject.COMPONENT_UI_PROPRTY_FILENAME));
            if (componentUiXml.exists()) {
            	try {
					componentUiXml.delete(true, true, null);
				} catch (CoreException e) {
					System.out.println(e.getMessage());
				}
            }
            IFile resourceUiXml = project.getFile(
	        		modelsFolder.append(ICWProject.RESOURCE_UI_PROPRTY_FILENAME));
            if (resourceUiXml.exists()) {
            	try {
            		resourceUiXml.delete(true, true, null);
				} catch (CoreException e) {
					System.out.println(e.getMessage());
				}
            }
            
            IPath configsFolder  = new Path(ICWProject.CW_PROJECT_CONFIG_DIR_NAME);
            IFile oldCompileConfigsXml = project.getFile(
            		configsFolder.append("CompileConfigs.xmi"));
            File oldCompileConfigsFile = new File(oldCompileConfigsXml.getLocation().toOSString());
            IFile newCompileConfigsXml = project.getFile(
            		configsFolder.append("compileconfigs.xml"));
            if (oldCompileConfigsXml.exists()) {
            	try {
					newCompileConfigsXml.create(new FileInputStream(oldCompileConfigsFile), true, null);
					oldCompileConfigsXml.delete(true, true, null);
				} catch (FileNotFoundException e) {
					System.out.println(e.getMessage());
				} catch (CoreException e) {
					System.out.println(e.getMessage());
				}
            }
		}
	}
	/**
	 * Creates the resource to alarm map file from the 
	 * old version format data
	 * 
	 * @param project - IProject
	 * @param oldResource - old resource from which data to be migrated
	 * @param oldVersion - old Version from which map file
	 *  to be generated
	 */
	private static void createResourceAlarmMapResource(IProject project,
			Resource oldResource, String oldVersion)
	{
		HashMap keyAlarmMap = getAlarmData(project, oldVersion);
		SubModelMapReader reader = SubModelMapReader.getSubModelMappingReader(
        		(IProject) project, "resource", "alarm");
        EObject linkObj = reader.getLinkObject(
        		"associatedAlarm");
        if (linkObj == null) {
        	linkObj = reader.createLink(
        			"associatedAlarm");
        }
        Model resourceAlarmMapModel = reader.getMapModel();
		List resourceList = oldResource.getContents();
		for (int i = 0; i < resourceList.size(); i++) {
			EObject resource = (EObject) resourceList.get(i);
			if (ResourceDataUtils.isNode(resource)) {
				EObject alarmMgmtObj = (EObject) EcoreUtils.getValue(
						resource, "AlarmManagement");
				if (alarmMgmtObj != null) {
					List alarmIds = (List) EcoreUtils.getValue(
							alarmMgmtObj, "AlarmIDs");
					List linkTargets = reader.createLinkTargets(
							ClassEditorConstants.ASSOCIATED_ALARM_LINK,
							EcoreUtils.getName(resource));
					for (int j = 0; j < alarmIds.size(); j++) {
						EObject alarm = (EObject) keyAlarmMap.get(alarmIds.get(j));
						if (alarm != null) {
							linkTargets.add(EcoreUtils.getValue(
									alarm, "AlarmID"));
							
						}
					}
				}
			}
		}
		resourceAlarmMapModel.save(true);

		
	}
	/**
	 * 
	 * @param project - IProject
	 * @param oldVersion - old Version from which map file to be generated
	 * @return the map of CWKEY and alarm objects
	 */
	private static HashMap getAlarmData(IProject project, String oldVersion)
	{
		HashMap keyAlarmMap = new HashMap();
		String ecorePath = "model_" + oldVersion + File.separator +  "alarm.ecore";
		URL url = FileLocator.find(DataPlugin.getDefault().getBundle(), new Path(ecorePath), null);
        try {
            File ecoreFile = new Path(Platform.resolve(url).getPath())
                    .toFile();
            EcoreModels.getUpdated(ecoreFile.getAbsolutePath());
        } catch (IOException e) {
            LOG.warn("Cannot read ecore file", e);
        }
        
        try {
            String resourcePath = ICWProject.CW_PROJECT_MODEL_DIR_NAME +
            	File.separator + "alarmdata.xmi";

            //Now get the xmi file from the project 
            String dataFilePath = project.getLocation().toOSString()
                                  + File.separator + resourcePath;
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);
            
            if (xmiFile.exists()) { 
                Resource resource = EcoreModels.getUpdatedResource(uri);
                List alarmsList = resource.getContents();
                for (int i = 0; i < alarmsList.size(); i++) {
                	EObject alarm = (EObject) alarmsList.get(i);
                	String alarmKey = EcoreUtils.getValue(alarm, "CWKEY").toString();
                	keyAlarmMap.put(alarmKey, alarm);
                }
                
            }
            
        } catch (Exception exception) {
            LOG.warn("Failed while loading resource", exception);
        }
		return keyAlarmMap;
		
		
	}
	
	/**
	 * Creates the component to resource map file from the 
	 * old version format data
	 * 
	 * @param project - IProject
	 * @param oldResource - old resource from which data to be migrated
	 * @param oldVersion - old Version from which map file
	 *  to be generated
	 */
	private static void createComponentResourceMapResource(IProject project,
			Resource oldResource, String oldVersion)
	{
		HashMap keyResMap = getResourceData(project, oldVersion);
		SubModelMapReader reader = SubModelMapReader.getSubModelMappingReader(
        		(IProject) project,"component", "resource");
        EObject linkObj = reader.getLinkObject(
        		"associatedResource");
        if (linkObj == null) {
        	linkObj = reader.createLink(
        			"associatedResource");
        }
        Model componentResourceMapModel = reader.getMapModel();
		List compList = oldResource.getContents();
		for (int i = 0; i < compList.size(); i++) {
			EObject comp = (EObject) compList.get(i);
			if (comp.eClass().getName().equals(ComponentEditorConstants.
					SAFCOMPONENT_NAME)) {
				List associatedResList = (List) EcoreUtils.getValue(
						comp, "Realizes");
				List linkTargets = reader.createLinkTargets(
						ComponentEditorConstants.ASSOCIATE_RESOURCES_NAME,
						EcoreUtils.getName(comp));
				for (int j = 0; j < associatedResList.size(); j++) {
					EObject associatedRes = (EObject) keyResMap.get(associatedResList.get(j));
					if (associatedRes != null) {
						linkTargets.add(EcoreUtils.getName(associatedRes));
						
					}
				}
				
			}
		}
		componentResourceMapModel.save(true);

		
	}
	/**
	 * 
	 * @param project - IProject
	 * @param oldVersion - old Version from which map file to be generated
	 * @return the map of CWKEY and resource objects
	 */
	private static HashMap getResourceData(IProject project, String oldVersion)
	{
		HashMap keyResourceMap = new HashMap();
		String ecorePath = "model_" + oldVersion +  File.separator + "resource.ecore";
		URL url = FileLocator.find(DataPlugin.getDefault().getBundle(), new Path(ecorePath), null);
        try {
            File ecoreFile = new Path(Platform.resolve(url).getPath())
                    .toFile();
            EcoreModels.getUpdated(ecoreFile.getAbsolutePath());
        } catch (IOException e) {
            LOG.warn("Cannot read ecore file", e);
        }
        
        try {
            String resourcePath = ICWProject.CW_PROJECT_MODEL_DIR_NAME +
            	File.separator + "resourcedata.xmi";

            //Now get the xmi file from the project 
            String dataFilePath = project.getLocation().toOSString()
                                  + File.separator + resourcePath;
            URI uri = URI.createFileURI(dataFilePath);
            File xmiFile = new File(dataFilePath);
            
            if (xmiFile.exists()) { 
                Resource resource = EcoreModels.getUpdatedResource(uri);
                List resourcesList = resource.getContents();
                for (int i = 0; i < resourcesList.size(); i++) {
                	EObject res = (EObject) resourcesList.get(i);
                	String resKey = EcoreUtils.getValue(res, "CWKEY").toString();
                	keyResourceMap.put(resKey, res);
                }
                
            }
            
        } catch (Exception exception) {
            LOG.warn("Failed while loading resource", exception);
        }
		return keyResourceMap;
		
		
	}


	/**
     * Adds objects in to the appropriate list contained in top level object
     * 
     * @param eObjects - EObjects to be added in to editor model
     * @param topObj - Top level object of the model
     */
    private static HashMap addObjectsToModel(List eObjects, EObject topObj)
    {
    	HashMap destSrcMap = new HashMap();
        for (int i = 0; i < eObjects.size(); i++) {
            EObject eobj = (EObject) eObjects.get(i);
            List refList = topObj.eClass().getEAllReferences();
            for (int j = 0; j < refList.size(); j++) {
                EReference ref = (EReference) refList.get(j);
                if (eobj.eClass().getName().equals(ref.
                        getEReferenceType().getName())) {
                    Object val = topObj.eGet(ref);
                    if (ref.getUpperBound() == 1) {
                    	EObject refObj = EcoreUtils.createEObject(ref.
                    			getEReferenceType(), true); 
                        topObj.eSet(ref, refObj);
                        destSrcMap.put(refObj, eobj);
                    } else if (ref.getUpperBound() == -1
                            || ref.getUpperBound() > 1) {
                    	EObject refObj = EcoreUtils.createEObject(ref.
                    			getEReferenceType(), true); 
                        ((List) val).add(refObj);
                        destSrcMap.put(refObj, eobj);
                    }
                
                }
            }
        }
        return destSrcMap;
    }

}
