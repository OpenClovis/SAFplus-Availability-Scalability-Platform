/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/ProjectValidator.java $
 * $Author: pushparaj $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.editor.ca.manageability.ui.AssociateResourceUtils;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.dialog.MoPathComboBoxCellEditor;
/**
 * @author Manish
 *
 * Model Validator class.
 */
public class ProjectValidator implements IProjectValidator, ValidationConstants
{
    private Map _connSrcMap = new HashMap();
	private Map _connTrgMap = new HashMap();
	private Map _parentsMap = new HashMap();
	private Map _childrensMap = new HashMap();
	private Map<String, EObject> _resNameObjectMap = new HashMap<String, EObject>();
	private Map<String, EObject> _compNameObjectMap = new HashMap<String, EObject>();
	private Map<String, Integer> _resNameMaxInstancesMap = new HashMap<String, Integer>();
	protected List _resEditorObjects = null;
    protected List _compEditorObjects = null;
    protected List _nodeProfileObjects = null;
    protected List _alarmProfileObjects = null;
    protected List _iocConfigObjects = null;
    protected ComponentDataUtils _compUtils = null;
    protected ResourceDataUtils _resUtils = null;
    protected ComplexValidator _cValidator = null;
    protected Validator _validator = null;
    protected XmlDataValidator _xmlDataValidator = null;
    protected IProject _project = null;
    protected HashMap _problemNumberObjMap = new HashMap();
    
    private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());
    
    public ProjectValidator()
    {
    	readAndinitializeProblems();
    }
    
    /**
     * @param pdm Project Data Model
     * @return the Problems List
     */
    public List validate(ProjectDataModel pdm)
    {
        //_resEditorObjects =  pdm.getEditorObjects(pdm.getCAModel());
        //_compEditorObjects = pdm.getEditorObjects(pdm.getComponentModel());
    	_resEditorObjects =  pdm.getCAModel().getEList();
    	_compEditorObjects = pdm.getComponentModel().getEList();
        _nodeProfileObjects = pdm.getNodeProfiles().getEList();
        EObject alarmInfoObj = (EObject) pdm.getAlarmProfiles().getEList().get(0);
        _alarmProfileObjects = (List) EcoreUtils.getValue(alarmInfoObj, "AlarmProfile");
        _iocConfigObjects = pdm.getIOCConfigList();
        _project = pdm.getProject();
        _cValidator = new ComplexValidator(_compEditorObjects, _problemNumberObjMap);
        _validator = new Validator(_project, _problemNumberObjMap, _resEditorObjects);
        _xmlDataValidator = new XmlDataValidator(_project, _problemNumberObjMap);
        _compUtils = new ComponentDataUtils(_compEditorObjects);
        _resUtils = new ResourceDataUtils(_resEditorObjects);
        parseComponentObjects(_compEditorObjects);
        parseResourceObjects(_resEditorObjects);
        parseRelationships();
                
        // check to see if the src link for the project is ok
        checkSrcLink();
        
        return getProblems(_resEditorObjects,
                _compEditorObjects);
        
    }

    /**
     * Check to see if the src link under the project exists. First we check if the project has
     *  as src directory under the corresponding project area directory. If it does and the
     *  project does not have an src link file then create a link file to the src directory
     */
    private void checkSrcLink()
    {
    	File linkFile  = new File(_project.getLocation().append(ICWProject.CW_PROJECT_SRC_DIR_NAME).toOSString());
		String projAreaLoc = CwProjectPropertyPage.getProjectAreaLocation(_project);
		File srcArea = new File(projAreaLoc + File.separator + _project.getName() + File.separator + ICWProject.CW_PROJECT_SRC_DIR_NAME);
		if (!srcArea.getAbsolutePath().equals(linkFile.getAbsolutePath()) && srcArea.exists() && !linkFile.exists())
		{
			ClovisFileUtils.createRelativeSourceLink(srcArea.getAbsolutePath(), linkFile.getAbsolutePath());
			
			try {
				_project.refreshLocal(IResource.DEPTH_INFINITE, null);
			} catch (CoreException ce) {
				ce.printStackTrace();
			}
		}
    }
    
    /**
    *
    * reads the properties file which holds the mapping of Mib datatypes
    * to clovis supported data types.
    */
   private void readAndinitializeProblems()
   {
       NotifyingList problemsList = null;
       URL url = DataPlugin.getDefault().getBundle().getEntry("/");
       try {
           url = Platform.resolve(url);
           String fileName = url.getFile() + "xml" + File.separator
           + PROBLEM_DATA_XMI_FILE;
           URI uri = URI.createFileURI(fileName);
           problemsList = (NotifyingList) EcoreModels.read(uri);
       } catch (IOException e) {
           LOG.error("Error reading problem details file", e);
       }
       for (
         int i = 0; problemsList != null && i < problemsList.size(); i++) {
           EObject eobj = (EObject) problemsList.get(i);
           Integer problemNumber = (Integer) EcoreUtils.getValue(
        		   eobj, PROBLEM_NUMBER);
           if (!_problemNumberObjMap.containsKey(problemNumber)) {
        	   _problemNumberObjMap.put(problemNumber, eobj);
           }

       }

   }
    /**
     *
     * @param resEditorObjects Resource Editor Objects
     * @param compEditorObjects Component Editor Objects
     * @return the problems list
     */
    private List getProblems(List resEditorObjects, List compEditorObjects)
    {
        List retList = new ArrayList();
        // validate the connection based on its dependency on some other
        //connection(s)
        _cValidator.validateComponentEditorConnections(retList);
        //First test whether any of the components does not have associted
        //resources
        List compList = _validator.getfilterList(compEditorObjects,
                ComponentEditorConstants.COMPONENT_NAME);
        
        List resList = _validator.getfilterList(resEditorObjects,
        		ClassEditorConstants.RESOURCE_NAME);

        //Validate that all components have valid CapabilityModel/Instantiation command - asp_<compname>
        _cValidator.validateComponentTypes (compList, retList);        
        
        validateForAssociatedResources(compList, resList, retList);
        
        validateProxiedComponents(retList);
        
        //1)Validate for Prov is Enabled but Prov Library being not selected
        //2)Also Validate for Prov is disabled for all the Associated resources
        // but Prov Lib is Selected
        validateForProvLibSelection(compList, retList);
        //1)Validate for Alarm is Enabled but Alarm Library being not selected
        //2)Also Validate for Alarm is disabled for all the Associated resources
        // but Alarm Lib is Selected
        validateForAlarmLibSelection(compList, retList);
        
        //Validate whether all associated Device IDs Exists or not. 
        validateDeviceIDsAndAlarmsOnResources(resList, retList);
        
        //Validate whether all SU types and SG Types defined in the editor
        // are linked using the association relationship.
        _cValidator.validateNonAssociatedObjects(retList);   
        
        _cValidator.validateEONamesOfComponents(retList);
        _cValidator.validateComponentsCsiType(retList);
        
        // Validate whether correct number of standbyAssignments 
        // is assigned to all SI
        _cValidator.validateStandbyAssignments(retList);
        //
        List sgTypeList = _validator.getfilterList(compEditorObjects,
                ComponentEditorConstants.SERVICEGROUP_NAME);
        validateSGs(retList, sgTypeList);
        
        List siTypeList = _validator.getfilterList(compEditorObjects,
                ComponentEditorConstants.SERVICEINSTANCE_NAME);
        validateSIs(retList, siTypeList);
        
        /*Checks if alarm/provisioning is enabled and there is no alarmprofiles/provAttributes
        * associated to the resource
        */
        _validator.checkAlarmProvisioningValidity(
                _resEditorObjects, _alarmProfileObjects, retList);
        //Validate all Node Profiles
        if (_nodeProfileObjects != null && _nodeProfileObjects.size() > 0) {
			validateNodeProfiles(retList);
		}
        else
        {
        	EObject noAMFConfiguration = (EObject) _problemNumberObjMap.get(new Integer(46));
            EcoreUtils.setValue(noAMFConfiguration, PROBLEM_MESSAGE,
                    "AMF configuration is not done ");
            retList.add(noAMFConfiguration);
        }
//      Validate all IOC Link Configuration
//      Commenting temporarily.
       /* if (_iocConfigObjects != null && _iocConfigObjects.size() > 0) {  
			validateIOCConfiguration(retList);
		}*/
        // Check for any of the hanging nodes in Component Editor
        checkForIsolatedComponents(retList);
        // check for any hanging resources which does not belong to
        //resource heirarchy in the resource editor
        checkForIsolatedResources(retList);
        //check for existence of system controller node in component editor
        List nodeList = _validator.getfilterList(compEditorObjects,
                ComponentEditorConstants.NODE_NAME);
        _validator.validatePresenceOfControllerNode(nodeList, retList);
        // Validate for no of clients defined in the IDL
        // for each EO
        List safCompList = _validator.getfilterList(compEditorObjects,
                ComponentEditorConstants.SAFCOMPONENT_NAME);
        List idlList = ProjectDataModel.getProjectDataModel(_project).
            getIDLConfigList();
        _validator.validateIDL(safCompList, idlList, retList);
        _validator.validateSNMPSubAgent(safCompList, _compUtils, retList);
        
        CheckSlotConfiguration(retList);
        CheckForIOCConfiguration(retList);
        //checkForAlarmAndProvisioning(retList);
        //Validates data in xml file
        _xmlDataValidator.validateResourceAlarmMapData(retList);
        _xmlDataValidator.validateComponentResourceMapData(retList);
        _xmlDataValidator.validateAmfConfig(retList);
        // Validate SUs
        List susList = _validator.getfilterList(compEditorObjects,
                ComponentEditorConstants.SERVICEUNIT_NAME);
        validateSUs(retList, susList);
        //validateCorOhMaskSum(retList);
        return retList;
    }

    /**
	 * Checks wether the IOC Configuration is done or not.
	 * 
	 * @param retList the problem list
	 */
	private void CheckForIOCConfiguration(List retList) {
		try {
			String dataFilePath = _project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
					+ File.separator + "clIocConfig.xml";
			File xmlFile = new File(dataFilePath);
			if (xmlFile.exists()) {
				CheckForLinkConfiguration(retList);
			} else {
				EObject problem = (EObject) _problemNumberObjMap.get(new Integer(71));
				EcoreUtils
						.setValue(problem, PROBLEM_MESSAGE,
								"The Intelligent Object Communication Configuration is not done");
				retList.add(problem);
			}
		} catch (Exception exc) {
			LOG.error("Error Reading clIocConfig Resource.", exc);
		}
	}
	/**
	 * Checks wether the Link Configuration is done or not.
	 * 
	 * @param retList the problem list
	 */
	private void CheckForLinkConfiguration(List retList) {
		try {
			EObject bootObj = (EObject) _iocConfigObjects.get(0);
			EObject iocObj = (EObject) EcoreUtils.getValue(bootObj, "ioc");
			EObject transObj = (EObject) EcoreUtils.getValue(iocObj,
					"transport");
			List linkList = (EList) EcoreUtils.getValue(transObj, "link");
			if (linkList.size() == 0) {
				EObject problem = (EObject) _problemNumberObjMap
						.get(new Integer(72));
				EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
						"The Link Configuration is not done");
				retList.add(problem);
			}

		} catch (Exception exc) {
			LOG.error("Error Reading clIocConfig Resource.", exc);
		}
	}
    /**
	 * Checks wether the The Slot Configuration is done or not.
	 * 
	 * @param retList
	 *            the problem list
	 */
	private void CheckSlotConfiguration(List retList) {
		try {
			String dataFilePath = _project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
					+ File.separator + ICWProject.SLOT_INFORMATION_XML_FILENAME;
			File xmlFile = new File(dataFilePath);
			if (xmlFile.exists()) {
			       
				URL url = DataPlugin.getDefault()
				 .find(new Path("model" + File.separator + ICWProject
				                 .CW_ASP_COMPONENTS_CONFIGS_FOLDER_NAME
				                 + File.separator + ICWProject
				                 .CW_SYSTEM_COMPONENTS_FOLDER_NAME
				                 + File.separator + ICWProject.SLOT_INFORMATION_ECORE));
				File ecoreFile = new Path(Platform.resolve(url).getPath())
				 	.toFile();
				EcoreModels.get(ecoreFile.getAbsolutePath());

	           URI uri = URI.createFileURI(xmlFile.getAbsolutePath());
	           List slotInfoList = (NotifyingList) EcoreModels.read(uri);
	           EObject bootConfigObj = (EObject) slotInfoList.get(0);
	           EObject slotsObj = (EObject) EcoreUtils.getValue(
	        		   bootConfigObj, ModelConstants.SLOTS_OBJECT_REF);
	           List slotList = (List) EcoreUtils.getValue(
	        		   slotsObj, ModelConstants.SLOT_LIST_REF);
	           List nodesList = ComponentDataUtils.getNodesList(
	        		   _compEditorObjects);
	           for (int i = 0; i < slotList.size(); i++) {
	        	   EObject classTypesObj = (EObject) EcoreUtils.getValue(
	        			   (EObject) slotList.get(i), ModelConstants.SLOT_CLASSTYPES_OBJECT_REF);
	        	   List classTypesList = (List) EcoreUtils.getValue(
	        			   classTypesObj, ModelConstants.SLOT_CLASSTYPE_LIST_REF);
	        	   int slotNumber = ((Integer) EcoreUtils.getValue((EObject) slotList.get(i),
        				   ModelConstants.SLOT_NUMBER_FEATURE)).intValue();
	        	   if (classTypesList.isEmpty()) {
	        		   EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(73));
	        		   EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
	   				   EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
	   								"Slot number:"+ String.valueOf(slotNumber)
	   								+ " does not have any class types configured");
	   			
	   				   /*EStructuralFeature srcFeature = problem.eClass()
	   						.getEStructuralFeature(PROBLEM_SOURCE);
	   				   problem.eSet(srcFeature, classTypesObj);*/
		   				List relatedObjects = (List) EcoreUtils.getValue(
		   						problem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
		   				relatedObjects.add(nodesList);
		   				relatedObjects.add(classTypesObj);
	   				    retList.add(problem);
	        	   } 
	           }
	           for (int i = 0; i < nodesList.size(); i++) {
	        	   EObject nodeObj = (EObject) nodesList.get(i);
	        	   boolean isNodeConfigured = false;
		           for (int j = 0; j < slotList.size(); j++) {
		        	   EObject classTypesObj = (EObject) EcoreUtils.getValue(
		        			   (EObject) slotList.get(j), ModelConstants.SLOT_CLASSTYPES_OBJECT_REF);
		        	   List classTypesList = (List) EcoreUtils.getValue(
		        			   classTypesObj, ModelConstants.SLOT_CLASSTYPE_LIST_REF);
		        	   for (int k = 0; k < classTypesList.size(); k++) {
		        		   String name = EcoreUtils.getValue((EObject) classTypesList.get(k), "name").toString();
	        		   if (name.equals(EcoreUtils.getName(nodeObj))) {
	        			   isNodeConfigured = true;
	        		   }
		        	   }
		           }
		           if (!isNodeConfigured) {
		        	   EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(74));
		        	   EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
	   				   EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
	   								EcoreUtils.getName(nodeObj) + " is not configured in any of the available slots");
	   				   EStructuralFeature srcFeature = problem.eClass()
	   						.getEStructuralFeature(PROBLEM_SOURCE);
	   				   problem.eSet(srcFeature, nodeObj);
	   				   List relatedObjects = (List) EcoreUtils.getValue(
	   						   problem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
	   				   relatedObjects.add(slotsObj);
	   				   retList.add(problem);
		           }
	           }
	           
	           EObject chassisObj = (EObject) ((List) EcoreUtils.getValue(
	        		   (EObject) _resEditorObjects.get(0), 
	        		   ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME)).get(0);
	           int numSlots = ((Integer) EcoreUtils.getValue(chassisObj,
	        		   ClassEditorConstants.CHASSIS_NUM_SLOTS)).intValue();
	           if (numSlots != slotList.size()) {
	        	   EObject problem = (EObject) _problemNumberObjMap.get(new Integer(75));
   				   EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
   						"Mismatch in 'Number of slots'"
						+ " specified in chassis and number of slots"
						+ " configured in Node Admission Control ");
   				   List relatedObjects = (List) EcoreUtils.getValue(
   						   problem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
   				   relatedObjects.add(slotsObj);
   				   relatedObjects.add(chassisObj);
   				   retList.add(problem);
	           }
				
			} else {
				//since ASP does not start up without this file IDE generates default config file
				/*EObject problem = (EObject) _problemNumberObjMap.get(new Integer(50));
				EcoreUtils
						.setValue(problem, PROBLEM_MESSAGE,
								"The Slot Configuration is not done");
				retList.add(problem);*/
			}
		} catch (Exception exc) {
			LOG.error("Error Reading clSlotInfo Resource.", exc);
		}
	}
	/**
	 * 
	 * Check for any of the hanging nodes in Component Editor
	 * 
	 * @param probsList -
	 *            List of all problems
	 */
    private void checkForIsolatedResources(List probsList) {
		EObject rootObject = (EObject) _resEditorObjects.get(0);
		String refList[] = ClassEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject obj = (EObject) list.get(j);
				if (!obj.eClass().getName().equals(
						ClassEditorConstants.MIB_RESOURCE_NAME)
						&& !isValidResource(obj)) {
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(7));
					EObject isolatedResProblem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils.setValue(isolatedResProblem, PROBLEM_MESSAGE,
							EcoreUtils.getName(obj) + " does not have "
									+ "relationship to any other object");
					EStructuralFeature srcFeature = isolatedResProblem.eClass()
							.getEStructuralFeature(PROBLEM_SOURCE);
					isolatedResProblem.eSet(srcFeature, obj);
					probsList.add(isolatedResProblem);
				}
			}
		}
	}
    /**
	 * check for any hanging resources which does not belong to resource
	 * heirarchy in the resource editor
	 * 
	 * @param probsList -
	 *            List of all problems
	 */
    private void checkForIsolatedComponents(List probsList) {
		EObject rootObject = (EObject) _compEditorObjects.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject obj = (EObject) list.get(j);
				if (!isValidSAFNode(obj)) {
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(43));
					EObject isolatedCompProblem = EcoreCloneUtils.cloneEObject(problemDataObj);
					EcoreUtils.setValue(isolatedCompProblem, PROBLEM_MESSAGE,
							EcoreUtils.getName(obj) + " does not have "
									+ "relationship to any other object");
					
					EStructuralFeature srcFeature = isolatedCompProblem
							.eClass().getEStructuralFeature(PROBLEM_SOURCE);
					isolatedCompProblem.eSet(srcFeature, obj);
					List relatedObjects = (List) EcoreUtils.getValue(
							isolatedCompProblem, PROBLEM_RELATED_OBJECTS);
					relatedObjects.add(obj);
					probsList.add(isolatedCompProblem);
				}
			}

		}
	}
        /**
		 * @param resList
		 *            List of Resources in Resource editor
		 * @param retList
		 *            Problems List
		 */
    private void validateDeviceIDsAndAlarmsOnResources(List resList, List retList)
    {
        HashMap idAlarmsMap = new HashMap();
        for(int i = 0; i < _alarmProfileObjects.size(); i++){
            EObject alarmObj = (EObject) _alarmProfileObjects.get(i);
            String alarmName = (String) EcoreUtils.getName(alarmObj); 
            idAlarmsMap.put(alarmName, alarmObj);
        }
    	for(int i = 0; i < resList.size(); i++){
    		EObject resObj = (EObject) resList.get(i);
    		HashMap alarmPcMap = new HashMap();
    		List associatedAlarms = ResourceDataUtils.getAssociatedAlarms(_project, resObj);
            HashSet probableCauseList = new HashSet();
            HashSet problemPcs = new HashSet();
            
            HashMap pcProblemMap = new HashMap();
            if (associatedAlarms != null)
            {
				for (int j = 0; j < associatedAlarms.size(); j++) {
					// check for any duplicate probable cause with in the associated alarms.
					String alarmID = (String)associatedAlarms.get(j);
	                EObject alarmObj = (EObject) idAlarmsMap.get(alarmID);
	                if (alarmObj != null)
	                {
	                    String pc = EcoreUtils.getValue(alarmObj,
	                        ClassEditorConstants.ALARM_PROBABLE_CAUSE).toString();
	                    String sp = EcoreUtils.getValue(alarmObj,
		                        ClassEditorConstants.ALARM_SPECIFIC_PROBLEM).toString();
	                    String pc_sp_id = pc + "_" + sp;
	                    alarmPcMap.put(alarmObj, pc_sp_id);
	                    if (!probableCauseList.add(pc_sp_id)) {
	                    	if (problemPcs.add(pc_sp_id)) {
	                    		EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(1));
	                    		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
	                            EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
	                                    "Resource is associated with alarms which have same"
	                                    + " probable cause '" + pc + "' and specific problem '" + sp + "'");
	                           
	                            EStructuralFeature srcFeature = problem.eClass().
	                                getEStructuralFeature(PROBLEM_SOURCE);
	                            problem.eSet(srcFeature, resObj);
	                            retList.add(problem);
	                            pcProblemMap.put(pc_sp_id, problem);
	                    	}
	                    }
	                } else {
	                    // check for alarm profile associated to resource is deleted from pool of
	                    //alarms
	
	                	EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(2));
	                	EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
	                    EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
	                            "Associated Alarm [" +alarmID+"]"
	                            + " does not exist in alarm profiles");
	                    
	                    EStructuralFeature srcFeature = problem.eClass().
	                        getEStructuralFeature(PROBLEM_SOURCE);
	                    problem.eSet(srcFeature, resObj);
	                    List relatedObjList = (List) EcoreUtils.getValue(
	                    		problem, PROBLEM_RELATED_OBJECTS);
	                    relatedObjList.add(resObj);
	                    relatedObjList.add(alarmID);
	                    retList.add(problem);
					}
	             
				}
            }
			Iterator iterator = problemPcs.iterator();
			while (iterator.hasNext()) {
				String pc = (String) iterator.next();
				List alarmObjList = ClovisUtils.getKeysForValue(alarmPcMap, pc);
				EObject problem = (EObject) pcProblemMap.get(pc);
				List relatedObjList = (List) EcoreUtils.getValue(problem, PROBLEM_RELATED_OBJECTS);
				relatedObjList.add(resObj);
				relatedObjList.add(alarmObjList);
			}
    	}    	
    }
    
	/**
    * @param alarmID Alarm ID string (Probable Cause)
    * @param boolean true if exists else false
    * Associated alarmIDs are same as the Probable Cause in Alarm profile 
    */
    private boolean alarmIDExists(String alarmID)
    {
    	boolean retValue = false;
    	if(_alarmProfileObjects != null){
    		for(int i=0; i < _alarmProfileObjects.size(); i++){
    			EObject alarmProfileObject = (EObject)_alarmProfileObjects.get(i);
    			String alarmKey = (String) EcoreUtils.
                getValue(alarmProfileObject, "CWKEY");    			
    			if (alarmKey != null && alarmKey.equals(alarmID)){
    				retValue = true;
    			}
    		}
    	}
    	return retValue;
    }
    /**
    *
    * @param compList Editor Component List
    * @param retList Problems List
    */
   private void validateForProvLibSelection(List compList, List retList) {
		for (int i = 0; i < compList.size(); i++) {
			EObject compObj = (EObject) compList.get(i);
			List associatedResList = ComponentDataUtils.getAssociatedResources(
					_project, compObj);
			HashSet resObjList = new HashSet();
			if (associatedResList != null) {
				for (int j = 0; j < associatedResList.size(); j++) {
					String resName = (String) associatedResList.get(j);
					EObject resObj = _resUtils.getObjectFrmName(resName);
					if (resObj != null) {
						resObjList.add(resObj);
					}
				}
			}
			EObject eoPropEObj = (EObject) EcoreUtils.getValue(compObj,
					ComponentEditorConstants.EO_PROPERTIES_NAME);
			if (eoPropEObj != null) {
				EObject eoASPLib = (EObject) EcoreUtils.getValue(eoPropEObj,
						ComponentEditorConstants.EO_ASPLIB_NAME);
				String provSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_PROVLIB_NAME).toString();
				HashMap resInfoMap = new HashMap();
				Iterator iterator = resObjList.iterator();
				boolean associatedDOExists = false;

				while (iterator.hasNext()) {
					EObject resObj = (EObject) iterator.next();
					EObject provObj = (EObject) EcoreUtils.getValue(resObj,
							ClassEditorConstants.RESOURCE_PROVISIONING);
					if (provObj != null) {
						boolean enable = ((Boolean) EcoreUtils.getValue(
								provObj, "isEnabled")).booleanValue();
						if (enable) {

							List associatedDOList = (List) EcoreUtils
									.getValue(provObj,
											ClassEditorConstants.ASSOCIATED_DO);
							if (associatedDOList != null
									&& !associatedDOList.isEmpty())
								associatedDOExists = true;
							if (resObj
									.eClass()
									.getName()
									.equals(
											ClassEditorConstants.SOFTWARE_RESOURCE_NAME)
									|| resObj
											.eClass()
											.getName()
											.equals(
													ClassEditorConstants.MIB_RESOURCE_NAME)) {
								resInfoMap.put(resObj,
										"ProvEnabled,SoftwareResource");
							} else {
								resInfoMap.put(resObj,
										"ProvEnabled,HardwareResource");
							}
						}
					}
				}
				boolean provEnable = resInfoMap
						.containsValue("ProvEnabled,HardwareResource")
						|| resInfoMap
								.containsValue("ProvEnabled,SoftwareResource");
				boolean provEnabledHardware = resInfoMap
						.containsValue("ProvEnabled,HardwareResource");
				boolean provEnabledOnlySoftware = resInfoMap
						.containsValue("ProvEnabled,SoftwareResource")
						&& !resInfoMap
								.containsValue("ProvEnabled,HardwareResource");
				boolean provNotEnabled = !resInfoMap
						.containsValue("ProvEnabled,HardwareResource")
						&& !resInfoMap
								.containsValue("ProvEnabled,SoftwareResource");
				String halSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_HALLIB_NAME).toString();
				String omSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_OMLIB_NAME).toString();
				String txnSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_TRANSACTIONLIB_NAME)
						.toString();
				// check for the case where one of the associated resource has
				// prov enabled, but HAL/OM library is not selected.
				if (provEnabledHardware) {
					if (associatedDOExists && halSelValue.equals("CL_FALSE")) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(14));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils
								.setValue(
										problem,
										PROBLEM_MESSAGE,
										"One of associated resources has provisioning"
												+ " as enabled with associated device object(s)"
												+ " but HAL Library is not selected");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
					}
					if (omSelValue.equals("CL_FALSE")) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(15));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils
								.setValue(
										problem,
										PROBLEM_MESSAGE,
										"One of associated resources has provisioning"
												+ " as enabled but OM Library is not selected");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
					}
				} else if (provEnabledOnlySoftware) {

					if (omSelValue.equals("CL_FALSE")) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(15));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils
								.setValue(
										problem,
										PROBLEM_MESSAGE,
										"One of associated resources has provisioning"
												+ " as enabled but OM Library is not selected");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
					}
				}
				// check for the case where prov lib is selected but associated
				// resources does not have prov enabled
				if (provSelValue.equals("CL_TRUE")) {
					if (provNotEnabled && !resObjList.isEmpty()) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(17));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils
								.setValue(
										problem,
										PROBLEM_MESSAGE,
										"PROV library for component is selected but none of "
												+ "associated resource has provisioning as enabled");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
					}
					if (txnSelValue.equals("CL_FALSE")) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(54));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils
								.setValue(problem, PROBLEM_MESSAGE,
										"Transaction lib is mandatory when Prov lib for component is selected");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
					}
				} else {
					if (provEnable) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(18));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
								"PROV library for component is not "
										+ "selected but associated resource "
										+ "has provisioning as enabled");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
					}
				}

			}

		}
	}
   /**
	 * 
	 * @param compList
	 *            Editor Component List
	 * @param retList
	 *            Problems List
	 */
  private void validateForAlarmLibSelection(List compList, List retList) {
		for (int i = 0; i < compList.size(); i++) {
			EObject compObj = (EObject) compList.get(i);
			List associatedResList = ComponentDataUtils.getAssociatedResources(
					_project, compObj);
			List resObjList = new Vector();
			if (associatedResList != null) {
				for (int j = 0; j < associatedResList.size(); j++) {
					String resName = (String) associatedResList.get(j);
					EObject resObj = _resUtils.getObjectFrmName(resName);
					if (resObj != null) {
						resObjList.add(resObj);
					}
				}
			}
			HashMap resInfoMap = new HashMap();
			EObject eoPropEObj = (EObject) EcoreUtils.getValue(compObj,
					ComponentEditorConstants.EO_PROPERTIES_NAME);
			if (eoPropEObj != null) {
				EObject eoASPLib = (EObject) EcoreUtils.getValue(eoPropEObj,
						ComponentEditorConstants.EO_ASPLIB_NAME);
				String alarmSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_ALARMLIB_NAME).toString();

				boolean associatedDOExists = false;
				for (int j = 0; j < resObjList.size(); j++) {
					EObject resObj = (EObject) resObjList.get(j);
					EObject alarmObj = (EObject) EcoreUtils.getValue(resObj,
							ClassEditorConstants.RESOURCE_ALARM);
					if (alarmObj != null) {
						boolean enable = ((Boolean) EcoreUtils.getValue(
								alarmObj, "isEnabled")).booleanValue();
						if (enable) {
							List associatedDOList = (List) EcoreUtils.getValue(
									alarmObj,
									ClassEditorConstants.ASSOCIATED_DO);
							if (associatedDOList != null
									&& !associatedDOList.isEmpty())
								associatedDOExists = true;
							if (resObj
									.eClass()
									.getName()
									.equals(
											ClassEditorConstants.SOFTWARE_RESOURCE_NAME)
									|| resObj
											.eClass()
											.getName()
											.equals(
													ClassEditorConstants.MIB_RESOURCE_NAME)) {
								resInfoMap.put(resObj,
										"AlarmEnabled,SoftwareResource");
							} else {
								resInfoMap.put(resObj,
										"AlarmEnabled,HardwareResource");
							}

						}
					}
				}
				boolean alarmEnable = resInfoMap
						.containsValue("AlarmEnabled,HardwareResource")
						|| resInfoMap
								.containsValue("AlarmEnabled,SoftwareResource");
				boolean alarmEnabledHardware = resInfoMap
						.containsValue("AlarmEnabled,HardwareResource");
				boolean alarmEnabledOnlySoftware = resInfoMap
						.containsValue("AlarmEnabled,SoftwareResource")
						&& !resInfoMap
								.containsValue("AlarmEnabled,HardwareResource");
				boolean alarmNotEnabled = !resInfoMap
						.containsValue("AlarmEnabled,HardwareResource")
						&& !resInfoMap
								.containsValue("AlarmEnabled,SoftwareResource");

				String halSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_HALLIB_NAME).toString();
				String omSelValue = EcoreUtils.getValue(eoASPLib,
						ComponentEditorConstants.EO_OMLIB_NAME).toString();
				// check for the case where one of the associated resource has
				// prov enabled, but HAL/OM library is not selected.
				if (alarmEnabledHardware) {
					if (associatedDOExists && halSelValue.equals("CL_FALSE")) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(14));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils
								.setValue(
										problem,
										PROBLEM_MESSAGE,
										"One of associated resources has alarm as enabled with associated device object(s) "
												+ " but HAL Library is not selected");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
					}
					if (omSelValue.equals("CL_FALSE")) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(15));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
								"One of associated resources has alarm as enabled"
										+ " but OM Library is not selected");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
					}
				} else if (alarmEnabledOnlySoftware) {

					if (omSelValue.equals("CL_FALSE")) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(15));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
								"One of associated resources has alarm as enabled"
										+ " but OM Library is not selected");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
					}
				}
				// check for the case where alarm lib is selected but associated
				// resources does not have alarm enabled
				if (alarmSelValue.equals("CL_TRUE")) {
					if (alarmNotEnabled && !resObjList.isEmpty()) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(19));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils
								.setValue(
										problem,
										PROBLEM_MESSAGE,
										"ALARM library for component is selected but none of "
												+ "associated resources has alarm as enabled");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
					}
				} else {
					if (alarmEnable) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(20));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
								"ALARM library for component is not "
										+ "selected but associated resource "
										+ "has alarm as enabled");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, compObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(eoASPLib);
						retList.add(problem);
						break;
					}
				}

			}

		}
	}
    /**
	 * 
	 * @param compList -
	 *            Editor Component List
	 * @param resList -
	 *            Editor resource list
	 * @param retList -
	 *            Problem List
	 */
    private void validateForAssociatedResources(List compList, List resList, List retList)
    {    	
        for (int i = 0; i < compList.size(); i++) {
        	List priorities = new ArrayList();
            EObject compObj = (EObject) compList.get(i);
            List associatedResList = ComponentDataUtils.
        		getAssociatedResources(_project, compObj);
            String compProperty = EcoreUtils.getValue(compObj,
                    ComponentEditorConstants.COMPONENT_PROPERTY).toString();
            if (!compProperty.equals(ComponentEditorConstants.
                    PROXIED_PREINSTANTIABLE)) {
                if (associatedResList == null || associatedResList.isEmpty()) {
                	/*
					 * EObject problemDataObj = (EObject)
					 * _problemNumberObjMap.get(new Integer(11)); EObject
					 * noAssociatedResProblem =
					 * EcoreCloneUtils.cloneEObject(problemDataObj);
					 * EcoreUtils.setValue(noAssociatedResProblem,
					 * PROBLEM_MESSAGE, "Component has no associated
					 * resources");
					 * 
					 * EStructuralFeature srcFeature =
					 * noAssociatedResProblem.eClass().
					 * getEStructuralFeature(PROBLEM_SOURCE);
					 * noAssociatedResProblem.eSet(srcFeature, compObj); List
					 * relatedObjects = (List) EcoreUtils.getValue(
					 * noAssociatedResProblem, PROBLEM_RELATED_OBJECTS);
					 * 
					 * relatedObjects.add(resList);
					 * retList.add(noAssociatedResProblem);
					 */
                }
            /*
			 * Check that DOs for associated resources must not have dupliate
			 * bootup priorities.
			 */             
                else{
	            	for( int j = 0; (associatedResList != null && j < associatedResList.size()); j++){
	            		String resName = (String) associatedResList.get(j);
	            		EObject resObj = _resNameObjectMap.get(resName);
	//            		Associated Resource does not exist in Resource Editor Model
	            		if (resObj == null) {
	            			EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(12));
	            			EObject missingAssociatedResProblem = EcoreCloneUtils.cloneEObject(problemDataObj);
	                        EcoreUtils.setValue(missingAssociatedResProblem, PROBLEM_MESSAGE,
	                                "Associated Resource [" + resName + "] does not exist in " +
	                                "List of Resources in the Model. Please update the List of " +
	                                "Associated Resources for the component.");
	                        
	                        EStructuralFeature srcFeature = missingAssociatedResProblem.eClass().
	                            getEStructuralFeature(PROBLEM_SOURCE);
	                        missingAssociatedResProblem.eSet(srcFeature, compObj);
	                        List relatedObjects = (List) EcoreUtils.getValue(
	                        		missingAssociatedResProblem, PROBLEM_RELATED_OBJECTS);
	                        relatedObjects.add(resName);
	            			retList.add(missingAssociatedResProblem);
	            		}
	            	}
                }
        	}
        }
    }
    
    /**
     * Validation for Proxied Component
     * @param compList proxied component list
     * @param retList problems list
     */
    private void validateProxiedComponents(List retList) {
    	List proxiedConnectionList  = _compUtils.getConnectionFrmType(
                ComponentEditorConstants.PROXY_PROXIED_NAME,
                ComponentEditorConstants.SAFCOMPONENT_NAME,
                ComponentEditorConstants.NONSAFCOMPONENT_NAME);
    	List proxiedCompList = new ArrayList();
    	for(int i = 0; i < proxiedConnectionList.size(); i++) {
    		EObject conObj = (EObject)proxiedConnectionList.get(i);
    		if(_connTrgMap.get(conObj) != null) {
    			proxiedCompList.add(_connTrgMap.get(conObj));
    		}
    	}
    	for (int i = 0; i < proxiedCompList.size(); i++) {
    		EObject comp = (EObject) proxiedCompList.get(i);
    		if(!isValidProxiedComponent(comp)) {
    			EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(13));
				EObject proxiedResProblem = EcoreCloneUtils.cloneEObject(problemDataObj); 
				EcoreUtils.setValue(proxiedResProblem, PROBLEM_MESSAGE,
						"Proxied component " + EcoreUtils.getName(comp) + " doesn't belongs to proper SG and Node hierarchy");
				EStructuralFeature srcFeature = proxiedResProblem.eClass()
						.getEStructuralFeature(PROBLEM_SOURCE);
				proxiedResProblem.eSet(srcFeature, comp);
				retList.add(proxiedResProblem);
    		}
    	}
    }
    /**
     * Verify proxied component hierarchy.
     * @param comp
     * @return boolean
     */
    private boolean isValidProxiedComponent(EObject comp) {
    	List parents1 = getParents(comp);
		boolean validSG = false;
		boolean validNode = false;
		for(int j = 0; j < parents1.size(); j++) {
			EObject obj2 = (EObject) parents1.get(j);
			if(obj2.eClass().getName().equals(ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME)) {
				List parents2 = getParents(obj2);
				for(int k = 0; k < parents2.size(); k++){
					EObject obj3 = (EObject) parents2.get(k);
					if(obj3.eClass().getName().equals(ComponentEditorConstants.SERVICEINSTANCE_NAME)) {
						List parents3 = getParents(obj3);
						for(int l = 0; l < parents3.size(); l++) {
							EObject obj4 = (EObject) parents3.get(k);
							if(obj4.eClass().getName().equals(ComponentEditorConstants.SERVICEGROUP_NAME)) {
								validSG = true;
							}
						}
					}
				}
			}
			if(obj2.eClass().getName().equals(ComponentEditorConstants.SERVICEUNIT_NAME)) {
				List parents2 = getParents(obj2);
				for(int k = 0; k < parents2.size(); k++){
					EObject obj3 = (EObject) parents2.get(k);
					if(obj3.eClass().getName().equals(ComponentEditorConstants.NODE_NAME)) {
						validNode = true;
					}
				}
			}
		}
		return validSG && validNode;
    }
    /**
    * @param priorities List of boot priorities
    * @param bootPriority - priority value that needs to be checked for duplicatio.
    * @return boolean
    */    
    private boolean isDuplicateBootPriority(Integer priority, List priorities)
    {
    	boolean retValue = false;
    	for(int i=0; i<priorities.size(); i++){
    		Integer pri = (Integer)priorities.get(i);
    		if(priority.intValue() == pri.intValue()){
    			retValue = true;
    			break;
    		}
    	}
    	return retValue;
    }
   /**
    * 
    * @return all the CPM Instance Objects defined in the Node Profile
    */
   public List getCPMInstListFrmNodeProfile()
   {
       List cpmObjs = new Vector();
       EObject amfObj = (EObject) _nodeProfileObjects.get(0);
       EReference cpmConfigsRef = (EReference) amfObj.eClass()
           .getEStructuralFeature(SafConstants.CPM_CONFIGS_NAME);
       EObject cpmConfigsObj = (EObject) amfObj.eGet(cpmConfigsRef);
       if (cpmConfigsObj != null) {
           EReference cpmConfigRef = (EReference) cpmConfigsObj.eClass()
                .getEStructuralFeature(SafConstants.CPM_CONFIGLIST_NAME);
           cpmObjs = (List) cpmConfigsObj.eGet(cpmConfigRef);
       }
       return cpmObjs;
   }
   /**
    * Goes thru all node profiles and Validate 
    * @param probsList problems List
    */
   private void validateNodeProfiles(List probsList)
   {
	   List nodeObjs = ProjectDataModel.getNodeInstListFrmNodeProfile(_nodeProfileObjects);
	   HashMap <String, EObject> suInstancesMap = new HashMap<String, EObject>();
       if (nodeObjs != null) {
           validateNodeInstances(nodeObjs, probsList);
           validateSUInstances(nodeObjs, probsList);
           suInstancesMap = getSUInstancesMap(nodeObjs);
           //validateAMFResourceList(nodeObjs, probsList, "primaryOI");
           //validateAMFResourceList(nodeObjs, probsList, "autoCreate");
           validateAMFResourceList(nodeObjs, probsList);
       }
       
       List sgObjs = ProjectDataModel.getSGInstListFrmNodeProfile(_nodeProfileObjects);
       if (sgObjs != null) {
    	   validateNumCSIsPerSI(sgObjs, probsList);
    	   validateSG_SI_CSI_Instances(sgObjs, _validator.getfilterList(_compEditorObjects,
                   ComponentEditorConstants.COMPONENT_NAME),suInstancesMap, probsList);
           validateRedundancyModel(sgObjs, probsList);
       }
       
       /*
        * check whether NodeInstance and ServiceGroup in AMF configurations
        *  are defined or not.
        */
       if ( nodeObjs.size() == 0 ) {
    	   EObject prob = (EObject) _problemNumberObjMap.get(new Integer(39));
           EcoreUtils.setValue(prob, PROBLEM_MESSAGE,
                   "There are no Node instances defined in AMF Configuration");
           EObject amfObj = (EObject) _nodeProfileObjects.get(0);
           EReference nodeInstsRef = (EReference) amfObj.eClass()
               .getEStructuralFeature("nodeInstances");
           EObject nodeInstsObj = (EObject) amfObj.eGet(nodeInstsRef);
           prob.eSet(prob.eClass().getEStructuralFeature("source"), nodeInstsObj);
           probsList.add(prob);
       }
       
       if ( sgObjs.size() == 0 ) {
           
    	   EObject prob = (EObject) _problemNumberObjMap.get(new Integer(40));
           EcoreUtils.setValue(prob, PROBLEM_MESSAGE,
                   "There are no ServiceGroup instances defined in AMF Configuration");
           EObject amfObj = (EObject) _nodeProfileObjects.get(0);
           EReference sgInstsRef = (EReference) amfObj.eClass()
               .getEStructuralFeature("serviceGroups");
           EObject sgInstsObj = (EObject) amfObj.eGet(sgInstsRef);
           prob.eSet(prob.eClass().getEStructuralFeature("source"), sgInstsObj);
           probsList.add(prob);
       }       
       
       
       /*
        * check for duplicate AMF entities 
        */
       _validator.checkForUniqueness(nodeObjs,sgObjs, probsList);
       
       /* commenting this code because now cmpConfig has hardcoded bootlevels
        * uncomment it once the user defined boot levels are active
       List cpmObjs = getCPMInstListFrmNodeProfile();
       if (cpmObjs != null) {
           validateCPMConfigs(cpmObjs, probsList);
       }*/
       validateSUInstAssociativity(probsList);
	   
   }
   
   /**
    * Validate Node Instances
    * @param nodeInsts Node Isntances List
    * @param probsList Problems List
    */
   private void validateNodeInstances(List nodeInsts, List probsList)
   {
	   HashMap <String,EList> dependenciesMap = new HashMap <String, EList>();
	   try {
	   for (int i = 0; i < nodeInsts.size(); i++) {
		   EObject node = (EObject) nodeInsts.get(i);
		   String nodetype = (String)node.eGet(node.eClass().getEStructuralFeature("type"));
		   String nodename = (String)node.eGet(node.eClass().getEStructuralFeature("name"));
		   EObject dependenciesRef = (EObject)node.eGet((EReference)node.eClass().getEStructuralFeature("dependencies"));
		   dependenciesMap.put(nodename, (EList) EcoreUtils.getValue(dependenciesRef, "node"));
		   if(isValidComponentType(nodetype, ComponentEditorConstants.NODE_NAME,
				   ComponentEditorConstants.CONTAINMENT_NAME)) {
			   EReference serviceUnitInstsRef = (EReference) node.eClass()
	           .getEStructuralFeature(SafConstants.SERVICEUNIT_INSTANCES_NAME);
			   EObject serviceUnitInstsObj = (EObject) node.eGet(serviceUnitInstsRef);
			   if (serviceUnitInstsObj != null) {
		    	   EReference serviceUnitInstRef = (EReference) serviceUnitInstsObj.eClass()
		           .getEStructuralFeature(SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
		    	   List suObjs = (List) serviceUnitInstsObj.eGet(serviceUnitInstRef);
		    	   for (int j = 0; j < suObjs.size(); j++) {
		    		   EObject su = (EObject) suObjs.get(j);
		    		   String sutype = (String)su.eGet(su.eClass().getEStructuralFeature("type"));
		    		   if (isValidComponentType(sutype, ComponentEditorConstants.SERVICEUNIT_NAME,
                               ComponentEditorConstants.CONTAINMENT_NAME)) {
		    			   EReference componentInstsRef = (EReference) su.eClass()
		    	           .getEStructuralFeature(SafConstants.COMPONENT_INSTANCES_NAME);
		    			   EObject componentInstsObj = (EObject) su.eGet(componentInstsRef);
		    			   if (componentInstsObj != null) {
		    		    	   EReference componentInstRef = (EReference) componentInstsObj.eClass()
		    		           .getEStructuralFeature(SafConstants.COMPONENT_INSTANCELIST_NAME);
		    		    	   List compObjs = (List) componentInstsObj.eGet(componentInstRef);
		    		    	   for (int k = 0; k < compObjs.size(); k++) {
		    		    		   EObject comp = (EObject) compObjs.get(k);
		    		    		   String comptype = (String) comp.eGet(comp.eClass().getEStructuralFeature("type"));
		    		    		   if (isValidComponent(comptype)) {
		    		    			   String compname = (String) comp.eGet(comp.eClass().getEStructuralFeature("name"));
		    		    			   checkCompInstanceType(comp, nodename, compname, comptype, probsList);
		    		    		   } else {
		    		    			   EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(33));
		    		    			   EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
                                       EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
                                               "ComponentInstances Configuration " +
                                                "have Invalid Component "+ "'" + comptype + "'");
                                       EStructuralFeature srcFeature = problem.eClass().
                                           getEStructuralFeature(PROBLEM_SOURCE);
                                       problem.eSet(srcFeature, comp);
                                       List relatedObjects = (List) EcoreUtils.getValue(
                                       		 problem, PROBLEM_RELATED_OBJECTS);
                                      
                                       relatedObjects.add(componentInstsObj);
                                       relatedObjects.add(compObjs);
		    			    		   probsList.add(problem);
		    		    		   }
		    		    	   }
		    			   }
		    		   } else {
		    			   EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(33));
		    			   EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
                           EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
                                   "ServiceUnitInstances Configuration " +
                                    "have Invalid ServiceUnit "+ "'" + sutype + "'");
                           
                           EStructuralFeature srcFeature = problem.eClass().
                               getEStructuralFeature(PROBLEM_SOURCE);
                           problem.eSet(srcFeature, su);
                           List relatedObjects = (List) EcoreUtils.getValue(
                             		 problem, PROBLEM_RELATED_OBJECTS);
                            relatedObjects.add(serviceUnitInstsObj);
                            relatedObjects.add(suObjs);
		    			    probsList.add(problem);
		    		   }
		    	   }
		       }
		   } else {
			   EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(33));
			   EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
               EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
                       "NodeInstances Configuration " +
                    "have Invalid Node "+ "'" + nodetype + "'");
               
               EStructuralFeature srcFeature = problem.eClass().
                   getEStructuralFeature(PROBLEM_SOURCE);
               problem.eSet(srcFeature, node);
               List relatedObjects = (List) EcoreUtils.getValue(
               		 problem, PROBLEM_RELATED_OBJECTS);
              
              relatedObjects.add(node.eContainer());
              relatedObjects.add(nodeInsts);
			  probsList.add(problem);
		   }
		   
	   }
	   for (int i = 0; i < nodeInsts.size(); i++) {
		   EObject node = (EObject) nodeInsts.get(i);
		   EObject dependenciesRef = (EObject)node.eGet((EReference)node.eClass().getEStructuralFeature("dependencies"));
		   EList dependenciesNames = (EList) EcoreUtils.getValue(dependenciesRef, "node");
		   validateCircularDependencies(node, dependenciesNames, dependenciesMap, probsList, new Integer(88));
	   }
	   } catch (Exception e) {
		   LOG.warn("Unhandled error while validating node instances", e);
	   }
    }
   /**
    *Checks whether the SU instance has any component instance inside it or not 
    * @param nodeInsts
    * @param probsList
    */
   private void validateSUInstances(List nodeInsts, List probsList)
   {
       try {
           for (int i = 0; i < nodeInsts.size(); i++) {
               
               EObject node = (EObject) nodeInsts.get(i);
               String nodetype = (String)node.eGet(node.eClass().getEStructuralFeature("type"));
               
               if(isValidComponentType(nodetype, ComponentEditorConstants.NODE_NAME,
                       ComponentEditorConstants.CONTAINMENT_NAME)) {
                   
                   EReference serviceUnitInstsRef = (EReference) node.eClass()
                   .getEStructuralFeature(SafConstants.SERVICEUNIT_INSTANCES_NAME);
                   EObject serviceUnitInstsObj = (EObject) node.eGet(serviceUnitInstsRef);
                   
                   if (serviceUnitInstsObj != null) {
                       EReference serviceUnitInstRef = (EReference) serviceUnitInstsObj.eClass()
                       .getEStructuralFeature(SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
                       List suObjs = (List) serviceUnitInstsObj.eGet(serviceUnitInstRef);
                       
                       for (int j = 0; j < suObjs.size(); j++) {
                           EObject su = (EObject) suObjs.get(j);
                           String sutype = (String)su.eGet(su.eClass().getEStructuralFeature("type"));
                           if (isValidComponentType(sutype, ComponentEditorConstants.SERVICEUNIT_NAME,
                                   ComponentEditorConstants.CONTAINMENT_NAME)) {
                               
                               EReference componentInstsRef = (EReference) su.eClass()
                               .getEStructuralFeature(SafConstants.COMPONENT_INSTANCES_NAME);
                               EObject componentInstsObj = (EObject) su.eGet(componentInstsRef);
                               
                               if (componentInstsObj != null) {
                               
                                   EReference componentInstRef = (EReference) componentInstsObj.eClass()
                                   .getEStructuralFeature(SafConstants.COMPONENT_INSTANCELIST_NAME);
                                   List compObjs = (List) componentInstsObj.eGet(componentInstRef);
                                   if (compObjs.isEmpty()) {
                                	   EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(34));
                                	   EObject noCompInstForSU = EcoreCloneUtils.cloneEObject(problemDataObj); 
                                       EcoreUtils.setValue(noCompInstForSU, PROBLEM_MESSAGE,
                                               "ServiceUnit instance does not have any Component "
                                               + "instances defined");
                                       
                                       EStructuralFeature srcFeature = noCompInstForSU.eClass().
                                           getEStructuralFeature(PROBLEM_SOURCE);
                                       noCompInstForSU.eSet(srcFeature, su);
                                       List relatedObjects = (List) EcoreUtils.getValue(
                                    		   noCompInstForSU, PROBLEM_RELATED_OBJECTS);
                                        
                                       relatedObjects.add(serviceUnitInstsObj);
                                       relatedObjects.add(suObjs);
                                       probsList.add(noCompInstForSU);
                                   } else {
                                	   validateResourcesForNodeProfileComponents(compObjs, probsList);
                                   }
                               }
                           } 
                       }
                   }
               }
           }
           } catch (Exception e) {
               LOG.warn("Unhandled error while validating su instances", e);
           }
       
   }

   	/**
	 * Checks for the missing associated resources for the components in the
	 * component list.
	 * 
	 * @param compList -
	 *            Component Instance List
	 * @param probsList -
	 *            Problem List
	 */
	private void validateResourcesForNodeProfileComponents(List compList,
			List probsList) {
		EObject rootObject = (EObject) _compEditorObjects.get(0);
		EReference ref = (EReference) rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.SAFCOMPONENT_REF_NAME);
		EList editorCompList = (EList) rootObject.eGet(ref);

		Iterator compItr = compList.iterator();
		while (compItr.hasNext()) {
			EObject comp = (EObject) compItr.next();
			EObject editorComp = null;
			String compType = EcoreUtils.getValue(comp, "type").toString();
			for (int i = 0; i < editorCompList.size(); i++) {
				if (compType.equals(EcoreUtils.getName((EObject) editorCompList
						.get(i)))) {
					editorComp = (EObject) editorCompList.get(i);
					break;
				}
			}
							
						
			List editorResourceList = null;
			if (editorComp != null) {
				editorResourceList = ComponentDataUtils.
        		getAssociatedResources(_project, editorComp);
			}
			
			List nodeProfileResourceList = (List) getResourceListForComponent(comp);
			
			if (editorResourceList != null) {
				for (int j = 0; j < editorResourceList.size(); j++) {
					String editorResource = editorResourceList.get(j).toString();
					boolean error = true;
					
					for (int k = 0; k < nodeProfileResourceList.size(); k++) {
						EObject nodeProfileResourceObj = (EObject) nodeProfileResourceList
								.get(k);
						String moID = EcoreUtils.getValue(nodeProfileResourceObj,
								"moID").toString();

						int start = moID.lastIndexOf("\\");
						int end = moID.lastIndexOf(":");

						if (start != -1 && end != -1) {
							String nodeProfileResource = moID.substring(start + 1,
									end);
							if (editorResource.equals(nodeProfileResource)) {
								error = false;
								break;
							}
						}
					}
					if (error) {
						EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(49));
						EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
						EObject nodeInstObj = comp.eContainer().eContainer()
								.eContainer().eContainer();
						EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
								"Resource '" + editorResource + "' is not associated for the Component '"
										+ EcoreUtils.getName(comp) + "' under '"
										+ EcoreUtils.getName(nodeInstObj) + "'");
			           
						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, comp);
						probsList.add(problem);
					}
				}
			}
		}
	}

	/**
	 * Returns the resource list associated with the given component.
	 * 
	 * @param component -
	 *            the component for which resources are to be found
	 * @return the resources for the component
	 */
	private List getResourceListForComponent(EObject component) {
		EObject nodeInstObj = component.eContainer().eContainer().eContainer()
				.eContainer();
		String nodeInstName = EcoreUtils.getName(nodeInstObj);

		Resource nodeInstResource = null;
		try {
			String dataFilePath = _project.getLocation().toOSString()
					+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
					+ File.separator + nodeInstName + "_"
					+ SafConstants.RT_SUFFIX_NAME;
			URI uri = URI.createFileURI(dataFilePath);
			File xmlFile = new File(dataFilePath);

			nodeInstResource = xmlFile.exists() ? EcoreModels
					.getUpdatedResource(uri) : null;

		} catch (Exception exc) {
			LOG.error("Error Reading CompRes Resource.", exc);
		}

		if (nodeInstResource != null
				&& nodeInstResource.getContents().size() > 0) {
			EObject compInstancesObj = (EObject) nodeInstResource.getContents()
					.get(0);
			List compInstList = (List) EcoreUtils.getValue(compInstancesObj,
					"compInst");
			String compName = EcoreUtils.getName(component);
			for (int j = 0; compInstList != null && j < compInstList.size(); j++) {
				EObject compObj = (EObject) compInstList.get(j);
				String compInstName = EcoreUtils.getName(compObj);
				if (compName.equals(compInstName)) {
					return (EList) EcoreUtils.getValue(compObj, "resource");
				}
			}
		}
		return new ArrayList();
	}
	
	private List getCSITypesForSGType(String serviceGroupType)
	{
		ArrayList<String> csiTypes = new ArrayList<String>();
		
		EObject serviceGroupTypeObject = _compNameObjectMap.get(serviceGroupType);
		//List serviceGroupChildren = _compUtils.getChildren(serviceGroupTypeObject);
		List serviceGroupChildren = (List)getChildrens(serviceGroupTypeObject);
		for (int i=0; i<serviceGroupChildren.size(); i++)
		{
			EObject serviceInstanceTypeObject = (EObject)serviceGroupChildren.get(i);
			String childType = serviceInstanceTypeObject.eClass().getName();
			if (childType.equals(ComponentEditorConstants.SERVICEINSTANCE_NAME))
			{
				//List serviceInstanceChildren = _compUtils.getChildren(serviceInstanceTypeObject);
				List serviceInstanceChildren = (List)getChildrens(serviceInstanceTypeObject);
				for (int j=0; j<serviceInstanceChildren.size(); j++)
				{
					EObject componentServiceInstanceTypeObject = (EObject)serviceInstanceChildren.get(j);
					String subChildType = componentServiceInstanceTypeObject.eClass().getName();
					if (subChildType.equals(ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME))
					{
						csiTypes.add((String)EcoreUtils.getName(componentServiceInstanceTypeObject));
					}
				}
			}
		}
		return csiTypes;
	}

	/**
	 * 
	 * @param sgInsts -
	 *            SG Instance List
	 * @param probsList -
	 *            problems to be returned
	 */
	private void validateNumCSIsPerSI(List sgInsts, List probsList)
	{
		// This code should be moved to SG_SI_Instances() method
		List siTypeList = _validator.getfilterList(_compEditorObjects, ComponentEditorConstants.SERVICEINSTANCE_NAME);
		HashMap nameObjectMap = new HashMap();
		for (int i = 0; i < siTypeList.size(); i++) {
			EObject si = (EObject) siTypeList.get(i);
			nameObjectMap.put(EcoreUtils.getName(si), si);
		}
		
		// Get all of the service group types and store them along with a list of all the
		//  component service instance types that they contain
		List sgTypeList = _validator.getfilterList(_compEditorObjects, ComponentEditorConstants.SERVICEGROUP_NAME);
		HashMap sgTypeCSIs = new HashMap();
		for (int i = 0; i < sgTypeList.size(); i++)
		{
			EObject sgType = (EObject) sgTypeList.get(i);
			String sgTypeName = EcoreUtils.getName(sgType);
			sgTypeCSIs.put(sgTypeName, getCSITypesForSGType(sgTypeName));
		}
		
        List<String> sg_csi_instances = new ArrayList<String>();
        
		try {
			for (int i = 0; i < sgInsts.size(); i++) {

				EObject sg = (EObject) sgInsts.get(i);
				String sgtype = (String) sg.eGet(sg.eClass().getEStructuralFeature("type"));
				if(isValidComponentType(sgtype, ComponentEditorConstants.SERVICEGROUP_NAME, null)) {
                   EReference serviceInstanceInstsRef = (EReference) sg.eClass()
                   	.getEStructuralFeature(SafConstants.SERVICE_INSTANCES_NAME);
                   EObject serviceInstancesObj = (EObject) sg.eGet(serviceInstanceInstsRef);
                   
                   if (serviceInstancesObj != null) {
                                                                                                                             
                       EReference serviceInstRef = (EReference) serviceInstancesObj.eClass()
                       	.getEStructuralFeature(SafConstants.SERVICE_INSTANCELIST_NAME);
                       List siObjs = (List) serviceInstancesObj.eGet(serviceInstRef);
                       for (int j = 0; j < siObjs.size(); j++) {
                                                                                                                             
                           EObject si = (EObject) siObjs.get(j);
                           String sitype = (String)si.eGet(si.eClass().getEStructuralFeature("type"));
                                                                                                                             
                           if (isValidComponentType(sitype, ComponentEditorConstants.SERVICEINSTANCE_NAME,
                                   ComponentEditorConstants.ASSOCIATION_NAME)) {
                               EReference csiInstsRef = (EReference) si.eClass()
                               .getEStructuralFeature(SafConstants.CSI_INSTANCES_NAME);
                               EObject csiInstsObj = (EObject) si.eGet(csiInstsRef);
                                                                                                                             
                               if (csiInstsObj != null) {
                                   EReference csiInstRef = (EReference) csiInstsObj.eClass()
                                       .getEStructuralFeature(SafConstants.CSI_INSTANCELIST_NAME);
                                   List csiInstances = (List) csiInstsObj.eGet(csiInstRef);
                                   
                                   // add the csi instances types to the list of found types
                                   for (int q=0; q<csiInstances.size(); q++)
                                   {
                            		   EObject csiInstance = (EObject) csiInstances.get(q);
                            		   String csiType = (String) EcoreUtils.getValue(csiInstance, "type");
                                	   sg_csi_instances.add(csiType);
                                   }
                                   
                                   EObject siTypeObj = (EObject) nameObjectMap.get(sitype);
                                   if (siTypeObj != null) {
                                       int numCSIs = ((Integer) EcoreUtils.
                                                       getValue(siTypeObj, "numCSIs")).intValue();
                                       String siType = ((String) EcoreUtils.
                                                       getValue(siTypeObj, "name"));
                                       //List <EObject> csiTypes = _compUtils.getChildren(siTypeObj);
                                       List <EObject> csiTypes = getChildrens(siTypeObj);
                                       if (csiInstances.size() != numCSIs) {
                                    	   EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(35));
                                    	   EObject extraCSIsProblem = EcoreCloneUtils.cloneEObject(problemDataObj);
                                           EcoreUtils.setValue(extraCSIsProblem, PROBLEM_MESSAGE,
                                                   "Number of CSI instances configured for" +
                                                   " SI instance is not consistence with what" +
                                                   " is specified in the SI type definition: " + siType);
                                         
                                           EStructuralFeature srcFeature = extraCSIsProblem.eClass().
                                               getEStructuralFeature(PROBLEM_SOURCE);
                                           extraCSIsProblem.eSet(srcFeature, si);
                                           List relatedObjects = (List) EcoreUtils.getValue(
                                        		   extraCSIsProblem, PROBLEM_RELATED_OBJECTS);
                                           relatedObjects.add(siTypeObj);
                                            relatedObjects.add(csiInstsObj);
                                            relatedObjects.add(csiInstances);
                                            
                                            probsList.add(extraCSIsProblem);
                                       }
                               } else {
									EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(33));
									EObject wrongSITypeProblem = EcoreCloneUtils.cloneEObject(problemDataObj);
									EcoreUtils.setValue(wrongSITypeProblem, PROBLEM_MESSAGE,
											"ServiceInstance instance configuration has"
											+ " invalid ServiceInstance type '"
											+ sitype + "'");
									
									EStructuralFeature srcFeature = wrongSITypeProblem.eClass().getEStructuralFeature(PROBLEM_SOURCE);
									wrongSITypeProblem.eSet(srcFeature, si);
									List relatedObjects = (List) EcoreUtils.getValue(wrongSITypeProblem, PROBLEM_RELATED_OBJECTS);
									relatedObjects.add(serviceInstancesObj);
									relatedObjects.add(siObjs);
									probsList.add(wrongSITypeProblem);
                               }
                           }
                       }
                   }
               }
           }
       }

		// if servive group instances have been defined then check if all of the component service
		//  instance types under each service group type have been created
		if (sgInsts.size() > 0)
		{
			// check to see if each of the component service instance types are represented
	        //  within a sg instance of the containing type
	        Iterator iter = sgTypeCSIs.keySet().iterator();
	        while (iter.hasNext())
	        {
	     	   String sgTypeName = (String)iter.next();
	     	   List csiTypes = (List)sgTypeCSIs.get(sgTypeName);
	            for (int w=0; w<csiTypes.size(); w++)
	            {
	         	   if (!sg_csi_instances.contains((String)csiTypes.get(w)))
	         	   {
	         		   EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(90));
	    	   		   EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
	    	   		   EcoreUtils.setValue(problem, PROBLEM_MESSAGE, "CSI type " + (String)csiTypes.get(w) + " is not used in the instance model.");
	    	   		   EStructuralFeature srcFeature = problem.eClass().
	    	   		   getEStructuralFeature(PROBLEM_SOURCE);
	    	   		   probsList.add(problem);
	         	   }
	            }
	        }
		}
                
       } catch (Exception e) {
           LOG.warn("Unhandled error while validating CSI instances", e);
       }
}
   
   /**
    *Checks whether the SU instance has any component instance inside it or not 
    * @param nodeInsts
    * @param probsList
    */
   private void validateSG_SI_CSI_Instances(List<EObject> sgInsts,
			List<EObject> compTypes, Map suInstancesMap, List<EObject> probsList) {
		try {
			HashMap<String, List<String>> compTypeMap = new HashMap<String, List<String>>();
			for (int i = 0; i < compTypes.size(); i++) {
				EObject comp = compTypes.get(i);
				//List <EObject> parents = _compUtils.getParents(comp);
				List <EObject> parents = getParents(comp);
				for (int j = 0; j < parents.size(); j++) {
					EObject parent = parents.get(j);
					if(parent.eClass().getName().equals(ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME)) {
						List<String> csiList = compTypeMap.get(EcoreUtils.getName(comp));
						if(csiList == null) {
							csiList = new ArrayList<String>();
							compTypeMap.put(EcoreUtils.getName(comp), csiList);
						}
						csiList.add(EcoreUtils.getName(parent));
					}
				}
			}
			HashMap<String, EList> dependenciesMap = new HashMap<String, EList>();
			HashMap<EObject, EList> instDependenciesMap = new HashMap<EObject, EList>();
			for (int i = 0; i < sgInsts.size(); i++) {

				EObject sg = (EObject) sgInsts.get(i);
				String sgtype = (String) sg.eGet(sg.eClass()
						.getEStructuralFeature("type"));
				if (isValidComponentType(sgtype,
						ComponentEditorConstants.SERVICEGROUP_NAME, null)) {

					EReference serviceInstanceInstsRef = (EReference) sg
							.eClass().getEStructuralFeature(
									SafConstants.SERVICE_INSTANCES_NAME);
					EObject serviceInstancesObj = (EObject) sg
							.eGet(serviceInstanceInstsRef);

					if (serviceInstancesObj != null) {

						EReference serviceInstRef = (EReference) serviceInstancesObj
								.eClass().getEStructuralFeature(
										SafConstants.SERVICE_INSTANCELIST_NAME);
						List siObjs = (List) serviceInstancesObj
								.eGet(serviceInstRef);
						if (siObjs.isEmpty()) {
							EObject problemDataObj = (EObject) _problemNumberObjMap
									.get(new Integer(36));
							EObject noSiInstForSG = EcoreCloneUtils
									.cloneEObject(problemDataObj);
							EcoreUtils.setValue(noSiInstForSG, PROBLEM_MESSAGE,
									"ServiceGroup instance does not have any Service"
											+ "Instance instances defined");

							EStructuralFeature srcFeature = noSiInstForSG
									.eClass().getEStructuralFeature(
											PROBLEM_SOURCE);
							noSiInstForSG.eSet(srcFeature, sg);
							List relatedObjects = (List) EcoreUtils.getValue(
									noSiInstForSG, PROBLEM_RELATED_OBJECTS);
							relatedObjects.add(sg.eContainer());
							relatedObjects.add(sgInsts);

							probsList.add(noSiInstForSG);
						}

						for (int j = 0; j < siObjs.size(); j++) {

							EObject si = (EObject) siObjs.get(j);
							String sitype = (String) si.eGet(si.eClass()
									.getEStructuralFeature("type"));
							EObject dependenciesRef = (EObject) si
									.eGet((EReference) si.eClass()
											.getEStructuralFeature(
													"dependencies"));
							EList dependencies = (EList) EcoreUtils.getValue(
									dependenciesRef, "serviceInstance");
							dependenciesMap.put(EcoreUtils.getName(si),
									dependencies);
							instDependenciesMap.put(si, dependencies);
							if (isValidComponentType(
									sitype,
									ComponentEditorConstants.SERVICEINSTANCE_NAME,
									ComponentEditorConstants.ASSOCIATION_NAME)) {
								EReference csiInstsRef = (EReference) si
										.eClass()
										.getEStructuralFeature(
												SafConstants.CSI_INSTANCES_NAME);
								EObject csiInstsObj = (EObject) si
										.eGet(csiInstsRef);

								if (csiInstsObj != null) {
									EReference csiInstRef = (EReference) csiInstsObj
											.eClass()
											.getEStructuralFeature(
													SafConstants.CSI_INSTANCELIST_NAME);
									List csiObjs = (List) csiInstsObj
											.eGet(csiInstRef);
									if (csiObjs.isEmpty()) {
										EObject problemDataObj = (EObject) _problemNumberObjMap
												.get(new Integer(37));
										EObject noCsiInstForSI = EcoreCloneUtils
												.cloneEObject(problemDataObj);
										EcoreUtils
												.setValue(
														noCsiInstForSI,
														PROBLEM_MESSAGE,
														"ServiceInstance instance does not have any ComponentService"
																+ "Instance instances defined");

										EStructuralFeature srcFeature = noCsiInstForSI
												.eClass()
												.getEStructuralFeature(
														PROBLEM_SOURCE);
										noCsiInstForSI.eSet(srcFeature, si);
										List relatedObjects = (List) EcoreUtils
												.getValue(noCsiInstForSI,
														PROBLEM_RELATED_OBJECTS);
										relatedObjects.add(serviceInstancesObj);
										relatedObjects.add(siObjs);
										probsList.add(noCsiInstForSI);
									} else {
										List<EObject> suList = new ArrayList<EObject>();

										List<String> compTypeList = new ArrayList<String>();
										EReference serviceUnitsInstsRef = (EReference) sg
												.eClass()
												.getEStructuralFeature(
														SafConstants.ASSOCIATED_SERVICEUNITS_NAME);
										EObject serviceUnitsListObj = (EObject) sg
												.eGet(serviceUnitsInstsRef);
										if (serviceUnitsListObj != null) {
											suList = (List) serviceUnitsListObj.eGet(serviceUnitsListObj.eClass().getEStructuralFeature(
													SafConstants.ASSOCIATED_SERVICEUNIT_LIST));
											for (int k = 0; k < suList.size(); k++) {
												EObject suInstObj = (EObject)suInstancesMap.get(suList.get(k));
												EReference compInstsRef = (EReference) suInstObj
														.eClass()
														.getEStructuralFeature(
																SafConstants.COMPONENT_INSTANCES_NAME);
												EObject compInstsListObj = (EObject) suInstObj
														.eGet(compInstsRef);

												if (compInstsListObj != null) {
													EReference compListRef = (EReference) compInstsListObj
															.eClass()
															.getEStructuralFeature(
																	SafConstants.COMPONENT_INSTANCELIST_NAME);
													List compList = (List) compInstsListObj
															.eGet(compListRef);
													for (int l = 0; l < compList
															.size(); l++) {
														EObject compInst = (EObject) compList
																.get(l);
														compTypeList
																.add((String) EcoreUtils
																		.getValue(
																				compInst,
																				"type"));
													}
												}
											}

											for (int k = 0; k < csiObjs.size(); k++) {
												EObject csiObj = (EObject) csiObjs
														.get(k);
												String csiType = (String) EcoreUtils
														.getValue(csiObj,
																"type");
												boolean isValidCSI = false;
												for (int l = 0; l < compTypeList.size(); l++) {
													if (compTypeMap.get(compTypeList.get(l)).contains(csiType)) {
														isValidCSI = true;
														break;
													}
												}
												if (!isValidCSI) {
													EObject problemDataObj = (EObject) _problemNumberObjMap
															.get(new Integer(89));
													EObject problem = EcoreCloneUtils
															.cloneEObject(problemDataObj);
													EcoreUtils.setValue( problem, PROBLEM_MESSAGE,EcoreUtils.getName(csiObj) + " should contain atleast one component in the SU list for the SI, which has a supported CSI type same as the type for the CSI in the CSI list");

													EStructuralFeature srcFeature = problem.eClass()
															.getEStructuralFeature(
																	PROBLEM_SOURCE);
													problem.eSet(srcFeature, csiObj);
													probsList.add(problem);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			Iterator<EObject> instKeys = instDependenciesMap.keySet()
					.iterator();
			while (instKeys.hasNext()) {
				EObject instObj = instKeys.next();
				validateCircularDependencies(instObj, instDependenciesMap
						.get(instObj), dependenciesMap, probsList, new Integer(
						87));
			}
		} catch (Exception e) {
			LOG.warn("Unhandled error while validating SG and SI instances", e);
		}
	}
   /**
	 * 
	 * @param nodeInstName
	 * @param compInstanceName
	 * @param compInstType
	 * @param probsList
	 */
    private void checkCompInstanceType(EObject obj, String nodeInstName,
			String compInstanceName, String compInstType, List probsList) {
    	String dataFilePath = _project.getLocation().toOSString()
				+ File.separator + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
				+ File.separator + nodeInstName + "_rt.xml";
		URI uri = URI.createFileURI(dataFilePath);
		File xmlFile = new File(dataFilePath);
		Resource compInstResource = xmlFile.exists() ? EcoreModels.getResource(uri)
				: EcoreModels.create(uri);
		List compInstList = compInstResource.getContents();
		if (compInstList == null || compInstList.size() == 0) {
			return;
		}
		EObject compInsts = (EObject) compInstList.get(0);
		
		EReference compInstRef = (EReference) compInsts.eClass()
        .getEStructuralFeature("compInst");
 	    List compInstObjs = (List) compInsts.eGet(compInstRef);
 	    for (int i = 0; i < compInstObjs.size(); i++) {
 	    	EObject compInst = (EObject) compInstObjs.get(i);
 	    	String compName = (String) compInst.eGet(compInst.eClass()
					.getEStructuralFeature("compName"));
 	    	if (compName.equals(compInstanceName)) {
 	    		EReference resourceRef = (EReference) compInst.eClass()
						.getEStructuralFeature("resource");
 	    		List resourceList = (List) compInst.eGet(resourceRef);
 	    		for(int j = 0; j < resourceList.size(); j++) {
 	    			EObject resourceObj = (EObject) resourceList.get(j);
 	    			String moID = (String) resourceObj.eGet(resourceObj.eClass()
							.getEStructuralFeature("moID"));
 	    			if (!(moID.equals("'\'")) && !(moID.equals("\\"))) {
						try {
							StringTokenizer tokenizer = new StringTokenizer(
									moID, "\\");
							while (tokenizer.hasMoreTokens()) {
								String token = tokenizer.nextToken();
								String resourceName = token.substring(0, token
										.indexOf(":"));
								if (!isValidResource(resourceName)) {
									EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(47));
									EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
                                   EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
                                           "ComponentInstances Configuration "
                                            + "have Invalid Resource "
                                            + "'" + resourceName + "'");
                                   problem.eSet(problem.eClass().
                                           getEStructuralFeature(PROBLEM_SOURCE), obj);
                                 
                                   List relatedObjects = (List) EcoreUtils.getValue(
                                           problem, PROBLEM_RELATED_OBJECTS);
                                   relatedObjects.add(resourceObj);
                                   relatedObjects.add(resourceList);
									probsList.add(problem);
								}
							}
						} catch (Exception e) {
                            LOG.warn("Unhandled error while validating component instances", e);
						}
					}
 	    		}
 	    	}
 	    }
	}
   /**
     * 
     * @param name
     *            Object Name
     * @param Class Name
     * @param parentConnectionType     
     * @return boolean
     */
  private boolean isValidComponentType(String name, String classType,
			String parentConnType) {
		EObject rootObject = (EObject) _compEditorObjects.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject obj = (EObject) list.get(j);
				String nodeName = EcoreUtils.getName(obj);
				if (obj.eClass().getName().equals(classType)
						&& nodeName.equals(name)) {
					if (parentConnType != null) {
						return hasValidComponentConnection(obj, parentConnType);
					}
					return true;
				}
			}
		}
		return false;
	}
   /**
	 * Checks the connections
	 * 
	 * @param obj
	 *            EObject(Node)
	 * @param parentConnType -
	 *            Parent Connection type (i.e Containment or Association)
	 * @return boolean
	 */
    private boolean hasValidComponentConnection(EObject nodeObj,
			String parentConnType) {
		EObject rootObject = (EObject) _compEditorObjects.get(0);
		String refList[] = ComponentEditorConstants.EDGES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject obj = (EObject) list.get(j);
				String className = obj.eClass().getName();
				if (className.equals(ComponentEditorConstants.AUTO_NAME)) { // This condition is not required
					String type = (String) EcoreUtils.getValue(obj,
							ComponentEditorConstants.CONNECTION_TYPE);
					if (type.equals(parentConnType)) {
						EObject targetObj = getTarget(obj);
						if (nodeObj.equals(targetObj)) {
							return true;
						}
					}
				}
			}
		}
		return false;
	}
    /**
     * 
     * @param resObj - Resource Object
     * @return true if the resource has any connection 
     * originating or terminating on it
     */
    private boolean isValidResource(EObject resObj) {
		EObject rootObject = (EObject) _resEditorObjects.get(0);
		String refList[] = ClassEditorConstants.EDGES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject obj = (EObject) list.get(j);
				EObject srcObj = _resUtils.getSource(obj);
				EObject targetObj = _resUtils.getTarget(obj);
				if (resObj.equals(srcObj) || resObj.equals(targetObj)) {
					return true;
				}

			}
		}
		return false;
	}
  /**
	 * 
	 * @param resObj -
	 *            Resource Object
	 * @return true if the resource has any connection originating or
	 *         terminating on it
	 */
	private boolean isValidSAFNode(EObject nodeObj) {
		EObject rootObject = (EObject) _compEditorObjects.get(0);
		String refList[] = ComponentEditorConstants.EDGES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject obj = (EObject) list.get(j);
				EObject srcObj = getSource(obj);
				EObject targetObj = getTarget(obj);
				if (nodeObj.equals(srcObj) || nodeObj.equals(targetObj)) {
					return true;
				}
			}
		}
		return false;
	}
   /**
	 * 
	 * @param name
	 *            Component Name
	 * @return boolean
	 */
   private boolean isValidComponent(String name) {
		EObject rootObject = (EObject) _compEditorObjects.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			if ((ref.getName().equals(
					ComponentEditorConstants.SAFCOMPONENT_REF_NAME) || ref.getName().equals(
							ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME))) {
				EList list = (EList) rootObject.eGet(ref);
				for (int j = 0; j < list.size(); j++) {
					EObject obj = (EObject) list.get(j);
					if ((EcoreUtils.getName(obj).equals(name))
							&& hasValidComponentConnection(obj,
									ComponentEditorConstants.CONTAINMENT_NAME)) {
						return true;
					}
				}
			}
		}
		return false;
	}
   /**
	 * 
	 * @param name
	 *            Resource name
	 * @return boolean
	 */
   private boolean isValidResource(String name) {
		EObject rootObject = (EObject) _resEditorObjects.get(0);
		String refList[] = ClassEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			String refName = ref.getName();
			if (refName
					.equals(ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME)
					|| refName
							.equals(ClassEditorConstants.SYSTEM_CONTROLLER_REF_NAME)
					|| refName
							.equals(ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME)
					|| refName
							.equals(ClassEditorConstants.HARDWARE_RESOURCE_REF_NAME)
					|| refName
							.equals(ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME)
					|| refName
							.equals(ClassEditorConstants.MIB_RESOURCE_REF_NAME)) {
				EList list = (EList) rootObject.eGet(ref);
				for (int j = 0; j < list.size(); j++) {
					EObject obj = (EObject) list.get(j);
					if (EcoreUtils.getName(obj).equals(name)) {
						return true;
					}
				}
			}
		}
		return false;
	}
   /**
    * 
    * @param nodeName Node name
    * @param suInstType SU Instance type
    * @return boolean
    */
   private boolean isValidSUInstanceType(String nodeName, String suInstType) {
	   EObject amfObj = (EObject) _nodeProfileObjects.get(0);
       EReference nodeInstsRef = (EReference) amfObj.eClass()
           .getEStructuralFeature("nodeInstances");
       EObject nodeInstsObj = (EObject) amfObj.eGet(nodeInstsRef);
       if (nodeInstsObj != null) {
    	   EReference nodeInstRef = (EReference) nodeInstsObj.eClass()
           .getEStructuralFeature("nodeInstance");
    	   List nodeObjs = (List) nodeInstsObj.eGet(nodeInstRef);
    	   for (int i = 0; i < nodeObjs.size(); i++) {
    		   EObject node = (EObject) nodeObjs.get(i);
    		   String nodetype = (String) node.eGet(node.eClass()
						.getEStructuralFeature("type"));
				EReference serviceUnitInstsRef = (EReference) node.eClass()
						.getEStructuralFeature("serviceUnitInstances");
				EObject serviceUnitInstsObj = (EObject) node
						.eGet(serviceUnitInstsRef);
				if (serviceUnitInstsObj != null && nodetype.equals(nodeName)) {
					EReference serviceUnitInstRef = (EReference) serviceUnitInstsObj
							.eClass().getEStructuralFeature(
									"serviceUnitInstance");
					List suObjs = (List) serviceUnitInstsObj
							.eGet(serviceUnitInstRef);
					for (int j = 0; j < suObjs.size(); j++) {
						EObject su = (EObject) suObjs.get(j);
						String suname = (String) su.eGet(su.eClass()
								.getEStructuralFeature("name"));
						if(suname.equals(suInstType)) {
							return true;
						}
					}
				}
    	   }
       }
	   return false;
   }
   /**
    * Validates the fields in the SG Properties different redundency models 
    * are selected.
    * 1)If NO_REDUNDANCY is selected, then we should validate the following fields of  SG properties
        ActiveSUs=1
        StandBySUs =0
        In Service Sus=1

       2) If TWO_N is selected, then we should validate the following fields of  SG properties
       numPrefActiveSUs     = 1
       numPrefStandbySUs    = 1
       numPrefInserviceSUs  >=  2   
       numPrefAssignedSUs   = 2 
       
       3) If M+N is selected , then we should validate the following fields of  SG properties
       Assined SU's >= configured ( numPrefActiveSUs + numPrefStandbySUs)
    *
    */
   private void validateRedundancyModel(List sgInsts, List probsList) 
   {
	   
	   List sgTypeList = _validator.getfilterList(_compEditorObjects,
               ComponentEditorConstants.SERVICEGROUP_NAME);
	   
	   List sgList = new ArrayList();
	   for (int i = 0; i < sgTypeList.size(); i++) {
			EObject sgTypeObj = (EObject) sgTypeList.get(i);
			sgList.add(EcoreUtils.getValue(sgTypeObj, "name").toString());
	   }
	   
	   for (int i = 0; i < sgInsts.size(); i++) {
           
           EObject sg = (EObject) sgInsts.get(i);
           String sgtype = (String) sg.eGet(sg.eClass().getEStructuralFeature("type"));
           String redundancyModel = getRedundancyModel(sgtype);
           
           if (redundancyModel != null){
        	   
        	   EObject associatedSUsObj = (EObject) EcoreUtils.
               	getValue(sg, SafConstants.ASSOCIATED_SERVICEUNITS_NAME);
        	   if (associatedSUsObj != null) {
               
        		   List suList = (List) EcoreUtils.getValue(associatedSUsObj,
                       SafConstants.ASSOCIATED_SERVICEUNIT_LIST);
        		   
        		   if(redundancyModel.endsWith(ComponentEditorConstants.
                           TWO_N_REDUNDANCY_MODEL)) {
	                  if (suList != null && suList.size() < 2) {
	                	  EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(38));
	                	  EObject minSUsProblem = EcoreCloneUtils.cloneEObject(problemDataObj);
	                       EcoreUtils.setValue(minSUsProblem, PROBLEM_MESSAGE,
	                               "ServiceGroup instance should have "
	                               + "at least two ServiceUnit instances"
	                               + " associated if its redundancy model is TWO_N");
	                       
	                       EStructuralFeature srcFeature = minSUsProblem.eClass().
	                           getEStructuralFeature(PROBLEM_SOURCE);
	                       minSUsProblem.eSet(srcFeature, sg);
	                       HashSet suInstList = getAvailableSUsForSG(sg, suList);
	                       List relatedObjects = (List) EcoreUtils.getValue(
	                    		   minSUsProblem, PROBLEM_RELATED_OBJECTS);
	                        relatedObjects.add(new Vector(suInstList));
	                        relatedObjects.add(new Vector(suList));
	                       probsList.add(minSUsProblem);
	                   }
	               }else if(redundancyModel.equals(ComponentEditorConstants.
                           M_PLUS_N_REDUNDANCY_MODEL)) {
	            	   	
	            	    int index = sgList.indexOf(sgtype);
	            	    
	            	    EObject sgTypeObj = null;
	            	    
	            	    if(index != -1)
	            	    	sgTypeObj = (EObject) sgTypeList.get(index);
	            	    
	            	    int prefActiveSUs = 2;
	            	    int prefStandBySUs = 1;
	            	    if(sgTypeObj != null){
		            	   	prefActiveSUs = ((Integer) EcoreUtils.getValue(sgTypeObj,
		       					SafConstants.SG_ACTIVE_SU_COUNT)).intValue();
		            	   	prefStandBySUs = ((Integer) EcoreUtils.getValue(sgTypeObj,
		       					SafConstants.SG_STANDBY_SU_COUNT)).intValue();
	            	    }
	            	    
	       				if (suList != null && suList.size() < (prefActiveSUs + prefStandBySUs)) {
	       					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(61));
	       					EObject minSUsProblem = EcoreCloneUtils.cloneEObject(problemDataObj);
		                       EcoreUtils.setValue(minSUsProblem, ValidationConstants.PROBLEM_MESSAGE,
										"Number of ServiceUnits associated with ServiceGroup instance " + "'" + EcoreUtils.getName(sg) + "'" + " should be"
												+ " >= 'configured (active + standby) SUs' for ServiceGroup type " + "'" + EcoreUtils.getName(sgTypeObj) + "'" + " when redundancy model is 'M+N'");
		                       
		                       EStructuralFeature srcFeature = minSUsProblem.eClass().
		                           getEStructuralFeature(PROBLEM_SOURCE);
		                       minSUsProblem.eSet(srcFeature, sg);
		                       HashSet suInstList = getAvailableSUsForSG(sg, suList);
		                       List relatedObjects = (List) EcoreUtils.getValue(
		                    		   minSUsProblem, PROBLEM_RELATED_OBJECTS);
		                       relatedObjects.add(new Vector(suInstList));
		                       relatedObjects.add(suList);
		                       probsList.add(minSUsProblem);
		                   }
	               } 
               }
               
           }
       }
       
   }
   /**
    * 
    * @param sg - Service Group Object
    * @param associatedSUs - Associated SU's of sg
    * @return the Remaining SU instances which can be associated
    * to sg instance.
    */
   private HashSet getAvailableSUsForSG(EObject sg, List associatedSUs)
   {
	   HashSet suInstList = new HashSet();
	   String sgType = (String) EcoreUtils.getValue(sg, "type");
	   EObject sgTypeObj = _compNameObjectMap.get(sgType);
	   //List children = _compUtils.getChildren(sgTypeObj);
	   List children = getChildrens(sgTypeObj);
	   if (!children.isEmpty()) {
       	// check for type to be of ServiceUnit since it can contain
       	// ServiceInstance also
	       for( int i = 0; i < children.size(); i++) {
	       		EObject obj = (EObject) children.get(i);
	       	    if( obj.eClass().getName().equals(ComponentEditorConstants.SERVICEUNIT_NAME))
	       	    {
	       	    		suInstList = getSUInstances(obj);
	       	    }
	       }
       }
	   Iterator iterator = suInstList.iterator();
	   while (iterator.hasNext()) {
		   EObject suInstObj = (EObject) iterator.next();
		   if (associatedSUs.contains(EcoreUtils.getName(suInstObj))) {
			   iterator.remove();
		   }
	   }
	   return suInstList;
   }
/**
    * 
    * @param SGName - SGName
    * @return the RedundancyModel if any else null
    */
   private String getRedundancyModel(String SGName)
   {
	   EObject rootObject = (EObject) _compEditorObjects.get(0);
		List sgList = (EList) rootObject.eGet((EReference) rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.SERVICEGROUP_REF_NAME));
		for (int i = 0; i < sgList.size(); i++) {
			EObject obj = (EObject) sgList.get(i);
			String name = EcoreUtils.getName(obj);
			if (name.equals(SGName)) {
				String redundancyModel = ((EEnumLiteral) EcoreUtils.getValue(
						obj, ComponentEditorConstants.SG_REDUNDANCY_MODEL))
						.getName();
				return redundancyModel;
			}
		}
       return null;
   }
   /**
    * Checks whether any SU instance which is not associated with any
    * SG instance exists
    *
    */
   private void validateSUInstAssociativity(List probsList)
   {
       HashSet assoSuInstNames = new HashSet();
       HashSet suInstList = new HashSet();
       List assoConnList = _compUtils.getConnectionFrmType(
               ComponentEditorConstants.ASSOCIATION_NAME,
               ComponentEditorConstants.SERVICEGROUP_NAME,
               ComponentEditorConstants.SERVICEUNIT_NAME);
       
       for (int i = 0; i < assoConnList.size(); i++) {
           EObject assoConnObj = (EObject) assoConnList.get(i);
           EObject sgObj = getSource(assoConnObj);
           EObject suObj = getTarget(assoConnObj);
           assoSuInstNames.addAll(getAssociatedSUInstances(sgObj));
           suInstList.addAll(getSUInstances(suObj));
       }
       Iterator iterator = suInstList.iterator();
       while (iterator.hasNext()) {
           EObject suInstObj = (EObject) iterator.next();
           String suInstName = EcoreUtils.getName(suInstObj);
           if (!assoSuInstNames.contains(suInstName)) {
        	   EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(42));
        	   EObject suNotAssoaciatedToSG = EcoreCloneUtils.cloneEObject(problemDataObj);
               EcoreUtils.setValue(suNotAssoaciatedToSG, PROBLEM_MESSAGE,
                       "ServiceUnit instance is not associated"
                       + " to any ServiceGroup instance");
               
               EStructuralFeature srcFeature = suNotAssoaciatedToSG.eClass().
                   getEStructuralFeature(PROBLEM_SOURCE);
               suNotAssoaciatedToSG.eSet(srcFeature, suInstObj);
               List sgInstList = getAvailableSGInstances(suInstObj);
               List relatedObjects = (List) EcoreUtils.getValue(
            		   suNotAssoaciatedToSG, PROBLEM_RELATED_OBJECTS);
               relatedObjects.add(sgInstList);
               probsList.add(suNotAssoaciatedToSG);
           }
       }
   }
   /**
    * 
    * @param suInstObj - SU Instance object
    * @return the SG instances to which SU instance can be associated to
    */
   private List getAvailableSGInstances(EObject suInstObj)
   {
	   List sgInstList = new Vector();
	   String suType = (String) EcoreUtils.getValue(suInstObj, "type");
	   EObject suTypeObj = _compNameObjectMap.get(suType);
	   //List parents = _compUtils.getParents(suTypeObj);
	   List parents = getParents(suTypeObj);
	   if (!parents.isEmpty()) {
	       	// check for type to be of ServiceUnit since it can contain
	       	// ServiceInstance also
	       for( int i = 0; i < parents.size(); i++) {
	       		EObject obj = (EObject) parents.get(i);
	       	    if( obj.eClass().getName().equals(ComponentEditorConstants.SERVICEGROUP_NAME))
	       	    {
	       	    	HashSet sgInstObjects = getSGInstances(obj);
	       	    	sgInstList.addAll(sgInstObjects);
	       	    }
	       }
	   }
	   return sgInstList;
	   
   }
   /**
    * 
    * @param suObj - SU Object type defined in the component editor
    * @return all the SU instances whose type matches with type of suObj
    */
    private HashSet getSUInstances(EObject suObj)
    {
        HashSet suInstList = new HashSet();
        List nodeInstList = ProjectDataModel.getNodeInstListFrmNodeProfile(_nodeProfileObjects);
        for (int i = 0; (nodeInstList != null && i < nodeInstList.size()); i++) {
            EObject nodeInstObj = (EObject) nodeInstList.get(i);
            EReference serviceUnitInstsRef = (EReference) nodeInstObj.eClass()
                .getEStructuralFeature("serviceUnitInstances");
            EObject serviceUnitInstsObj = (EObject) nodeInstObj.eGet(serviceUnitInstsRef);
            if (serviceUnitInstsObj != null) {
                EReference serviceUnitInstRef = (EReference) serviceUnitInstsObj.eClass()
                    .getEStructuralFeature("serviceUnitInstance");
                List suObjs = (List) serviceUnitInstsObj.eGet(serviceUnitInstRef);
                for (int j = 0; j < suObjs.size(); j++) {
                    EObject su = (EObject) suObjs.get(j);
                    String suTypeName = EcoreUtils.getName(suObj);
                    String suInstType = (String) EcoreUtils.getValue(su, "type");
                    if (suTypeName.equals(suInstType)) {
                        suInstList.add(su);
                    }
                }
            }
        }
    
        return suInstList;
    }
    /**
     * 
     * @param sgObj - SG Object type defined in the component editor
     * @return all the SG instances whose type matches with type of sgObj
     */
     private HashSet getSGInstances(EObject sgObj)
     {
         HashSet suInstList = new HashSet();
         List sgInstList = ProjectDataModel.getSGInstListFrmNodeProfile(_nodeProfileObjects);
         
         for (int i = 0; (sgInstList != null && i < sgInstList.size()); i++) {
             EObject sg = (EObject) sgInstList.get(i);
             String sgTypeName = EcoreUtils.getName(sgObj);
             String sgInstType = (String) EcoreUtils.getValue(sg, "type");
             if (sgTypeName.equals(sgInstType)) {
                 suInstList.add(sg);
             }
         }
             
         return suInstList;
     }
    /**
     * 
     * @param sgObj The SG type defined in the component editor
     * @return the List of all associated SU instances of the SG's whose type
     * match with sgObj type.
     */
    private HashSet getAssociatedSUInstances(EObject sgObj)
    {
        HashSet suList = new HashSet();
        String sgTypeName = EcoreUtils.getName(sgObj); 
        List sgObjs = ProjectDataModel.getSGInstListFrmNodeProfile(_nodeProfileObjects);
        for (int i = 0; i < sgObjs.size(); i++) {
            EObject sgInstObj = (EObject) sgObjs.get(i);
            String sgInstType = (String) EcoreUtils.getValue(sgInstObj, "type");
            if (sgInstType.equals(sgTypeName)) {
                EObject assoSUObj = (EObject) EcoreUtils.getValue(sgInstObj,
                        "associatedServiceUnits");
                if (assoSUObj != null) {
                    List assoSUList = (List) EcoreUtils.getValue(assoSUObj,
                        "associatedServiceUnit");
                    suList.addAll(assoSUList);
                }
            }
        }
        return suList;
        
    }

    /**
	 * Checks if either of the provisioning or alarm is not enabled for the
	 * resources.
	 * 
	 * @param probsList
	 *            the Problem List
	 */
    private void checkForAlarmAndProvisioning(List probsList) {
		List list = _validator.getfilterList(_resEditorObjects,
				ClassEditorConstants.RESOURCE_NAME);

		for (int j = 0; j < list.size(); j++) {
			EObject resObj = (EObject) list.get(j);
			boolean provEnabled = false;
			boolean alarmEnabled = false;

			EObject provObj = (EObject) EcoreUtils.getValue(resObj,
					ClassEditorConstants.RESOURCE_PROVISIONING);
			EObject alarmObj = (EObject) EcoreUtils.getValue(resObj,
					ClassEditorConstants.RESOURCE_ALARM);

			if (provObj != null)
				provEnabled = ((Boolean) EcoreUtils.getValue(provObj,
						"isEnabled")).booleanValue();
			if (alarmObj != null)
				alarmEnabled = ((Boolean) EcoreUtils.getValue(alarmObj,
						"isEnabled")).booleanValue();

			if (provEnabled == false && alarmEnabled == false) {
				EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(51));
				EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
						"Either Provisioning or Alarm should be enabled for the resource '"
								+ EcoreUtils.getName(resObj) + "'");
				
				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(PROBLEM_SOURCE);
				problem.eSet(srcFeature, resObj);
				probsList.add(problem);
			}
		}
	}
    /**
	 * @param nodeList
	 * @param probsList
	 */
    private void validateAMFResourceList(List nodeList, List probsList) {
		Iterator nodeIterator = nodeList.iterator();
		while (nodeIterator.hasNext()) {
			EObject nodeInstObj = (EObject) nodeIterator.next();
			String nodeMoId = EcoreUtils.getValue(nodeInstObj, "nodeMoId").toString();
			String nodeResName = AssociateResourceUtils.getResourceTypeFromInstanceID(nodeMoId);
			int nodeInstID = AssociateResourceUtils.getInstanceNumberFromMoID(nodeMoId);
			Integer maxInstances = _resNameMaxInstancesMap.get(nodeResName);
			if(maxInstances.intValue() != 0 && maxInstances.intValue() <= nodeInstID) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
				.get(new Integer(69));
				EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils.setValue(problem, PROBLEM_MESSAGE, "Invalid MO ID for the node instance '"+ EcoreUtils.getName(nodeInstObj) +"'");
				String desc = "DETAILS: Invalid instance number for the resource instance \n\n\n" + "SOLUTION: a)It should be less than the maximum instances configured in " + nodeResName +"\n" + "		or\n" + "Change the maximum instances value as 0 for " + nodeResName;
				EcoreUtils.setValue(problem, PROBLEM_DESCRIPTION, desc);
				EStructuralFeature srcFeature = problem.eClass()
				.getEStructuralFeature(
						ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, nodeInstObj);
				probsList.add(problem);
			}
			Resource rtResource = NodeProfileDialog.getCompResResource(EcoreUtils
					.getName(nodeInstObj), false, ProjectDataModel
					.getProjectDataModel((IContainer) _project));
			if (rtResource != null && rtResource.getContents().size() > 0) {
				EObject compInstancesObj = (EObject) rtResource.getContents()
						.get(0);
				List compInstList = (List) EcoreUtils.getValue(compInstancesObj,
						"compInst");
				for (int j = 0; compInstList != null && j < compInstList.size(); j++) {
					EObject compObj = (EObject) compInstList.get(j);
					List resList = (List) EcoreUtils.getValue(compObj,
								SafConstants.RESOURCELIST_NAME);
					Iterator resIterator = resList.iterator();

					while (resIterator.hasNext()) {
						EObject resObj = (EObject) resIterator.next();
						String moId = EcoreUtils.getValue(resObj, "moID")
								.toString();
						String resName = AssociateResourceUtils.getResourceTypeFromInstanceID(moId);
						int instID = AssociateResourceUtils.getInstanceNumberFromMoID(moId);
						Integer instances = _resNameMaxInstancesMap.get(resName);
						if(instances != null && instances.intValue() != 0 && instances.intValue() <= instID) {
							EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(69));
							EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
							EcoreUtils.setValue(problem, PROBLEM_MESSAGE, "Invalid MO ID for the resource instance'"+ moId +"'");
							String desc = "DETAILS: Invalid instance number for the resource instance \n\n\n" + "SOLUTION: a)It should be less than the maximum instances configured in " + resName +"\n" + "		or\n" + "Change the maximum instances value as 0 for " + resName;
							EcoreUtils.setValue(problem, PROBLEM_DESCRIPTION, desc);
							EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
							problem.eSet(srcFeature, nodeInstObj);
							probsList.add(problem);
						}
					}	
					
				}
			}
		}
    }
    /**
	 * @param nodeList
	 * @param probsList
	 * @param featureName
	 */
    private void validateAMFResourceList(List nodeList, List probsList, String featureName) {
		HashMap<String, HashMap<String, EObject>> moIdCompMap = new HashMap<String, HashMap<String, EObject>>();

		Iterator nodeIterator = nodeList.iterator();
		while (nodeIterator.hasNext()) {
			EObject nodeObj = (EObject) nodeIterator.next();

			EObject SUInstances = (EObject) EcoreUtils.getValue(nodeObj,
					SafConstants.SERVICEUNIT_INSTANCES_NAME);
			List SUList = (List) EcoreUtils.getValue(SUInstances,
					SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
			Iterator SUIterator = SUList.iterator();

			while (SUIterator.hasNext()) {
				EObject SUObj = (EObject) SUIterator.next();

				EObject compInstances = (EObject) EcoreUtils.getValue(SUObj,
						SafConstants.COMPONENT_INSTANCES_NAME);
				List compList = (List) EcoreUtils.getValue(compInstances,
						SafConstants.COMPONENT_INSTANCELIST_NAME);
				Iterator compIterator = compList.iterator();

				while (compIterator.hasNext()) {
					EObject compObj = (EObject) compIterator.next();

					List resList = (List) EcoreUtils.getValue(compObj,
							SafConstants.RESOURCES_NAME);
					Iterator resIterator = resList.iterator();

					while (resIterator.hasNext()) {
						EObject resObj = (EObject) resIterator.next();

						if (((Boolean) EcoreUtils.getValue(resObj, featureName))
								.booleanValue()) {
							String moId = EcoreUtils.getValue(resObj, "moID")
									.toString();

							HashMap<String, EObject> compPathResMap = moIdCompMap.get(moId);
							String compPath = EcoreUtils.getName(nodeObj)
									+ " -> " + EcoreUtils.getName(SUObj)
									+ " -> " + EcoreUtils.getName(compObj)
									+ "#" + moId;

							if (compPathResMap != null) {
								compPathResMap.put(compPath, resObj);

							} else {
								compPathResMap = new HashMap<String, EObject>();
								compPathResMap.put(compPath, resObj);

								moIdCompMap.put(moId, compPathResMap);
							}
						}
					}
				}
			}
		}

		if(featureName.equals("primaryOI")) {
			HashMap<String, List<String>> wildCardMap = new HashMap<String, List<String>>();
			Iterator<String> tmpA = moIdCompMap.keySet().iterator();

			while(tmpA.hasNext()) {
				String strA = tmpA.next();

				if(strA.contains("*")) {

/*					String str = null;
					int index = strA.lastIndexOf(":");
					if (index != -1) {
						str = strA.substring(0, index);
					}
*/
					List<String> list = new ArrayList<String>();
					Iterator<String> tmpB = moIdCompMap.keySet().iterator();

					while(tmpB.hasNext()) {
						String strB = tmpB.next();

						if(!strB.equals(strA)) {
							if (MoPathComboBoxCellEditor
									.getDefaultMoIdVal(strB).equals(
											MoPathComboBoxCellEditor
													.getDefaultMoIdVal(strA))) {
								list.add(strB);
							}
						}
					}

					boolean insertFlag = true;
					Iterator<String> itr = wildCardMap.keySet().iterator();
					while(itr.hasNext()) {
						String strWild = itr.next();
						if (MoPathComboBoxCellEditor.getDefaultMoIdVal(strA)
								.equals(MoPathComboBoxCellEditor.getDefaultMoIdVal(strWild))) {
							insertFlag = false;
							break;
						}
					}
					if(insertFlag) {
						wildCardMap.put(strA, list);
					}
				}
			}

			Iterator<String> wildCardItr = wildCardMap.keySet().iterator();
			while(wildCardItr.hasNext()) {

				String wildCardEntry = wildCardItr.next();
				HashMap<String, EObject> compPathResMap= moIdCompMap.get(wildCardEntry);

				Iterator<String> itr = wildCardMap.get(wildCardEntry).iterator();
				while(itr.hasNext()) {
					String str = itr.next();
					compPathResMap.putAll(moIdCompMap.get(str));
					moIdCompMap.remove(str);
				}
			}
		}

		String message = null;
		int problemNum = 0;
		List resList = ResourceDataUtils.getResourcesList(_resEditorObjects);

		Iterator<String> moIdItr = moIdCompMap.keySet().iterator();
		while(moIdItr.hasNext()) {
			String moId = moIdItr.next();
			HashMap<String, EObject> compPathResMap = moIdCompMap.get(moId);

			if(featureName.equals("primaryOI")) {

				int start = moId.lastIndexOf("\\");
				int end = moId.lastIndexOf(":");

				if(start != -1 && end != -1) {
					String resName = moId.substring(start + 1, end);
					Iterator<EObject> resItr = resList.iterator();
					while(resItr.hasNext()) {
						EObject resObj = resItr.next();
						if(resName.equals(EcoreUtils.getName(resObj))) {
							resItr.remove();
							break;
						}
					}
				}

				if(compPathResMap.size() == 1) {
					continue;
				}

				message = "Primary OI is true for the moId '"
						+ MoPathComboBoxCellEditor.getDefaultMoIdVal(moId)
						+ "' for more than one component";
				problemNum = 62;

			} else if(featureName.equals("autoCreate")) {
				continue;
				/*if(compPathResMap.size() <= 1) {
					continue;
				}
				message = "To Be Created By Component is true for the moId '" + moId
				+ "' for more than one component";
				problemNum = 69;*/
			}

			EObject problemDataObj = (EObject) _problemNumberObjMap
				.get(new Integer(problemNum));
			EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);

			EcoreUtils.setValue(problem, PROBLEM_MESSAGE, message);
			((List) EcoreUtils.getValue(problem, PROBLEM_RELATED_OBJECTS)).add(compPathResMap);

			StringBuffer buf = new StringBuffer("LOCATION:\n");
			Iterator<String> pathItr = compPathResMap.keySet().iterator();
			while (pathItr.hasNext()) {
				buf.append(pathItr.next() + "\n");
			}
			String des = (String) EcoreUtils.getValue(problem,
					PROBLEM_DESCRIPTION);
			des = des + "\n" + buf;

			EcoreUtils.setValue(problem, PROBLEM_DESCRIPTION, des);
			probsList.add(problem);
		}

		/*if (featureName.equals("primaryOI")) {
			Iterator<EObject> resItr = resList.iterator();
			while (resItr.hasNext()) {
				EObject resObj = resItr.next();
				if (ResourceDataUtils.hasTransientAttr(resObj)) {

					EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(70));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);

					EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
							"There should be atleast one Primary OI for the Resource : "
									+ EcoreUtils.getName(resObj));

					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(PROBLEM_SOURCE);
					problem.eSet(srcFeature, resObj);
					probsList.add(problem);
				}
			}
		}*/
	}
    /**
     * Validate SU Types.Relationship / Dependencies Validations
     * @param serviceunits SUs List
     * @param probsList Problems List
     */
    private void validateSUs(List probsList, List serviceunits) {
    	//All the SU Relationship Validations should be moved here. 
		for (int i = 0; i < serviceunits.size(); i++) {
			EObject su = (EObject) serviceunits.get(i);
			String isRestartable = String.valueOf(EcoreUtils.getValue(su, "isRestartable"));
			if (isRestartable.equals("CL_TRUE")) {
				//List childrens = _compUtils.getChildren(su);
				List childrens = getChildrens(su);
/*				for (int j = 0; j < childrens.size(); j++) {
					EObject comp = (EObject) childrens.get(j);
					isRestartable = String.valueOf(EcoreUtils.getValue(comp, "isRestartable"));
					if (isRestartable.equals("CL_FALSE")) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(76));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils
								.setValue(
										problem,
										PROBLEM_MESSAGE,
										"SU '"
												+ EcoreUtils.getName(su)
												+ "' is configured as restartable but the constituent Component '"
												+ EcoreUtils.getName(comp)
												+ "' is not restartable");
						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(PROBLEM_SOURCE);
						problem.eSet(srcFeature, comp);
						probsList.add(problem);
					}
				}
*/				if (childrens.size() == 0) {
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(85));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils.setValue( problem, PROBLEM_MESSAGE, "Service Unit "
											//+ EcoreUtils.getName(su)
											+ "should contains atleast one component");
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(PROBLEM_SOURCE);
					problem.eSet(srcFeature, su);
					probsList.add(problem);
				}
			}
		}
	}

    /**
	 * Validates whether the ohMaskSum for the COR is correct.
	 * 
	 * @param probsList
	 *            the problem list for this project
	 */
	private void validateCorOhMaskSum(List probsList) {
		ResourceDataUtils rdu = new ResourceDataUtils(_resEditorObjects);

		EObject rootObj = (EObject) _resEditorObjects.get(0);
		EObject chassisObj = (EObject) ((List) EcoreUtils.getValue(rootObj,
				ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME)).get(0);

		List<EObject> children = rdu.getChildren(chassisObj);
		String ohMask = getOhMask(rdu, children, "1:1", children.size());

		if (caluculateOhMaskSum(ohMask) > 48) {

			EObject problemDataObj = (EObject) _problemNumberObjMap
					.get(new Integer(77));
			EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);

			EcoreUtils.setValue(problem, PROBLEM_MESSAGE,
					"Resource Instance tree has crossed its limit.");
			probsList.add(problem);
		}
	}

	/**
	 * Retruns the OH Mask for the COR.
	 * 
	 * @param rdu
	 *            Resource Data Utils Instance.
	 * @param list
	 *            resource list.
	 * @param ohMask
	 *            OH Mask for COR
	 * @return OH Mask for the COR
	 */
	private String getOhMask(ResourceDataUtils rdu, List<EObject> list,
			String ohMask, int maxChildren) {
		ohMask += ":" + getOhMaskForLevel(list, maxChildren);
		List<EObject> children = new ArrayList<EObject>();

		maxChildren = 0;
		List<EObject> tempList;
		Iterator<EObject> itr = list.iterator();
		while (itr.hasNext()) {
			tempList = rdu.getChildren(itr.next());
			if(tempList.size() > maxChildren) {
				maxChildren = tempList.size();
			}
			children.addAll(tempList);
		}

		if (!children.isEmpty()) {
			return getOhMask(rdu, children, ohMask, maxChildren);
		}
		return ohMask;
	}

	/**
	 * Returns the OH Mask for COR for the particular level of resource
	 * hierarchy.
	 * 
	 * @param list
	 *            resource list.
	 * @return OH Mask for COR for particular level
	 */
	private String getOhMaskForLevel(List<EObject> list, int maxChildren) {
		String ohMaskForLevel = "";
		EObject obj;
		int maxInstances = 0, temp;

		Iterator<EObject> itr = list.iterator();
		while (itr.hasNext()) {
			obj = itr.next();

			temp = Integer.parseInt(EcoreUtils.getValue(obj,
					ClassEditorConstants.CLASS_MAX_INSTANCES).toString());
			maxInstances = temp > maxInstances ? temp : maxInstances;
		}

		if (maxChildren == 1)
			ohMaskForLevel += 1;
		else
			ohMaskForLevel += (int) Math.ceil(Math.abs(Math.log(maxChildren + 1)
					/ Math.log(2)));

		ohMaskForLevel += ":";

		if (maxInstances == 1)
			ohMaskForLevel += 1;
		else
			ohMaskForLevel += (int) Math.ceil(Math.abs(Math
					.log(maxInstances)
					/ Math.log(2)));

		return ohMaskForLevel;
	}

	/**
	 * Calculates the sum for the given OH Mask
	 * 
	 * @param ohMask
	 *            OH Mask for COR
	 * @return sum for the OH Mask
	 */
	private int caluculateOhMaskSum(String ohMask) {
		int ohMaskSum = 0;

		StringTokenizer str = new StringTokenizer(ohMask, ":");
		while (str.hasMoreTokens()) {
			ohMaskSum += Integer.parseInt(str.nextToken());
		}

		return ohMaskSum;
	}

    /**
     * Validate SG Types. Relationship / Dependencies
     * @param probsList Problems List
     * @param servicegroups ServiceGroup List
     */
    private void validateSGs(List<EObject> probsList,
			List<EObject> servicegroups) {
		// All SG Relationship Validations should be moved here.
		for (int i = 0; i < servicegroups.size(); i++) {
			EObject sg = (EObject) servicegroups.get(i);
			String redundancyModel = EcoreUtils.getValue(sg,
					ComponentEditorConstants.SG_REDUNDANCY_MODEL).toString();

			int prefActiveSUs = ((Integer) EcoreUtils.getValue(sg,
					SafConstants.SG_ACTIVE_SU_COUNT)).intValue();

			int prefStandBySUs = ((Integer) EcoreUtils.getValue(sg,
					SafConstants.SG_STANDBY_SU_COUNT)).intValue();

			int prefInServiceSUs = ((Integer) EcoreUtils.getValue(sg,
					SafConstants.SG_INSERVICE_SU_COUNT)).intValue();

			int prefAssignedSUs = ((Integer) EcoreUtils.getValue(sg,
					SafConstants.SG_ASSIGNED_SU_COUNT)).intValue();

			int prefActiveSUsPerSI = ((Integer) EcoreUtils.getValue(sg,
					SafConstants.SG_ACTIVE_SUS_PER_SI)).intValue();

			if (redundancyModel
					.equals(ComponentEditorConstants.NO_REDUNDANCY_MODEL)) {
				switch (prefActiveSUs) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(26));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils
							.setValue(
									problem,
									ValidationConstants.PROBLEM_MESSAGE,
									"'Active SUs' field value should be"
											+ " 1 when redundancy model is 'NO_REDUNDANCY'");

					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
				switch (prefStandBySUs) {
				case 0:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(27));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils
							.setValue(
									problem,
									ValidationConstants.PROBLEM_MESSAGE,
									"'Standby SUs' field value should be"
											+ " 0 when redundancy model is 'NO_REDUNDANCY'");

					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
				switch (prefInServiceSUs) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(28));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils
							.setValue(
									problem,
									ValidationConstants.PROBLEM_MESSAGE,
									"'Inservice SUs' field value should be"
											+ " 1 when redundancy model is 'NO_REDUNDANCY'");

					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
				switch (prefActiveSUsPerSI) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(80));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils
							.setValue(
									problem,
									ValidationConstants.PROBLEM_MESSAGE,
									"'Active SUs Per SI' field value should be"
											+ " 1 when redundancy model is 'NO_REDUNDANCY'");

					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
			} else if (redundancyModel
					.equals(ComponentEditorConstants.TWO_N_REDUNDANCY_MODEL)) {
				switch (prefActiveSUs) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(29));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem,
							ValidationConstants.PROBLEM_MESSAGE,
							"'Active SUs' field value should be"
									+ " 1 when redundancy model is 'TWO_N'");

					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
				switch (prefStandBySUs) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(30));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem,
							ValidationConstants.PROBLEM_MESSAGE,
							"'Standby SUs' field value should be"
									+ " 1 when redundancy model is 'TWO_N'");

					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
				switch (prefInServiceSUs) {
				case 0:
				case 1:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(31));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem,
							ValidationConstants.PROBLEM_MESSAGE,
							"'Inservice SUs' field value should be"
									+ " >=2 when redundancy model is 'TWO_N'");
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
				switch (prefAssignedSUs) {
				case 2:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(32));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem,
							ValidationConstants.PROBLEM_MESSAGE,
							"'Assigned SUs' field value should be"
									+ " 2 when redundancy model is 'TWO_N'");
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
				switch (prefActiveSUsPerSI) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(81));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem,
							ValidationConstants.PROBLEM_MESSAGE,
							"'Active SUs Per SI' field value should be"
									+ " 1 when redundancy model is 'TWO_N'");

					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
			} else if (redundancyModel
					.equals(ComponentEditorConstants.M_PLUS_N_REDUNDANCY_MODEL)) {
				switch (prefActiveSUs) {
				case 0:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(57));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem,
							ValidationConstants.PROBLEM_MESSAGE,
							"'Active SUs' field value should be"
									+ " >0 when redundancy model is 'M+N'");
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
				switch (prefStandBySUs) {
				case 0:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(30));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem,
							ValidationConstants.PROBLEM_MESSAGE,
							"'Standby SUs' field value should be"
									+ " >0 when redundancy model is 'M+N'");

					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
				switch (prefInServiceSUs) {
				default:
					if (prefInServiceSUs < (prefActiveSUs + prefStandBySUs)) {
						EObject problemDataObj = (EObject) _problemNumberObjMap
								.get(new Integer(59));
						EObject problem = EcoreCloneUtils
								.cloneEObject(problemDataObj);
						EcoreUtils
								.setValue(
										problem,
										ValidationConstants.PROBLEM_MESSAGE,
										"'Inservice SUs' field value should be"
												+ " >= 'Active SUs + StandbySUs' when redundancy model is 'M+N'");

						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(
										ValidationConstants.PROBLEM_SOURCE);
						problem.eSet(srcFeature, sg);
						probsList.add(problem);
					}
				}
				switch (prefActiveSUsPerSI) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap
							.get(new Integer(82));
					EObject problem = EcoreCloneUtils
							.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem,
							ValidationConstants.PROBLEM_MESSAGE,
							"'Active SUs Per SI' field value should be"
									+ " 1 when redundancy model is 'M+N'");

					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(
									ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sg);
					probsList.add(problem);
					break;
				}
			}
			 if (!redundancyModel
					.equals("CL_AMS_SG_REDUNDANCY_MODEL_NO_REDUNDANCY")
					&& !redundancyModel
							.equals("CL_AMS_SG_REDUNDANCY_MODEL_TWO_N")
					&& !redundancyModel
							.equals("CL_AMS_SG_REDUNDANCY_MODEL_M_PLUS_N")) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(78));
				EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils.setValue(problem,
						ValidationConstants.PROBLEM_MESSAGE, EcoreUtils
								.getName(sg)
								+ " have Invalid Redundancy Model");
				problem.eSet(problem.eClass().getEStructuralFeature(
						ValidationConstants.PROBLEM_MESSAGE), sg);
				probsList.add(problem);
			}
			String loadingStrategy = EcoreUtils.getValue(sg, "loadingStrategy")
					.toString();
			if (!loadingStrategy
					.equals("CL_AMS_SG_LOADING_STRATEGY_LEAST_SI_PER_SU")
					&& !loadingStrategy
							.equals("CL_AMS_SG_LOADING_STRATEGY_LEAST_SU_ASSIGNED")
					&& !loadingStrategy
							.equals("CL_AMS_SG_LOADING_STRATEGY_LEAST_LOAD_PER_SU")
					&& !loadingStrategy
							.equals("CL_AMS_SG_LOADING_STRATEGY_BY_SI_PREFERENCE")
					&& !loadingStrategy
							.equals("CL_AMS_SG_LOADING_STRATEGY_USER_DEFINED")) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(79));
				EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils.setValue(problem,
						ValidationConstants.PROBLEM_MESSAGE, EcoreUtils
								.getName(sg)
								+ " have Invalid Loading Strategy");
				problem.eSet(problem.eClass().getEStructuralFeature(
						ValidationConstants.PROBLEM_MESSAGE), sg);
				probsList.add(problem);
			}
			 
			int maxActiveSIsPerSU = ((Integer)EcoreUtils.getValue(sg, "maxActiveSIsPerSU")).intValue();
			int maxStandbySIsPerSU =((Integer)EcoreUtils.getValue(sg, "maxStandbySIsPerSU")).intValue();
			if(maxActiveSIsPerSU != 1 || maxStandbySIsPerSU != 0) {
				//List <EObject> sus = _compUtils.getChildren(sg);
				List <EObject> sus = getChildrens(sg);
				for (int j = 0; j < sus.size(); j++) {
					EObject su = sus.get(j);
					//List <EObject> comps = _compUtils.getChildren(su);
					List <EObject> comps = getChildrens(su);
					for (int k = 0; k < comps.size(); k++) {
						EObject comp = comps.get(k);
						if(comp.eClass().getName().equals(ComponentEditorConstants.NONSAFCOMPONENT_NAME)) {
							String prop = String.valueOf(EcoreUtils.getValue(comp, "property"));
							if (prop
									.equals("CL_AMS_PROXIED_NON_PREINSTANTIABLE")
									|| prop
											.equals("CL_AMS_NON_PROXIED_NON_PREINSTANTIABLE")) {
								if (maxActiveSIsPerSU != 1) {
									EObject problemDataObj = (EObject) _problemNumberObjMap
											.get(new Integer(83));
									EObject problem = EcoreCloneUtils
											.cloneEObject(problemDataObj);
									EcoreUtils
											.setValue(
													problem,
													PROBLEM_MESSAGE,
													"MaxActiveSIsPerSU's value should be 1 when associated ServiceUnit is Non-Preinstantiable");
									EStructuralFeature srcFeature = problem
											.eClass().getEStructuralFeature(
													PROBLEM_SOURCE);
									problem.eSet(srcFeature, sg);
									probsList.add(problem);
								}
								if (maxStandbySIsPerSU != 0) {
									EObject problemDataObj = (EObject) _problemNumberObjMap
											.get(new Integer(84));
									EObject problem = EcoreCloneUtils
											.cloneEObject(problemDataObj);
									EcoreUtils
											.setValue(
													problem,
													PROBLEM_MESSAGE,
													"MaxStandbySIsPerSU's value should be 0 when associated ServiceUnit is Non-Preinstantiable");
									EStructuralFeature srcFeature = problem
											.eClass().getEStructuralFeature(
													PROBLEM_SOURCE);
									problem.eSet(srcFeature, sg);
									probsList.add(problem);
								}
							}
						}
					}
				}
			}
		}
    }
    /**
     * Validate SIs Relationship
     * @param probsList Problems List
     * @param serviceinstances Service Instance List
     */
    private void validateSIs(List <EObject> probsList, List <EObject> serviceinstances) {
    	for (int i = 0; i < serviceinstances.size(); i++) {
			EObject si = (EObject) serviceinstances.get(i);
			//List childrens = _compUtils.getChildren(si);
			List childrens = getChildrens(si);
			if(childrens.size() < 1) {
				EObject problemDataObj = (EObject) _problemNumberObjMap.get(86);
    			EObject problem = EcoreCloneUtils
				.cloneEObject(problemDataObj);
    			EcoreUtils.setValue(problem, PROBLEM_MESSAGE, "Service Instance should contains atleast one Component Service Instance");
    			problem.eSet(problem.eClass().getEStructuralFeature(PROBLEM_SOURCE), si);
    			probsList.add(problem);
			}
    	}
    }
    /**
     * Check Circular Dependencies
     * @param dependentName Dependent Name
     * @param dependenciesNames Dependencies Names List
     * @param dependenciesMap Dependencies Names Map
     * @param problems Problems List
     */
    private void validateCircularDependencies(EObject dependentNodeInst, List dependenciesNames, Map dependenciesMap, List <EObject> problems, Integer probNumber) {
    	String dependentName = EcoreUtils.getName(dependentNodeInst);
    	for (int i = 0; i < dependenciesNames.size(); i++) {
    		String nodeInstName = (String) dependenciesNames.get(i);
    		EList dependencies = (EList) dependenciesMap.get(nodeInstName);
    		if(dependencies.contains(dependentName)) {
    			EObject problemDataObj = (EObject) _problemNumberObjMap.get(probNumber);
    			EObject problem = EcoreCloneUtils
				.cloneEObject(problemDataObj);
    			EcoreUtils.setValue(problem, PROBLEM_MESSAGE, nodeInstName + " has circular dependency");
    			problem.eSet(problem.eClass().getEStructuralFeature(PROBLEM_SOURCE), dependentNodeInst);
    			((List) EcoreUtils.getValue(problem, PROBLEM_RELATED_OBJECTS)).add(nodeInstName);
    			problems.add(problem);
    		}
    	}
    }
    /**
     * Returns su instnaces name, Object map.
     * @param nodeInsts Node Instances List
     * @return name, Object map
     */
    private HashMap getSUInstancesMap(List nodeInsts) {
		// this method should be merged with some other method at the time of code cleaning.
		HashMap<String, EObject> suInstancesMap = new HashMap<String, EObject>();
		for (int i = 0; i < nodeInsts.size(); i++) {
			EObject node = (EObject) nodeInsts.get(i);
			String nodetype = (String) node.eGet(node.eClass()
					.getEStructuralFeature("type"));
			EReference serviceUnitInstsRef = (EReference) node.eClass()
					.getEStructuralFeature(
							SafConstants.SERVICEUNIT_INSTANCES_NAME);
			EObject serviceUnitInstsObj = (EObject) node
					.eGet(serviceUnitInstsRef);

			if (serviceUnitInstsObj != null) {
				EReference serviceUnitInstRef = (EReference) serviceUnitInstsObj
						.eClass().getEStructuralFeature(
								SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
				List suObjs = (List) serviceUnitInstsObj
						.eGet(serviceUnitInstRef);
				for (int j = 0; j < suObjs.size(); j++) {
					EObject su = (EObject) suObjs.get(j);
					suInstancesMap.put(EcoreUtils.getName(su), su);
				}
			}
		}
		return suInstancesMap;
	}
    
    /**
     * Returns Map of Childrens List for each Object
     * @return HashMap
     */
    private Map getChildrensListMap() {
    	Map childrensListMap = new HashMap();
    	return childrensListMap;
    }
    
    /**
     * Parse All the connection List and creates Map for
     * <edge,source>, <edge,target>, <node, childs>, <node,parents>, <name, node>
     */
    private void parseRelationships() {
    	_connSrcMap.clear();
    	_connTrgMap.clear();
    	_parentsMap.clear();
    	_childrensMap.clear();
    	
    	//_nameObjectMap.clear(); // This map also needs to be filled
    	
    	// Parse relationships for Component Editor Objects
    	parseComponentRelationships();
    	
		// Parse relationships for Resource Editor Objects
    	//parseResourceRelationships();
    }
    /**
     * Parse relationships in component Editor
     *
     */
    private void parseComponentRelationships() {
    	EObject rootObject = (EObject) _compEditorObjects.get(0);
    	String refList[] = ComponentEditorConstants.EDGES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject edgeObj = (EObject) list.get(j);
				EObject srcObj = _compUtils.getSource(edgeObj);
				EObject targetObj = _compUtils.getTarget(edgeObj);
				_connSrcMap.put(edgeObj, srcObj);
				_connTrgMap.put(edgeObj,targetObj);
				Object parentsList = _parentsMap.get(targetObj);
				if(parentsList == null) {
					parentsList = new ArrayList();
					_parentsMap.put(targetObj, parentsList);
				}
				((List)parentsList).add(srcObj);
				Object childrensList = _childrensMap.get(srcObj);
				if(childrensList == null) {
					childrensList = new ArrayList();
					_childrensMap.put(srcObj, childrensList);
				}
				((List)childrensList).add(targetObj);
			}
		}
    }
    /**
     * Parse relationships in Resource Editor.
     *
     */
    private void parseResourceRelationships() {
    	EObject rootObject = (EObject) _resEditorObjects.get(0);
    	String refList[] = ClassEditorConstants.EDGES_REF_TYPES;
    	for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject edgeObj = (EObject) list.get(j);
				EObject srcObj = _resUtils.getSource(edgeObj);
				EObject targetObj = _resUtils.getTarget(edgeObj);
				_connSrcMap.put(edgeObj, srcObj);
				_connTrgMap.put(edgeObj,targetObj);
				Object parentsList = _parentsMap.get(targetObj);
				if(parentsList == null) {
					parentsList = new ArrayList();
					_parentsMap.put(targetObj, parentsList);
				}
				((List)parentsList).add(srcObj);
				Object childrensList = _childrensMap.get(srcObj);
				if(childrensList == null) {
					childrensList = new ArrayList();
					_childrensMap.put(srcObj, childrensList);
				}
				((List)childrensList).add(targetObj);
			}
		}
    }
    /**
     * Return Source EObject for the connection object
     * @param edgeObj Connection EObject
     * @return source EObject
     */
    private EObject getSource(EObject edgeObj) {
    	return (_connSrcMap.get(edgeObj) != null) ? (EObject)_connSrcMap.get(edgeObj) : null;
    }
    /**
     * Return Target EObject for the connection object
     * @param edgeObj Connection EObject
     * @return target EObject
     */
    private EObject getTarget(EObject edgeObj) {
    	return (_connTrgMap.get(edgeObj) != null) ? (EObject)_connTrgMap.get(edgeObj) : null;
    }
    /**
     * Returns Parents List
     * @param targObj target obj
     * @return List
     */
    private List getChildrens(EObject targObj) {
    	Object childrens = _childrensMap.get(targObj);
    	return childrens != null ? (List)childrens : new ArrayList();
    }
    /**
     * Returns Childrens List
     * @param srcObj source obj
     * @return List
     */
    private List getParents(EObject srcObj) {
    	Object parents = _parentsMap.get(srcObj);
    	return parents != null ? (List)parents : new ArrayList();
    }
    /**
     * Populate resource objects and creates name object map
     * @param resObjects
     */
    private void parseResourceObjects(List<EObject> resObjects)
    {
    	EObject rootObject = (EObject) resObjects.get(0);
		String refList[] = ClassEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject obj = (EObject) list.get(j);
				String name = EcoreUtils.getName(obj);
	            _resNameObjectMap.put(name, obj);
	            _resNameMaxInstancesMap.put(name, (Integer)EcoreUtils.getValue(obj, "maxInstances"));
			}
		}
    }
    /**
     * Populate component objects and creates name object map
     * @param compObjects
     */
    private void parseComponentObjects(List<EObject> compObjects)
    {
    	EObject rootObject = (EObject) compObjects.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject obj = (EObject) list.get(j);
				String name = EcoreUtils.getName(obj);
	            _compNameObjectMap.put(name, obj);
			}
		}
    }
}


