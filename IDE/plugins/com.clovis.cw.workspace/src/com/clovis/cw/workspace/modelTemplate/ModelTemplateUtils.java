/**
 * 
 */
package com.clovis.cw.workspace.modelTemplate;

import java.io.File;
import java.net.URL;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.genericeditor.model.EdgeModel;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Class which provides Utility methods for the model template.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ModelTemplateUtils implements ModelTemplateConstants {

	/**
	 * Obtains the child hierarchy upto the level specified.
	 * 
	 * @param nodeModel
	 * @param edgeList
	 * @param nodeList
	 * @param level
	 */
	@SuppressWarnings("unchecked")
	public static void getChildHierarchy(NodeModel nodeModel,
			List<EdgeModel> edgeList, List<NodeModel> nodeList, int level) {
		if (level != 0 && !nodeList.contains(nodeModel)) {
			nodeList.add(nodeModel);

			if (level == 1)
				return;

			Iterator<EdgeModel> itr = nodeModel.getSourceConnections()
					.iterator();
			while (itr.hasNext()) {
				EdgeModel edge = itr.next();
				if (!edgeList.contains(edge)) {
					edgeList.add(edge);
				}
				NodeModel node = edge.getTarget();
				int newLevel = level - 1;
				getChildHierarchy(node, edgeList, nodeList, newLevel);
			}
		}
	}

	/**
	 * Returns the list of the Eobjects that are contained inside the references
	 * of this Eobject
	 * 
	 * @param parentObj
	 * @return
	 */
	@SuppressWarnings("unchecked")
	public static List<EObject> getEObjListFromChildReferences(EObject parentObj) {
		List<EObject> list = new ArrayList<EObject>();
		Iterator<EReference> itr = parentObj.eClass().getEAllReferences()
				.iterator();

		while (itr.hasNext()) {
			EReference ref = itr.next();
			Object obj = parentObj.eGet(ref);

			if (obj instanceof List) {
				list.addAll((List<EObject>) obj);
			} else if (obj instanceof EObject) {
				list.add((EObject) obj);
			}
		}
		return list;
	}

	/**
	 * Returns the object from the given list which is having the same value for
	 * the given feature as value.
	 * 
	 * @param list
	 * @param feature
	 * @param value
	 * @return
	 */
	public static Object getObjectFrmFeature(List list, String feature,
			Object value) {

		for (int i = 0; i < list.size(); i++) {
			EObject obj = (EObject) list.get(i);
			Object objVal = EcoreUtils.getValue(obj, feature);

			if (objVal != null && objVal.equals(value)) {
				return obj;
			}
		}

		return null;
	}

	/**
	 * Returns the class object with name as class name from the same package of
	 * the geiven object.
	 * 
	 * @param eObj
	 * @param className
	 * @return
	 */
	public static EClass getClassFromSamePackage(EObject eObj, String className) {
		return (EClass) eObj.eClass().getEPackage().getEClassifier(className);
	}

	/**
	 * Creates the instance of given class from the given ecore.
	 * 
	 * @param relFilePath
	 * @param className
	 * @return
	 */
	public static EObject createEObjFromEcore(String relFilePath,
			String className) {
/*		URL url = FileLocator.find(DataPlugin.getDefault().getBundle(),
				new Path(relFilePath), null);
*/
		URL url = DataPlugin.getDefault().find(new Path(relFilePath));
		EObject obj = null;

		try {
/*			File ecoreFile = new Path(FileLocator.resolve(url).getPath())
			.toFile();
*/
			File ecoreFile = new Path(Platform.resolve(url).getPath())
			.toFile();
			EPackage pack = EcoreModels.get(ecoreFile.getAbsolutePath());

			EClass objClass = (EClass) pack.getEClassifier(className);
			obj = EcoreUtils.createEObject(objClass, true);

		} catch (Exception exception) {
			WorkspacePlugin.LOG.error("Error while Loading " + relFilePath
					+ " Ecore", exception);
		}

		return obj;
	}

	/**
	 * Checks whether the given file is a model template file or not.
	 * 
	 * @param fileName
	 * @return true if the file is a model template file, false otherwise
	 */
	public static boolean isModelTemplateFile(String fileName) {
		if (getModelTemplateTypeFromFile(fileName) != null) {
			return true;
		}
		return false;
	}

	/**
	 * Checks whether the given file is model template archive or not.
	 * 
	 * @param archieveName
	 * @return
	 */
	public static boolean isModelTemplateArchieve(String archieveName) {
		if (archieveName.endsWith(MODEL_TEMPLATE_ARCHIEVE_EXT)) {
			return true;
		}
		return false;
	}

	/**
	 * Checks whether the given location is a model template folder location or
	 * not.
	 * 
	 * @param location
	 * @return true if model template folder, false otherwise
	 */
	public static boolean isModelTempalteFolder(String location) {
		if (location.equals(MODEL_TEMPLATE_FOLDER_PATH)) {
			return true;
		}
		return false;
	}

	/**
	 * Returns the type of model template
	 * 
	 * @param fileName
	 * @return the model template type, null if not a model template file
	 */
	public static String getModelTemplateTypeFromFile(String fileName) {
		if (fileName.endsWith(SUFFIX_RESOURCE_MODEL_TEMPLATE_FILE)) {
			return MODEL_TYPE_RESOURCE;
		} else if (fileName.endsWith(SUFFIX_COMPONENT_MODEL_TEMPLATE_FILE)) {
			return MODEL_TYPE_COMPONENT;
		}
		return null;
	}

	/**
	 * Returns the list of object names contained in the objList.
	 * 
	 * @param objList
	 * @return the list of object names
	 */
	public static List<String> getNameListFromEObjList(List<EObject> objList) {
		List<String> nameList = new ArrayList<String>();
		Iterator<EObject> objItr = objList.iterator();

		String name = null;
		while (objItr.hasNext()) {

			name = EcoreUtils.getName(objItr.next());
			if (name != null) {
				nameList.add(name);
			}
		}

		return nameList;
	}

	/**
	 * Returns the names of object contained in the objList, if they are
	 * duplicated in nameList.
	 * 
	 * @param objList
	 * @param nameList
	 * @return the list of duplicate object names
	 */
	public static List<String> getDuplicateEObjNameList(List<EObject> objList,
			List<String> nameList) {
		List<String> duplicateNameList = new ArrayList<String>();
		Iterator<EObject> objItr = objList.iterator();

		String name = null;
		while (objItr.hasNext()) {

			name = EcoreUtils.getName(objItr.next());
			if (name != null) {
				if (nameList.contains(name)) {
					duplicateNameList.add(name);
				}
			}
		}

		return duplicateNameList;
	}

	/**
	 * Returns the object contained in the objList, if their names are in
	 * nameList.
	 * 
	 * @param objList
	 * @param nameList
	 * @return the list of duplicate object names
	 */
	public static List<EObject> getEObjectsHavingNames(List<EObject> objList,
			List<String> nameList) {

		List<EObject> eObjList = new ArrayList<EObject>();
		String name = null;

		for (EObject eObj : objList) {
			name = EcoreUtils.getName(eObj);

			if (name != null && nameList.contains(name)) {
				eObjList.add(eObj);
			}
		}

		return eObjList;
	}

	/**
	 * Sets the rdn of the template objects in the list.
	 * 
	 * @param list
	 *            the list of EObjects
	 * @param blankFlag
	 *            sets the value as blank it true, otherwise to object's
	 *            hash-code
	 */
	public static void setRDN(List<EObject> list, boolean blankFlag) {
		Iterator<EObject> itr = list.iterator();
		while (itr.hasNext()) {
			setRDN(itr.next(), blankFlag);
		}
	}

	/**
	 * Sets the rdn of the template object.
	 * 
	 * @param eobj
	 * @param blankFlag
	 *            sets the value as blank it true, otherwise to object's
	 *            hash-code
	 */
	@SuppressWarnings("unchecked")
	public static void setRDN(EObject eobj, boolean blankFlag) {
		if (blankFlag) {
			EcoreUtils.setValue(eobj, ModelConstants.RDN_FEATURE_NAME, "");
		} else {
			EcoreUtils.setValue(eobj, ModelConstants.RDN_FEATURE_NAME, String
					.valueOf(eobj.hashCode()));
		}

		Iterator<EReference> referenceItr = eobj.eClass().getEAllReferences()
				.iterator();
		while (referenceItr.hasNext()) {

			Object refObj = eobj.eGet(referenceItr.next());
			if (refObj != null) {

				if (refObj instanceof EObject) {
					setRDN((EObject) refObj, blankFlag);

				} else if (refObj instanceof List) {
					setRDN((List<EObject>) refObj, blankFlag);
				}
			}
		}
	}
}
