/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/ResourceDataUtils.java $
 * $Author: srajyaguru $
 * $Date: 2007/05/18 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GEDataUtils;
import com.clovis.cw.project.data.SubModelMapReader;

/**
 * 
 * @author shubhada
 *
 * Implementation of GEUtils for Resource Editor.
 */
public class ResourceDataUtils extends GEDataUtils
{
    /**
     * 
     * @param objs - List of resource editor objects
     */
    public ResourceDataUtils(List objs)
    {
        super(objs);
    }
    /**
     * @see com.clovis.cw.genericeditor.GEDataUtils#getParent(org.eclipse.emf.ecore.EObject)
     */
    public EObject getParent(EObject key) {
		if (key.eClass().getName().equals(
				ClassEditorConstants.MIB_RESOURCE_NAME)) {
			String mibName = (String) EcoreUtils.getValue(key,
					ClassEditorConstants.MIB_NAME_FEATURE);
			EObject rootObject = (EObject) _eObjects.get(0);
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(ClassEditorConstants.MIB_REF_NAME);
			EList list = (EList) rootObject.eGet(ref);
			for (int i = 0; i < list.size(); i++) {
				EObject obj = (EObject) list.get(i);
				if (mibName.equals(EcoreUtils.getName(obj))) {
					return obj;
				}

			}
		} else {

		}
		return null;
	}
    /**
	 * @see com.clovis.cw.genericeditor.GEUtils#getTarget(
	 *      org.eclipse.emf.ecore.EObject)
	 */
	public EObject getTarget(EObject key) {
		EObject rootObject = (EObject) _eObjects.get(0);
		String refList[] = ClassEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				if (hasKey((String) key.eGet(key.eClass()
						.getEStructuralFeature(ClassEditorConstants.CONNECTION_END)), eobj)) {
					return eobj;
				}
			}
		}
		return null;
	}

	/**
	 * Checks whether this EObject has the same key This method is used to found
	 * source and target objects for connections.
	 * 
	 * @param key
	 *            key for Connection
	 * @param obj
	 *            EObject
	 * @return boolean
	 */
	private boolean hasKey(String key, EObject obj) {
		return (key.equals(obj
				.eGet(obj.eClass().getEStructuralFeature(
						ModelConstants.RDN_FEATURE_NAME)))) ? true
				: false;
	}

	/**
	 * @see com.clovis.cw.genericeditor.GEUtils#getSource(
	 *      org.eclipse.emf.ecore.EObject)
	 */
	public EObject getSource(EObject key) {
		EObject rootObject = (EObject) _eObjects.get(0);
		String refList[] = ClassEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				if (hasKey((String) key.eGet(key.eClass()
						.getEStructuralFeature(ClassEditorConstants.CONNECTION_START)), eobj)) {
					return eobj;
				}
			}
		}
		return null;
	}
    /**
     * 
     * @param obj - EObject to be checked
     * @return true if obj is a node not edge
     */
    public static boolean isNode(EObject obj)
    {
        if (obj.eClass().getName().equals(
                ClassEditorConstants.COMPOSITION_NAME)
            || obj.eClass().getName().equals(
                ClassEditorConstants.ASSOCIATION_NAME)
            || obj.eClass().getName().equals(
                ClassEditorConstants.INHERITENCE_NAME)) {
            return false;
        }
        return true;
    }
    /**
     * Goes thru all the Editor Objects to find out the Object with the
     * given name and returns the Object
     * @param name Name of the Object to be fetched
     * @return the Object corresponding to Name
     */
    public EObject getObjectFrmName(String name)
    {
    	EObject rootObject = (EObject) _eObjects.get(0);
		String refList[] = ClassEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				String objName = EcoreUtils.getName(eobj);
	            if (objName != null && objName.equals(name)) {
	                return eobj;
	            }
			}
		}
        return null;
    }
    /**
     * Goes thru all the Editor Objects to find out the Object with the
     * given name and returns the Object
     * @param name Name of the Object to be fetched
     * @return the Object corresponding to Name
     */
    public static EObject getObjectFrmName(List resObjects, String name)
    {
    	EObject rootObject = (EObject) resObjects.get(0);
		String refList[] = ClassEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				String objName = EcoreUtils.getName(eobj);
	            if (objName != null && objName.equals(name)) {
	                return eobj;
	            }
			}
		}
        return null;
    }

    /**
     * Returns EList with all resources(S/W and H/W Resources)
     * @param rootObject
     * @return
     */
    public static List getResourcesList(EObject rootObject) {
        List resourcesList = new ClovisNotifyingListImpl();
        resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.HARDWARE_RESOURCE_REF_NAME)));
		resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME)));
		resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME)));
		resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.SYSTEM_CONTROLLER_REF_NAME)));
		resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.MIB_RESOURCE_REF_NAME)));
        return resourcesList;
    }

    /**
     * Returns EList with all resources(S/W and H/W Resources)
     * @param list main list
     * @return EList
     */
    public static List getResourcesList(List list)
    {
        return getResourcesList((EObject) list.get(0));
    }
    /**
     * Returns MO's Model List
     *
     * @return MO's List
     */
    public static List getMoList(List list)
    {
        Vector resourcesList = new Vector();
        EObject rootObject = (EObject) list.get(0);
        resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.HARDWARE_RESOURCE_REF_NAME)));
		resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME)));
		resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME)));
		resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME)));
		resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.SYSTEM_CONTROLLER_REF_NAME)));
		resourcesList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.MIB_RESOURCE_REF_NAME)));
        return resourcesList;
    }
    /**
     * 
     * @param project - IProject 
     * @param resObj - resource object 
     * @return - associated alarms of the resource
     */
    public static List getAssociatedAlarms(IProject project, EObject resObj)
    {
    	SubModelMapReader reader = SubModelMapReader.getSubModelMappingReader(
    			project, "resource", "alarm");
    	return (List) reader.getLinkTargetObjects(
        		ClassEditorConstants.ASSOCIATED_ALARM_LINK,
        		EcoreUtils.getName(resObj));
    }

	/**
	 * Checks wether the given resource has transient attribute.
	 * 
	 * @param resObj
	 * @return
	 */
	public static boolean hasTransientAttr(EObject resObj) {

		List attrList = (List) EcoreUtils.getValue(resObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);
		Iterator<EObject> attrItr = attrList.iterator();
		while (attrItr.hasNext()) {
			EObject obj = attrItr.next();
			if (EcoreUtils.getValue(obj,
					ClassEditorConstants.ATTRIBUTE_ATTRIBUTETYPE).toString()
					.equals("RUNTIME")) {
				return true;
			}
		}

		EObject provObj = (EObject) EcoreUtils.getValue(resObj,
				ClassEditorConstants.RESOURCE_PROVISIONING);
		List provAttrList = (List) EcoreUtils.getValue(provObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);
		Iterator<EObject> provAttrItr = provAttrList.iterator();
		while (provAttrItr.hasNext()) {
			EObject obj = provAttrItr.next();
			if (EcoreUtils.getValue(obj,
					ClassEditorConstants.ATTRIBUTE_ATTRIBUTETYPE).toString()
					.equals("RUNTIME")) {
				return true;
			}
		}
		return false;
	}

	/**
	 * Checks wether the given resource has intialized attribute.
	 * 
	 * @param resObj
	 * @return
	 */
	public static boolean hasInitializedAttr(EObject resObj) {

		List attrList = (List) EcoreUtils.getValue(resObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);
		Iterator<EObject> attrItr = attrList.iterator();
		while (attrItr.hasNext()) {
			EObject obj = attrItr.next();
			if (((Boolean) EcoreUtils.getValue(obj,
					ClassEditorConstants.ATTRIBUTE_INITIALIZED)).booleanValue()) {
				return true;
			}
		}

		EObject provObj = (EObject) EcoreUtils.getValue(resObj,
				ClassEditorConstants.RESOURCE_PROVISIONING);
		List provAttrList = (List) EcoreUtils.getValue(provObj,
				ClassEditorConstants.CLASS_ATTRIBUTES);
		Iterator<EObject> provAttrItr = provAttrList.iterator();
		while (provAttrItr.hasNext()) {
			EObject obj = provAttrItr.next();
			if (((Boolean) EcoreUtils.getValue(obj,
					ClassEditorConstants.ATTRIBUTE_INITIALIZED)).booleanValue()) {
				return true;
			}
		}
		return false;
	}

	/**
	 * Checks whether this resource has valid hierarchy or not.
	 * 
	 * @param eObj
	 * @param parentList
	 * @return
	 */
	public boolean isResourceHierarchyValid(EObject eObj, List<EObject> parentList) {
		EObject rootObject = (EObject) _eObjects.get(0);
		String refList[] = ComponentEditorConstants.EDGES_REF_TYPES;

		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);

			for (int j = 0; j < list.size(); j++) {
				EObject nodeObj = (EObject) list.get(j);
				EObject srcObj = getSource(nodeObj);
				EObject targetObj = getTarget(nodeObj);
				if (targetObj.equals(eObj)) {
					if(eObj.eClass().getName().equals(ClassEditorConstants.CHASSIS_RESOURCE_NAME)) {
						return true;
					} else if(!parentList.contains(eObj)) {
						parentList.add(srcObj);
					}
				}
			}
		}
		return false;
	}

	/**
	 * Finds all the parents for the given resource object.
	 * 
	 * @param eObj
	 * @param parentList
	 */
	public void findAllParents(EObject eObj, List<EObject> parentList) {
		findAllParents(eObj, parentList, ClassEditorConstants.EDGES_REF_TYPES);
	}

	/**
	 * Finds all the parents for the given resource object for the given edges.
	 * 
	 * @param eObj
	 * @param parentList
	 * @param edgeRefTypes
	 */
	public void findAllParents(EObject eObj, List<EObject> parentList, String[] edgeRefTypes) {
		EObject rootObject = (EObject) _eObjects.get(0);

		for (int i = 0; i < edgeRefTypes.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(edgeRefTypes[i]);
			if(ref == null) return;
			EList list = (EList) rootObject.eGet(ref);

			for (int j = 0; j < list.size(); j++) {
				EObject nodeObj = (EObject) list.get(j);
				EObject srcObj = getSource(nodeObj);
				EObject targetObj = getTarget(nodeObj);
				if (targetObj.equals(eObj)) {
					parentList.add(srcObj);
					findAllParents(srcObj, parentList, edgeRefTypes);
				}
			}
		}
	}

	/**
	 * Finds all the parents for the given resource object for the given edges.
	 * 
	 * @param eObj
	 * @param parentList
	 * @param edgeRefTypes
	 * @param parentMap
	 */
	public void findAllParents(EObject eObj, List<EObject> parentList,
			String[] edgeRefTypes, HashMap<EObject, List<EObject>> parentMap) {

		List<EObject> mapList = parentMap.get(eObj);
		if (mapList != null) {
			parentList.addAll(mapList);
			return;
		}

		EObject rootObject = (EObject) _eObjects.get(0);

		for (int i = 0; i < edgeRefTypes.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(edgeRefTypes[i]);
			if(ref == null) return;
			EList list = (EList) rootObject.eGet(ref);

			for (int j = 0; j < list.size(); j++) {
				EObject nodeObj = (EObject) list.get(j);
				EObject srcObj = getSource(nodeObj);
				EObject targetObj = getTarget(nodeObj);
				if (targetObj.equals(eObj)) {
					parentList.add(srcObj);
					findAllParents(srcObj, parentList, edgeRefTypes, parentMap);
				}
			}
		}

		parentMap.put(eObj, parentList);
	}

	/**
	 * Finds the children of the given resource object for the next level.
	 * 
	 * @param eObj
	 * @return
	 */
	public List<EObject> getChildren(EObject eObj) {
		List<EObject> children = new ArrayList<EObject>();
		EObject rootObject = (EObject) _eObjects.get(0);
		String refList[] = ClassEditorConstants.EDGES_REF_TYPES;

		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);

			if(ref == null) continue;
			EList list = (EList) rootObject.eGet(ref);

			for (int j = 0; j < list.size(); j++) {
				EObject nodeObj = (EObject) list.get(j);
				EObject srcObj = getSource(nodeObj);
				EObject targetObj = getTarget(nodeObj);

				if (srcObj.equals(eObj)
						&& (!targetObj.eClass().getName().equals(
								ClassEditorConstants.MIB_CLASS_NAME))) {
					children.add(targetObj);
				}
			}
		}
		return children;
	}
}
