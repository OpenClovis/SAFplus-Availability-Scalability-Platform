package com.clovis.cw.workspace.project;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.data.SubModelMapReader;

/**
 * @author ravi
 *	Xml data validator class
 */
public class XmlDataValidator {

	protected IProject _project = null;
	protected HashMap _problemNumberObjMap = null;
	
	public XmlDataValidator(IProject project, HashMap problemNumberObjMap){
		
		_project = project;
		_problemNumberObjMap = problemNumberObjMap;
	}
	
	/**
	 * Validates the resource name in resource_alarm_map.xml
	 * @param project
	 */
	public void validateResourceAlarmMapData(List retList) {
		
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
				
		Model resourceModel = pdm.getCAModel();
		List resourceElist = resourceModel.getEList();
		List resourceObjList = ResourceDataUtils.getResourcesList(resourceElist);
		List resourceNameList = new ArrayList();
		for(int i = 0; i< resourceObjList.size(); i++){
			resourceNameList.add((String)EcoreUtils.
					getName((EObject) resourceObjList.get(i)));
		}
					
		Model resourceAlarmModel = pdm.getResourceAlarmMapModel();
        EObject mapObj = resourceAlarmModel.getEObject();
        EObject linkObj = SubModelMapReader.getLinkObject(mapObj,
        						ClassEditorConstants.ASSOCIATED_ALARM_LINK);
        
        List linkDetailList = (List) EcoreUtils.getValue(linkObj, "linkDetail");
        
        for(int i = 0; i < linkDetailList.size(); i++){
        	
        	EObject linkDetailObj = (EObject) linkDetailList.get(i);
        	String resName = (String) EcoreUtils.getValue(linkDetailObj, "linkSource");
        	
        	if( !resourceNameList.contains(resName) ){
        		EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(63));
        		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
								"Resource Alarm map has invalid resource '"+ resName +"'");
   	            retList.add(problem);
        	}
   	
        }

	}
	/**
	 * Validates the component name in component_resource_map.xml
	 * @param retList - List of problems
	 */
	public void validateComponentResourceMapData(List retList) {
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		
		EObject componentInfoObj = (EObject) pdm.getComponentModel().getEList().get(0);
        List safCompObjects = (List) EcoreUtils.getValue(componentInfoObj,
				ComponentEditorConstants.SAFCOMPONENT_REF_NAME);
        List nonSafCompObjects = (List) EcoreUtils.getValue(componentInfoObj,
				ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME);
        
        List compNameList = new ArrayList();
        
        for(int i = 0; i< safCompObjects.size() ; i++){
           	EObject safCompObj = (EObject) safCompObjects.get(i);
        	compNameList.add((String)EcoreUtils.getName(safCompObj));
        }
        for(int i = 0; i< nonSafCompObjects.size() ; i++){
           	EObject nonSafCompObj = (EObject) nonSafCompObjects.get(i);
        	compNameList.add((String)EcoreUtils.getName(nonSafCompObj));
        }
		
		Model compResModel = pdm.getComponentResourceMapModel();
        EObject mapObj = compResModel.getEObject();
        EObject linkObj = SubModelMapReader.getLinkObject(mapObj,
        		ComponentEditorConstants.ASSOCIATE_RESOURCES_NAME);
        
        List linkDetailList = (List) EcoreUtils.getValue(linkObj, "linkDetail");
        
        for(int i = 0; i < linkDetailList.size(); i++){
        	
        	EObject linkDetailObj = (EObject) linkDetailList.get(i);
        	String compName = (String) EcoreUtils.getValue(linkDetailObj, "linkSource");
        	
        	if( !compNameList.contains(compName) ){
        		EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(64));
        		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
								"Component Resource map has invalid component '"+ compName +"'");
   	            retList.add(problem);
        	}
        }        
    
	}
	
	/**
	 * Validates data in clAmfConfig.xml
	 * @param retList - List of problems
	 */
	public void validateAmfConfig(List retList) {
		
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
			
		EObject amfConfigObj = (EObject) pdm.getNodeProfiles().getEList().get(0);
		
		EObject compInfoObj = (EObject) pdm.getComponentModel().getEList().get(0);
		
		
		validateCpmConfigs(amfConfigObj, compInfoObj, retList);
		validateServiceGroups(amfConfigObj, compInfoObj, retList);
	}
	
	/**
	 * Validates Node type in cpm config
	 * @param amfConfigObj - amfConfig EObject
	 * @param compInfoObj - component info EObject
	 * @param retList - List of problems
	 */
	public void validateCpmConfigs(EObject amfConfigObj, EObject  compInfoObj, List retList) {
		List nodeObjList = (List) EcoreUtils.getValue(compInfoObj, "node");
		
		List nodeNameList = new ArrayList();
		for(int i = 0; i < nodeObjList.size(); i++){
			nodeNameList.add((String)EcoreUtils.getName((EObject)(nodeObjList.get(i))));
		}
		
		EObject cpmConfigsObj = (EObject) EcoreUtils.getValue(amfConfigObj, SafConstants.CPM_CONFIGS_NAME);
		List cpmConfigObjList = (List) EcoreUtils.getValue(cpmConfigsObj, SafConstants.CPM_CONFIGLIST_NAME);
		for(int i = 0; i < cpmConfigObjList.size(); i++){
			String nodeTypeName = (String)EcoreUtils.getValue((EObject)cpmConfigObjList.get(i), "nodeType");
			if(!nodeNameList.contains(nodeTypeName)){
				EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(65));
        		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
								"AMF configuration has invalid Node '"+ nodeTypeName +"'");
   	            retList.add(problem);
			}
		}
		
	}
	/**
	 * 
	 * @param amfConfigObj - amfConfig EObject
	 * @param compInfoObj - component info EObject
	 * @param retList - List of problems
	 */
	/*public void validateNodeInstanceList(EObject amfConfigObj, EObject  compInfoObj, List <EObject> retList) {
		List nodeTypeList = (List) EcoreUtils.getValue(compInfoObj, ComponentEditorConstants.NODE_REF_NAME);
		List <String> nodeNameList = new ArrayList <String> ();
		for(int i = 0; i < nodeTypeList.size(); i++){
			nodeNameList.add((String)EcoreUtils.getName((EObject)(nodeTypeList.get(i))));
		}
		List suTypeList = (List) EcoreUtils.getValue(compInfoObj, ComponentEditorConstants.SERVICEUNIT_REF_NAME);
		List <String> suNameList = new ArrayList <String> ();
		for(int i = 0; i < suTypeList.size(); i++){
			suNameList.add((String)EcoreUtils.getName((EObject)(suTypeList.get(i))));
		}
		List compTypeList = (List) EcoreUtils.getValue(compInfoObj, ComponentEditorConstants.SAFCOMPONENT_REF_NAME);
		List <String> compNameList = new ArrayList <String> ();
		for(int i = 0; i < compTypeList.size(); i++){
			compNameList.add((String)EcoreUtils.getName((EObject)(compTypeList.get(i))));
		}
		compTypeList = (List) EcoreUtils.getValue(compInfoObj, ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME);
		for(int i = 0; i < compTypeList.size(); i++){
			compNameList.add((String)EcoreUtils.getName((EObject)(compTypeList.get(i))));
		}
		EObject nodeObj = (EObject) EcoreUtils.getValue(amfConfigObj, SafConstants.NODE_INSTANCES_NAME);
		List amfNodeObjList = (List) EcoreUtils.getValue(nodeObj, SafConstants.NODE_INSTANCELIST_NAME);
		for(int i = 0; i < amfNodeObjList.size(); i++){
			String nodeName =  (String) EcoreUtils.getValue((EObject)amfNodeObjList.get(i), "type");
			if(!nodeNameList.contains(nodeName)){
				EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(88));
        		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
								"AMF configuration has invalid Node Type '"+ nodeName +"'");
				problem.eSet(problem.eClass().getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE), amfNodeObjList.get(i));
   	            retList.add(problem);
			}
			EObject amfSuInstObj = (EObject) EcoreUtils.getValue((EObject)amfNodeObjList.get(i), SafConstants.SERVICEUNIT_INSTANCES_NAME);
			List suInstanceObjList = (List) EcoreUtils.getValue(amfSuInstObj, SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
			for(int j = 0; j < suInstanceObjList.size(); j++){
				String suName = (String) EcoreUtils.getValue((EObject)suInstanceObjList.get(j), "type");
				if(!suNameList.contains(suName)){
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(78));
	        		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
									"AMF configuration has invalid SU Type '"+ suName +"'");
					problem.eSet(problem.eClass().getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE), suInstanceObjList.get(j));
	   	            retList.add(problem);
				}
				EObject amfCompInstObj = (EObject) EcoreUtils.getValue((EObject)suInstanceObjList.get(j), SafConstants.COMPONENT_INSTANCES_NAME);
				List compInstanceObjList = (List) EcoreUtils.getValue(amfCompInstObj, SafConstants.COMPONENT_INSTANCELIST_NAME);
				for(int k = 0; k < compInstanceObjList.size(); k++){
					String compName = (String) EcoreUtils.getValue((EObject)compInstanceObjList.get(k), "type");
					if(!compNameList.contains(compName)){
						EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(79));
		        		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
						EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
										"AMF configuration has invalid Component Type '"+ compName +"'");
						problem.eSet(problem.eClass().getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE), compInstanceObjList.get(k));
		   	            retList.add(problem);
					}
				}
			}
		}
	}*/
	/**
	 * 
	 * @param amfConfigObj - amfConfig EObject
	 * @param compInfoObj - component info EObject
	 * @param retList - List of problems
	 */
	public void validateServiceGroups(EObject amfConfigObj, EObject  compInfoObj, List retList) {
		List sgObjList = (List) EcoreUtils.getValue(compInfoObj, ComponentEditorConstants.SERVICEGROUP_REF_NAME);
		List sgNameList = new ArrayList();
		for(int i = 0; i < sgObjList.size(); i++){
			sgNameList.add((String)EcoreUtils.getName((EObject)(sgObjList.get(i))));
		}
		
		List siObjList = (List) EcoreUtils.getValue(compInfoObj, ComponentEditorConstants.SERVICEINSTANCE_REF_NAME);
		List siNameList = new ArrayList();
		for(int i = 0; i < siObjList.size(); i++){
			siNameList.add((String)EcoreUtils.getName((EObject)(siObjList.get(i))));
		}
		List csiObjList = (List) EcoreUtils.getValue(compInfoObj, ComponentEditorConstants.COMPONENTSERVICEINSTANCE_REF_NAME);
		List csiNameList = new ArrayList();
		for(int i = 0; i < csiObjList.size(); i++){
			csiNameList.add((String)EcoreUtils.getName((EObject)(csiObjList.get(i))));
		}
		EObject sgsObj = (EObject) EcoreUtils.getValue(amfConfigObj, SafConstants.SERVICEGROUP_INSTANCES_NAME);
		List amfSgObjList = (List) EcoreUtils.getValue(sgsObj, SafConstants.SERVICEGROUP_INSTANCELIST_NAME);
		for(int i = 0; i < amfSgObjList.size(); i++){
			String sgName =  (String) EcoreUtils.getValue((EObject)amfSgObjList.get(i), "type");
			if(!sgNameList.contains(sgName)){
				EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(66));
        		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
								"AMF configuration has invalid SG '"+ sgName +"'");
   	            retList.add(problem);
			}
			EObject amfSiInstObj = (EObject) EcoreUtils.getValue((EObject)amfSgObjList.get(i), SafConstants.SERVICE_INSTANCES_NAME);
			List siInstanceObjList = (List) EcoreUtils.getValue(amfSiInstObj, SafConstants.SERVICE_INSTANCELIST_NAME);
			for(int j = 0; j < siInstanceObjList.size(); j++){
				String siName = (String) EcoreUtils.getValue((EObject)siInstanceObjList.get(j), "type");
				if(!siNameList.contains(siName)){
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(67));
	        		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
									"AMF configuration has invalid SI '"+ siName +"'");
					
	   	            retList.add(problem);
					
				}
				EObject amfCsiInstObj = (EObject) EcoreUtils.getValue((EObject)siInstanceObjList.get(j), SafConstants.CSI_INSTANCES_NAME);
				List csiInstanceObjList = (List) EcoreUtils.getValue(amfCsiInstObj, SafConstants.CSI_INSTANCELIST_NAME);
				
				for(int k = 0; k < csiInstanceObjList.size(); k++){
					String csiName = (String) EcoreUtils.getValue((EObject)csiInstanceObjList.get(k), "type");
					if(!csiNameList.contains(csiName)){
						EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(68));
		        		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
						EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
										"AMF configuration has invalid CSI '"+ csiName +"'");
		   	            retList.add(problem);
						
					}
					
				}
			}
		}
		
	}
		
}
