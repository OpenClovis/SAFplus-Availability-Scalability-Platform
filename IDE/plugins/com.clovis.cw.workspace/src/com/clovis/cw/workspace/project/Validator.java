/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/Validator.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
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
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;

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
import com.clovis.cw.editor.ca.snmp.ClovisMibUtils;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.ireasoning.protocol.snmp.MibUtil;
import com.ireasoning.protocol.snmp.SnmpOID;
import com.ireasoning.util.MibTreeNode;

/**
 * 
 * @author shubhada
 *  Validator class to handle specific validations.
 *  Project Validator will delegate specific validations
 *  to this class.
 *  
 */
public class Validator
{
	protected HashMap _problemNumberObjMap = null;
	protected IProject _project = null;
	private List _resEditorObjects;
	private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());
	/**
	 * Constructor 
	 * @param problemNumberObjMap - ProblemNumberObject Map
	 */
	public Validator(IProject project, HashMap problemNumberObjMap, List resObjects)
	{
		_project = project;
		_resEditorObjects = resObjects;
		_problemNumberObjMap = problemNumberObjMap;
	}
	/**
	 * 
	 * @param nodeInsts - All Node instances defined in AMFConfigurations
	 * @param sgInsts - All SG instances defined in AMFConfigurations
	 * @param probsList - List of problems
	 */
	public void checkForUniqueness(List nodeInsts, List sgInsts,
			List probsList) {
		HashSet uniqueNamesList = new HashSet();
		List amfConfigObjList = new Vector();
		HashSet problemSet = new HashSet();
		try {
			for (int i = 0; i < nodeInsts.size(); i++) {
				EObject node = (EObject) nodeInsts.get(i);
				amfConfigObjList.add(node);
				String nodeName = EcoreUtils.getName(node);
				if (!uniqueNamesList.add(nodeName)) {
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(41));
					EObject nameDuplicateProb = EcoreCloneUtils.cloneEObject(problemDataObj);
					EcoreUtils.setValue(nameDuplicateProb, ValidationConstants.PROBLEM_MESSAGE,
							"Node instance name is duplicate");
					
					EStructuralFeature srcFeature = nameDuplicateProb.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					nameDuplicateProb.eSet(srcFeature, node);
					List relatedObjList = (List) EcoreUtils.getValue(
							nameDuplicateProb, ValidationConstants.PROBLEM_RELATED_OBJECTS);
					relatedObjList.add(new Integer(i));
					probsList.add(nameDuplicateProb);
					problemSet.add(nameDuplicateProb);
				}

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
						amfConfigObjList.add(su);
						String suName = EcoreUtils.getName(su);
						if (!uniqueNamesList.add(suName)) {
							EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(41));
							EObject nameDuplicateProb = EcoreCloneUtils.cloneEObject(problemDataObj);
							EcoreUtils.setValue(nameDuplicateProb, ValidationConstants.PROBLEM_MESSAGE,
									"ServiceUnit instance name is duplicate");
							
							EStructuralFeature srcFeature = nameDuplicateProb
									.eClass().getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
							nameDuplicateProb.eSet(srcFeature, su);
							List relatedObjList = (List) EcoreUtils.getValue(
									nameDuplicateProb, ValidationConstants.PROBLEM_RELATED_OBJECTS);
							relatedObjList.add(new Integer(j));

							probsList.add(nameDuplicateProb);
							problemSet.add(nameDuplicateProb);
						}
						EReference componentInstsRef = (EReference) su.eClass()
								.getEStructuralFeature(
										SafConstants.COMPONENT_INSTANCES_NAME);
						EObject componentInstsObj = (EObject) su
								.eGet(componentInstsRef);

						if (componentInstsObj != null) {

							EReference componentInstRef = (EReference) componentInstsObj
									.eClass()
									.getEStructuralFeature(
											SafConstants.COMPONENT_INSTANCELIST_NAME);
							List compObjs = (List) componentInstsObj
									.eGet(componentInstRef);
							for (int k = 0; k < compObjs.size(); k++) {
								EObject comp = (EObject) compObjs.get(k);
								amfConfigObjList.add(comp);
								String compName = EcoreUtils.getName(comp);
								if (!uniqueNamesList.add(compName)) {
									EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(41));
									EObject nameDuplicateProb = EcoreCloneUtils.cloneEObject(problemDataObj);
									EcoreUtils
											.setValue(nameDuplicateProb,
													ValidationConstants.PROBLEM_MESSAGE,
													"Component instance name is duplicate");
									
					   	            
									EStructuralFeature srcFeature = nameDuplicateProb
											.eClass().getEStructuralFeature(
													ValidationConstants.PROBLEM_SOURCE);
									nameDuplicateProb.eSet(srcFeature, comp);
									List relatedObjList = (List) EcoreUtils
											.getValue(nameDuplicateProb,
													ValidationConstants.PROBLEM_RELATED_OBJECTS);
									relatedObjList.add(new Integer(k));

									probsList.add(nameDuplicateProb);
									problemSet.add(nameDuplicateProb);
								}
							}
						}
					}
				}
			}
		} catch (Exception e) {
			LOG
					.warn(
							"Error while validating for unique names in AMFConfigurations",
							e);
		}

		try {
			for (int i = 0; i < sgInsts.size(); i++) {
				EObject sg = (EObject) sgInsts.get(i);
				amfConfigObjList.add(sg);
				String sgName = EcoreUtils.getName(sg);
				if (!uniqueNamesList.add(sgName)) {
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(41));
					EObject nameDuplicateProb = EcoreCloneUtils.cloneEObject(problemDataObj);
					EcoreUtils.setValue(nameDuplicateProb, ValidationConstants.PROBLEM_MESSAGE,
							"ServiceGroup instance name is duplicate");
					
	   	            
					EStructuralFeature srcFeature = nameDuplicateProb.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					nameDuplicateProb.eSet(srcFeature, sg);
					List relatedObjList = (List) EcoreUtils.getValue(
							nameDuplicateProb, ValidationConstants.PROBLEM_RELATED_OBJECTS);
					relatedObjList.add(new Integer(i));

					probsList.add(nameDuplicateProb);
					problemSet.add(nameDuplicateProb);
				}

				EReference siInstsRef = (EReference) sg.eClass()
						.getEStructuralFeature(
								SafConstants.SERVICE_INSTANCES_NAME);
				EObject serviceInstanceInstsObj = (EObject) sg.eGet(siInstsRef);

				if (serviceInstanceInstsObj != null) {
					EReference siInstRef = (EReference) serviceInstanceInstsObj
							.eClass().getEStructuralFeature(
									SafConstants.SERVICE_INSTANCELIST_NAME);
					List siObjs = (List) serviceInstanceInstsObj
							.eGet(siInstRef);

					for (int j = 0; j < siObjs.size(); j++) {
						EObject si = (EObject) siObjs.get(j);
						amfConfigObjList.add(si);
						String siName = EcoreUtils.getName(si);
						if (!uniqueNamesList.add(siName)) {
							EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(41));
							EObject nameDuplicateProb = EcoreCloneUtils.cloneEObject(problemDataObj);
							EcoreUtils
									.setValue(nameDuplicateProb, ValidationConstants.PROBLEM_MESSAGE,
											"ServiceInstance instance name is duplicate");
							
							EStructuralFeature srcFeature = nameDuplicateProb
									.eClass().getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
							nameDuplicateProb.eSet(srcFeature, si);
							List relatedObjList = (List) EcoreUtils.getValue(
									nameDuplicateProb, ValidationConstants.PROBLEM_RELATED_OBJECTS);
							relatedObjList.add(new Integer(j));

							probsList.add(nameDuplicateProb);
							problemSet.add(nameDuplicateProb);
						}
						EReference csiInstsRef = (EReference) si.eClass()
								.getEStructuralFeature(
										SafConstants.CSI_INSTANCES_NAME);
						EObject csiInstsObj = (EObject) si.eGet(csiInstsRef);

						if (csiInstsObj != null) {

							EReference csiInstRef = (EReference) csiInstsObj
									.eClass().getEStructuralFeature(
											SafConstants.CSI_INSTANCELIST_NAME);
							List csiObjs = (List) csiInstsObj.eGet(csiInstRef);
							for (int k = 0; k < csiObjs.size(); k++) {
								EObject csi = (EObject) csiObjs.get(k);
								amfConfigObjList.add(csi);
								String csiName = EcoreUtils.getName(csi);
								if (!uniqueNamesList.add(csiName)) {
									EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(41));
									EObject nameDuplicateProb = EcoreCloneUtils.cloneEObject(problemDataObj);
									EcoreUtils
											.setValue(nameDuplicateProb,
													ValidationConstants.PROBLEM_MESSAGE,
													"ComponentServiceInstance instance name is duplicate");
									
									EStructuralFeature srcFeature = nameDuplicateProb
											.eClass().getEStructuralFeature(
													ValidationConstants.PROBLEM_SOURCE);
									nameDuplicateProb.eSet(srcFeature, csi);
									List relatedObjList = (List) EcoreUtils
											.getValue(nameDuplicateProb,
													ValidationConstants.PROBLEM_RELATED_OBJECTS);
									relatedObjList.add(new Integer(k));

									probsList.add(nameDuplicateProb);
									problemSet.add(nameDuplicateProb);
								}
							}
						}
					}
				}
			}
		} catch (Exception e) {
			LOG
					.warn(
							"Error while validating for unique names in AMFConfigurations",
							e);
		}
		Iterator iterator = problemSet.iterator();
		while (iterator.hasNext()) {
			EObject problem = (EObject) iterator.next();
			List relatedObjList = (List) EcoreUtils.getValue(problem,
					ValidationConstants.PROBLEM_RELATED_OBJECTS);
			relatedObjList.add(amfConfigObjList);
		}

	}

	/**
	 * Checks for different field validity depending on the 
	 * Redundancy Model
	 * @param sgTypeList - SGTypes in the component editor
	 * @param retList - List of problems
	 */
	public void checkSGTypeValidity(List sgTypeList, List <EObject> retList) {
		for (int i = 0; i < sgTypeList.size(); i++) {
			EObject sgTypeObj = (EObject) sgTypeList.get(i);
			String redundancyModel = EcoreUtils.getValue(sgTypeObj,
					ComponentEditorConstants.SG_REDUNDANCY_MODEL).toString();
			
			int prefActiveSUs = ((Integer) EcoreUtils.getValue(sgTypeObj,
					SafConstants.SG_ACTIVE_SU_COUNT)).intValue();

			int prefStandBySUs = ((Integer) EcoreUtils.getValue(sgTypeObj,
					SafConstants.SG_STANDBY_SU_COUNT)).intValue();

			int prefInServiceSUs = ((Integer) EcoreUtils.getValue(sgTypeObj,
					SafConstants.SG_INSERVICE_SU_COUNT)).intValue();

			int prefAssignedSUs = ((Integer) EcoreUtils.getValue(sgTypeObj,
					SafConstants.SG_ASSIGNED_SU_COUNT)).intValue();
			
			int prefActiveSUsPerSI = ((Integer) EcoreUtils.getValue(sgTypeObj,
					SafConstants.SG_ACTIVE_SUS_PER_SI)).intValue();
			
			if (redundancyModel
					.equals(ComponentEditorConstants.NO_REDUNDANCY_MODEL)) {
				switch (prefActiveSUs) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(26));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils
							.setValue(
									problem,
									ValidationConstants.PROBLEM_MESSAGE,
									"'Active SUs' field value should be"
											+ " 1 when redundancy model is 'NO_REDUNDANCY'");
					
	   	            
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
					break;
				}
				switch (prefStandBySUs) {
				case 0:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(27));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils
							.setValue(
									problem,
									ValidationConstants.PROBLEM_MESSAGE,
									"'Standby SUs' field value should be"
											+ " 0 when redundancy model is 'NO_REDUNDANCY'");
					
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
					break;
				}
				switch (prefInServiceSUs) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(28));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils
							.setValue(
									problem,
									ValidationConstants.PROBLEM_MESSAGE,
									"'Inservice SUs' field value should be"
											+ " 1 when redundancy model is 'NO_REDUNDANCY'");
					
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
					break;
				}
				switch (prefActiveSUsPerSI) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(80));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils
							.setValue(
									problem,
									ValidationConstants.PROBLEM_MESSAGE,
									"'Active SUs Per SI' field value should be"
											+ " 1 when redundancy model is 'NO_REDUNDANCY'");
					
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
					break;
				}
			} else if (redundancyModel
					.equals(ComponentEditorConstants.TWO_N_REDUNDANCY_MODEL)) {
				switch (prefActiveSUs) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(29));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
							"'Active SUs' field value should be"
									+ " 1 when redundancy model is 'TWO_N'");
					
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
					break;
				}
				switch (prefStandBySUs) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(30));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
							"'Standby SUs' field value should be"
									+ " 1 when redundancy model is 'TWO_N'");
					
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
					break;
				}
				switch (prefInServiceSUs) {
				case 0:
				case 1:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(31));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
							"'Inservice SUs' field value should be"
									+ " >=2 when redundancy model is 'TWO_N'");
	   	            EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
					break;
				}
				switch (prefAssignedSUs) {
				case 2:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(32));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
							"'Assigned SUs' field value should be"
									+ " 2 when redundancy model is 'TWO_N'");
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
					break;
				}
				switch (prefActiveSUsPerSI) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(81));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils
							.setValue(
									problem,
									ValidationConstants.PROBLEM_MESSAGE,
									"'Active SUs Per SI' field value should be"
											+ " 1 when redundancy model is 'TWO_N'");
					
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
					break;
				}
			}else if (redundancyModel
					.equals(ComponentEditorConstants.M_PLUS_N_REDUNDANCY_MODEL)) {
				switch (prefActiveSUs) {
				case 0: 
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(57));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
							"'Active SUs' field value should be"
									+ " >0 when redundancy model is 'M+N'");
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
	
					break;
				}
				switch (prefStandBySUs) {
				case 0:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(30));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
							"'Standby SUs' field value should be"
									+ " >0 when redundancy model is 'M+N'");
	   	            
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
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
						problem.eSet(srcFeature, sgTypeObj);
						retList.add(problem);
					}
				}
				switch (prefActiveSUsPerSI) {
				case 1:
					break;
				default:
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(82));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
					EcoreUtils
							.setValue(
									problem,
									ValidationConstants.PROBLEM_MESSAGE,
									"'Active SUs Per SI' field value should be"
											+ " 1 when redundancy model is 'M+N'");
					
					EStructuralFeature srcFeature = problem.eClass()
							.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, sgTypeObj);
					retList.add(problem);
					break;
				}
			}
		}
	}

	/**
	 *
	 * @param list List
	 * @param className - Name on which filtering to be done
	 * @return th filtered list
	 */
	public List getfilterList(List objList, String className) {
		List compList = new Vector();
		if (className.equals(ComponentEditorConstants.COMPONENT_NAME)) {
			EObject rootObject = (EObject) objList.get(0);
			String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
			for (int i = 0; i < refList.length; i++) {
				String refName = refList[i];
				if (refName.equals(
						ComponentEditorConstants.SAFCOMPONENT_REF_NAME)
						|| refName.equals(
								ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME)) {
					EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refName);
					EList list = (EList) rootObject.eGet(ref);
					compList.addAll(list);
				}
			}
		} else if (className.equals(ClassEditorConstants.RESOURCE_NAME)) {
			EObject rootObject = (EObject) objList.get(0);
			String refList[] = ClassEditorConstants.NODES_REF_TYPES;
			for (int i = 0; i < refList.length; i++) {
				String refName = refList[i];
				if (refName.equals(
						ClassEditorConstants.SYSTEM_CONTROLLER_REF_NAME)
						|| refName.equals(
								ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME)
						|| refName.equals(
								ClassEditorConstants.HARDWARE_RESOURCE_REF_NAME)
						|| refName.equals(
								ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME)
						|| refName.equals(
								ClassEditorConstants.MIB_RESOURCE_REF_NAME)) {
					EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refName);
					EList list = (EList) rootObject.eGet(ref);
					compList.addAll(list);
				}
			}
		} else {
			// This code needs to be verified/modified
			EObject rootObject = (EObject) objList.get(0);
			List refList = rootObject.eClass().getEAllReferences();
			for (int i = 0; i < refList.size(); i++) {
				EReference ref = (EReference) refList.get(i);
				if(ref.getEType().getName().equals(className)) {
					EList list = (EList) rootObject.eGet(ref);
					compList.addAll(list);
				}
			}
		}
		return compList;
	}

	/**
	 * This method validates the presence of atleast 1 
	 * System Controller node in the component editor
	 * @param nodes - List of nodes in component editor
	 * @param probsList - List of problems
	 */
	public void validatePresenceOfControllerNode(List nodes,
			List probsList) {
		boolean controllerExists = false;
		for (int i = 0; i < nodes.size(); i++) {
			EObject node = (EObject) nodes.get(i);
			String classType = ((EEnumLiteral) EcoreUtils.getValue(node,
					ComponentEditorConstants.NODE_CLASS_TYPE)).toString();
			if (classType.equals(ComponentEditorConstants.NODE_CLASS_A)
					|| classType.equals(ComponentEditorConstants.NODE_CLASS_B)) {
				controllerExists = true;
				break;
			}
		}
		if (!controllerExists) {
			EObject noControllerProblem = (EObject) _problemNumberObjMap.get(new Integer(44));
			EcoreUtils.setValue(noControllerProblem, ValidationConstants.PROBLEM_MESSAGE,
					"Atleast one node of type 'Class A' or 'Class B' should be"
							+ " present in the component editor");
			List relatedObjects = (List) EcoreUtils.getValue(
					noControllerProblem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
			relatedObjects.add(nodes);
			probsList.add(noControllerProblem);
		}
	}

	/**
	 * Checks if alarm/provisioning is enabled and there is no alarmprofiles/provAttributes
	 * associated to the resource
	 * 
	 * @param resEditorList - List of resources in component editor
	 * @param alarmProfileList - Alarm Profile Objects
	 * @param probsList - List of problems
	 */
	public void checkAlarmProvisioningValidity(List resEditorList,
			List alarmObjList, List probsList) {
		List resList = getfilterList(resEditorList,
				ClassEditorConstants.RESOURCE_NAME);
		Iterator iterator = resList.iterator();
		while (iterator.hasNext()) {
			EObject resObj = (EObject) iterator.next();
			//check for provisioning
			EObject provObj = (EObject) EcoreUtils.getValue(resObj,
					ClassEditorConstants.RESOURCE_PROVISIONING);
			if (provObj != null) {
				boolean enable = ((Boolean) EcoreUtils.getValue(provObj,
						"isEnabled")).booleanValue();
				if (enable) {
					boolean hasProblem = true;
					List provAttrList = (List) EcoreUtils.getValue(provObj,
							ClassEditorConstants.CLASS_ATTRIBUTES);
					if (provAttrList.isEmpty()) {
						List baseClassList = getBaseClasses(resObj,
								resEditorList);
						for (int i = 0; i < baseClassList.size(); i++) {
							EObject baseObj = (EObject) baseClassList.get(i);
							EObject baseProvObj = (EObject) EcoreUtils
									.getValue(
											baseObj,
											ClassEditorConstants.RESOURCE_PROVISIONING);
							if (baseProvObj != null) {
								List baseProvAttrList = (List) EcoreUtils
										.getValue(
												baseProvObj,
												ClassEditorConstants.CLASS_ATTRIBUTES);
								if (!baseProvAttrList.isEmpty()) {
									hasProblem = false;
									break;
								}
							}
						}
						if (hasProblem) {
							EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(5));
							EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
							EcoreUtils
									.setValue(
											problem,
											ValidationConstants.PROBLEM_MESSAGE,
											"Provisioning should not be enabled "
													+ "without having any attribute to be provisioned");
							
							EStructuralFeature srcFeature = problem.eClass()
									.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
							problem.eSet(srcFeature, resObj);
							List relatedObjects = (List) EcoreUtils.getValue(
									problem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
							relatedObjects.add(provObj);
							probsList.add(problem);
						}
					}
				}
			}

			//         check for alarm management
			EObject alarmObj = (EObject) EcoreUtils.getValue(resObj,
					ClassEditorConstants.RESOURCE_ALARM);
			if (alarmObj != null) {
				boolean enable = ((Boolean) EcoreUtils.getValue(alarmObj,
						"isEnabled")).booleanValue();
				if (enable) {
					List alarmList = ResourceDataUtils.getAssociatedAlarms(_project, resObj);
					if (alarmList == null || alarmList.isEmpty()) {
						EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(6));
						EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj); 
						EcoreUtils
								.setValue(
										problem,
										ValidationConstants.PROBLEM_MESSAGE,
										"Alarm Management should not be enabled "
												+ "without having any alarms associated to the resource");
						
						EStructuralFeature srcFeature = problem.eClass()
								.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
						problem.eSet(srcFeature, resObj);
						List relatedObjects = (List) EcoreUtils.getValue(
								problem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
						relatedObjects.add(alarmObj);
						probsList.add(problem);
					}
				}
			}
		}
	}

	/**
	 * Checks the connections
	 * @param obj EObject(Node)
	 * @param parentConnType - Parent Connection type (i.e Containment or Association)
	 * @return boolean
	 */
	private static List getBaseClasses(EObject nodeObj, List objects) {
		List baseList = new Vector();
		ResourceDataUtils resUtils = new ResourceDataUtils(objects);
		for (int i = 0; i < objects.size(); i++) {
			EObject obj = (EObject) objects.get(i);
			String className = obj.eClass().getName();
			if (className.equals(ClassEditorConstants.INHERITENCE_NAME)) {
				EObject sourceObj = resUtils.getSource(obj);
				EObject targetObj = resUtils.getTarget(obj);
				if (nodeObj.equals(sourceObj)) {
					baseList.add(targetObj);
				}
			}
		}
		return baseList;
	}

	/**
	 * Validates the number of clients defined for a EO
	 * @param safCompList - All SAFComponents which have EO
	 * @param probsList - List of problems
	 */
	public void validateIDL(List safCompList, List idlList,
			List probsList) {
		if (idlList.isEmpty()) {
			return;
		}
		List eoList = getEOs(safCompList);
		HashMap nameEOMap = new HashMap();
		for (int i = 0; i < eoList.size(); i++) {
			EObject eoObj = (EObject) eoList.get(i);
			String eoName = EcoreUtils.getName(eoObj);
			nameEOMap.put(eoName, eoObj);
		}
		EObject idlObj = (EObject) idlList.get(0);
		List idlEoList = (List) EcoreUtils.getValue(idlObj, "Service");
		for (int i = 0; i < idlEoList.size(); i++) {
			EObject idlEoObj = (EObject) idlEoList.get(i);
			String eoName = EcoreUtils.getValue(idlEoObj, "name").toString();
			EObject eoObj = (EObject) nameEOMap.get(eoName);
			if (eoObj == null) {
				continue;
			}
			int additinalClients = ((Integer) EcoreUtils.getValue(eoObj,
					ComponentEditorConstants.EO_MAX_CLIENTS)).intValue();
			List clientList = (List) EcoreUtils.getValue(idlEoObj, "Port");
			if (clientList.size() != additinalClients) {
				EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(45));
				EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Mismatch in 'Number of additional port(client)s'"
										+ " defined in RMD and 'Number of additional port(client)s'"
										+ " specified in EOProperties for '"
										+ eoName + "'");
				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, idlEoObj);
				List relatedObjects = (List) EcoreUtils.getValue(problem,
						ValidationConstants.PROBLEM_RELATED_OBJECTS);

				relatedObjects.add(clientList);
				relatedObjects.add(new Integer(additinalClients));
				relatedObjects.add(eoObj);
				probsList.add(problem);
			}

			Iterator itr = clientList.iterator();
			while(itr.hasNext()) {
				EObject clientObj = (EObject) itr.next();
				List groupList = (List) EcoreUtils.
					getValue(clientObj, "Group");

				if (groupList.isEmpty()) {
					EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(48));
					EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
					EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
							"There is no Operation Group defined for the Port '"
							+ EcoreUtils.getName(clientObj) + "'");
					
					EStructuralFeature srcFeature = problem.eClass()
					.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
					problem.eSet(srcFeature, clientObj);
					probsList.add(problem);
				}
			}
		}

	}

	/**
	 * 
	 * @param safCompList - List of SAF Components
	 * @return the EO objects
	 */
	private static List getEOs(List safCompList) {
		List eoList = new Vector();
		for (int i = 0; i < safCompList.size(); i++) {
			EObject compObj = (EObject) safCompList.get(i);
			EObject eoObj = (EObject) EcoreUtils.getValue(compObj,
					ComponentEditorConstants.EO_PROPERTIES_NAME);
			if (eoObj != null) {
				eoList.add(eoObj);
			}
		}
		return eoList;
	}

	/**
	 * Validates the number of SAFComponent with "Is SNMP sub-agent" enabled
	 * should not be greater than 1.
	 * @param safCompList -
	 *            All SAFComponents
	 * @param utils 
	 * @param probsList -
	 *            List of problems
	 */
	public void validateSNMPSubAgent(List safCompList, ComponentDataUtils utils, List probsList) {
		List snmpSubagentlist = new Vector();

		for (int i = 0; i < safCompList.size(); i++) {
			EObject compObj = (EObject) safCompList.get(i);
			boolean isSnmpSubagent = ((Boolean) EcoreUtils.getValue(compObj,
					ComponentEditorConstants.IS_SNMP_SUBAGENT)).booleanValue();
			if (isSnmpSubagent) {
				snmpSubagentlist.add(compObj);
			}
		}

		if(snmpSubagentlist.size() == 0) {
			return;
		}

		if (snmpSubagentlist.size() > 1) {
			EObject problem = (EObject) _problemNumberObjMap.get(new Integer(52));
			EcoreUtils
					.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
							"There cannot be more than one SNMP Sub Agent configured in Component Editor");
			
			probsList.add(problem);
			((List) EcoreUtils.getValue(problem, ValidationConstants.PROBLEM_RELATED_OBJECTS)).add(snmpSubagentlist);
		}

		checkSNMPAgentIsProxy(snmpSubagentlist, utils, probsList);
		validateSnmpSubAgentConfiguration(snmpSubagentlist, probsList);
	}

	/**
	 * Checks if SNMPAgent Component proxies any other component, if so 
	 * flags error
	 * @param safCompList - List of SAF Components defined in the editor
	 * @param utils - Utils Object
	 * @param retList - List of problems to be returned after validation
	 */
	private void checkSNMPAgentIsProxy(List snmpSubagentlist,
			ComponentDataUtils utils, List retList) {

		List proxyProxiedList = utils.getConnectionFrmType(
				ComponentEditorConstants.PROXY_PROXIED_NAME,
				ComponentEditorConstants.SAFCOMPONENT_NAME,
				ComponentEditorConstants.NONSAFCOMPONENT_NAME);

		for (int i = 0; i < proxyProxiedList.size(); i++) {
			EObject connObj = (EObject) proxyProxiedList.get(i);
			EObject srcObj = utils.getSource(connObj);

			if (snmpSubagentlist.contains(srcObj)) {
				EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(53));
				EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
								"Component should not be proxy when it is a SNMP Sub Agent");
				
				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, srcObj);
				retList.add(problem);
			}
		}
	}

	/**
	 * Validates SnmpSubAgent Configuration whether it is done or not and if done then checks for its validity.
	 * 
	 * @param snmpSubagentList
	 * @param retList
	 */
	private void validateSnmpSubAgentConfiguration(List snmpSubagentList, List retList)
	{
		List<EObject> compMibMappingList = getCompMibMappingList();
		EObject snmpSubAgentObj;

		for (int i = 0; i < snmpSubagentList.size(); i++) 
		{
			snmpSubAgentObj = (EObject) snmpSubagentList.get(i);
			String compName = EcoreUtils.getValue(snmpSubAgentObj, ComponentEditorConstants.NAME).toString();
			boolean mibEntryFound = false;

			for (int j = 0; j < compMibMappingList.size(); j++)
			{
				EObject compMibObj = (EObject) compMibMappingList.get(j);
	           	String name = (String) EcoreUtils.getValue(compMibObj, "compName");
	           	if (name != null && name.equals(compName))
	           	{
	           		mibEntryFound = true;
	        		String moduleName = (String) EcoreUtils.getValue(compMibObj, "moduleName");
	        		String mibPath = (String) EcoreUtils.getValue(compMibObj, "mibPath");
	        		if(validateSubAgentProperties(snmpSubAgentObj, moduleName, mibPath, compName, retList)) {
	        			String[] vals = moduleName.split(",");
	        			for (int k=0; k<vals.length; k++)
	        			{
			        		validateMibs(vals[k], mibPath, snmpSubAgentObj, compName, retList);
	        			}
	        		}
	           		break;
	           	}
			}
			
			if (!mibEntryFound)
			{
    			EObject problem = (EObject) _problemNumberObjMap.get(new Integer(105));
				EObject mibConfigProb = EcoreCloneUtils.cloneEObject(problem);
    			EcoreUtils.setValue(mibConfigProb, ValidationConstants.PROBLEM_MESSAGE,
    						"The SNMP Sub-agent component " + compName + " has not been configured with MIB information");

    			EStructuralFeature srcFeature = mibConfigProb.eClass()
					.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
				mibConfigProb.eSet(srcFeature, snmpSubAgentObj);
    			retList.add(mibConfigProb);
			}
		}
	}

	/**
	 * Validates moduleName and mibPath for the sub agent.
	 * 
	 * @param subAgentCompObj
	 * @param moduleName
	 * @param mibPath
	 * @param compName
	 * @param retList
	 * @return true if both of them are valid, false otherwise
	 */
	private boolean validateSubAgentProperties(EObject subAgentCompObj,
			String moduleName, String mibPath, String compName, List retList) {

		boolean isValid = true;

		if (moduleName == null || moduleName.equals("")) {
			EObject problem = (EObject) _problemNumberObjMap.get(new Integer(
					103));
			EObject mibNameProb = EcoreCloneUtils.cloneEObject(problem);
			EcoreUtils.setValue(mibNameProb,
					ValidationConstants.PROBLEM_MESSAGE,
					"The SNMP Sub-agent component " + compName
							+ " must have a MIB module name.");
			EStructuralFeature srcFeature = mibNameProb.eClass()
					.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
			mibNameProb.eSet(srcFeature, subAgentCompObj);
			retList.add(mibNameProb);
			isValid = false;
		}

		if (mibPath == null || mibPath.equals("")) {
			EObject problem = (EObject) _problemNumberObjMap.get(new Integer(
					104));
			EObject mibDirProb = EcoreCloneUtils.cloneEObject(problem);
			EcoreUtils.setValue(mibDirProb,
					ValidationConstants.PROBLEM_MESSAGE,
					"Missing MIB Directory Location for SNMP Sub-agent component "
							+ compName + ".");
			EStructuralFeature srcFeature = mibDirProb.eClass()
					.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
			mibDirProb.eSet(srcFeature, subAgentCompObj);
			retList.add(mibDirProb);
			isValid = false;
		}

		return isValid;
	}

	/**
	 * Returns the list containing comp to Mib mapping.
	 * 
	 * @return
	 */
	private List<EObject> getCompMibMappingList() {

		URL caURL = DataPlugin.getDefault().find(
				new Path("model" + File.separator + "compmibmap.ecore"));
		try {
			File ecoreFile = new Path(Platform.resolve(caURL).getPath())
					.toFile();
			EcoreModels.getUpdated(ecoreFile.getAbsolutePath());
		} catch (IOException e) {
			e.printStackTrace();
		}

		String dataFilePath = _project.getLocation().toOSString()
				+ File.separator + ICWProject.CW_PROJECT_MODEL_DIR_NAME
				+ File.separator + "compmibmap.xmi";
		File xmiFile = new File(dataFilePath);
		URI uri = URI.createFileURI(dataFilePath);

		Resource resource = xmiFile.exists() ? EcoreModels
				.getUpdatedResource(uri) : EcoreModels.create(uri);
		return resource.getContents();
	}

	/**
	 * Validates Mibs for the module name.
	 * 
	 * @param mibPath
	 * @param moduleName
	 * @param snmpSubAgentObj 
	 * @param compName 
	 * @param retList 
	 * @return true if valid, otherwise false
	 */
	private boolean validateMibs(String moduleName, String mibPath, EObject snmpSubAgentObj, String compName, List retList)
	{
		boolean isValid = true;
		List<String> errorList = new ArrayList<String>();
		
		//List<String> mibLocationList = ClovisMibUtils.getSystemMibLocations(_project);
		String dirs[] = mibPath.split(":"), dir;
		boolean isValidModule = isValidMibModuleName(moduleName, _resEditorObjects, dirs, ProjectDataModel.getProjectDataModel(_project), errorList);
		/*SnmpOID moduleID = null;
		for (int d = 0; d < dirs.length; d++) {
			dir = dirs[d];
			// skips directories in the system path
			if (!mibLocationList.contains(dir)) {
				loadMibsFromDir(dir, errorList);
			}
		}
		moduleID = MibUtil.lookupOID(moduleName);
		if (moduleID == null) {
			// If the specified mib is in system path 
			for (int d = 0; d < dirs.length; d++) {
				dir = dirs[d];
				// load mibs in the system path
				if (mibLocationList.contains(dir)) {
					loadMibsFromDir(dir, errorList);
				}
			}
			moduleID = MibUtil.lookupOID(moduleName);
		}*/
		if (!isValidModule) {
			EObject problem = null;
			EObject mibProb = null;
			EStructuralFeature srcFeature = null;
			if (!isValidMibNode(moduleName, _resEditorObjects, dirs, ProjectDataModel.getProjectDataModel(_project), errorList)) {
				problem = (EObject) _problemNumberObjMap
						.get(new Integer(106));
				mibProb = EcoreCloneUtils.cloneEObject(problem);
				EcoreUtils
						.setValue(
								mibProb,
								ValidationConstants.PROBLEM_MESSAGE,
								"The SNMP Sub-agent component '"
										+ compName
										+ "' has been configured with invalid module name '"
										+ moduleName + "'.");

				srcFeature = mibProb.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				mibProb.eSet(srcFeature, snmpSubAgentObj);
				retList.add(mibProb);
			} else {
				problem = (EObject) _problemNumberObjMap.get(new Integer(109));
				mibProb = EcoreCloneUtils.cloneEObject(problem);
				EcoreUtils
						.setValue(
								mibProb,
								ValidationConstants.PROBLEM_MESSAGE,
								"In SNMP sub-agent properties, module name should be a SNMP node of type MODULE-IDENTITY. However legacy standards (in particular IF-MIB) sometimes reference mib-2 directly without any MODULE-IDENTITY. Only skip this warning if you are using standard legacy MIB(s)");

				srcFeature = mibProb.eClass().getEStructuralFeature(
						ValidationConstants.PROBLEM_SOURCE);
				mibProb.eSet(srcFeature, snmpSubAgentObj);
				retList.add(mibProb);
			}
			if(errorList.size() > 0) {
				problem = (EObject) _problemNumberObjMap.get(new Integer(
						107));
				mibProb = EcoreCloneUtils.cloneEObject(problem);
				EcoreUtils.setValue(mibProb,
						ValidationConstants.PROBLEM_MESSAGE,
						"The SNMP Sub-agent component '" + compName + "' has not been configured with valid MIBs.");

				srcFeature = mibProb.eClass()
						.getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
				mibProb.eSet(srcFeature, snmpSubAgentObj);

				String description = EcoreUtils.getValue(mibProb, "description").toString(), newDescription = "";
				newDescription = description.substring(0, description.indexOf("SOLUTION:"));
				for(int i=0 ; i<errorList.size() ; i++) {
					newDescription += (i+1) + " :  " + errorList.get(i) + "\n";
				}
				newDescription += "\n\n" + description.substring(description.indexOf("SOLUTION:"));
				EcoreUtils.setValue(mibProb, "description", newDescription);

				retList.add(mibProb);
			}
			isValid = false;
		}

		MibUtil.unloadAllMibs();
		return isValid;
	}

	/**
	 * Loads all the valid mib files from the given directory.
	 * 
	 * @param mibDir
	 */
	private void loadMibsFromDir(String mibDir, List<String> errorList) {
		if (new File(mibDir).exists()) {
			String files[] = new File(mibDir).list();
			String file;

			for (int i = 0; i < files.length; i++) {
				if (!files[i].startsWith(".")) {
					file = mibDir + File.separator + files[i];

					if (new File(file).isFile()) {
						try {
							MibUtil.loadMib(file, false);

						} catch (Exception e) {
							errorList.add("'" + file + "' : " + e.getMessage()
									+ ".");
						}
					}
				}
			}
		}
	}
	
	/**
	 * Validates the MIB's module name
	 * @param moduleName
	 * @param resObjects
	 * @param dirs
	 * @return
	 */
	public static boolean isValidMibModuleName(String moduleName, List <EObject>resObjects, String dirs[], ProjectDataModel pdm, List errorList) {
		List<String> mibNames = getImportedMibNames(resObjects);
		for(int i = 0; i < dirs.length; i++) {
			String dir = dirs[i];
			for (int j = 0; j < mibNames.size(); j++) {
				File mibFile = new File(dir + File.separator + mibNames.get(j));
				if(mibFile.exists()) {
					try {
						MibUtil.unloadAllMibs();
						ClovisMibUtils.loadSystemMibs(pdm.getProject());
						MibUtil.setResolveSyntax(true);
						MibTreeNode node = MibUtil.parseMib(mibFile.getAbsolutePath(), false);
						if(node.getModuleIdentity().equals(moduleName)) {
							return true;
						}
					} catch (Exception e) {
						errorList.add("'" + mibFile.getAbsolutePath() + "' : " + e.getMessage()
								+ ".");
					}
				}
			}
		}
		return false;
	}
	/**
	 * Validates the specified module name is a valid node name.
	 * @param moduleName
	 * @param resObjects
	 * @param dirs
	 * @return
	 */
	public static boolean isValidMibNode(String moduleName, List <EObject>resObjects, String dirs[], ProjectDataModel pdm, List errorList) {
		List<String> mibNames = getImportedMibNames(resObjects);
		for(int i = 0; i < dirs.length; i++) {
			String dir = dirs[i];
			for (int j = 0; j < mibNames.size(); j++) {
				File mibFile = new File(dir + File.separator + mibNames.get(j));
				if(mibFile.exists()) {
					try {
						MibUtil.unloadAllMibs();
						ClovisMibUtils.loadSystemMibs(pdm.getProject());
						MibUtil.setResolveSyntax(true);
						MibUtil.loadMib(mibFile.getAbsolutePath(), false);
						SnmpOID moduleID = MibUtil.lookupOID(moduleName);
						if(moduleID != null)
							return true;
					} catch (Exception e) {
						errorList.add("'" + mibFile.getAbsolutePath() + "' : " + e.getMessage()
								+ ".");
					}
				}
			}
		}
		return false;
	}
	/**
	 * Returns the List of Mib Names
	 * @param resObjects
	 * @return
	 */
	public static List getImportedMibNames( List <EObject>resObjects){
		List<String> mibNames = new ArrayList<String>();
		EObject rootObject = resObjects.get(0);
		EReference ref = (EReference) rootObject.eClass()
		.getEStructuralFeature(ClassEditorConstants.MIB_RESOURCE_REF_NAME);
		EList list = (EList) rootObject.eGet(ref);
		for (int i = 0; i < list.size(); i++) {
			String mibName = (String)EcoreUtils.getValue((EObject)list.get(i), "mibName");
			if(!mibNames.contains(mibName)) {
				mibNames.add(mibName);
			}
		}
		return mibNames;
	}
}
