/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ecore/EcoreCloneUtils.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ecore;

import java.util.Iterator;
import java.util.Map;
import java.util.Vector;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.common.notify.NotifyingList;

import com.clovis.common.utils.log.Log;
import com.clovis.common.utils.UtilsPlugin;
/**
 * @author nadeem
 *
 * This class provide static utility methods for copying and
 * cloning EObject and EList.
 */
public class EcoreCloneUtils
{
    private static final Log LOG = Log.getLog(UtilsPlugin.getDefault());
    /**
     * @param list EList to be cloned
     * @return cloned EList
     */
    public static ClovisNotifyingListImpl cloneList(NotifyingList list)
    {
        return cloneList(list, null, null);
    }
    /**
     * @param list EList to be cloned
     * @param clonedToOriginalMap Map of clonedObject to original Object
     * @param originalToClonedMap Map of original Object to cloned object
     * @return cloned EList
     */
    public static ClovisNotifyingListImpl cloneList(NotifyingList list,
            Map clonedToOriginalMap, Map originalToClonedMap)
    {
        ClovisNotifyingListImpl clonedList = new ClovisNotifyingListImpl();
        try {
            if (list != null) {
                for (int i = 0; i < list.size(); i++) {
                    Object element = list.get(i);
                    if (element instanceof EList) {
                        LOG.warn("List inside List not supported.");
                    } else if (element instanceof EObject) {
                        EObject eObj = (EObject) element;
                        EObject clonedObj = cloneEObject(eObj,
                                clonedToOriginalMap, originalToClonedMap);
                        clonedList.add(clonedObj);
                    } else {
                        clonedList.add(element);
                    }
                }
            }
            // maps will be null if cloneEList is used as a standalone method
            if (clonedToOriginalMap != null && originalToClonedMap != null) {
                clonedToOriginalMap.put(clonedList, list);
                originalToClonedMap.put(list, clonedList);
            }
        } catch (Exception ex) {
            LOG.error("EcoreUtils: cloneList(): Error in cloning EList.", ex);
        }
        return clonedList;
    }
    /**
     * @param obj EObject to be cloned
     * @return cloned EObject
     */
    public static EObject cloneEObject(EObject obj)
    {
        return cloneEObject(obj, null, null);
    }
    /**
     * @param obj EObject to be cloned
     * @param clonedToOriginalMap Map of clonedObject to original Object
     * @param originalToClonedMap Map of original Object to cloned object
     * Else passed as "False" if view model's EObject is being cloned.
     * @return cloned EObject
     */
    public static EObject cloneEObject(EObject obj,
            Map clonedToOriginalMap, Map originalToClonedMap)
    {
        EObject clonedObj = EcoreUtils.createEObject(obj.eClass(), true);
        try {
            EList structFeatures  = obj.eClass().getEAllStructuralFeatures();
            for (int i = 0; i < structFeatures.size(); i++) {
                EStructuralFeature feature =
                    (EStructuralFeature) structFeatures.get(i);
                Object value = obj.eGet(feature);
                if (value instanceof NotifyingList) {
                    clonedObj.eSet(feature, cloneList((NotifyingList) value,
                                clonedToOriginalMap, originalToClonedMap));
                } else if (value instanceof EEnumLiteral) {
                    clonedObj.eSet(feature, value);
                } else if (value instanceof EObject) {
                    clonedObj.eSet(feature, cloneEObject((EObject) value,
                                clonedToOriginalMap, originalToClonedMap));
                } else {
                    clonedObj.eSet(feature, value);
                }
            }
            // maps will be null if cloneEList is used as a standalone method
            if (clonedToOriginalMap != null && originalToClonedMap != null) {
                clonedToOriginalMap.put(clonedObj, obj);
                originalToClonedMap.put(obj, clonedObj);
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        }
        return clonedObj;
    }
    /**
     * Commits the source Elist onto the target EList.
     * @param sourceList EList from which EObjects need to be copied.
     * @param targetList EList into which EObjects need to be copied.
     * @param viewModelToModelMap Map containing clonedObject to original
     * @param modelToViewModelMap Map containing original to clonedObject
     */
    public static void copyEList(EList sourceList, EList targetList,
            Map viewModelToModelMap, Map modelToViewModelMap)
    {
    	/*************** This code is required only in few conditions ***********/
    	if(targetList.isEmpty()) {
    		targetList.clear();
    	}
    	// Some times first object is not able to add in to List 
    	/************************************************************************/
    	/* Note : This method copy EList which contains either EObject or
         * java.lang.String. while copying its clones only EObjects. For others its
         * simply copies.
         */
        try {
            Vector objsToBeRetained = new Vector();
            //First copy from src to target and add new Objects in
            //target if not already present.
            for (int i = 0; i < sourceList.size(); i++) {
                Object obj = sourceList.get(i);
                if (obj instanceof EObject) {
                    EObject sourceObj = (EObject) sourceList.get(i);
                    EObject targetObj = (EObject) viewModelToModelMap
                            .get(sourceObj);
                    if (targetObj == null) {
                        //sourceObj is a newly added EObject in ViewModel.
                        //This needs to be cloned and added into Model
                        EObject clonedObj = cloneEObject(sourceObj,
                                modelToViewModelMap, viewModelToModelMap);
                        targetList.add(clonedObj);
                        objsToBeRetained.addElement(clonedObj);
                    } else {
                        copyEObject(sourceObj, targetObj,
                                viewModelToModelMap, modelToViewModelMap);
                        objsToBeRetained.addElement(targetObj);
                    }
                } else if (obj instanceof String) {
                	
                	Object targetObj = viewModelToModelMap.get(obj);
					if (targetObj == null) {
						// obj is a newly added Object in ViewModel.
						// This needs to be added into Model.
						targetList.add(obj);
					}
					objsToBeRetained.addElement(obj);
				} else {
					objsToBeRetained.addAll(sourceList);
                    targetList.clear();
                    targetList.addAll(sourceList);
                    break;
				}
            }
            // Now clear those elements from target whose clone is
            // not present in source. These are the
            // EObjects that have been removed in View Model.
            Iterator itr = targetList.iterator();
            while (itr.hasNext()) {
                Object obj = itr.next();
                if (!objsToBeRetained.contains(obj)) {
                    itr.remove();
                    viewModelToModelMap.remove(modelToViewModelMap.get(obj));
                    modelToViewModelMap.remove(obj);
                }
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }
    /**
     * Copies source EObject into the target EObject.
     * @param source source EObject
     * @param target target EObject
     * @param viewModelToModelMap Map containing clonedObject to original
     * @param modelToViewModelMap Map containing original to clonedObject
     */
    public static void copyEObject(EObject source, EObject target,
            Map viewModelToModelMap, Map modelToViewModelMap)
    {
        try {
            EList structFeatures = source.eClass().getEAllStructuralFeatures();
            for (int i = 0; i < structFeatures.size(); i++) {
                EStructuralFeature feature = (EStructuralFeature) structFeatures
                        .get(i);
                Object value = source.eGet(feature);
                if (value instanceof EList) {
                    EList sourceList = (EList) value;
                    EList targetList = (EList) target.eGet(feature);
                    copyEList(sourceList, targetList, viewModelToModelMap,
                            modelToViewModelMap);
                } else if (value instanceof EEnumLiteral) {
                    target.eSet(feature, value);
                } else if (value instanceof EObject) {
                    EObject srcObj = (EObject) value;
                    EObject targetObj = (EObject) target.eGet(feature);
                    if (targetObj == null) {
                        if (feature instanceof EReference) {
                            EClass refClass = ((EReference) feature)
                                    .getEReferenceType();
                            targetObj = EcoreUtils
                                    .createEObject(refClass, true);
                            target.eSet(feature, targetObj);
                        }
                    }
                    copyEObject(srcObj, targetObj, viewModelToModelMap,
                            modelToViewModelMap);
                } else {
                    target.eSet(feature, value);
                }
            }
        } catch (Exception ex) {
            LOG.error("Error in copyEObject", ex);
        }
    }
}
