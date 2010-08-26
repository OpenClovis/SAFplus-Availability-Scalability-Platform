/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/ComponentUtils.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GEUtils;

/**
 * @author pushparaj
 * 
 * Implementation of GEUtils for Component Editor.
 */
public class ComponentUtils extends GEUtils {
	/**
	 * @param objs
	 *            Array of EObjects
	 */
	public ComponentUtils(Object[] objs) {
		super(objs);
	}
	/**
	 * @see com.clovis.cw.genericeditor.GEUtils#getTarget(
	 *      org.eclipse.emf.ecore.EObject)
	 */
	public EObject getTarget(EObject key) {
		EObject rootObject = (EObject) _eObjects[0];
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
		EObject rootObject = (EObject) _eObjects[0];
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
}
