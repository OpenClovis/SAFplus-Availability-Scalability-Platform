package com.clovis.cw.workspace.project;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.common.utils.ui.factory.ValidatorFactory;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.ComponentValidator;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.ResourceValidator;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.project.data.ProjectDataModel;
/**
 * 
 * @author shubhada
 * Class which checks the submodel problems 
 * and reports the errors if any
 */
public class SubModelProblemReporter
{
	public void reportSubModelProblems(IProject project)
	{
		ProjectDataModel pdm = ProjectDataModel.
			getProjectDataModel(project);
		validateResourceModel(pdm);
		validateComponentModel(pdm);
		validateAlarmModel(pdm);
		validateIocConfiguration(pdm);
		validateGMSConfiguration(pdm);
		validateASPBuildConfig(pdm);
		validateAMFConfiguration(pdm);
		validateIDLConfiguration(pdm);
		
	}

	private void validateResourceModel(ProjectDataModel pdm)
	{
		String message = null;
		String resourcePath = ICWProject.CW_PROJECT_MODEL_DIR_NAME +
			File.separator +ICWProject.RESOURCE_XML_DATA_FILENAME;
	
		Model caModel = pdm.getCAModel();
		if (caModel.getEList().isEmpty()) {
			message = resourcePath + " : File does not contain any data";
			System.err.print("\n ERROR : " + "[NONE] : " + message);
			return;
		}
		
		EObject rootObject = (EObject) caModel.getEList().get(0);
		
		checkForDuplicatesAndPattern(rootObject, resourcePath);
		ResourceValidator modelValidator = new ResourceValidator(caModel, null);
		ResourceDataUtils utils = new ResourceDataUtils(caModel.getEList());
		String [] refs = ClassEditorConstants.EDGES_REF_TYPES;
		for (int i = 0; i < refs.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refs[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				EObject src = utils.getSource(eobj);
				EObject target = utils.getTarget(eobj);
				message = null;
				message = modelValidator.isConnectionValid(src, target, eobj);
				if (message != null) {
					System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
				}
			}
		}
		refs = ClassEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refs.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refs[i]);
			if(ref.getName().equals(ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME)) {
				EList list = (EList) rootObject.eGet(ref);
				if(list.size() == 0) {
		            message = "Model should contain at least one Chassis Resource";
		            System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
		            
				} else if(list.size() > 1) {
                    message = "Model should not contain more than one Chassis Resource";
                    System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
                    
				}
			} 
		}
		
	}
	
	private void validateComponentModel(ProjectDataModel pdm)
	{
		Model compModel = pdm.getComponentModel();
		String resourcePath = ICWProject.CW_PROJECT_MODEL_DIR_NAME +
			File.separator +ICWProject.COMPONENT_XMI_DATA_FILENAME;
		String message = null;
		if (compModel.getEList().isEmpty()) {
			message = "File does not contain any data" + " : " + resourcePath;
			System.err.print("\n ERROR : " + "[NONE] : " + message);
			return;
		}
		EObject rootObject = (EObject) compModel.getEList().get(0);
		checkForDuplicatesAndPattern(rootObject, resourcePath);
		ComponentValidator modelValidator = new ComponentValidator(compModel, null);
		ComponentDataUtils utils = new ComponentDataUtils(compModel.getEList());
		String [] refs = ComponentEditorConstants.EDGES_REF_TYPES;
		for (int i = 0; i < refs.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refs[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				EObject src = utils.getSource(eobj);
				EObject target = utils.getTarget(eobj);
				message = null;
				message = modelValidator.isConnectionValid(src, target, eobj);
				if (message != null) {
					System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
				}
			}
		}
		refs = ComponentEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refs.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refs[i]);
			if(ref.getName().equals(ComponentEditorConstants.CLUSTER_REF_NAME)) {
				EList list = (EList) rootObject.eGet(ref);
				if(list.size() == 0) {
		            message = "Model should contain at least one Cluster";
		            System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
		            
				} else if(list.size() > 1) {
                    message = "Model should not contain more than one Cluster";
                    System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
				}
			} 
		}
	}
	private void validateAlarmModel(ProjectDataModel pdm)
	{
		Model alarmModel = pdm.getAlarmProfiles();
		String message = null;
		String resourcePath = ICWProject.CW_PROJECT_MODEL_DIR_NAME +
			File.separator +ICWProject.ALARM_PROFILES_XMI_DATA_FILENAME;
		if (alarmModel.getEList().isEmpty()) {
			message = "File does not contain any data" + " : " + resourcePath;
			System.err.print("\n ERROR : " + "[NONE] : " + message);
			return;
		}
		EObject rootObject = (EObject) alarmModel.getEList().get(0);
		checkForDuplicatesAndPattern(rootObject, resourcePath);
	}
	
	private void validateIocConfiguration(ProjectDataModel pdm)
	{
		String message = null;
		String resourcePath = ICWProject.CW_PROJECT_CONFIG_DIR_NAME
			+ File.separator + ICWProject.IOCBOOT_XML_DATA_FILENAME;
		List iocConfigList = pdm.getIOCConfigList();
		if (iocConfigList.isEmpty()) {
			message = "File does not contain any data" + " : " + resourcePath;
			System.err.print("\n ERROR : " + "[NONE] : " + message);
			return;
		}
		EObject iocObj = (EObject) EcoreUtils.getValue((EObject)
				iocConfigList.get(0), "ioc");
		EObject transportObj = (EObject) EcoreUtils.getValue(iocObj, "transport");
		message = ObjectValidator.checkPatternAndBlankValue(transportObj);
		if (message != null) {
			System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
		}
		List links = (List) EcoreUtils.getValue(transportObj, "link");
		for (int i = 0; i < links.size(); i++) {
			EObject link = (EObject) links.get(i);
			checkObject(links, link, resourcePath);
			/*List locationList = (List) EcoreUtils.getValue(link, "location");
			if (locationList != null) {
				for (int j = 0; j < locationList.size(); j++) {
					EObject location = (EObject) locationList.get(j);
					checkObject(locationList, location, resourcePath);
				}
			}*/
		}
		
	}
	
	private void validateGMSConfiguration(ProjectDataModel pdm)
	{
		String message = null;
		String resourcePath = ICWProject.CW_PROJECT_CONFIG_DIR_NAME
			+ File.separator + ICWProject.GMS_CONFIGURATION_XML_FILENAME;
		List gmsList = null;
		try {
			// reading the ecore here is written here mainly because
			// without ecore being in memory, resource cannot be read
			URL url = DataPlugin.getDefault()
			 .find(new Path("model" + File.separator + ICWProject
			                 .CW_ASP_COMPONENTS_CONFIGS_FOLDER_NAME
			                 + File.separator + ICWProject
			                 .CW_SYSTEM_COMPONENTS_FOLDER_NAME
			                 + File.separator + ICWProject.GMS_CONFIGURATION_ECORE_FILENAME));
			File ecoreFile = new Path(Platform.resolve(url).getPath())
			 	.toFile();
    	    EcoreModels.get(ecoreFile.getAbsolutePath());
    	    String dataFilePath = pdm.getProject().getLocation().toOSString()
    	    	+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
    	    	+ File.separator + ICWProject.GMS_CONFIGURATION_XML_FILENAME;
			URI uri = URI.createFileURI(dataFilePath);
			File xmlFile = new File(dataFilePath);
			Resource gmsResource = xmlFile.exists() ? EcoreModels.getUpdatedResource(uri)
					: EcoreModels.create(uri);
			gmsList = gmsResource.getContents();
		} catch (IOException e) {
			e.printStackTrace();
		}
		if (gmsList.isEmpty()) {
			message = "File does not contain any data" + " : " + resourcePath;
			System.err.print("\n ERROR : " + "[NONE] : " + message);
			return;
		}
		EObject bootConfigObj = (EObject) gmsList.get(0);
		EObject gmsObj = (EObject) EcoreUtils.getValue(bootConfigObj, "gms");
		message = ObjectValidator.checkPatternAndBlankValue(gmsObj);
		if (message != null) {
			System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
		}
	}

	
	private void validateASPBuildConfig(ProjectDataModel pdm)
	{
		String message = null;
		String resourcePath = ICWProject.CW_PROJECT_CONFIG_DIR_NAME
			+ File.separator + ICWProject.COMPILE_CONFIGS_XMI_FILENAME;
		List compConfigList = pdm.getBuildTimeComponentList();
		if (compConfigList.isEmpty()) {
			message = "File does not contain any data" + " : " + resourcePath;
			System.err.print("\n ERROR : " + "[NONE] : " + message);
			return;
		}
		EObject configObj = (EObject) compConfigList.get(0);
		List refs = configObj.eClass().getEAllReferences();
		for (int i = 0; i < refs.size(); i++) {
			EReference ref = (EReference) refs.get(i);
			Object val = configObj.eGet(ref);
			if (val instanceof EObject) {
				message = ObjectValidator.checkPatternAndBlankValue((EObject) val);
				if (message != null) {
					System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
				}
			}
		}
		EObject fmObj = (EObject) EcoreUtils.getValue(configObj, "FM");
		List localSeqTableEntries = (List) EcoreUtils.getValue(fmObj, "localSeqTableEntry");
		
		for (int i = 0; i < localSeqTableEntries.size(); i++) {
			EObject seqTableEntry = (EObject) localSeqTableEntries.get(i);
			checkObject(localSeqTableEntries, seqTableEntry, resourcePath);
		}
		
	}
	
	private void validateAMFConfiguration(ProjectDataModel pdm)
	{
		String message = null;
		String resourcePath = ICWProject.CW_PROJECT_CONFIG_DIR_NAME
			+ File.separator + ICWProject.CPM_XML_DATA_FILENAME;
		List amfConfigList = pdm.getNodeProfiles().getEList();
		if (amfConfigList.isEmpty()) {
			message = "File does not contain any data" + " : " + resourcePath;
			System.err.print("\n ERROR : " + "[NONE] : " + message);
			return;
		}
		EObject amfObj = (EObject) amfConfigList.get(0);
		//validate Node, SU and Component instances
		EReference nodeInstsRef = (EReference) amfObj.eClass()
        	.getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
		EObject nodeInstsObj = (EObject) amfObj.eGet(nodeInstsRef);
		if (nodeInstsObj != null) {
	        EReference nodeInstRef = (EReference) nodeInstsObj.eClass()
	            .getEStructuralFeature(SafConstants.NODE_INSTANCELIST_NAME);
	        List nodeObjs = (List) nodeInstsObj.eGet(nodeInstRef);
	        validateNodeInstances(nodeObjs, resourcePath);
		}
		//validate SG, SI and CSI instances
		EReference sgInstsRef = (EReference) amfObj.eClass()
        	.getEStructuralFeature(SafConstants.SERVICEGROUP_INSTANCES_NAME);
		EObject sgInstsObj = (EObject) amfObj.eGet(sgInstsRef);
		if (sgInstsObj != null) {
			EReference sgInstRef = (EReference) sgInstsObj.eClass().
				getEStructuralFeature(SafConstants.
						SERVICEGROUP_INSTANCELIST_NAME);
			List sgObjs = (List) sgInstsObj.eGet(sgInstRef);
			validateSGInstances(sgObjs, resourcePath);
		}
		
	}
	private void validateNodeInstances(List  nodeInsts, String resourcePath)
	{
		try {
	           for (int i = 0; i < nodeInsts.size(); i++) {
	               
	               EObject node = (EObject) nodeInsts.get(i);
	               checkObject(nodeInsts, node, resourcePath);
	               
                   EReference serviceUnitInstsRef = (EReference) node.eClass()
                   	    .getEStructuralFeature(SafConstants.SERVICEUNIT_INSTANCES_NAME);
                   EObject serviceUnitInstsObj = (EObject) node.eGet(serviceUnitInstsRef);
                   
                   if (serviceUnitInstsObj != null) {
                       EReference serviceUnitInstRef = (EReference) serviceUnitInstsObj.eClass()
                       		.getEStructuralFeature(SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
                       List suObjs = (List) serviceUnitInstsObj.eGet(serviceUnitInstRef);
                       
                       for (int j = 0; j < suObjs.size(); j++) {
                           EObject su = (EObject) suObjs.get(j);
                           checkObject(suObjs, su, resourcePath);
                           EReference componentInstsRef = (EReference) su.eClass()
                           	  .getEStructuralFeature(SafConstants.COMPONENT_INSTANCES_NAME);
                           EObject componentInstsObj = (EObject) su.eGet(componentInstsRef);
                           
                           if (componentInstsObj != null) {
                           
                               EReference componentInstRef = (EReference) componentInstsObj.eClass()
                               		.getEStructuralFeature(SafConstants.COMPONENT_INSTANCELIST_NAME);
                               List compObjs = (List) componentInstsObj.eGet(componentInstRef);
                               for (int k = 0; k < compObjs.size(); k++) {
                            	   EObject comp = (EObject) compObjs.get(k);
                            	   checkObject(compObjs, comp, resourcePath);
                               }
                           }   
                       }
                   }    
	           }
	       } catch (Exception e) {
	               e.printStackTrace();
	       }
		
	}
	
	private void validateSGInstances(List sgInsts, String resourcePath)
	{
		try {
	           for (int i = 0; i < sgInsts.size(); i++) {
	               
	               EObject sg = (EObject) sgInsts.get(i);
	               checkObject(sgInsts, sg, resourcePath);
                   EReference serviceInstanceInstsRef = (EReference) sg.eClass()
                       .getEStructuralFeature(SafConstants.SERVICE_INSTANCES_NAME);
                   EObject serviceInstancesObj = (EObject) sg.eGet(serviceInstanceInstsRef);
                   
                   if (serviceInstancesObj != null) {
                   
                       EReference serviceInstRef = (EReference) serviceInstancesObj.eClass()
                           .getEStructuralFeature(SafConstants.SERVICE_INSTANCELIST_NAME);
                       List siObjs = (List) serviceInstancesObj.eGet(serviceInstRef);
                       for (int j = 0; j < siObjs.size(); j++) {
                           
                           EObject si = (EObject) siObjs.get(j);
                           checkObject(siObjs, si, resourcePath);
                           EReference csiInstsRef = (EReference) si.eClass()
                           	  .getEStructuralFeature(SafConstants.CSI_INSTANCES_NAME);
                           EObject csiInstsObj = (EObject) si.eGet(csiInstsRef);
                     
                           if (csiInstsObj != null) {
                               EReference csiInstRef = (EReference) csiInstsObj.eClass()
                                   .getEStructuralFeature(SafConstants.CSI_INSTANCELIST_NAME);
                               List csiObjs = (List) csiInstsObj.eGet(csiInstRef);
                               for (int k = 0; k < csiObjs.size(); k++) {
                            	   EObject csi = (EObject) csiObjs.get(k);
                            	   checkObject(csiObjs, csi, resourcePath);
                               }
                           }
                            
                       }
                   }
	               
	           }
	       } catch (Exception e) {
	               e.printStackTrace();
	       }
	}

	private void validateIDLConfiguration(ProjectDataModel pdm)
	{
		String message = null;
		String resourcePath = ICWProject.CW_PROJECT_IDL_DIR_NAME
			+ File.separator + ICWProject.IDL_XML_DATA_FILENAME;
		List idlList = pdm.getIDLConfigList();
		if (idlList.isEmpty()) {
			return;
		}
		EObject idlSpecsObj = (EObject) idlList.get(0);
		message = null;
		message = ObjectValidator.checkPatternAndBlankValue(idlSpecsObj);
		if (message != null) {
			System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
		}
		
		checkReferencedObject(idlSpecsObj, resourcePath, message);
	}
	
	private void checkReferencedObject(EObject eobj,
			String resourcePath, String message)
	{
		List refs = eobj.eClass().getEAllReferences();
		for (int i = 0; i < refs.size(); i++) {
			EReference ref = (EReference) refs.get(i);
			Object val = eobj.eGet(ref);
			if (ref.getUpperBound() == -1 || ref.getUpperBound() > 1) {
				List refValList = (List) val;
				for (int j = 0; j < refValList.size(); j++) {
					EObject refValObj = (EObject) refValList.get(j); 
					checkObject(refValList, refValObj, resourcePath);
					checkReferencedObject(refValObj, resourcePath, message);
				}
			} else if (ref.getUpperBound() == 1) {
				message = null;
				message = ObjectValidator.checkPatternAndBlankValue((EObject) val);
				if (message != null) {
					System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
				}
				checkReferencedObject((EObject) val, resourcePath, message);
			}
		}
	}
	private void checkForDuplicatesAndPattern(EObject rootObject,
			String resourcePath)
	{
		List objList = ClovisUtils.getModelObjects(rootObject);
		List refList = rootObject.eClass().getEAllReferences();
		for (int i = 0; i < refList.size(); i++) {
			List list = (List) rootObject.eGet((EReference) refList
					.get(i));
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				checkObject(objList, eobj, resourcePath);
			}
		}
	}
	private void checkObject(List objList, EObject eobj, String resourcePath)
	{
		String message = null;
		ObjectValidator validator = ValidatorFactory.getValidatorFactory()
			.getValidator(eobj.eClass());
		if (validator != null) {
			List messages = validator.getAllErrors(eobj, objList);
			for (int i = 0; i < messages.size(); i++) {
				message = (String) messages.get(i);
				if (message != null) {
					System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
				}
				
			}
			message = null;
			message = ObjectValidator.checkPatternAndBlankValue(eobj);
			if (message != null) {
				System.err.print("\n ERROR : " + "[NONE] : " + message + " : " + resourcePath);
			}
		}
	}
}
