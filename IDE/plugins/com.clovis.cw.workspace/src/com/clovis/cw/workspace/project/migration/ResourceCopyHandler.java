/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/migration/ResourceCopyHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project.migration;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.cw.data.MigrationConstants;
import com.clovis.cw.workspace.project.ProblemEcoreReader;
import com.clovis.cw.workspace.project.ValidationConstants;

/**
 * 
 * @author shubhada
 *
 * ResourceCopyHandler class which will take
 * care of simple and complex copy of objects
 * using the Migration Info
 */
public class ResourceCopyHandler implements MigrationConstants
{
    private EObject _changeInfoObj = null;
    private HashMap _nameEclassObjMap = new HashMap();
    private HashMap _oldNameFeatureMap = new HashMap();
    private HashMap _newNameFeatureMap = new HashMap();
    private List _newEClassList = new Vector();
    /**
     * Constructor
     * @param changeInfoObj - Migration info object
     */
    public ResourceCopyHandler(EObject changeInfoObj)
    {
        _changeInfoObj = changeInfoObj;
        
        EObject ePackObj = (EObject) EcoreUtils.getValue(_changeInfoObj,
				MIGRATION_PACKAGE_NAME);
        populateMap(ePackObj);
        
    }
    /**
     * populates the map using migration info
     * @param eClassHolder  - Top level eClass
     */
    private void populateMap(EObject eClassHolder)
    {
		List eClassList = (List) EcoreUtils.getValue(eClassHolder,
				MIGRATION_ECLASS_NAME);
		if (eClassList == null || eClassList.isEmpty()) {
			return;
		}
		for (int i = 0; i < eClassList.size(); i++) {
			EObject eClassObj = (EObject) eClassList.get(i);
			String oldEclassName = (String) EcoreUtils.getValue(eClassObj,
					MIGRATION_OLD_FIELDNAME);
            String newEclassName = (String) EcoreUtils.getValue(eClassObj,
                    MIGRATION_NEW_FIELDNAME);
            if (!oldEclassName.equals("NULL")) {
                _nameEclassObjMap.put(oldEclassName, eClassObj);
            } else {
                _newEClassList.add(eClassObj);
            }
            List eFeatureList = (List) EcoreUtils.getValue(eClassObj,
                    MIGRATION_FEATURE_NAME);
            for (int j = 0; j < eFeatureList.size(); j++) {
                EObject featureObj = (EObject) eFeatureList.get(j);
                if (!oldEclassName.equals("NULL")) {
                    String oldFeatureName = (String) EcoreUtils.getValue(featureObj,
                            MIGRATION_OLD_FIELDNAME);
                    String oldQualifiedFeatureName = oldEclassName + ":" + oldFeatureName;
                    _oldNameFeatureMap.put(oldQualifiedFeatureName, featureObj);
                }
                String newFeatureName = (String) EcoreUtils.getValue(featureObj,
                        MIGRATION_NEW_FIELDNAME);
                String newQualifiedFeatureName = newEclassName + ":" + newFeatureName;
                _newNameFeatureMap.put(newQualifiedFeatureName, featureObj);
            }
        
			populateMap(eClassObj);
			
		}
	}
    /**
     * Copies all the data (simple and complex) from top level EObjects in old resource
     * to new resource. and also copies recursively all the levels down
     * @param destSrcMap - Map with the source and destination
     *      objects to copied
     * @param retList - List of problems encountered during migration
     */
	public void copyResourceInfo(HashMap destSrcMap, List retList)
    {
        HashMap destSrcObjMap = new HashMap();
        destSrcObjMap.putAll(destSrcMap);
        //search for matching objects and then copy the values
        //from oldResourceObject to newResourceObject
        Iterator iterator = destSrcMap.keySet().iterator();
        while (iterator.hasNext()) {
            EObject destObj = (EObject) iterator.next();
            EObject srcObj = (EObject) destSrcMap.get(destObj);
            createReferencedFeatures(destObj, destSrcObjMap);
            if (srcObj != null) {
                copy (srcObj, destObj, retList);
                
                //copy the referenced/complex features
                copyReferencedFeatures(srcObj, destObj, destSrcObjMap, retList);
                
            }     
        }
        copyChangedValues(destSrcObjMap, retList);
    }
    /**
     * This method copies the values for different destination features
     * from the source object which is searched based on the "sourceCopyPath" 
     * attribute specified for the feature
     * 
     * @param destSrcObjMap - Map which has objects which are mapping objects
     *      of source resource and destination resource
     * @param retList - List of problems encountered during migration
     */
    private void copyChangedValues(HashMap destSrcObjMap, List retList)
    {
        Iterator iterator = destSrcObjMap.keySet().iterator();
        while (iterator.hasNext()) {
            
            EObject destObj = (EObject) iterator.next();
            EObject srcObj = (EObject) destSrcObjMap.get(destObj);
            
            List destFeatures = destObj.eClass().getEAllStructuralFeatures();
            for (int i = 0; i < destFeatures.size(); i++) {
            
                EStructuralFeature destFeature = (EStructuralFeature) 
                    destFeatures.get(i);
                String qualifiedFeature = destObj.eClass().getName() + ":"
                    + destFeature.getName();
                EObject featureObj = (EObject) _newNameFeatureMap.
                    get(qualifiedFeature);
                
                if (featureObj != null) {
                    String destPath = (String) EcoreUtils.getValue(featureObj,
                            MigrationConstants.DESTINATION_FEATURE_PATH);
                    String copyPath = (String) EcoreUtils.getValue(featureObj,
                            MigrationConstants.SOURCE_FEATURE_COPY_PATH);
                    if (!destPath.equals("")) {
                        EObject destRefObj = getObjectFromPath(destObj, destPath, retList);
                        srcObj = (EObject) destSrcObjMap.get(destRefObj);
                    }
                    if (srcObj != null) {
                        if (!copyPath.equals("")) {
                            //parse the copyPath string
                            StringTokenizer tokenizer = new StringTokenizer(copyPath, ":");
                            String objPath = tokenizer.nextToken();
                            String featureName = tokenizer.nextToken();
                    
                            Object val = null;
                            if (!objPath.equals("NONE")) {
                                EObject eObj = getObjectFromPath(srcObj, objPath, retList);
                                if (eObj != null) {
                                    val = EcoreUtils.getValue(eObj, featureName);
                                }
                                
                            } else {
                                val = EcoreUtils.getValue(srcObj, featureName);
                            }
                            
                            if (val != null) { 
                                if ((val instanceof EObject || val instanceof List)
                                    && !(val instanceof EEnumLiteral)) {
                                    EReference destRef = (EReference) destFeature;
                                    copyValue(destRef, destObj, val, retList);
                                } else {
                                    EcoreUtils.setValue(destObj, destFeature.getName(),
                                        val.toString());
                                }
                            }
                        }
                    }
                }
                    
            }
        
        }
        
    }
    /**
     * Method which copies the complex data (either EObject or List) 
     * to the destination object.
     * @param destRef - Destination Reference
     * @param destObj - Destination Object
     * @param srcVal - Source Value to be set on destination reference
     *      object
     * @param retList - List of problems encountered during migration
     */
    private void copyValue(EReference destRef, EObject destObj,
            Object srcVal, List retList)
    {
        Object dObj = destObj.eGet(destRef); 
        if (destRef.getUpperBound() == 1) {
            if (srcVal instanceof EObject) {
                
                EObject destRefObj = EcoreUtils.createEObject(destRef.
                    getEReferenceType(), true);
                destObj.eSet(destRef, destRefObj);
                copy((EObject) srcVal, destRefObj, retList);
                
            } else if (srcVal instanceof List) {
                ////Report the problem as we don't know which object 
                //to choose from list to copy to destination object
                EObject problem = EcoreUtils.createEObject(
                        ProblemEcoreReader.getInstance().
                        getProblemClass(), true);
                EcoreUtils.setValue(problem, "message",
                        "For the feature '" + destRef.getName()
                        + "', Object is encountered at the destination. So cannot copy the list of values from source");
                EcoreUtils.setValue(problem, "level",
                        ValidationConstants.ERROR);
                EStructuralFeature srcFeature = problem.eClass().
                    getEStructuralFeature("source");
                problem.eSet(srcFeature, dObj);
                EcoreUtils.setValue(problem, "category",
                         ValidationConstants.CAT_MIGRATION);
                EcoreUtils.setValue(problem,
                        "problemNumber", String.valueOf(2));
                retList.add(problem);
            }
            
        } else if (destRef.getUpperBound() > 1
                || destRef.getUpperBound() == -1) {
            
            if (srcVal instanceof EObject) {
                
                EObject destRefObj = EcoreUtils.createEObject(destRef.
                        getEReferenceType(), true);
                ((List) dObj).add(destRefObj);
                
                copy((EObject) srcVal, destRefObj, retList);
            
            } else if (srcVal instanceof List) {
                
                List srcObjList = (List) srcVal;
                for (int j = 0; j < srcObjList.size(); j++) {
                    EObject srcRefObj = (EObject) srcObjList.get(j);
                    EObject destRefObj = EcoreUtils.createEObject(destRef.
                            getEReferenceType(), true);
                    ((List) dObj).add(destRefObj);
                    
                    copy(srcRefObj, destRefObj, retList);
                }
            }
            
            
        }
        
    }
    /**
     * 
     * @param srcObj - Source Object 
     * @param objPath - Object Path to reach the appropriate object
     * @param retList - List of problems encountered during migration
     * @return the encoutered object after trversing thru given path
     */
    private EObject getObjectFromPath(EObject srcObj, String objPath,
            List retList)
    {
        EObject eobj = srcObj;
        StringTokenizer tokenizer = new StringTokenizer(objPath, "/");
        while (tokenizer.hasMoreTokens()) {
            if (eobj == null) {
                return null;
            }
            String path = tokenizer.nextToken();
            if (path.equals("..")) {
                eobj = eobj.eContainer();
            } else {
                Object refObj = EcoreUtils.getValue(eobj, path);
                if (refObj instanceof EObject) {
                    eobj = (EObject) refObj;
                } else if (refObj instanceof List) {
                    //Report the problem as we don't know which object 
                    //to choose from list to reach the destination object
                    EObject problem = EcoreUtils.createEObject(
                            ProblemEcoreReader.getInstance().
                            getProblemClass(), true);
                    EcoreUtils.setValue(problem, "message",
                            "List is encountered, when navigating to source object in copy method");
                    EcoreUtils.setValue(problem, "level",
                            ValidationConstants.ERROR);
                    EStructuralFeature srcFeature = problem.eClass().
                        getEStructuralFeature("source");
                    problem.eSet(srcFeature, srcObj);
                    EcoreUtils.setValue(problem, "category",
                             ValidationConstants.CAT_MIGRATION);
                    EcoreUtils.setValue(problem,
                            "problemNumber", String.valueOf(2));
                    retList.add(problem);
                    return null;
                }
            }
        }
        return eobj;
    }
    /**
     * 
     * @param destObj - Object on which referenced objects have to be set
     * @param destSrcObjMap - Map with destination and source objects 
     */
    private void createReferencedFeatures(EObject destObj, HashMap destSrcObjMap)
    {
       List destRefList = destObj.eClass().getEAllReferences();
       
       for (int i = 0; i < destRefList.size(); i++) {
           EReference destRef = (EReference) destRefList.get(i);
       
           if (!destObj.eIsSet(destRef)) {
               if (destRef.getUpperBound() == 1
            		   && destRef.getLowerBound() == 1) {
            	   EObject eobj = EcoreUtils.createEObject(destRef.getEReferenceType(), true);
                   destSrcObjMap.put(eobj, null);
                   destObj.eSet(destRef, eobj);
                   createReferencedFeatures(eobj, destSrcObjMap);
               } 
               
           } else {
               Object obj = destObj.eGet(destRef);
               if (obj instanceof EObject) {
                   destSrcObjMap.put(obj, null);
                   createReferencedFeatures((EObject) obj, destSrcObjMap);
               } else if (obj instanceof List) {
                   List eobjList = (List) obj;
                   for (int j = 0; j < eobjList.size(); j++) {
                       EObject refObj = (EObject) eobjList.get(j);
                       destSrcObjMap.put(refObj, null);
                       createReferencedFeatures(refObj, destSrcObjMap);
                   }
               }
           }
           
       }
        
    }
    /**
     * does copy of simple features
     * @param srcObj - Source object
     * @param destObj - Destination object
     * @param retList - List of problems encountered during migration
     */
    public void copy(EObject srcObj, EObject destObj, List retList)
    {
        //copy simple features
        List features = srcObj.eClass().getEAllAttributes();
        Properties map = new Properties();
        for (int i = 0; i < features.size(); i++) {
            
            EAttribute feature = (EAttribute)
                features.get(i);
            String qualifiedFeature = srcObj.eClass().getName() + ":"
                + feature.getName(); 
            EObject featureObj = (EObject) _oldNameFeatureMap.
                get(qualifiedFeature);
            if (featureObj != null) {
                String copyPath = (String) EcoreUtils.getValue(featureObj,
                        SOURCE_FEATURE_COPY_PATH);
                if (copyPath != null && !copyPath.equals("")) {
                    // later write code to copy the features
                    continue;
                }
                String newFeatureName = (String) EcoreUtils.getValue(featureObj,
                        MIGRATION_NEW_FIELDNAME);
                map.put(feature.getName(), newFeatureName);
            
            } else {
                map.put(feature.getName(), feature.getName());
            }
        }
        List problems = EcoreUtils.copyValues(srcObj, destObj, map);
        for (int i = 0; i < problems.size(); i++) {
            String msg = (String) problems.get(i);
            EObject problem = EcoreUtils.createEObject(
                    ProblemEcoreReader.getInstance().
                    getProblemClass(), true);
            EcoreUtils.setValue(problem, "message", msg);
            EcoreUtils.setValue(problem, "level",
                    ValidationConstants.WARNING);
            EStructuralFeature srcFeature = problem.eClass().
                getEStructuralFeature("source");
            problem.eSet(srcFeature, destObj);
            EcoreUtils.setValue(problem, "category",
                     ValidationConstants.CAT_MIGRATION);
            EcoreUtils.setValue(problem,
                    "problemNumber", String.valueOf(3));
            retList.add(problem);
        }
        // Check for any invalid data being moved to destination object
        String message = ObjectValidator.checkPatternAndBlankValue(destObj);
        if (message != null) {
            EObject problem = EcoreUtils.createEObject(
                    ProblemEcoreReader.getInstance().
                    getProblemClass(), true);
            EcoreUtils.setValue(problem, "message", message);
            EcoreUtils.setValue(problem, "level",
                    ValidationConstants.ERROR);
            EStructuralFeature srcFeature = problem.eClass().
                getEStructuralFeature("source");
            problem.eSet(srcFeature, destObj);
            EcoreUtils.setValue(problem, "category",
                     ValidationConstants.CAT_MIGRATION);
            EcoreUtils.setValue(problem,
                    "problemNumber", String.valueOf(3));
            retList.add(problem);
        }
        
    }
    
    /**
     * Copies complex data from top level EObjects in old resource
     * to new resource. and also copies recursively all the levels down
     * @param srcObj - Source Object
     * @param destObj - Destination object
     * @param destSrcMap - Map with destination and source objects
     * @param retList - List of problems encountered during migration
     */
    private void copyReferencedFeatures(EObject srcObj, EObject destObj,
            HashMap destSrcMap, List retList)
    {
        List references = srcObj.eClass().getEAllReferences();
        for (int i = 0; i < references.size(); i++) {
            EReference srcRef = (EReference) references.get(i);
            String qualifiedFeature = srcObj.eClass().getName() + ":"
                + srcRef.getName(); 
            String newRefName = "";
            EObject featureObj = (EObject) _oldNameFeatureMap.
                get(qualifiedFeature);
            if (featureObj != null) {
                newRefName = (String) EcoreUtils.getValue(featureObj,
                        MIGRATION_NEW_FIELDNAME);
            } else {
                newRefName = srcRef.getName();
            }
            
            EReference destRef = (EReference) destObj.eClass().
                getEStructuralFeature(newRefName);
            Object sObj = srcObj.eGet(srcRef);
            //The possible case where in new Feature name specified in the 
            // migration info is "NULL" and it might have shifted to lower/upper
            //levels in the reference heirarchy
            if (destRef != null) {
                Object dObj = destObj.eGet(destRef);
                // The case wherein destRef is not set on the destination EObject
                if (!destObj.eIsSet(destRef)) {
                    if (destRef.getUpperBound() == 1) {
                        EObject destRefObj = EcoreUtils.createEObject(destRef.
                                getEReferenceType(), true);
                        destObj.eSet(destRef, destRefObj);
                        EObject srcRefObj = null;
                        
                        if (sObj instanceof EObject) {
                            srcRefObj = (EObject) sObj;
                            destSrcMap.put(destRefObj, srcRefObj);
                            copy(srcRefObj, destRefObj, retList);
                            copyReferencedFeatures(srcRefObj, destRefObj,
                                    destSrcMap, retList);
                        } else if (sObj instanceof List) {
                            // to handle the case where a List is converted to a single object
                            //Report the error
                            EObject problem = EcoreUtils.createEObject(
                                    ProblemEcoreReader.getInstance().
                                    getProblemClass(), true);
                            EcoreUtils.setValue(problem, "message",
                                    "For the feature '" + destRef.getName()
                                    + "', Object is encountered at the destination. So cannot copy the list of values from source");
                            EcoreUtils.setValue(problem, "level",
                                    ValidationConstants.ERROR);
                            EStructuralFeature srcFeature = problem.eClass().
                                getEStructuralFeature("source");
                            problem.eSet(srcFeature, destObj);
                            EcoreUtils.setValue(problem, "category",
                                     ValidationConstants.CAT_MIGRATION);
                            EcoreUtils.setValue(problem,
                                    "problemNumber", String.valueOf(1));
                            retList.add(problem);
                            
                        }
                        
                    } else if (destRef.getUpperBound() > 1
                            || destRef.getUpperBound() == -1) {
                        
                        if (sObj instanceof EObject) {
                            EObject destRefObj = EcoreUtils.createEObject(destRef.
                                    getEReferenceType(), true);
                            ((List) dObj).add(destRefObj);
                            destSrcMap.put(destRefObj, sObj);
                            copy((EObject) sObj, destRefObj, retList);
                            copyReferencedFeatures((EObject) sObj, destRefObj,
                                    destSrcMap, retList);
                        } else if (sObj instanceof List) {
                            List srcObjList = (List) sObj;
                            for (int j = 0; j < srcObjList.size(); j++) {
                                EObject srcRefObj = (EObject) srcObjList.get(j);
                                EObject destRefObj = EcoreUtils.createEObject(destRef.
                                        getEReferenceType(), true);
                                ((List) dObj).add(destRefObj);
                                destSrcMap.put(destRefObj, srcRefObj);
                                copy(srcRefObj, destRefObj, retList);
                                copyReferencedFeatures(srcRefObj, destRefObj,
                                        destSrcMap, retList);
                            }
                        }
                        
                    }
                } else {
                    // This is the case wherein destFeature is already set on the EObject
                    if (dObj instanceof EObject) {
                        EObject srcRefObj = null;
                        
                        if (sObj instanceof EObject) {
                            srcRefObj = (EObject) sObj;
                            destSrcMap.put(dObj, srcRefObj);
                            copy(srcRefObj, (EObject) dObj, retList);
                            copyReferencedFeatures(srcRefObj, (EObject) dObj,
                                    destSrcMap, retList);
                        } else if (sObj instanceof List) {
                            // to handle the case where a List is converted to a single object
                            // Report the error
                            EObject problem = EcoreUtils.createEObject(
                                    ProblemEcoreReader.getInstance().
                                    getProblemClass(), true);
                            EcoreUtils.setValue(problem, "message",
                                    "For the feature '" + destRef.getName()
                                    + "', Object is encountered at the destination. So cannot copy the list of values from source");
                            EcoreUtils.setValue(problem, "level",
                                    ValidationConstants.ERROR);
                            EStructuralFeature srcFeature = problem.eClass().
                                getEStructuralFeature("source");
                            problem.eSet(srcFeature, dObj);
                            EcoreUtils.setValue(problem, "category",
                                     ValidationConstants.CAT_MIGRATION);
                            EcoreUtils.setValue(problem,
                                    "problemNumber", String.valueOf(1));
                            retList.add(problem);
                        }
                        
                        
                    } else if (dObj instanceof List) {
                        
                        if (sObj instanceof EObject) {
                            EObject destRefObj = (EObject) ((List) dObj).get(0);
                            destSrcMap.put(destRefObj, sObj);
                            copy((EObject) sObj, destRefObj, retList);
                            copyReferencedFeatures((EObject) sObj, destRefObj,
                                    destSrcMap, retList);
                        } else if (sObj instanceof List) {
                            List srcObjList = (List) sObj;
                            List destObjList = (List) dObj;
                            destObjList.clear();
                            for (int j = 0; j < srcObjList.size(); j++) {
                                EObject srcRefObj = (EObject) srcObjList.get(j);
                                EObject destRefObj = EcoreUtils.createEObject(destRef.
                                        getEReferenceType(), true);
                                destObjList.add(destRefObj);
                                destSrcMap.put(destRefObj, srcRefObj);
                                copy(srcRefObj, destRefObj, retList);
                                copyReferencedFeatures(srcRefObj, destRefObj,
                                        destSrcMap, retList);
                            }
                        }
                       
                    }
                }
            }
        }
        
    }

}
