/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/editor/EditorUtils.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.common.utils.editor;

import java.math.BigInteger;
import java.util.HashSet;
import java.util.List;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

import com.clovis.common.utils.ecore.EcoreUtils;


/**
 * This class provide many utilities to manipulate
 * Eobjects which are used in Editors.
 * This class is used for both online and offline.
 * @author pushparaj
 */
public class EditorUtils {

	/**
	 * Gets next value for the feature. The next value is always unique
	 * @param prefix Prefix String before unique number
     * @param list   List of EObjects
     * @param featureName feature name
     * @return Unique value generated
     */
	public static String getNextValue(String prefix, List editorList,
			String featureName) {
		HashSet set = new HashSet();
		String newVal = null;

		EObject rootObject = (EObject) editorList.get(0);
		List refList = rootObject.eClass().getEAllReferences();
		for (int i = 0; i < refList.size(); i++) {
			EReference ref = (EReference) refList.get(i);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject eobj = (EObject) list.get(j);
				Object val = EcoreUtils.getValue(eobj, featureName);
				if (val instanceof Integer || val instanceof Short
						|| val instanceof Long || val instanceof BigInteger) {
					set.add(String.valueOf(val));
				} else if (val != null) {
					set.add((String) val);
				}
			}
		}
		int i = 0;
		do {
			newVal = prefix + String.valueOf(i);
			i++;
		} while (set.contains(newVal));
		return newVal;
	}
	/**
	 * 
	 * @param editorList Editor Objects List
	 * @param name Name
	 * @return EObject
	 */
	public static EObject getObjectFromName(List editorList, String name) {
		EObject rootObject = (EObject) editorList.get(0);
		List refList = rootObject.eClass().getEAllReferences();
		for (int i = 0; i < refList.size(); i++) {
			EReference ref = (EReference) refList.get(i);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject obj = (EObject) list.get(j);
				String objName = EcoreUtils.getName(obj);
				if (objName != null && objName.equals(name)) {
					return obj;
				}
			}
		}
		return null;
	}
}
