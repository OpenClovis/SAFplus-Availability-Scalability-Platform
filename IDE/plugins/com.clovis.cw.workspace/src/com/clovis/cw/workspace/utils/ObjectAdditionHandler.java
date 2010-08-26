/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/utils/ObjectAdditionHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.utils;

import java.io.File;
import java.net.URL;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.ui.IWorkbenchPage;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.editor.EditorUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.model.EditorModel;
import com.clovis.cw.genericeditor.model.GENodeElementFactory;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.utils.FormatConversionUtils;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.action.OpenComponentEditorAction;
import com.clovis.cw.workspace.action.OpenResourceEditorAction;
import com.clovis.cw.workspace.project.ValidationConstants;

/**
 * 
 * @author shubhada
 * 
 * The Class which will handle the Modeling of Resource Editor
 * elements through Wizard/It will allow addition of editor objects through
 * backend
 */
public class ObjectAdditionHandler
{
    private IProject _project = null;
    private String _editorType = null;
    private EPackage _ePackage = null;
    private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());
    
    /**
     * Constructor
     * @param project - Project
     * @param editorType - Editor Type(i.e either 'Resource Editor'
     * or 'Component Editor' 
     * 
     */
    public ObjectAdditionHandler(IProject project, String editorType)
    {
        _project = project;
        _editorType = editorType;
        readEcoreFile();
    }
    /**
     * reads the 'resource.ecore' file or 'component.ecore' file based
     * on the '_editorType' value
     *
     */
    private void readEcoreFile()
    {
        if (_editorType.equals(ValidationConstants.CAT_RESOURCE_EDITOR)) {
            URL caURL = DataPlugin.getDefault().find(
                    new Path("model" + File.separator
                             + ICWProject.RESOURCE_ECORE_FILENAME));
            try {
                File ecoreFile = new Path(Platform.resolve(caURL).getPath())
                        .toFile();
                _ePackage = EcoreModels.get(ecoreFile.getAbsolutePath());
            } catch (Exception e) {
                LOG.warn("Resource Editor ecore file cannot be read", e);
            }
        } else if (_editorType.equals(ValidationConstants.CAT_COMPONENT_EDITOR)) {
            URL caURL = DataPlugin.getDefault().find(
                    new Path("model" + File.separator
                             + ICWProject.COMPONENT_ECORE_FILENAME));
            try {
                File ecoreFile = new Path(Platform.resolve(caURL).getPath())
                        .toFile();
                _ePackage = EcoreModels.get(ecoreFile.getAbsolutePath());
            } catch (Exception e) {
                LOG.warn("Component Editor ecore file cannot be read", e);
            }

        }
        

    }
    /**
     * Reads the template blade/node file
     * @param filePrefix - Prefix name of the selected Blade/Node
     * For e.g 'GigEBlade' String will be passed as prefix
     * for the template file GigEBlade.xml
     * @return the Template Objects read
     */
    public List readTemplateFile(String filePrefix)
    {
    	String dataFilePath = "";
    	if (_editorType.equals(ValidationConstants.CAT_RESOURCE_EDITOR)) {
	        dataFilePath = _project.getLocation().toOSString()
	                            + File.separator
	                            + ICWProject.CW_PROJECT_MODEL_DIR_NAME
	                            + File.separator
	                            + ICWProject.RESOURCE_TEMPLATE_FOLDER
	                            + File.separator
	                            + filePrefix.concat(".xmi");
    	} else if (_editorType.equals(ValidationConstants.CAT_COMPONENT_EDITOR)) {
    		dataFilePath = _project.getLocation().toOSString()
            + File.separator
            + ICWProject.CW_PROJECT_MODEL_DIR_NAME
            + File.separator
            + ICWProject.COMPONENT_TEMPLATE_FOLDER
            + File.separator
            + filePrefix.concat(".xmi");
    	}
        URI uri = URI.createFileURI(dataFilePath);
        Resource resource = EcoreModels.
                getUpdatedResource(uri);
        NotifyingList list = (NotifyingList) resource.getContents();
        return list;
    }
    /**
     * Converts all template objects to internal editor EObjects
     * which have internal information like rdn
     * @param list - list of template objects
     * @param targetName - Name of the object to which containment from Chassis/Cluster
     * will end.
     * @param numSoftwareRes - Number of Software Resources to be added
     * @param - Maximum Instances of the blade objects
     * @return - Return the resource objects
     */
    public List processTemplateData(List list, String targetName,
            int numSoftwareRes, int maxInst)
    {
        List extraObjList = new Vector();
        EditorModel editorModel = null;
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);

        GenericEditorInput geInput = (GenericEditorInput) pdm.
            getCAEditorInput();
        GenericEditor editor = geInput.getEditor();
        editorModel = editor.getEditorModel();
        List editorViewModelList = geInput.getModel().getEList();
        ClovisUtils.setKey(list);
        
        List edgeList = new Vector();
        
        Iterator iterator = list.iterator();
        while (iterator.hasNext()) {
            EObject eobj = (EObject) iterator.next();
            List containedObjList = (List) EcoreUtils.getValue(
                    eobj, ClassEditorConstants.
                    CONTAINMENT_FEATURE);
            processConnectionObjects(containedObjList, edgeList, eobj,
            		ClassEditorConstants.CONNECTION_START, null);
 
            
            List associatedObjList = (List) EcoreUtils.getValue(
                    eobj, ClassEditorConstants.
                    ASSOCIATION_FEATURE); 
            processConnectionObjects(associatedObjList, edgeList, eobj,
            		ClassEditorConstants.CONNECTION_START, null);
            
            List inheritedObjList = (List) EcoreUtils.getValue(
                    eobj, ClassEditorConstants.
                    INHERITANCE_FEATURE);
            processConnectionObjects(inheritedObjList, edgeList, eobj,
                    ClassEditorConstants.CONNECTION_START, null);
            
            
        }
        for (int i = 0; i < list.size(); i++) {
            EObject eobj = (EObject) list.get(i);
            if (!isUniqueObject(eobj, editorViewModelList)) {
                return null;
            } else {
                editorModel.addEObject(eobj);
                extraObjList.add(eobj);
            }
        }
        for (int i = 0; i < edgeList.size(); i++) {
            EObject connObj = (EObject) edgeList.get(i);
            
            String start = (String) EcoreUtils.getValue(connObj,
            		ClassEditorConstants.CONNECTION_START);
            String end = (String) EcoreUtils.getValue(connObj,
            		ClassEditorConstants.CONNECTION_END);
            if (start != null && end != null) {
                
                EObject srcObj = ClovisUtils.getObjectFrmName(list, start);
                String startKey = (String) EcoreUtils.getValue(srcObj,
                		ModelConstants.RDN_FEATURE_NAME);
                EcoreUtils.setValue(connObj,ClassEditorConstants.
                		CONNECTION_START, startKey);
                
                EObject targetObj = ClovisUtils.getObjectFrmName(list, end);
                String endKey = (String) EcoreUtils.getValue(targetObj,
                		ModelConstants.RDN_FEATURE_NAME);
                EcoreUtils.setValue(connObj, ClassEditorConstants.
                		CONNECTION_END, endKey);
                    
            }
            editorModel.addEObject(connObj);
        }
        //Add all the edges to the view model
        //editorViewModelList.addAll(edgeList);
        extraObjList.addAll(edgeList);
        
        EObject selObj = ResourceDataUtils.getObjectFrmName(
                editorViewModelList, targetName);
        addParentConnection(targetName, editorViewModelList, extraObjList);
        addHardwareAndSoftwareResources(editor, editorViewModelList, extraObjList, 0, numSoftwareRes, selObj);
        
        EcoreUtils.setValue(selObj, ClassEditorConstants.CLASS_MAX_INSTANCES,
                String.valueOf(maxInst));
        return extraObjList;
    }
    
    /**
     * Converts all project xml objects to internal editor EObjects
     * which have internal information like rdn
     * @param topObj - top level xml object in the project xml file
     *
     * @return - Return the resource objects
     */
    public void processProjectXMLData(EObject topObj)
    {
        List list = new Vector();
        List refList = topObj.eClass().getEAllReferences();
        for (int i = 0; i < refList.size(); i++) {
            EReference ref = (EReference) refList.get(i);
            Object val = topObj.eGet(ref);
            if (val != null) {
                if (val instanceof EObject) {
                    list.add(val);
                } else if (val instanceof List) {
                    List valList = (List) val;
                    for (int j = 0; j < valList.size(); j++) {
                        list.add(valList.get(j));
                       
                    }
                    
                }
            }
        }
        GenericEditorInput geInput = null;
        Model editorModel = null; 
        IWorkbenchPage page = WorkspacePlugin
                            .getDefault()
                            .getWorkbench()
                            .getActiveWorkbenchWindow()
                            .getActivePage();
        
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        ClovisUtils.setKey(list);
        if (_editorType.equals(ValidationConstants.CAT_RESOURCE_EDITOR)) {
            geInput = (GenericEditorInput) pdm.
                getCAEditorInput();
            editorModel = pdm.getCAModel();
            if (page != null && geInput != null) {
                page.closeEditor(geInput.getEditor(), false);
            }
            // In this case 'topObj' provided has to be EObject of resource schema
            FormatConversionUtils.convertToEditorSupportedData(topObj, (EObject) editorModel.
            		getEList().get(0), ValidationConstants.CAT_RESOURCE_EDITOR);
            
        } else if (_editorType.equals(ValidationConstants.CAT_COMPONENT_EDITOR)) {
            
            geInput = (GenericEditorInput) pdm.
                getComponentEditorInput();
            editorModel = pdm.getComponentModel();
            if (page != null && geInput != null) {
                page.closeEditor(geInput.getEditor(), false);
            }
//          In this case 'topObj' provided has to be EObject of component schema
            FormatConversionUtils.convertToEditorSupportedData(topObj, (EObject) editorModel.
            		getEList().get(0), ValidationConstants.CAT_COMPONENT_EDITOR);
            
        }
        
        editorModel.save(true);
        
        //Reopen the editor
        if (_editorType.equals(ValidationConstants.CAT_RESOURCE_EDITOR)) {
            pdm.setCAEditorInput(null);
            OpenResourceEditorAction.openResourceEditor(_project);
        } else if (_editorType.equals(ValidationConstants.CAT_COMPONENT_EDITOR)) {
            pdm.setComponentEditorInput(null);
            OpenComponentEditorAction.openComponentEditor(_project);
        }
    }
    /**
     * Adds the relevent information to the connection
     * objects contained inside resources 
     * @param connList - List of connection objects contained in node
     * @param currentEdgeList - Edge List for the editor format
     * @param eobj - Resource Object
     * @param connSrcFeature - Connection Source Feature Name
     * @param connType - Connection Type
     */
    private void processConnectionObjects(List connList, List currentEdgeList,
    		EObject eobj, String connSrcFeature, String connType)
    {
        if (connList != null) {
        	
            for (int i = 0; i < connList.size(); i++) {
                EObject connObj = (EObject) connList.get(i);
                EcoreUtils.setValue(connObj,
                		connSrcFeature,
                        EcoreUtils.getName(eobj));
                if (connType != null) {
                	EcoreUtils.setValue(connObj, ComponentEditorConstants.
                			CONNECTION_TYPE, connType);
                }
            }
            currentEdgeList.addAll(connList);
            connList.clear();
        }
    }
    /**
     * 
     * @param eobj - EObject which has to be checked for duplicate
     * @param editorViewModelList - Editor View Model
     * @return - true if eobj is a non - duplicate object, else return false
     */
    public static boolean isUniqueObject(EObject eobj, List editorViewModelList)
    {
        String name = EcoreUtils.getName(eobj);
		EObject rootObject = (EObject) editorViewModelList.get(0);
		List refList = rootObject.eClass().getEAllReferences();
		for (int i = 0; i < refList.size(); i++) {
			List list = (EList) rootObject.eGet((EReference) refList.get(i));
			for (int j = 0; j < list.size(); j++) {
				EObject editorObj = (EObject) list.get(j);
				String objName = EcoreUtils.getName(editorObj);
				if (name != null && objName != null && name.equals(objName)) {
					return false;
				}
			}
		}
		return true;
    }
    /**
	 * 
	 * @param eobj -
	 *            EObject which has to be checked for duplicate
	 * @param editorViewModelList -
	 *            Editor View Model
	 * @return - true if eobj is a non - duplicate object, else return false
	 */
    public static boolean isUniqueObject(String name, List editorViewModelList) {
		EObject rootObject = (EObject) editorViewModelList.get(0);
		List refList = rootObject.eClass().getEAllReferences();
		for (int i = 0; i < refList.size(); i++) {
			List list = (EList) rootObject.eGet((EReference) refList.get(i));
			for (int j = 0; j < list.size(); j++) {
				EObject editorObj = (EObject) list.get(j);
				String objName = EcoreUtils.getName(editorObj);
				if (name != null && objName != null && name.equals(objName)) {
					return false;
				}

			}
		}
		return true;
	}
        /**
		 * Adds the default Resource Objects
		 * 
		 * @param numHardwareResource -
		 *            Number of hardware resources
		 * @param numSoftwareResource -
		 *            Number of software resources
		 * @param bladeMaxInstance -
		 *            Maximum instance of the blade
		 * @return the List of new objects added to the Editor View Model
		 */
    public List addDefaultResourceObjects(int numHardwareResource,
            int numSoftwareResource, int bladeMaxInstance, String bladeName)
    {
        List extraObjList = new Vector();
        List editorViewModelList = null;
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.
            getCAEditorInput();
        GenericEditor editor = geInput.getEditor();
        editorViewModelList = geInput.getModel().getEList();
        
        
        EObject bladeObj = ((GENodeElementFactory) editor.getCreationFactory(ClassEditorConstants.
        		NODE_HARDWARE_RESOURCE_NAME)).getNewEObject();
                
        EcoreUtils.setValue(bladeObj, EcoreUtils.getNameField(
                bladeObj.eClass()), bladeName);
        EcoreUtils.setValue(bladeObj, ClassEditorConstants.CLASS_MAX_INSTANCES,
                String.valueOf(bladeMaxInstance));
        EObject rootObject = (EObject) editorViewModelList.get(0);
        List nodehardwareList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME));
        nodehardwareList.add(bladeObj);
        extraObjList.add(bladeObj);
        
        addParentConnection(EcoreUtils.getName(bladeObj), editorViewModelList,
                extraObjList);
        addHardwareAndSoftwareResources(editor, editorViewModelList, extraObjList,
                numHardwareResource, numSoftwareResource, bladeObj);
       
        return extraObjList;
    }
    /**
     * Adds the default Hardware and Software resources to the editor view model
     * @param editorViewModelList - Editor View Model Objects
     * @param extraObjList - Additional objects added from backend
     * in to editor view model
     * @param numHardwareResource - Number of hardware resources in the system
     * @param numSoftwareResource - Number of software resources in the system
     * @param bladeObj - The Blade Object under which hardware and software
     * resources reside
     */
    private void addHardwareAndSoftwareResources(GenericEditor editor, List editorViewModelList,
            List extraObjList, int numHardwareResource,
            int numSoftwareResource, EObject bladeObj)
    {
       EObject rootObject = (EObject) editorViewModelList.get(0);
       List hardwareList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.HARDWARE_RESOURCE_REF_NAME));
       List softwareList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME));
       List compositionList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.COMPOSITION_REF_NAME));
    	
       EClass compositionClass = (EClass) _ePackage.getEClassifier(
               ClassEditorConstants.COMPOSITION_NAME);
       String srcKey = (String) EcoreUtils.getValue(bladeObj,
    		   ModelConstants.RDN_FEATURE_NAME);
       for (int i = 0; i < numHardwareResource; i++) {
           EObject hardwareObj = ((GENodeElementFactory) editor.getCreationFactory(ClassEditorConstants.
           		HARDWARE_RESOURCE_NAME)).getNewEObject();
           String name = EditorUtils.getNextValue(hardwareObj.eClass().getName(),
                   editorViewModelList, EcoreUtils.getNameField(
                           hardwareObj.eClass()));
           EcoreUtils.setValue(hardwareObj, EcoreUtils.getNameField(
                   hardwareObj.eClass()), name);
           hardwareList.add(hardwareObj);
           extraObjList.add(hardwareObj);
           
           EObject compositionObj = EcoreUtils.createEObject(
                   compositionClass, true);
           String targetKey = (String) EcoreUtils.getValue(hardwareObj,
        		   ModelConstants.RDN_FEATURE_NAME);
           EcoreUtils.setValue(compositionObj,
                   ClassEditorConstants.CONNECTION_END, targetKey);
           EcoreUtils.setValue(compositionObj,
                   ClassEditorConstants.CONNECTION_START, srcKey);
           compositionList.add(compositionObj);
           extraObjList.add(compositionObj);
           
       }
       for (int i = 0; i < numSoftwareResource; i++) {
           EObject softwareObj = ((GENodeElementFactory) editor.getCreationFactory(ClassEditorConstants.
              		SOFTWARE_RESOURCE_NAME)).getNewEObject();

           // until we rename the SoftwareObject in ecore, force the prefix
           // for these objects to be ManagedResource
           //
           //String name = EditorUtils.getNextValue(softwareObj.eClass().getName(),
           String name = EditorUtils.getNextValue("ManagedResource",
                   editorViewModelList, EcoreUtils.getNameField(
                           softwareObj.eClass()));
           EcoreUtils.setValue(softwareObj, EcoreUtils.getNameField(
                   softwareObj.eClass()), name);
           softwareList.add(softwareObj);
           extraObjList.add(softwareObj);
           
           EObject compositionObj = EcoreUtils.createEObject(
                   compositionClass, true);
           String targetKey = (String) EcoreUtils.getValue(softwareObj,
        		   ModelConstants.RDN_FEATURE_NAME);
           EcoreUtils.setValue(compositionObj,
                   ClassEditorConstants.CONNECTION_END, targetKey);
           EcoreUtils.setValue(compositionObj,
                   ClassEditorConstants.CONNECTION_START, srcKey);
           compositionList.add(compositionObj);
           extraObjList.add(compositionObj);
       }
        
    }
    /**
     * Adds the default component objects to the editor view model
     * @param nodeName - Node Name
     * @param nodeClassType - Node Class Type
     * @param numComponents - Number of components under the node
     * @return the Extra Objects added from backend to the editor view model 
     */
    public List addDefaultComponentObjects(String nodeName, String nodeClassType,
            int numComponents, List compsList)
    {
        List extraObjList = new Vector();
        List editorViewModelList = null;
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.
            getComponentEditorInput();
        GenericEditor editor = geInput.getEditor();
        /*editorViewModelList = geInput.getModelObjects();
        
        EClass nodeEClass = (EClass) _ePackage.getEClassifier(
                ComponentEditorConstants.NODE_NAME);
        EObject nodeObj = EcoreUtils.createEObject(nodeEClass, true);
        EcoreUtils.setValue(nodeObj, EcoreUtils.
                getNameField(nodeEClass), nodeName);
        EcoreUtils.setValue(nodeObj, ComponentEditorConstants.
                NODE_CLASS_TYPE, nodeClassType);
        editorViewModelList.add(nodeObj);
        extraObjList.add(nodeObj);
        addParentConnection(nodeName, editorViewModelList, extraObjList);
        addServiceUnitsAndComponents(numComponents, editorViewModelList,
                extraObjList, nodeObj, compsList);*/
        
        editorViewModelList = geInput.getModel().getEList();
        
        EObject nodeObj = ((GENodeElementFactory) editor.getCreationFactory(
        		ComponentEditorConstants.NODE_NAME)).getNewEObject();
        EcoreUtils.setValue(nodeObj, EcoreUtils.
                getNameField(nodeObj.eClass()), nodeName);
        EcoreUtils.setValue(nodeObj, ComponentEditorConstants.
                NODE_CLASS_TYPE, nodeClassType);
        EObject rootObject = (EObject) editorViewModelList.get(0);
        List nodeList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(ComponentEditorConstants.NODE_REF_NAME));  
        nodeList.add(nodeObj);
        extraObjList.add(nodeObj);
        addParentConnection(nodeName, editorViewModelList, extraObjList);
        addServiceUnitsAndComponents(editor, numComponents, editorViewModelList,
                extraObjList, nodeObj, compsList);
        
        return extraObjList;
    }
    /**
     * Adds ServiceUnits and Components to the editor view model
     * @param numComponents - Number of components under the node
     * @param editorViewModelList - Component editor view model list
     * @param extraObjList - Extra objects added to the editor view model
     * @param nodeObj - Parent Node object
     * @param compsList - List of components
     */
    private void addServiceUnitsAndComponents(GenericEditor editor, int numComponents,
            List editorViewModelList, List extraObjList,
            EObject nodeObj, List compsList)
    {
        EObject rootObject = (EObject) editorViewModelList.get(0);
        
        List safList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.SAFCOMPONENT_REF_NAME));
        List suList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.SERVICEUNIT_REF_NAME));
        List sgList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.SERVICEGROUP_REF_NAME));
        List siList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.SERVICEINSTANCE_REF_NAME));
        List csiList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.COMPONENTSERVICEINSTANCE_REF_NAME));
        List autoList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.AUTO_REF_NAME));
        
        
        EClass relationEClass = (EClass) _ePackage.getEClassifier(
                ComponentEditorConstants.AUTO_NAME);
        for (int i = 0; i < numComponents; i++) {
            EObject suObj = ((GENodeElementFactory) editor.getCreationFactory(
            		ComponentEditorConstants.SERVICEUNIT_NAME)).getNewEObject();
            
            EObject compObj = (EObject) compsList.get(i);
            
            String name = EcoreUtils.getName(compObj).concat("SU");
            EcoreUtils.setValue(suObj, EcoreUtils.getNameField(
            		suObj.eClass()), name);
            suList.add(suObj);
            extraObjList.add(suObj);
            
            EObject containmentObj = EcoreUtils.createEObject(
            		relationEClass, true);
            EcoreUtils.setValue(containmentObj, ComponentEditorConstants.
            		CONNECTION_TYPE, ComponentEditorConstants.CONTAINMENT_NAME);
            String startKey = (String) EcoreUtils.getValue(nodeObj,
            		ModelConstants.RDN_FEATURE_NAME);
            String endKey = (String) EcoreUtils.getValue(suObj,
            		ModelConstants.RDN_FEATURE_NAME);
            EcoreUtils.setValue(containmentObj, ComponentEditorConstants.
            		CONNECTION_START, startKey);
            EcoreUtils.setValue(containmentObj, ComponentEditorConstants.
            		CONNECTION_END, endKey);
            autoList.add(containmentObj);
            extraObjList.add(containmentObj);
            
            
            safList.add(compObj);
            extraObjList.add(compObj);
            
            EObject suCompConnObj = EcoreUtils.createEObject(
            		relationEClass, true);
            EcoreUtils.setValue(suCompConnObj, ComponentEditorConstants.
            		CONNECTION_TYPE, ComponentEditorConstants.CONTAINMENT_NAME);
            startKey = (String) EcoreUtils.getValue(suObj,
            		ModelConstants.RDN_FEATURE_NAME);
            endKey = (String) EcoreUtils.getValue(compObj,
            		ModelConstants.RDN_FEATURE_NAME);
            EcoreUtils.setValue(suCompConnObj, ComponentEditorConstants.
            		CONNECTION_START, startKey);
            EcoreUtils.setValue(suCompConnObj, ComponentEditorConstants.
            		CONNECTION_END, endKey);
            autoList.add(suCompConnObj);
            extraObjList.add(suCompConnObj);
            
            EObject sgObj = ((GENodeElementFactory) editor.getCreationFactory(
            		ComponentEditorConstants.SERVICEGROUP_NAME)).getNewEObject();
            name = EcoreUtils.getName(compObj).concat("SG");
            EcoreUtils.setValue(sgObj, EcoreUtils.getNameField(
                    sgObj.eClass()), name);
            sgList.add(sgObj);
            extraObjList.add(sgObj);
            
            EObject sgSuConnObj = EcoreUtils.createEObject(
            		relationEClass, true);
            EcoreUtils.setValue(sgSuConnObj, ComponentEditorConstants.
            		CONNECTION_TYPE, ComponentEditorConstants.ASSOCIATION_NAME);
            startKey = (String) EcoreUtils.getValue(sgObj,
            		ModelConstants.RDN_FEATURE_NAME);
            endKey = (String) EcoreUtils.getValue(suObj,
            		ModelConstants.RDN_FEATURE_NAME);
            EcoreUtils.setValue(sgSuConnObj, ComponentEditorConstants.
            		CONNECTION_START, startKey);
            EcoreUtils.setValue(sgSuConnObj, ComponentEditorConstants.
            		CONNECTION_END, endKey);
            autoList.add(sgSuConnObj);
            extraObjList.add(sgSuConnObj);
            
            EObject siObj = ((GENodeElementFactory) editor.getCreationFactory(
            		ComponentEditorConstants.SERVICEINSTANCE_NAME)).getNewEObject();
            name = EcoreUtils.getName(compObj).concat("SI");
            EcoreUtils.setValue(siObj, EcoreUtils.getNameField(
            		siObj.eClass()), name);
            siList.add(siObj);
            extraObjList.add(siObj);
            
            EObject sgSiConnObj = EcoreUtils.createEObject(
            		relationEClass, true);
            EcoreUtils.setValue(sgSiConnObj, ComponentEditorConstants.
            		CONNECTION_TYPE, ComponentEditorConstants.ASSOCIATION_NAME);
            startKey = (String) EcoreUtils.getValue(sgObj,
            		ModelConstants.RDN_FEATURE_NAME);
            endKey = (String) EcoreUtils.getValue(siObj,
            		ModelConstants.RDN_FEATURE_NAME);
            EcoreUtils.setValue(sgSiConnObj, ComponentEditorConstants.
            		CONNECTION_START, startKey);
            EcoreUtils.setValue(sgSiConnObj, ComponentEditorConstants.
            		CONNECTION_END, endKey);
            autoList.add(sgSiConnObj);
            extraObjList.add(sgSiConnObj);
            
            EObject csiObj = ((GENodeElementFactory) editor.getCreationFactory(
            		ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME)).
            		getNewEObject();
            name = EcoreUtils.getName(compObj).concat("CSI");
            EcoreUtils.setValue(csiObj, EcoreUtils.getNameField(
            		csiObj.eClass()), name);
            String initializationInfo = EcoreUtils.getAnnotationVal(
            		csiObj.eClass(), null, "initializationFields");
            if( initializationInfo != null ) {
                EcoreUtils.initializeFields(csiObj, csiList,
                		initializationInfo);
            }
            csiList.add(csiObj);
            extraObjList.add(csiObj);
            
            EObject siCSiConnObj = EcoreUtils.createEObject(
            		relationEClass, true);
            EcoreUtils.setValue(siCSiConnObj, ComponentEditorConstants.
            		CONNECTION_TYPE, ComponentEditorConstants.CONTAINMENT_NAME);
            startKey = (String) EcoreUtils.getValue(siObj,
            		ModelConstants.RDN_FEATURE_NAME);
            endKey = (String) EcoreUtils.getValue(csiObj,
            		ModelConstants.RDN_FEATURE_NAME);
            EcoreUtils.setValue(siCSiConnObj, ComponentEditorConstants.
            		CONNECTION_START, startKey);
            EcoreUtils.setValue(siCSiConnObj, ComponentEditorConstants.
            		CONNECTION_END, endKey);
            autoList.add(siCSiConnObj);
            extraObjList.add(siCSiConnObj);
            
            EObject csiCompConnObj = EcoreUtils.createEObject(
            		relationEClass, true);
            EcoreUtils.setValue(csiCompConnObj, ComponentEditorConstants.
            		CONNECTION_TYPE, ComponentEditorConstants.ASSOCIATION_NAME);
            startKey = (String) EcoreUtils.getValue(csiObj,
            		ModelConstants.RDN_FEATURE_NAME);
            endKey = (String) EcoreUtils.getValue(compObj,
            		ModelConstants.RDN_FEATURE_NAME);
            EcoreUtils.setValue(csiCompConnObj, ComponentEditorConstants.
            		CONNECTION_START, startKey);
            EcoreUtils.setValue(csiCompConnObj, ComponentEditorConstants.
            		CONNECTION_END, endKey);
            autoList.add(csiCompConnObj);
            extraObjList.add(csiCompConnObj);
        }
    }
    /**
     * After adding the Blade/Node objects in to the editor,
     * this method is called to connect it to parent Chassis/Cluster
     * @param targetObjName - Connection Target Object Name
     */
    public void addParentConnection(String targetObjName,
			List editorViewModelList, List extraObjList) {

		if (_editorType.equals(ValidationConstants.CAT_RESOURCE_EDITOR)) {
			EClass compositionClass = (EClass) _ePackage
					.getEClassifier(ClassEditorConstants.COMPOSITION_NAME);
			EObject compositionObj = EcoreUtils.createEObject(compositionClass,
					true);
			EObject bladeObj = EditorUtils.getObjectFromName(
					editorViewModelList, targetObjName);
			String targetKey = (String) EcoreUtils.getValue(bladeObj,
					ModelConstants.RDN_FEATURE_NAME);
			EcoreUtils.setValue(compositionObj,
					ClassEditorConstants.CONNECTION_END, targetKey);
			
			 
			EObject rootObject = (EObject) editorViewModelList.get(0);
			List chassisList = (EList) rootObject.eGet(rootObject.eClass()
					.getEStructuralFeature(
							ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME));
			EObject eobj = (EObject) chassisList.get(0);
			String srcKey = (String) EcoreUtils.getValue(eobj,
					ModelConstants.RDN_FEATURE_NAME);
			EcoreUtils.setValue(compositionObj,
					ClassEditorConstants.CONNECTION_START, srcKey);
			List compositionList = (EList) rootObject.eGet(rootObject.eClass()
					.getEStructuralFeature(
							ClassEditorConstants.COMPOSITION_REF_NAME));
			compositionList.add(compositionObj);
			extraObjList.add(compositionObj);

		} else if (_editorType.equals(ValidationConstants.CAT_COMPONENT_EDITOR)) {
			EClass relationClass = (EClass) _ePackage
					.getEClassifier(ComponentEditorConstants.AUTO_NAME);
			EObject relationObj = EcoreUtils.createEObject(relationClass, true);
			EcoreUtils.setValue(relationObj,
					ComponentEditorConstants.CONNECTION_TYPE,
					ComponentEditorConstants.CONTAINMENT_NAME);
			EObject obj = EditorUtils.getObjectFromName(editorViewModelList,
					targetObjName);
			String targetKey = (String) EcoreUtils.getValue(obj,
					ModelConstants.RDN_FEATURE_NAME);
			EcoreUtils.setValue(relationObj,
					ComponentEditorConstants.CONNECTION_END, targetKey);
			
			 
			EObject rootObject = (EObject) editorViewModelList.get(0);
			List clusterList = (EList) rootObject.eGet(rootObject.eClass()
					.getEStructuralFeature(
							ComponentEditorConstants.CLUSTER_REF_NAME));
			EObject eobj = (EObject) clusterList.get(0);
			String srcKey = (String) EcoreUtils.getValue(eobj,
					ModelConstants.RDN_FEATURE_NAME);
			EcoreUtils.setValue(relationObj,
					ComponentEditorConstants.CONNECTION_START, srcKey);

			List relationList = (EList) rootObject.eGet(rootObject.eClass()
					.getEStructuralFeature(
							ComponentEditorConstants.AUTO_REF_NAME));
			relationList.add(relationObj);
			extraObjList.add(relationObj);
		}
	}

    /**
	 * Creates the specified number of components
	 * 
	 * @param numComponents -
	 *            Number of components to be created
	 * @param editorViewModelList -
	 *            Editor View Model List
	 * @return the List of components created
	 */
    public List createComponents(int numComponents, List editorViewModelList)
    {
    	return createComponents(numComponents, editorViewModelList, null);
    }
    
    /**
	 * Creates the specified number of components
	 * 
	 * @param numComponents -
	 *            Number of components to be created
	 * @param editorViewModelList -
	 *            Editor View Model List
	 * @param programNames -
	 *            Optional list of program root name (null to use defaults)
	 * @return the List of components created
	 */
    public List createComponents(int numComponents, List editorViewModelList, ArrayList programNames)
    {
        List compsList = new ClovisNotifyingListImpl();
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        GenericEditorInput geInput = (GenericEditorInput) pdm.
            getComponentEditorInput();
        GenericEditor editor = geInput.getEditor();
        // List tempList = new Vector(editorViewModelList);
        EObject rootObject = (EObject) editorViewModelList.get(0);
        List tempList = new Vector((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.SAFCOMPONENT_REF_NAME)));
        for (int i = 0; i < numComponents; i++) {
            EObject compObj = ((GENodeElementFactory) editor.getCreationFactory(
            		ComponentEditorConstants.SAFCOMPONENT_NAME)).getNewEObject();
            
            String name;
            if (programNames != null)
            {
            	name = (String)programNames.get(i);
            } else {
	            name = EcoreUtils.getNextValue(compObj.eClass().getName(),
	                    tempList, EcoreUtils.getNameField(compObj.eClass()));
            }
            
            EcoreUtils.setValue(compObj, EcoreUtils.getNameField(
            		compObj.eClass()), name);
            String initializationInfo = EcoreUtils.getAnnotationVal(
            		compObj.eClass(), null, "initializationFields");
            if( initializationInfo != null ) {
                EcoreUtils.initializeFields(compObj, tempList,
                        initializationInfo);
            }
            compsList.add(compObj);
            tempList.add(compObj);
        }
        return compsList;
    }

    /**
     * 
     * @return the EPackage
     */
    public EPackage getEPackage()
    {
    	return _ePackage;
    }
}
