/*
 * @(#) $RCSfile: ClovisUtils.java,v $
 * $Revision: #4 $ $Date: 2007/01/03 $
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
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ClovisUtils.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/


/*
 * @(#) $RCSfile: ClovisUtils.java,v $
 * $Revision: #4 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.common.utils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
/**
 * @author nadeem
 *
 * A place holder for general utility functions.
 */
public class ClovisUtils
{
    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
    
    /**
     * Loads a class from given path. The format of the path
     * is [plugin-id:]full-class-name
     * @param classPath class path
     * @return Class object, null on error.
     */
    public static Class loadClass(String classPath)
    {
        int index = classPath.indexOf(':');
        String plugin     = null;
        String className  = classPath;
        if (index != -1) {
            plugin    = classPath.substring(0, index);
            className = classPath.substring(index + 1);
        }

        Class clazz   = null;
        try {
            clazz = (plugin != null)
                ?  Platform.getBundle(plugin).loadClass(className)
                 : Class.forName(className);
        } catch (ClassNotFoundException cnfException) {
        	// is this needed? Unnecessary errors show in error log
            //LOG.error("Class can not be loaded:" + classPath, cnfException);
        }
        return clazz;
    }
    /**
     * @param map - HashMap
     * @param val - The value corresponding to which keys have to be returned
     * @return the List of matched keys for value
     */
    public static List getKeysForValue(HashMap map, Object val)
    {
    	List keysList = new Vector();
    	Iterator iterator = map.keySet().iterator();
    	while (iterator.hasNext()) {
    		Object key = iterator.next();
    		Object value = map.get(key);
    		if (value.equals(val)) {
    			keysList.add(key);
    		}
    	}
    	return keysList;
	}

    /**
     * Goes thru all the Editor Objects to find out the Object with the
     * given key and returns the Object
     * @param key Key of the Object to be fetched
     * @param list Editor Object List
     * @return the Object corresponding to Name
     */
    public static EObject getObjectFrmName(List list, String name) {
        for (int i = 0; i < list.size(); i++) {
            EObject obj = (EObject) list.get(i);
            String objName = (String) EcoreUtils.getName(obj);
            if (objName != null && objName.equals(name)) {
                return obj;
            }
        }
        return null;
    }

    /**
     * Initializes the dependency framework requirements.
     * @param eIist EList
     */
    public static void initializeDependency(EList eList) {
    	Iterator itr = eList.iterator();
    	while(itr.hasNext()) {
    		Object obj = itr.next();
    		if(obj instanceof EObject) {
    			initializeDependency((EObject)obj);
    		}
    	}
    }
    
    /**
     * Initializes the dependency framework requirements.
     * @param eObject EObject
     */
    public static void initializeDependency(EObject eObject) {
    	EList attrList = eObject.eClass().getEAllAttributes();
    	Iterator itr = attrList.iterator();
    	while(itr.hasNext()) {
    		EStructuralFeature feature = (EStructuralFeature) itr.next();
    		createDisableList(eObject, feature);
    	}
    	
    	EList refList = eObject.eClass().getEAllReferences();
    	itr = refList.iterator();
    	while(itr.hasNext()) {
    		EStructuralFeature feature = (EStructuralFeature) itr.next();
    		Object obj = eObject.eGet(feature);
    		if(obj instanceof EObject) {
    			initializeDependency((EObject)obj);
    		} else if(obj instanceof EList) {
    			initializeDependency((EList) obj);
    		}
    	}
    }

    /**
     * Creates the Disable attribute list for the give Object if
     * applicable.
     * @param eObject EObject
     * @param feature EStructuralFeature
     */
    private static void createDisableList(EObject eObject, EStructuralFeature feature) {
        String str = EcoreUtils.getAnnotationVal(feature, null, "Dependency");
        if(str != null) {
        	StringTokenizer entryTokenizer = new StringTokenizer(str, "|");

        	while(entryTokenizer.hasMoreTokens()) {
        		String entry = entryTokenizer.nextToken();
        		StringTokenizer detailTokenizer = new StringTokenizer(entry, ";=");
        		String entryValue = detailTokenizer.nextToken();

                Object val = eObject.eGet(feature);
        		if((val.toString()).equalsIgnoreCase(entryValue)) {
        			while(detailTokenizer.hasMoreTokens()) {
        				String featureName = detailTokenizer.nextToken();
        				String value = detailTokenizer.nextToken();

        				if(featureName.contains(":")) {
        					int index = featureName.indexOf(":");
        					String disableType = featureName.substring(index+1, featureName.length());
        					featureName = featureName.substring(0, index);
        					EList list = (EList)EcoreUtils.getValue(eObject, featureName);

        					if(disableType.equals("add")) {
        						list.add(value);
        					} else if(disableType.equals("remove")) {
        						list.remove(value);
        					}
        				}
        			}
        		}
        	}
        }
	}
    
    /**
     * Adds objects in to the appropriate list contained in top level object
     * 
     * @param eObjects - EObjects to be added in to editor model
     * @param topObj - Top level object of the model
     */
    public static void addObjectsToModel(List eObjects, EObject topObj)
    {
        for (int i = 0; i < eObjects.size(); i++) {
            EObject eobj = (EObject) eObjects.get(i);
            List refList = topObj.eClass().getEAllReferences();
            for (int j = 0; j < refList.size(); j++) {
                EReference ref = (EReference) refList.get(j);
                if (eobj.eClass().getName().equals(ref.
                        getEReferenceType().getName())) {
                    Object val = topObj.eGet(ref);
                    if (ref.getUpperBound() == 1) {
                        topObj.eSet(ref, eobj);
                    } else if (ref.getUpperBound() == -1
                            || ref.getUpperBound() > 1) {
                        ((List) val).add(eobj);
                    }
                
                }
            }
        }
    }
    
    /**
     * removes objects from appropriate list contained in top level object
     * 
     * @param eObjects - EObjects to be removed from editor model
     * @param topObj - Top level object of the model
     */
    public static void removeObjectFrmModel(List eObjects, EObject topObj)
    {
        
        for (int i = 0; i < eObjects.size(); i++) {
            EObject eobj = (EObject) eObjects.get(i);
            List refList = topObj.eClass().getEAllReferences();
            for (int j = 0; j < refList.size(); j++) {
                EReference ref = (EReference) refList.get(j);
                if (eobj.eClass().getName().equals(ref.
                        getEReferenceType().getName())) {
                    Object val = topObj.eGet(ref);
                    if (val != null) {
                        if (val instanceof List) {
                            ((List) val).remove(eobj);
                        } else if (val instanceof EObject) {
                            topObj.eUnset(ref);
                        }
                    }
                }
            }
        }
    }
    public static List getModelObjects(EObject topObj)
    {
    	List modelObjList = new Vector();
    	List refList = topObj.eClass().getEAllReferences();
		for (int i = 0; i < refList.size(); i++) {
			List list = (List) topObj.eGet((EReference) refList
					.get(i));
			modelObjList.addAll(list);
		}
    	return modelObjList;
    }
    /**
     * Sets the rdn of the template objects
     * @param list - List of objects on which rdn field has to be set.
     */
    public static void setKey(List list)
    {
        for (int i = 0; i < list.size(); i++) {
            EObject eobj = (EObject) list.get(i);
            String key = (String) EcoreUtils.getValue(eobj,
            		ModelConstants.RDN_FEATURE_NAME);
            if (key != null && key.equals("")) {
            	int code = eobj.hashCode();
                EcoreUtils.setValue(eobj, ModelConstants.RDN_FEATURE_NAME,
                		String.valueOf(code));
            }
            List references = eobj.eClass().getEAllReferences();
            for (int j = 0; j < references.size(); j++) {
                EReference ref = (EReference) references.get(j);
                if (ref.getUpperBound() == 1) {
                    EObject refObj = (EObject) eobj.eGet(ref);
                    if (refObj != null) {
                        key = (String) EcoreUtils.getValue(refObj,
                        		ModelConstants.RDN_FEATURE_NAME);
                        if (key != null && key.equals("")) {
                        	int code = eobj.hashCode();
                            EcoreUtils.setValue(refObj, ModelConstants.
                            		RDN_FEATURE_NAME, String.valueOf(code));
                        } 
                    }
                } else if (ref.getUpperBound() > 1
                        || ref.getUpperBound() == -1) {
                    List refObjList = (List) eobj.eGet(ref);
                    setKey(refObjList);
                }
                
            }
        }
        
    }

	/**
	 * Returns the object with given feature name and feature value combination
	 * from the give list.
	 * 
	 * @param eObjList
	 * @param featureName
	 * @param featureVal
	 * @return
	 */
	public static EObject getEobjectWithFeatureVal(List<EObject> eObjList,
			String featureName, String featureVal) {
		EObject eObj;

		for (int i = 0; i < eObjList.size(); i++) {
			eObj = eObjList.get(i);

			if (EcoreUtils.getValue(eObj, featureName).toString().equals(
					featureVal)) {
				return eObj;
			}
		}

		return null;
	}

	/**
	 * Returns the value list for the given feature.
	 * 
	 * @param eObjList
	 * @param featureName
	 * @return
	 */
	public static List getFeatureValueList(List<EObject> eObjList,
			String featureName) {
		List featureValues = new ArrayList();

		for (int i = 0; i < eObjList.size(); i++) {
			featureValues
					.add(EcoreUtils.getValue(eObjList.get(i), featureName));
		}

		return featureValues;
	}

	/**
	 * Returns the value-object map for the given feature.
	 * 
	 * @param eObjList
	 * @param featureName
	 * @return
	 */
	public static HashMap<Object, EObject> getFeatureValueObjMap(
			List<EObject> eObjList, String featureName) {

		HashMap<Object, EObject> valueObjMap = new HashMap<Object, EObject>();
		EObject eObj;

		for (int i = 0; i < eObjList.size(); i++) {
			eObj = eObjList.get(i);
			valueObjMap.put(EcoreUtils.getValue(eObj, featureName), eObj);
		}

		return valueObjMap;
	}
}