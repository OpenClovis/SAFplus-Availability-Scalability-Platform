/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/ComponentDataUtils.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.ArrayList;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GEDataUtils;
import com.clovis.cw.project.data.SubModelMapReader;
/**
 * 
 * @author shubhada
 *
 * Implementation of GEUtils for Component Editor.
 */
public class ComponentDataUtils extends GEDataUtils
{

    public ComponentDataUtils(List objs)
    {
        super(objs);
    }
    /**
	 * @see com.clovis.cw.genericeditor.GEUtils#getTarget(
	 *      org.eclipse.emf.ecore.EObject)
	 */
	public EObject getTarget(EObject key) {
		EObject rootObject = (EObject) _eObjects.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				if (hasKey((String) key.eGet(key.eClass()
						.getEStructuralFeature(ComponentEditorConstants.CONNECTION_END)), eobj)) {
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
					ModelConstants.RDN_FEATURE_NAME)))) ? true : false;
	}

	/**
	 * @see com.clovis.cw.genericeditor.GEUtils#getSource(
	 *      org.eclipse.emf.ecore.EObject)
	 */
	public EObject getSource(EObject key) {
		EObject rootObject = (EObject) _eObjects.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				if (hasKey((String) key.eGet(key.eClass()
						.getEStructuralFeature(ComponentEditorConstants.CONNECTION_START)), eobj)) {
					return eobj;
				}
			}
		}                          
		return null;
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
    public List getConnectionFrmType(String connType, String sourceType,
			String targetType) {
		List filteredConnList = new Vector();
		EObject rootObject = (EObject) _eObjects.get(0);
		String refList[] = ComponentEditorConstants.EDGES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass().getEStructuralFeature(refList[i]);
			String refName = ref.getName();
			if (refName.equals(ComponentEditorConstants.AUTO_REF_NAME)) {
				EList list = (EList) rootObject.eGet(ref);;
				for (int j = 0; j < list.size();j++) {
					EObject connObj = (EObject) list.get(j);
					// return all the connections of type connType
					if (connObj.eClass().getName().equals(
							ComponentEditorConstants.AUTO_NAME)) {
						String type = (String) EcoreUtils.getValue(connObj,
								ComponentEditorConstants.CONNECTION_TYPE);
						if (connType.equals(type)) {
							if (sourceType == null || targetType == null) {
								filteredConnList.add(connObj);

							} else { // return all the connections of type
										// connType
								// with source
								// class name as sourceType and target class
								// name as
								// targetType
								EObject sourceObj = getSource(connObj);
								EObject targetObj = getTarget(connObj);
								if (sourceType.equals(sourceObj.eClass()
										.getName())
										&& targetType.equals(targetObj.eClass()
												.getName())) {
									filteredConnList.add(connObj);
								}
							}
						}
					}
				}
			}
		}
		return filteredConnList;
	}
    /**
	 * 
	 * @param obj -
	 *            EObject to be checked
	 * @return true if obj is a node not edge
	 */
    public  static boolean isNode(EObject obj)
    {
        if (obj.eClass().getName().equals(
                ComponentEditorConstants.AUTO_NAME)) {
            return false;
        }
        return true;
    }
    /**
	 * Goes thru all the Editor Objects to find out the Object with the given
	 * name and returns the Object
	 * 
	 * @param name
	 *            Name of the Object to be fetched
	 * @return the Object corresponding to Name
	 */
    public EObject getObjectFrmName(String name)
    {
    	EObject rootObject = (EObject) _eObjects.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
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
    *
    * @param resourceObj EObject
    * @return the Children of the resourceObj
    */
   public List getChildren(EObject resourceObj) {
		List children = new Vector();
		EObject rootObject = (EObject) _eObjects.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject nodeObj = (EObject) list.get(j);
				List parentList = getParents(nodeObj);
				for (int k = 0; k < parentList.size(); k++) {
					EObject parentObj = (EObject) parentList.get(k);
					if (parentObj != null && parentObj.equals(resourceObj)) {
						children.add(nodeObj);
					}
				}
			}
		}
		return children;
	}
   /**
	 * 
	 * @param resourceObj
	 *            EObject
	 * @return the Parent EObject of resourceObj
	 */
	public List getParents(EObject resourceObj) {
		List parentList = new Vector();
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
				if (targetObj.equals(resourceObj)) {
					parentList.add(srcObj);
				}
			}
		}
		return parentList;
	}
	/**
     * Returns Nodes Model List
     *
     * @return Nodes List
     */
    public static List getNodesList(List editorList)
    {
        List list = new ClovisNotifyingListImpl();
        EObject rootObject = (EObject) editorList.get(0);
        list.addAll((EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature("node")));
        return list;
    }
    /**
     * 
     * @param project - IProject 
     * @param compObj - component object 
     * @return - associated resources of the component
     */
    public static List getAssociatedResources(IProject project, EObject compObj)
    {
    	SubModelMapReader reader = SubModelMapReader.getSubModelMappingReader(
    			project, "component", "resource");
    	return (List) reader.getLinkTargetObjects(
        		ComponentEditorConstants.ASSOCIATE_RESOURCES_NAME,
        		EcoreUtils.getName(compObj));
    }

    /**
	 * Returns Component editor entities.
	 * 
	 * @return
	 */
	public List<EObject> getEntityList() {
		return getEntityList((EObject) _eObjects.get(0));
	}

	/**
	 * Returns Component editor entities for the given editor root object.
	 * 
	 * @param rootObject
	 *            editor root object
	 * @return
	 */
	@SuppressWarnings("unchecked")
	public static List<EObject> getEntityList(EObject rootObject) {
		List<EObject> entityList = new ArrayList<EObject>();
		EClass rootClass = rootObject.eClass();

		entityList
				.addAll((List<EObject>) rootObject
						.eGet(rootClass
								.getEStructuralFeature(ComponentEditorConstants.NODE_REF_NAME)));
		entityList
				.addAll((List<EObject>) rootObject
						.eGet(rootClass
								.getEStructuralFeature(ComponentEditorConstants.SERVICEUNIT_REF_NAME)));
		entityList
				.addAll((List<EObject>) rootObject
						.eGet(rootClass
								.getEStructuralFeature(ComponentEditorConstants.SAFCOMPONENT_REF_NAME)));
		entityList
				.addAll((List<EObject>) rootObject
						.eGet(rootClass
								.getEStructuralFeature(ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME)));
		entityList
				.addAll((List<EObject>) rootObject
						.eGet(rootClass
								.getEStructuralFeature(ComponentEditorConstants.SERVICEGROUP_REF_NAME)));
		entityList
				.addAll((List<EObject>) rootObject
						.eGet(rootClass
								.getEStructuralFeature(ComponentEditorConstants.SERVICEINSTANCE_REF_NAME)));
		entityList
				.addAll((List<EObject>) rootObject
						.eGet(rootClass
								.getEStructuralFeature(ComponentEditorConstants.COMPONENTSERVICEINSTANCE_REF_NAME)));

		return entityList;
	}
}
