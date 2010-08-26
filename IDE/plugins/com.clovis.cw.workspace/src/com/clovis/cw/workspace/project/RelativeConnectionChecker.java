/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/RelativeConnectionChecker.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project;
import java.util.HashSet;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GEDataUtils;


/**
 * @author shubhada
 * 
 * RelativeConnectionChecker class to check the validity of the relative
 * connection with respect to the connection to be validated.
 */
public class RelativeConnectionChecker
{
private List _connectionList = null;
private GEDataUtils _utils = null;;

    /**
     * Constructor
     */
    public RelativeConnectionChecker(List connList, GEDataUtils utils)
    {
        _connectionList = connList;
        _utils = utils;
    }
    /**
     * Checks the permutation of connections b/w the matched sources and targets
     * based on the exists and sourceNegate and targetNegate  
     * 
     * @param matchedSourceList List of nodes matched for source of relative connection
     * @param matchedTargetList List of nodes matched for target of relative connection
     * @param relatedConnObj - relative connetion mentioned in xmi
     * @return the Error message or null on no error
     */
    public String checkRelativeConnection(HashSet matchedSourceList,
            HashSet matchedTargetList, EObject relatedConnObj,
            boolean sourceNegate, boolean targetNegate)
    {
        String relatedConnName = (String) EcoreUtils.
            getValue(relatedConnObj, "connectionName");
        boolean isConnExists = ((Boolean) EcoreUtils.
                getValue(relatedConnObj, "exists")).booleanValue();
        String message = (String) EcoreUtils.
            getValue(relatedConnObj, "message");
        String relatedConnSrc = (String) EcoreUtils.
            getValue(relatedConnObj, "source");
        String relatedConnTarget = (String) EcoreUtils.
            getValue(relatedConnObj, "target");
        String sMultiplicity = ((EEnumLiteral) EcoreUtils.
                getValue(relatedConnObj, "sourceMultiplicity")).getName();
        String tMultiplicity = ((EEnumLiteral) EcoreUtils.
                getValue(relatedConnObj, "targetMultiplicity")).getName();
        
        List filteredConnList = getConnectionFrmType(relatedConnName,
                relatedConnSrc, relatedConnTarget);
        HashSet srcList = new HashSet();
        HashSet targetList = new HashSet();
        for (int i = 0; i < filteredConnList.size(); i++) {
            EObject connObj = (EObject) filteredConnList.get(i);
            EObject sObj = _utils.getSource(connObj);
            EObject tObj = _utils.getTarget(connObj);
            srcList.add(sObj);
            targetList.add(tObj);
            
        }
        // check for existence of connection b/w matched sources and targets 
        if (isConnExists) {
            if (!sourceNegate && !targetNegate) {
                boolean connMatch = checkConnMultiplicity(matchedSourceList,
                        matchedTargetList, sMultiplicity, tMultiplicity, filteredConnList);
                if (connMatch == true) {
                    return message;
                }
                
            } else if (!sourceNegate && targetNegate) {
                HashSet negateTargetList = new HashSet();
                negateTargetList.addAll(targetList);
                negateTargetList.removeAll(matchedTargetList);
                boolean connMatch = checkConnMultiplicity(matchedSourceList,
                        negateTargetList, "1", "1", filteredConnList);
                if (connMatch == true) {
                    return message;
                }
            } else if (sourceNegate && !targetNegate) {
                HashSet negateSrcList = new HashSet();
                negateSrcList.addAll(srcList);
                negateSrcList.removeAll(matchedSourceList);
                boolean connMatch = checkConnMultiplicity(negateSrcList,
                        matchedTargetList, "1", "1", filteredConnList);
                if (connMatch == true) {
                    return message;
                }
            } else if (sourceNegate && targetNegate) {
                HashSet negateSrcList = new HashSet();
                HashSet negateTargetList = new HashSet();
                negateSrcList.addAll(srcList);
                negateSrcList.removeAll(matchedSourceList);
                negateTargetList.addAll(targetList);
                negateTargetList.removeAll(matchedTargetList);
                boolean connMatch = checkConnMultiplicity(negateSrcList,
                        negateTargetList, "1", "1", filteredConnList);
                if (connMatch == true) {
                    return message;
                }
            } 
            
        } else {
            boolean exists = false;
            if (!sourceNegate && !targetNegate) {
                exists = checkExistenceOfRelativeConnection(matchedSourceList,
                        matchedTargetList, filteredConnList);
                
            } else if (!sourceNegate && targetNegate) {
                HashSet negateTargetList = new HashSet();
                negateTargetList.addAll(targetList);
                negateTargetList.removeAll(matchedTargetList);
                exists = checkExistenceOfRelativeConnection(matchedSourceList,
                        negateTargetList, filteredConnList);
                
            } else if (sourceNegate && !targetNegate) {
                HashSet negateSrcList = new HashSet();
                negateSrcList.addAll(srcList);
                negateSrcList.removeAll(matchedSourceList);
                exists = checkExistenceOfRelativeConnection(negateSrcList,
                        matchedTargetList, filteredConnList);
                
            } else if (sourceNegate && targetNegate) {
                HashSet negateSrcList = new HashSet();
                HashSet negateTargetList = new HashSet();
                negateSrcList.addAll(srcList);
                negateSrcList.removeAll(matchedSourceList);
                negateTargetList.addAll(targetList);
                negateTargetList.removeAll(matchedTargetList);
                exists = checkExistenceOfRelativeConnection(negateSrcList,
                        negateTargetList, filteredConnList);
                
            }
            if (!exists) {
                return message;
            }
        }
        return null;
    }
    /**
     * checks the existence of connection
     * 
     * @param srcList - List of source objects 
     * @param targetList - List of target objects
     * @param connList - Connection List
     * @return the Error Message or null on no error
     */
    private boolean checkExistenceOfRelativeConnection(HashSet srcList,
            HashSet targetList, List connList)
    {
        for (int i = 0; i < connList.size(); i++) {
            EObject connObj = (EObject) connList.get(i);
            EObject sObj = _utils.getSource(connObj);
            EObject tObj = _utils.getTarget(connObj);
            if (srcList.contains(sObj)
                    && targetList.contains(tObj)) {
                return true;
                
            }
        }
        return false;
    }
    /**
     * 
     * Returns the matched connection instances. 
     * 
     * @param connType Connection Type
     * @param sourceType Source Object Class Name
     * @param targetType Target Object Class Name
     * @return the list of filtered connections
     */
    private List getConnectionFrmType(String connType, String sourceType,
            String targetType)
    {
        List filteredConnList = new Vector();
        for (int i = 0; i < _connectionList.size(); i++) {
            EObject connObj = (EObject) _connectionList.get(i);
            // return all the connections of type connType
            if (sourceType == null || targetType == null) {
                if (connType.equals((String) EcoreUtils.getValue(connObj,
        				ComponentEditorConstants.CONNECTION_TYPE))) {
                    filteredConnList.add(connObj);
                }
            } else { // return all the connections of type connType with source
              //class name as sourceType and target class name as targetType
                EObject sourceObj = (EObject) _utils.getSource(connObj);
                EObject targetObj = (EObject) _utils.getTarget(connObj);
                if (connType.equals((String) EcoreUtils.getValue(connObj,
        				ComponentEditorConstants.CONNECTION_TYPE))
                        && sourceType.equals(sourceObj.eClass().getName())
                        && targetType.equals(targetObj.eClass().getName())) {
                    filteredConnList.add(connObj);
                }
            }
        }
        return filteredConnList;
    }
    /**
     * Checks for connection multplicity
     * 
     * @param matchedSourceList
     * @param matchedTargetList
     * @param sMul
     * @param tMul
     * @param connList
     * @return true if connection multiplicity matches with the invalid scenario specified in xmi
     */
    private boolean checkConnMultiplicity(HashSet matchedSourceList, HashSet matchedTargetList, String sMul, String tMul, List connList)
    {
        if (sMul == "1" && tMul == "1") {
            boolean exists = false;
            exists = checkExistenceOfRelativeConnection(matchedSourceList,
                    matchedTargetList, connList);
            if (exists) {
                return true;
            }
            // The following 4 cases are left for future implementation
        } else if (sMul == "1" && tMul == "n") {
            
        } else if (sMul == "n" && tMul == "1") {
            
        } else if (sMul == "1...*" && tMul == "n") {
            
        } else if (sMul == "n...*" && tMul == "1") {
            
        } else if (sMul == "n" && tMul == "n") {
            List srcList = new Vector();
            List targetList = new Vector();
            srcList.addAll(matchedSourceList);
            targetList.addAll(matchedTargetList);
            for (int i = 0; i < connList.size(); i++) {
                EObject connObj = (EObject) connList.get(i);
                EObject sObj = _utils.getSource(connObj);
                EObject tObj = _utils.getTarget(connObj);
                if (matchedSourceList.contains(sObj)
                        && matchedTargetList.contains(tObj)) {
                     srcList.remove(sObj);
                     targetList.remove(tObj);
                }
            }
            if (!matchedSourceList.isEmpty() && !matchedTargetList.isEmpty()
                    && srcList.isEmpty() && targetList.isEmpty()) {
                return true;
            }
        }
            
        return false;
    }
}
