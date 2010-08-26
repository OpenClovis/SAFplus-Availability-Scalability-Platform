/*
 * @(#) $RCSfile: ComplexValidator.java,v $
 * $Revision: #6 $ $Date: 2007/04/30 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/ComplexValidator.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * @(#) $RCSfile: ComplexValidator.java,v $
 * $Revision: #6 $ $Date: 2007/04/30 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
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

import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.genericeditor.GEDataUtils;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * 
 * @author shubhada
 * complexvalidator class to do complex validations 
 */
public class ComplexValidator
{
    private GEDataUtils _utils = null;
    private List _invalidScenarioList = null;
    private List _elist = null;
    private List _connectionList = new Vector();
    private HashMap _problemNumberObjMap = null;
    private static final int OPERATOR_AND = 0;
    private static final int OPERATOR_OR = 1;
    private static final Log LOG = Log.getLog(WorkspacePlugin.getDefault());
    private final int DIRECTION_IN = 0, DIRECTION_OUT = 1;
    /**
     * Constructor
     */
    public ComplexValidator(List resEditorObjects, HashMap problemNumberObjMap)
	{
		_problemNumberObjMap = problemNumberObjMap;
        _elist = resEditorObjects;
        _utils = new ComponentDataUtils(_elist);
        initConnectionList();
        readInvalidScenarios();        
    }
    /**
    *
    * initializes the existing connections
    */
   public void initConnectionList()
   {
	   if (!_elist.isEmpty()) {
		   EObject rootObject = (EObject) _elist.get(0);
			String refList[] = ComponentEditorConstants.EDGES_REF_TYPES;
			for (int i = 0; i < refList.length; i++) {
				EReference ref = (EReference) rootObject.eClass()
						.getEStructuralFeature(refList[i]);
				EList list = (EList) rootObject.eGet(ref);
				for (int j = 0; j < list.size(); j++) {
					EObject eobj = (EObject) list.get(j);
					_connectionList.add(eobj);
				}
			}
	   }
   }
   /**
   *
   * reads the xmi file which has details about invalid scenarios.
   */
  private void readInvalidScenarios()
  {
      URL url = DataPlugin.getDefault().getBundle().getEntry("/");
      try {
          url = Platform.resolve(url);
          String fileName = url.getFile() + "xml" + File.separator
          + "validations.xmi";
          URI uri = URI.createFileURI(fileName);
          _invalidScenarioList = (NotifyingList) EcoreModels.read(uri);
      } catch (IOException e) {
          LOG.error("Error reading validations file", e);
      }
  }
  /**
   * Validate all the connections in the ComponentEditorObjects
   * based on the dependency with any other connection. 
   * @param retList - Return the list of problems encountered
   */
  public void validateComponentEditorConnections(List retList)
  {
      for (int i = 0; i < _connectionList.size(); i++) {
          EObject connObj = (EObject) _connectionList.get(i);
          EObject sourceObj = _utils.getSource(connObj);
          EObject targetObj = _utils.getTarget(connObj);
          String message = checkConnectionDependencyValidity(sourceObj,
                  targetObj, connObj);
          if (message != null) {
              String srcName = EcoreUtils.getName(sourceObj);
              String targetName = EcoreUtils.getName(targetObj);
              EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(8));
              EObject edgeIncorrectProblem = EcoreCloneUtils.cloneEObject(problemDataObj);
              EcoreUtils.setValue(edgeIncorrectProblem, ValidationConstants.PROBLEM_MESSAGE,
                      "Invalid '" + EcoreUtils.getValue(connObj,
                              ComponentEditorConstants.CONNECTION_TYPE)
                      + "' connection from " + srcName + " to "
                      + targetName + ".\n" + message);
              EStructuralFeature srcFeature = edgeIncorrectProblem.eClass().
                  getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
              edgeIncorrectProblem.eSet(srcFeature, sourceObj);
              List relatedObjects = (List) EcoreUtils.getValue(
            		  edgeIncorrectProblem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
              relatedObjects.add(targetObj);
              relatedObjects.add(connObj);
              
              retList.add(edgeIncorrectProblem);
          }
              
      }
  }
  /**
   * 
   * @param sourceObj Connection Source
   * @param targetObj Connection Target
   * @param connObj Connection Object
   * @return the Error Message or null
   */
  private String checkConnectionDependencyValidity(EObject sourceObj,
          EObject targetObj, EObject connObj)
  {
      String msg = null;
      
      for (int i = 0; i < _invalidScenarioList.size(); i++) {
          EObject scenarioObj = (EObject) _invalidScenarioList.get(i);
          boolean match = checkForConnectionTypeMatch(scenarioObj,
                  connObj, sourceObj, targetObj);
          if (match) {
              /* Assumption is that either a operation or a connection will be
               * specified in the xml under InvalidScenario 
               * but not both of them.
               */ 
              EObject relatedConnObj = (EObject) EcoreUtils.getValue(
                      scenarioObj, "connection");
              if (relatedConnObj != null) {
                  msg = validateRelativeConnection(relatedConnObj,
                          sourceObj, targetObj);
              } else {
                  EObject operObj = (EObject) EcoreUtils.getValue(
                          scenarioObj, "operation");
                  msg = processOperations(operObj, sourceObj, targetObj);
                  
                  
              }
          }
      }
      return msg;
  }
  /**
   * Processes the operation objects from the xmi
   * @param operObj Operation Object specified in XMI
   * @param sourceObj
   * @return the Error Message if any
   */
  private String processOperations(EObject operObj, EObject sourceObj,
          EObject targetObj)
  {
      int operator = ((EEnumLiteral) EcoreUtils.getValue(
              operObj, "type")).getValue();
      List connectionList = (List) EcoreUtils.getValue(operObj,
              "connection");
      if (connectionList.size() == 2) {
          EObject relConnObj1 = (EObject) connectionList.get(0);
          EObject relConnObj2 = (EObject) connectionList.get(1);
          String msg1 = validateRelativeConnection(relConnObj1,
                  sourceObj, targetObj);
          String msg2 = validateRelativeConnection(relConnObj2,
                  sourceObj, targetObj);
          String msg3 = performOperation(msg1, msg2, operator);
          return msg3;
      } else {
          EObject nextOperObj = (EObject) EcoreUtils.getValue(
                  operObj, "operation");
          String msg1 = processOperations(nextOperObj, sourceObj, targetObj);
          EObject relConnObj = (EObject) ((List) EcoreUtils.getValue(
                  operObj, "operation")).get(0);
          String msg2 = validateRelativeConnection(relConnObj,
                  sourceObj, targetObj);
          String msg3 = performOperation(msg1, msg2, operator);
          return msg3;
      }
  }
  /**
   * Performs the operation based on the operator value
   * 
   * @param msg1 Message returned on validating 1st relative conn
   * @param msg2 Message returned on validating 2nd relative conn
   * @param operator - type of operation to be done (AND, OR)
   * @return the new message
   */
  private String performOperation(String msg1, String msg2, int operator)
  {
     switch(operator) {
     /*In case of "AND" return Error message only if both relative connection
      * satisfy the condition for error.
      * In case of "OR" return error message if any of the relative connection
      * satisfy the condition for error.
     */
     case OPERATOR_AND: if (msg1 == null && msg2 == null) {
                             return null;
                        } else if (msg1 == null && msg2 != null) {
                            return msg1;
                        } else {
                            return null;
                        }
     case OPERATOR_OR:  if (msg1 == null && msg2 == null) {
                             return null;
                         } else if (msg1 != null && msg2 != null) {
                             return msg1;
                         } else if(msg1 != null && msg2 == null) {
                             return msg1;
                         } else if(msg1 == null && msg2 != null) {
                             return msg2;
                         }                                     
     }
      return null;
  }
  /**
   * Based on the the relative connection info specified in xmi,
   * it will get the matched source and target list and calls
   * checkRelativeConnection to check further information.
   * 
   * @param relatedConnObj
   * @param sourceObj
   * @param targetObj
   * @return
   */
  private String validateRelativeConnection(EObject relatedConnObj,
          EObject sourceObj, EObject targetObj)
  {
      HashSet matchedSourceList = new HashSet();
      HashSet matchedTargetList = new HashSet();
      EObject sourcePathInfoObj = (EObject) EcoreUtils.
          getValue(relatedConnObj, "sourcePathInfo");
      
      List sourcePathList = (List) EcoreUtils.
          getValue(sourcePathInfoObj, "sourcePath");
      for (int i = 0; i < sourcePathList.size(); i++) {
          EObject sourcePathObj = (EObject) sourcePathList.get(i);
          int sourceStart = ((EEnumLiteral) EcoreUtils.
              getValue(sourcePathObj, "start")).getValue();
          List nodeList =  (List) EcoreUtils.
              getValue(sourcePathObj, "node");
          switch (sourceStart) {
          case 0: 
                  traverse(sourceObj, nodeList, matchedSourceList, 0);
                  break;
                  
          case 1: 
                  traverse(targetObj, nodeList, matchedSourceList, 0);
                  break;
          }
      }
      EObject targetPathInfoObj = (EObject) EcoreUtils.
          getValue(relatedConnObj, "targetPathInfo");
      List targetPathList = (List) EcoreUtils.
          getValue(targetPathInfoObj, "targetPath");
      for (int i = 0; i < targetPathList.size(); i++) {
          EObject targetPathObj = (EObject) targetPathList.get(i);
          int targetStart = ((EEnumLiteral) EcoreUtils.
                  getValue(targetPathObj, "start")).getValue();
          List nodeList =  (List) EcoreUtils.
              getValue(targetPathObj, "node");
          switch (targetStart) {
          case 0: 
              traverse(sourceObj, nodeList, matchedTargetList, 0);
              break;
          case 1: 
              traverse(targetObj, nodeList, matchedTargetList, 0);
              break;
          }
      }
      boolean sNegate = ((Boolean) EcoreUtils.getValue(
              sourcePathInfoObj, "negate")).booleanValue();
      boolean tNegate = ((Boolean) EcoreUtils.getValue(
              targetPathInfoObj, "negate")).booleanValue();
      
      
      RelativeConnectionChecker checker = new RelativeConnectionChecker(_connectionList, _utils);
      String message = checker.checkRelativeConnection(matchedSourceList, matchedTargetList,
            relatedConnObj, sNegate, tNegate);
      return message;
  }
  /**
   * Checks whether connection name , source , target of the current conn to
   * be checked matches with info specified in xmi.
   * 
   * @param scenarioObj Object from XMI file
   * @param connObj Object to be validated
   * @param sourceObj Source of connection
   * @param targetObj Target of connection
   * @return true if match else return false 
   */
  private boolean checkForConnectionTypeMatch(EObject scenarioObj, EObject connObj, EObject sourceObj, EObject targetObj)
  {
      String connName = (String) EcoreUtils.getValue(scenarioObj, "connectionName");
      String source = (String) EcoreUtils.getValue(scenarioObj, ValidationConstants.PROBLEM_SOURCE);
      String target = (String) EcoreUtils.getValue(scenarioObj, "target");
      if (connName.equals(EcoreUtils.getValue(connObj,
				ComponentEditorConstants.CONNECTION_TYPE))) {
			if (source.equals(sourceObj.eClass().getName())
					&& target.equals(targetObj.eClass().getName())) {
				return true;
			} else {
				boolean sourceMatch = false, targetMatch = false;
				// if source does not match with SourceObj of the conn,
				// go to super classes to find the match
				if (!source.equals(sourceObj.eClass().getName())
						&& target.equals(targetObj.eClass().getName())) {
					List sourceSuperClasses = sourceObj.eClass()
							.getEAllSuperTypes();
					for (int i = 0; i < sourceSuperClasses.size(); i++) {
						EClass superClass = (EClass) sourceSuperClasses.get(i);
						if (source.equals(superClass.getName())) {
							return true;
						}
					}
				} else if (source.equals(sourceObj.eClass().getName())
						&& !target.equals(targetObj.eClass().getName())) {
					List targetSuperClasses = targetObj.eClass()
							.getEAllSuperTypes();
					for (int i = 0; i < targetSuperClasses.size(); i++) {
						EClass superClass = (EClass) targetSuperClasses.get(i);
						if (target.equals(superClass.getName())) {
							return true;
						}
					}
				} else {
					List sourceSuperClasses = sourceObj.eClass()
							.getEAllSuperTypes();
					for (int i = 0; i < sourceSuperClasses.size(); i++) {
						EClass superClass = (EClass) sourceSuperClasses.get(i);
						if (source.equals(superClass.getName())) {
							sourceMatch = true;
							break;
						}
					}
					List targetSuperClasses = targetObj.eClass()
							.getEAllSuperTypes();
					for (int i = 0; i < targetSuperClasses.size(); i++) {
						EClass superClass = (EClass) targetSuperClasses.get(i);
						if (target.equals(superClass.getName())) {
							targetMatch = true;
							break;
						}
					}
				}
				if (sourceMatch == true && targetMatch == true) {
					return true;
				}
			}
		}

		return false;
	}
  /**
	 * Returns all target instances based on connection type, targettype and
	 * source instance.
	 * 
	 * @param connType
	 *            Connection Type
	 * @param sourceObj
	 *            Source Object
	 * @param targetType
	 *            Target Class Type
	 * @param direction -
	 *            Indicates whether the current Obj is the source of the
	 *            connection or target of the connection
	 * @return the filtered connections
	 */
  private List getTargetOfConnection(String connType, EObject sourceObj,
          String targetType, EEnumLiteral direction) 
  {
      List targetList = new Vector();
      for (int i = 0; i < _connectionList.size(); i++) {
          EObject connObj = (EObject) _connectionList.get(i);
          EObject sObj = _utils.getSource(connObj);
          EObject tObj = _utils.getTarget(connObj);
          switch(direction.getValue()) {
          case DIRECTION_IN:
				if (connType.equals(EcoreUtils.getValue(connObj,
						ComponentEditorConstants.CONNECTION_TYPE))
						&& tObj.equals(sourceObj)
						&& targetType.equals(sObj.eClass().getName())) {
					targetList.add(sObj);
				}
				break;
			case DIRECTION_OUT:
				if (connType.equals(EcoreUtils.getValue(connObj,
						ComponentEditorConstants.CONNECTION_TYPE))
						&& sObj.equals(sourceObj)
						&& targetType.equals(tObj.eClass().getName())) {
					targetList.add(tObj);
				}
				break;
          }
          
          
      }
      return targetList;
  }
  
  /**
	 * Traverses recursively the graph to reach the nodes whose info is
	 * specified in xmi.
	 * 
	 * @param srcObj -
	 *            Traversal Start Object
	 * @param nodeList -
	 *            Traversal Node List specified in xmi
	 * @param resultList -
	 *            List containing successfully reached target nodes.
	 * @param index -
	 *            current index to
	 */
  private void traverse(EObject srcObj, List nodeList, HashSet resultList, int index)
  {
      EObject nodeObj = (EObject) nodeList.get(index);
      String connType = (String) EcoreUtils.getValue(nodeObj, "connType");
      String targetType = (String) EcoreUtils.getValue(nodeObj, "targetNodeName");
      EEnumLiteral direction = (EEnumLiteral) EcoreUtils.getValue(nodeObj, "direction");
      List targetConnList = getTargetOfConnection(connType, srcObj, targetType, direction);
      //if List is empty, then there is no connection of type specified in xmi
      if (targetConnList.isEmpty()) {
          return;
      } else {
          index++;
          if (index == nodeList.size()) {
              resultList.addAll(targetConnList);
          } else {
              for (int i = 0; i < targetConnList.size(); i++) {
                  EObject targetObj = (EObject) targetConnList.get(i);
                  traverse(targetObj, nodeList, resultList, index);
                  
              }
          }
      }  
  }
  /**
   * Checks if all the objects in the srcList and targetList
   * are covered or not.
   * 
   * @param filteredList - All filtered conn list between
   * source type and target type
   * @param srcList - all source type objects List
   * @param targetList - all target type object List
   * @param srcProblem
   * @param targetProblem
   * @param retList - Problems list
   */
  private void processFilteredConnections(List filteredList, List srcList,
          List targetList, String srcProblem, String targetProblem, List retList) 
  {
      for (int i = 0; i < filteredList.size(); i++) {
          EObject connObj = (EObject) filteredList.get(i);
          EObject srcObj = _utils.getSource(connObj);
          EObject targetObj = _utils.getTarget(connObj);
          srcList.remove(srcObj);
          targetList.remove(targetObj);
      }
      if (!srcList.isEmpty()) {
          for (int i = 0; i < srcList.size(); i++) {
              EObject srcObj = (EObject) srcList.get(i);
              EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(22));
              EObject srcWithoutTarget = EcoreCloneUtils.cloneEObject(problemDataObj);
              EcoreUtils.setValue(srcWithoutTarget, ValidationConstants.PROBLEM_MESSAGE,
                      srcProblem);
              EStructuralFeature srcFeature = srcWithoutTarget.eClass().
                  getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
              srcWithoutTarget.eSet(srcFeature, srcObj);
              retList.add(srcWithoutTarget);
          }
      }
      if (!targetList.isEmpty()) {
          for (int i = 0; i < targetList.size(); i++) {
              EObject targetObj = (EObject) targetList.get(i);
              EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(23));
              EObject targetWithoutSrc = EcoreCloneUtils.cloneEObject(problemDataObj);
              EcoreUtils.setValue(targetWithoutSrc, ValidationConstants.PROBLEM_MESSAGE,
                      targetProblem);
              EStructuralFeature srcFeature = targetWithoutSrc.eClass().
                  getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
              targetWithoutSrc.eSet(srcFeature, targetObj);
              retList.add(targetWithoutSrc);
          }
      }
      
  }
  /**
   * Validates whether any of the SU types or Component types defined in 
   * component editor does not have association to SG/CSI
   * 
   * @param retList - List of problems encountered
   */
  public void validateNonAssociatedObjects(List retList)
  {
      List suList = getListForEReferenceType(ComponentEditorConstants.SERVICEUNIT_REF_NAME);
      List sgList = getListForEReferenceType(ComponentEditorConstants.SERVICEGROUP_REF_NAME);
      List compList = new ArrayList();
      compList.addAll(getListForEReferenceType(ComponentEditorConstants.SAFCOMPONENT_REF_NAME));
      /*compList
		.addAll(getListForEReferenceType(ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME));*/
	  List csiList = getListForEReferenceType(ComponentEditorConstants.COMPONENTSERVICEINSTANCE_REF_NAME);
	  	  
      List filteredList = _utils.getConnectionFrmType(
              ComponentEditorConstants.ASSOCIATION_NAME,
              ComponentEditorConstants.SERVICEGROUP_NAME,
              ComponentEditorConstants.SERVICEUNIT_NAME);
      processFilteredConnections(filteredList, sgList, suList, "ServiceGroup is not "
                      + "associated to any of the ServiceUnits",
                      "ServiceUnit is not associated to any of the ServiceGroups", retList);
      
      List filteredList1 = _utils.getConnectionFrmType(
              ComponentEditorConstants.ASSOCIATION_NAME,
              ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME,
              ComponentEditorConstants.SAFCOMPONENT_NAME);
      List list2  = _utils.getConnectionFrmType(
              ComponentEditorConstants.ASSOCIATION_NAME,
              ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME,
              ComponentEditorConstants.NONSAFCOMPONENT_NAME);
      filteredList1.addAll(list2);
      processFilteredConnections(filteredList1, csiList, compList, "ComponentServiceInstance is not "
              + "associated to any of the Components",
              "Component is not associated to any of the ComponentServiceInstances",
              retList);
  }
  /**
   * Returns List of object for EReference
   * @param name EReference name
   * @return List of eobjects
   */
  private List getListForEReferenceType(String name)
  {
	  List list = new ArrayList();
	  EObject rootObject = (EObject) _elist.get(0);
	  EReference ref = (EReference) rootObject.eClass().getEStructuralFeature(name);
	  list.addAll((List) rootObject.eGet(ref));
	  return list;
  }
  /**
  *
  * @param compList Editor Component List
  * @param retList Problems List
  */
 public void validateComponentTypes(List compList, List retList)
 {
     for (int i = 0; i < compList.size(); i++) {
       EObject compObj = (EObject) compList.get(i);
         String instCommand = (String) EcoreUtils.
             getValue(compObj, ComponentEditorConstants.INSTANTIATION_COMMAND);
         if (instCommand == null || instCommand.equals("")){
        	 EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(9));
        	 EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
             EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
                     "Component image name cannot be empty");
             EStructuralFeature srcFeature = problem.eClass().
                 getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
             problem.eSet(srcFeature, compObj);
             
             List relatedObjects = (List) EcoreUtils.getValue(
            		 problem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
             String newImageName = EcoreUtils.getNextValue(EcoreUtils.getName(
                     compObj) + "_image", compList,
            		 ComponentEditorConstants.INSTANTIATION_COMMAND);
             relatedObjects.add(newImageName);
             retList.add(problem);
         }
         validateComponentCapabilityModel(compObj, retList); 
		}
 }
    /**
     * 
     * @param map - HashMap 
     * @param val - Value to be searched
     * @return the Key corresponding to passed value
     */
    public static Object getKey(HashMap map, Object val)
    {
        Iterator iterator = map.keySet().iterator();
        while (iterator.hasNext()) {
            Object keyObj = iterator.next();
            Object value = map.get(keyObj);
            if (value.equals(val)) {
                return keyObj;
            }
        }
        return null;
    }
    
    /**
     * Validates whether the Components defined in 
     * component editor does not have duplicate EO names
     * 
     * @param retList - List of problems encountered
     */
    public void validateEONamesOfComponents(List retList)
    {   
    	List compList = getListForEReferenceType(ComponentEditorConstants.SAFCOMPONENT_REF_NAME); 
    	List eoList = new Vector();
        /*for (int i = 0; i < _elist.size(); i++) {
            EObject eobj = (EObject) _elist.get(i);
           if (eobj.eClass().getName().equals(
                    ComponentEditorConstants.SAFCOMPONENT_NAME)
                       ) {
                compList.add(eobj);
            } 
        }      */
        
        String[] eoName = new String[compList.size()];
        for (int i = 0; i < compList.size(); i++) {
            EObject eobj = (EObject) compList.get(i);
            
            EReference eoRef = (EReference) eobj.eClass().getEStructuralFeature(
            		ComponentEditorConstants.EO_PROPERTIES_NAME);
            EObject eoObj = (EObject) eobj.eGet(eoRef);
            if (eoObj != null) {
                eoList.add(eoObj);
            }
            
            eoName[i] =  (String) EcoreUtils.
            	getValue(eoObj, ComponentEditorConstants.EO_NAME);                       
            
        }

        HashSet uniqueEos = new HashSet();       

        for (int i = 0; i < eoName.length; i++) 
        {
            if (!uniqueEos.add(eoName[i])) 
            {
            	EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(24));
            	EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
                EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
                        "Component has duplicate EO name " + "'"
                        + eoName[i] + "'");
                EStructuralFeature srcFeature = problem.eClass().
                    getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
                EObject compObj = (EObject) compList.get(i);
                problem.eSet(srcFeature, compObj);
                List relatedObjects = (List) EcoreUtils.getValue(
                  		 problem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
                String uniqueEOName = EcoreUtils.getNextValue(EcoreUtils.getName(
                        compObj) + "_EO", eoList, ComponentEditorConstants.EO_NAME);
                relatedObjects.add(uniqueEOName);
                retList.add(problem);
            }
        }
    }   
    /**
     * Validates whether the single CSI type shared 
     * by a Proxy and Proxied which is not valid.
     * 
     * @param retList - List of problems encountered
     */
    public void validateComponentsCsiType(List retList)
    {   
    	List nonSAFcompList = getListForEReferenceType(ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME);        
        /*for (int i = 0; i < _elist.size(); i++) {
            EObject eobj = (EObject) _elist.get(i);
           if (eobj.eClass().getName().equals(
                    ComponentEditorConstants.NONSAFCOMPONENT_NAME)
                       ) {
               nonSAFcompList.add(eobj);
            } 
        }            */
        for (int i = 0; i < nonSAFcompList.size(); i++) {
            EObject nonSAFObj = (EObject) nonSAFcompList.get(i);
    		EObject csiTypesObj = ((EObject) EcoreUtils.getValue(nonSAFObj,
					ComponentEditorConstants.COMPONENT_CSI_TYPES));
            List csiTypeList = (List) EcoreUtils.getValue(csiTypesObj,
                    ComponentEditorConstants.COMPONENT_CSI_TYPE);
            String proxyCSIType =  (String) EcoreUtils.
                getValue(nonSAFObj, ComponentEditorConstants.COMPONENTPROXY_CSI_TYPE);                       
            if(csiTypeList.size() > 0 && proxyCSIType != "")
            {
                List<String> csiTypes = new ArrayList<String>();
                Iterator<EObject> csiTypeIterator = csiTypeList.iterator();
                while(csiTypeIterator.hasNext()) {
                	csiTypes.add(EcoreUtils.getName(csiTypeIterator.next()));
                }

                if(csiTypes.contains(proxyCSIType))
            	{
            		EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(25));
            		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
                    EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
                            "Single CSI type shared by a Proxy and"
                            + " Proxied is not valid");
                    EStructuralFeature srcFeature = problem.eClass().
                        getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
                    problem.eSet(srcFeature, nonSAFObj);
                    List relatedObjects = (List) EcoreUtils.getValue(
                     		 problem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
                
                    List connList = getConnectionObjects(nonSAFObj,
                    		ComponentEditorConstants.ASSOCIATION_NAME);
                    if (!connList.isEmpty()) {
                        EObject connObj = (EObject) connList.get(0);
                        relatedObjects.add(_utils.getSource(connObj));
                    	relatedObjects.add(connObj);
                    }
                    
            		retList.add(problem);
            	}
            }
        }
    }
    /**
     * 
     * @param targetObj - Target Object of connection
     * @param type - Connection type
     * @return the List of source objects of the connection with
     * connection of type 'type' and connection target is 'targetObj' 
     */
	private List getConnectionObjects(EObject targetObj, String type)
	{
		List sourceList = new Vector();
		for (int i = 0; i < _connectionList.size(); i++) {
			EObject connObj = (EObject) _connectionList.get(i);
	        EObject tObj = _utils.getTarget(connObj);
			String connType = (String) EcoreUtils.getValue(connObj,
					ComponentEditorConstants.CONNECTION_TYPE);
			
			if (connType.equals(type) && tObj.equals(targetObj)) {
				sourceList.add(connObj);
			}
		}
		return sourceList;
	}
	
	/**
	 * Validate whether correct number of standbyAssignments
	 * is assigned to all SI
	 * 
	 * @param retList- List of problems encountered 
	 */
	public void validateStandbyAssignments(List retList){
		//List sgToSiConnectionList = new ArrayList();
		
		for (int i = 0; i<_connectionList.size(); i++){
			EObject connObj = (EObject) _connectionList.get(i);
			EObject srcObj = _utils.getSource(connObj);
			EObject tarObj = _utils.getTarget(connObj);
			
			if(srcObj.eClass().getName().equals(ComponentEditorConstants.SERVICEGROUP_NAME) && 
					tarObj.eClass().getName().equals(ComponentEditorConstants.SERVICEINSTANCE_NAME)) {
				
				String redundancy = EcoreUtils.getValue(srcObj, ComponentEditorConstants.SG_REDUNDANCY_MODEL).toString();
				int numStandbyAssignments = ((Integer)(EcoreUtils.getValue(tarObj, "numStandbyAssignments"))).intValue();
				if(redundancy.equals(ComponentEditorConstants.TWO_N_REDUNDANCY_MODEL) && numStandbyAssignments != 1){
						setProblemForStandbyAssignments(retList, tarObj, ComponentEditorConstants.TWO_N_REDUNDANCY_MODEL);
				}else if(redundancy.equals(ComponentEditorConstants.NO_REDUNDANCY_MODEL) && numStandbyAssignments != 0){
					setProblemForStandbyAssignments(retList, tarObj, ComponentEditorConstants.NO_REDUNDANCY_MODEL);
				}else if(redundancy.equals(ComponentEditorConstants.M_PLUS_N_REDUNDANCY_MODEL) && numStandbyAssignments < 1){
					setProblemForStandbyAssignments(retList, tarObj, ComponentEditorConstants.M_PLUS_N_REDUNDANCY_MODEL);
				}
			}
		}
		
	}
	/**
	 * Creates the problem object for problem no. 55
	 * and adds it to the problem list
	 * 
	 * @param retList- List of problems encountered 
	 * @param tarObj- The Service Instance
	 */
	public void setProblemForStandbyAssignments(List retList, EObject tarObj, String redundancyModel){
		
		EObject problemDataObj = (EObject) _problemNumberObjMap.get(new Integer(55));
		EObject problem = EcoreCloneUtils.cloneEObject(problemDataObj);
        EcoreUtils.setValue(problem, ValidationConstants.PROBLEM_MESSAGE,
                "No. of standby assignments is incorrect for " + tarObj.eClass().getName() + "");
        EStructuralFeature srcFeature = problem.eClass().
            getEStructuralFeature(ValidationConstants.PROBLEM_SOURCE);
        problem.eSet(srcFeature, tarObj);
        List relatedObjects = (List) EcoreUtils.getValue(
          		 problem, ValidationConstants.PROBLEM_RELATED_OBJECTS);
        relatedObjects.add(redundancyModel);
        retList.add(problem);
	}
	
	private void validateComponentCapabilityModel(EObject compObj, List<EObject> retList) {
		String capabilityModel = EcoreUtils.getValue(compObj,
				ComponentEditorConstants.COMPONENT_CAPABILITY_MODEL).toString();
     int maxActiveCSIs = ((Integer) EcoreUtils.getValue(compObj,
				SafConstants.COMPONENT_MAX_ACTIVE_CSI)).intValue();
     int maxStandbyCSIs = ((Integer) EcoreUtils.getValue(compObj,
				SafConstants.COMPONENT_MAX_STANDBY_CSI)).intValue();
     if (capabilityModel
				.equals(ComponentEditorConstants.COMPONENT_CAP_ONE_ACTIVE_OR_ONE_STANDBY)) {
			if (maxActiveCSIs != 1 || maxStandbyCSIs != 1) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(92));
				EObject problem = EcoreCloneUtils
						.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Number of Maximum Active CSIs and Number of Maximum Standby CSIs value should be 1 when Capability Model is 'One Active or One Standby'");

				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, compObj);
				retList.add(problem);
			}
		} else if (capabilityModel
				.equals(ComponentEditorConstants.COMPONENT_CAP_X_ACTIVE_OR_Y_STANDBY)) {
			if (maxStandbyCSIs <= 0) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(93));
				EObject problem = EcoreCloneUtils
						.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Number of Maximum Standby CSIs value should be > 0 when Capability Model is 'X Active or Y Standby'");

				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, compObj);
				retList.add(problem);
			}
		} else if (capabilityModel
				.equals(ComponentEditorConstants.COMPONENT_CAP_ONE_ACTIVE_OR_X_STANDBY)) {
			if (maxActiveCSIs != 1) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(94));
				EObject problem = EcoreCloneUtils
						.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Number of Maximum Active CSIs value should be 1 when Capability Model is 'One Active or X Standby'");

				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, compObj);
				retList.add(problem);
			}
			if (maxStandbyCSIs <= 0) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(95));
				EObject problem = EcoreCloneUtils
						.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Number of Maximum Standby CSIs value should be > 0 when Capability Model is 'One Active or X Standby'");

				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, compObj);
				retList.add(problem);
			}
		} else if (capabilityModel
				.equals(ComponentEditorConstants.COMPONENT_CAP_X_ACTIVE)) {
			if (maxStandbyCSIs != 0) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(96));
				EObject problem = EcoreCloneUtils
						.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Number of Maximum Standby CSIs value should be 0 when Capability Model is 'X Active'");

				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, compObj);
				retList.add(problem);
			}
		} else if (capabilityModel
				.equals(ComponentEditorConstants.COMPONENT_CAP_ONE_ACTIVE)) {
			if (maxActiveCSIs != 1) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(97));
				EObject problem = EcoreCloneUtils
						.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Number of Maximum Active CSIs value should be 1 when Capability Model is 'One Active'");

				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, compObj);
				retList.add(problem);
			}
			if (maxStandbyCSIs != 0) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(98));
				EObject problem = EcoreCloneUtils
						.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Number of Maximum Standby CSIs value should be 0 when Capability Model is 'One Active'");

				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, compObj);
				retList.add(problem);
			}
		} else if (capabilityModel
				.equals(ComponentEditorConstants.COMPONENT_CAP_NON_PREINSTANTIABLE)) {
			if (maxActiveCSIs != 1) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(99));
				EObject problem = EcoreCloneUtils
						.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Number of Maximum Active CSIs value should be 1 when Capability Model is 'Non Preinstantiable'");

				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, compObj);
				retList.add(problem);
			}
			if (maxStandbyCSIs != 0) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(100));
				EObject problem = EcoreCloneUtils
						.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Number of Maximum Standby CSIs value should be 0 when Capability Model is 'Non Preinstantiable'");

				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, compObj);
				retList.add(problem);
			}
		} else if (capabilityModel
				.equals(ComponentEditorConstants.COMPONENT_CAP_X_ACTIVE_AND_Y_STANDBY)) {
			if (maxStandbyCSIs <= 0) {
				EObject problemDataObj = (EObject) _problemNumberObjMap
						.get(new Integer(101));
				EObject problem = EcoreCloneUtils
						.cloneEObject(problemDataObj);
				EcoreUtils
						.setValue(
								problem,
								ValidationConstants.PROBLEM_MESSAGE,
								"Number of Maximum Standby CSIs value should be > 0 when Capability Model is 'X Active and Y Standby'");

				EStructuralFeature srcFeature = problem.eClass()
						.getEStructuralFeature(
								ValidationConstants.PROBLEM_SOURCE);
				problem.eSet(srcFeature, compObj);
				retList.add(problem);
			}
		}
	}
}
