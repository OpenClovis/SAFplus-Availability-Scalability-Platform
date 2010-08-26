/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/CAUtils.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.genericeditor.GEUtils;
/**
 * @author ashish
 *
 * Implementation of GEUtils for Class Association
 * Editor.
 */
public class CAUtils extends GEUtils
{
	/**
     * Constructor. Provides list of object.
     * @param list Objects list
     */
    public CAUtils(Object[] list)
    {
        super(list);
    }
    /**
     * @see com.clovis.cw.genericeditor.GEUtils#getParent(org.eclipse.emf.ecore.EObject)
     */
    public EObject getParent(EObject key) {
		if (key.eClass().getName().equals(
				ClassEditorConstants.MIB_RESOURCE_NAME)) {
			String mibName = (String) EcoreUtils.getValue(key,
					ClassEditorConstants.MIB_NAME_FEATURE);
			EObject rootObject = (EObject) _eObjects[0];
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
		EObject rootObject = (EObject) _eObjects[0];
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
					ModelConstants.RDN_FEATURE_NAME)))) ? true : false;
	}

	/**
	 * @see com.clovis.cw.genericeditor.GEUtils#getSource(
	 *      org.eclipse.emf.ecore.EObject)
	 */
	public EObject getSource(EObject key) {
		EObject rootObject = (EObject) _eObjects[0];
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
}
