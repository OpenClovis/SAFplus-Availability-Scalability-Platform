package com.clovis.cw.editor.ca.manageability.ui;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;

import org.eclipse.emf.common.util.BasicEList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;

/**
 * 
 * @author Pushparaj
 * Utils class for Associate Resources
 */
public class AssociateResourceUtils {

	/**
	 * Verifys the shared instance ID.
	 * @param instID
	 * @param resName
	 * @param moPath
	 * @param compInstObjList
	 * @return true if the instID is valid shared instance otherwise false.
	 */
	public static boolean isExistingSharedInstance(int instID, String resName, String moPath, List<EObject> compInstObjList) {
		for (int i = 0; i < compInstObjList.size(); i++) {
			EObject compInstObj = compInstObjList.get(i);
			BasicEList<EObject> resourceList = (BasicEList) EcoreUtils.getValue(
					compInstObj, "resources");
			List<String> moIDs = getAllMoIDs(resourceList);
			String moID =  moPath + "\\"+ resName + ":" + instID;
			if(moIDs.contains(moID)) {
				return true;
			}
		}
		return false;
	}
	/**
	 * Filter and return existing shared resources list
	 * @param instID instance ID
	 * @param resNames List of resource's names
	 * @param moPath mo path
	 * @param compInstObjList components List
	 * @return Existing Shared Resources List
	 */
	public static ArrayList filterExistingSharedResources(int instID, List <String>resNames, String moPath, List<EObject> compInstObjList) {
		ArrayList existingSharedResources = new ArrayList();
		for (int j = 0; j < resNames.size(); j++) {
			String resName = resNames.get(j);
			if(isExistingSharedInstance(instID, resName, moPath, compInstObjList)) {
				existingSharedResources.add(resName);
			}
		}
		return existingSharedResources;
	}
	/**
	 * Creates Shared resources for the components
	 * @param compInstObjList comp list
	 * @param resNames resources names
	 * @param moPath mo path
	 * @param instID shared instance ID
	 * @param initializedResList List of initialized resources
	 */
	public static void createSharedResourceForComponents(
			List<EObject> compInstObjList, List<String> resNames, String moPath, int instID, List<String> initializedResList) {
		if(compInstObjList != null && compInstObjList.size() > 0) {
			EClass eClass = (EClass) compInstObjList.get(0).eClass().getEPackage().getEClassifier("resourceType");
			for (int i = 0; i < compInstObjList.size(); i++) {
				EObject compInstObj = compInstObjList.get(i);
				BasicEList<EObject> resourceList = (BasicEList) EcoreUtils.getValue(
						compInstObj, "resources");
				List<String> moIDs = getAllMoIDs(resourceList);
				ArrayList<EObject> list = new ArrayList<EObject>(resNames.size());
				for (int j = 0; j < resNames.size(); j++) {
					String resName = resNames.get(j);
					boolean autoCreate = !(initializedResList.contains(resName));
					String moID = moPath + "\\" + resName + ":" + instID;
					if (!moIDs.contains(moID)) {
						EObject resObj = EcoreUtils.createEObject(eClass, true);
						EcoreUtils.setValue(resObj, "moID", moID);
						EcoreUtils.setValue(resObj, "autoCreate", String
								.valueOf(autoCreate));
						list.add(resObj);
					}
				}
				resourceList.grow(list.size());
				resourceList.addAllUnique(list);
			}
		}
	}
	/**
	 * Identify next valid shared instance id and creates shared resources.
	 * @param compInstObjList List of components
	 * @param resNames List of resource's names
	 * @param moPath mo path
	 * @param moIDsMap map between resource name and mo ids
	 * @param initializedResList List of initialized resources
	 * @param message contains created resource's names
	 */
	public static void createSharedResourceForComponents(
			List<EObject> compInstObjList, List<String> resNames, String moPath, Map<String, BasicEList<String>> moIDsMap, List<String> initializedResList, StringBuffer message) {
		if(compInstObjList != null && compInstObjList.size() > 0) {
			EClass eClass = (EClass) compInstObjList.get(0).eClass().getEPackage().getEClassifier("resourceType");
			for (int j = 0; j < resNames.size(); j++) {
				String resName = resNames.get(j);
				boolean autoCreate = !initializedResList.contains(resName);
				BasicEList moIDs = moIDsMap.get(resName);
				int nextID = 0;
				if (moIDs == null) {
					moIDs = new BasicEList<String>();
					moIDsMap.put(resName, moIDs);
				} else {
					nextID = findNextSharedInstance(compInstObjList, resName, moIDsMap, moPath);
				}
				message.append(resName+ ":" + nextID + " ");
				for (int i = 0; i < compInstObjList.size(); i++) {
					EObject compInstObj = compInstObjList.get(i);
					BasicEList<EObject> resourceList = (BasicEList) EcoreUtils.getValue(
							compInstObj, "resources");
					EObject resObj = EcoreUtils.createEObject(eClass, true);
					String moID = moPath + "\\" + resName + ":" + nextID;
					moIDs.add(moID);
					EcoreUtils.setValue(resObj, "moID", moID);
					EcoreUtils.setValue(resObj, "autoCreate", String.valueOf(autoCreate));
					resourceList.addUnique(resObj);
				}
			}
		}
	}
	/**
	 * Finds next valid shared instance for the resources
	 * @param compInstObjList List of components
	 * @param resName Resource name
	 * @param moIDsMap Map between resource name and mo ids
	 * @param moPath mo path
	 * @return shared instance id
	 */
	public static int findNextSharedInstance(List<EObject> compInstObjList,
			String resName, Map<String, BasicEList<String>> moIDsMap, String moPath) {
		BasicEList moIDs = moIDsMap.get(resName);
		if(moIDs.size() == 0)
			return 0;
		int lastSharedIndex = 0;
		int nextSharedIndex = 0;
		lastSharedIndex = moIDs.size() / compInstObjList.size();
		Map<EObject, List<String>> compMoIdsMap = new HashMap<EObject, List<String>>();
		for (int i = 0; i < compInstObjList.size(); i++) {
			EObject compObj = compInstObjList.get(i);
			BasicEList<EObject> resourceList = (BasicEList) EcoreUtils.getValue(
					compObj, "resources");
			compMoIdsMap.put(compObj, getAllMoIDs(resourceList));
		}
		for (int i = lastSharedIndex; i < (moIDs.size() * compInstObjList.size()) + 1; i++) {
			String moID =  moPath + "\\"+ resName + ":" + i;
			boolean contains = false;
			Iterator itr = compMoIdsMap.keySet().iterator();
			while(itr.hasNext()) {
				List<String> Ids = compMoIdsMap.get(itr.next());
				if(Ids.contains(moID)) {
					contains = true;
					break;
				}
			}
			if(!contains) {
				nextSharedIndex = i;
				break;
			}
		}
		return nextSharedIndex;
	}
	/**
	 * Find the avilable instance ID and creates the resources for components
	 * @param compInstObjList List of components
	 * @param resNames List of resource's names
	 * @param moPath mo path
	 * @param moIDsMap Map between resource name mo ids
	 * @param initializedResList List of initialized resources
	 */
	public static void createOneResourceForComponents(
			List<EObject> compInstObjList, List<String> resNames, String moPath, Map<String, BasicEList<String>> moIDsMap, List<String> initializedResList) {
		if(compInstObjList != null && compInstObjList.size() > 0 ) {
			EClass eClass = (EClass) compInstObjList.get(0).eClass().getEPackage().getEClassifier("resourceType");
			for (int j = 0; j < resNames.size(); j++) {
				String resName = resNames.get(j);
				boolean autoCreate = !initializedResList.contains(resName);
				BasicEList moIDs = moIDsMap.get(resName);
				if (moIDs == null) {
					moIDs = new BasicEList<String>();
					moIDsMap.put(resName, moIDs);
				}
				int startID = 0;
				for (int i = 0; i < compInstObjList.size(); i++) {
					EObject compInstObj = compInstObjList.get(i);
					BasicEList<EObject> resourceList = (BasicEList) EcoreUtils.getValue(
							compInstObj, "resources");
					EObject resObj = EcoreUtils.createEObject(eClass, true);
					String moID = moPath + "\\" + resName + ":" + startID;
					if(!moIDs.contains(moID)) {
						moIDs.add(moID);
					} else {
						startID = getAvailableMoID(moIDs, moPath, resName,startID);
						moID = moPath + "\\" + resName + ":" + startID;
						moIDs.add(moID);
					}
					startID = startID + 1;
					EcoreUtils.setValue(resObj, "moID", moID);
					EcoreUtils.setValue(resObj, "autoCreate", String.valueOf(autoCreate));
					resourceList.addUnique(resObj);
				}
			}
		}
	}
	/**
	 * Creates resources for the the components with in the range
	 * @param compInstObjList List of component instances
	 * @param resNames List of resource's names
	 * @param moPath mo path
	 * @param fromID From instance ID
	 * @param toID To instance OD
	 * @param initializedResList List of initialized resources
	 */
	public static void createResourcesForComponentsWithInRange(
			List<EObject> compInstObjList, List<String> resNames, String moPath, int fromID, int toID, List<String> initializedResList) {
		if(compInstObjList != null && compInstObjList.size() > 0) {
			EClass eClass = (EClass) compInstObjList.get(0).eClass().getEPackage().getEClassifier("resourceType");
			for (int i = 0; i < compInstObjList.size(); i++) {
				EObject compInstObj = compInstObjList.get(i);
				BasicEList<EObject> resourceList = (BasicEList) EcoreUtils.getValue(
						compInstObj, "resources");
				ArrayList list = new ArrayList(resNames.size() * ((toID - fromID)));
				List<String> moIDs = getAllMoIDs(resourceList);
				for (int j = 0; j < resNames.size(); j++) {
					String resName = resNames.get(j);
					boolean autoCreate = !initializedResList.contains(resName); 	
					for (int k = fromID; k <= toID; k++ ) {
						String moID = moPath + "\\"	+ resName + ":" + k;
						if (!moIDs.contains(moID)) {
							EObject resObj = EcoreUtils.createEObject(eClass,
									true);
							EcoreUtils.setValue(resObj, "moID", moID);
							EcoreUtils.setValue(resObj, "autoCreate", String.valueOf(autoCreate));
							list.add(resObj);
						}
					}
				}
				resourceList.grow(list.size());
				resourceList.addAllUnique(list);
			}
		}
	}
	/**
	 * Creates array of resources for the components
	 * @param compInstObjList List of components
	 * @param resNames List of resource's names
	 * @param moPath mo path
	 * @param size Array size
	 * @param moIDsMap Map between resource name and mo ids
	 * @param initializedResList List of initialized resources
	 */
	public static void createArrayOfResourcesForComponents(
			List<EObject> compInstObjList, List<String> resNames,
			String moPath, int size, Map<String, BasicEList<String>> moIDsMap, List<String> initializedResList) {
		if (compInstObjList != null && compInstObjList.size() > 0) {
			EClass eClass = (EClass) compInstObjList.get(0).eClass().getEPackage().getEClassifier("resourceType");
			
			for (int i = 0; i < compInstObjList.size(); i++) {
				EObject compInstObj = compInstObjList.get(i);
				BasicEList<EObject> resourceList = (BasicEList) EcoreUtils.getValue(
						compInstObj, "resources");
				ArrayList list = new ArrayList(resNames.size()*size);
				for (int j = 0; j < resNames.size(); j++) {
					int startID = 0;
					String resName = resNames.get(j);
					boolean autoCreate = !initializedResList.contains(resName);
					BasicEList<String> moIDs = moIDsMap.get(resName);
					if(moIDs == null) {
						moIDs = new BasicEList<String>();
						moIDsMap.put(resName, moIDs);
					}
					moIDs.grow(size);
					//startID = moIDs.size();
					for (int k = 0; k < size; k++ ) {
						String moID = moPath + "\\" + resName + ":" + startID;
						if(!moIDs.contains(moID)) {
							moIDs.add(moID);
						}
						else {
							startID = getAvailableMoID(moIDs, moPath, resName, startID);
							moID = moPath + "\\" + resName + ":" + startID;
							moIDs.add(moID);
						}
						startID = startID + 1;
						EObject resObj = EcoreUtils.createEObject(eClass,
									true);
						EcoreUtils.setValue(resObj, "moID", moID);
						EcoreUtils.setValue(resObj, "autoCreate", String.valueOf(autoCreate));
						list.add(resObj);
					}
				}
				resourceList.grow(list.size());
				resourceList.addAllUnique(list);
			}
		}
	}
	/**
	 * Creates scalar resources for the components
	 * @param compInstObjList List of components
	 * @param resNames List of resource's names
	 * @param moPath mo path
	 * @param initializedResList List of initialized resources
	 */
	public static void createScalarResources(List<EObject> compInstObjList, List<String> resNames,
			String moPath, List<String> initializedResList) {
		if(compInstObjList != null && compInstObjList.size() > 0) {
			BasicEList<String> moIDs = new BasicEList<String>();  
			EClass eClass = (EClass) compInstObjList.get(0).eClass().getEPackage().getEClassifier("resourceType");
			for (int i = 0; i < compInstObjList.size(); i++) {
				EObject compInstObj = compInstObjList.get(i);
				BasicEList<EObject> resourceList = (BasicEList) EcoreUtils.getValue(
						compInstObj, "resources");
				moIDs.grow(resourceList.size());
				moIDs.addAll(getAllMoIDs(resourceList));
				
			}
			for (int i = 0; i < 1; i++) {
				EObject compInstObj = compInstObjList.get(i);
				BasicEList<EObject> resourceList = (BasicEList) EcoreUtils.getValue(
						compInstObj, "resources");
				ArrayList list = new ArrayList(resNames.size());
				for (int j = 0; j < resNames.size(); j++) {
					String resName = resNames.get(j);
					boolean autoCreate = !initializedResList.contains(resName);
					String moID =  moPath + "\\"+ resName + ":" + 0;
					if(!moIDs.contains(moID)) {
						EObject resObj = EcoreUtils.createEObject(eClass, true);
						EcoreUtils.setValue(resObj, "moID", moID);
						EcoreUtils.setValue(resObj, "autoCreate", String.valueOf(autoCreate));
						list.add(resObj);
					}
				}
				resourceList.grow(list.size());
				resourceList.addAllUnique(list);
			}
		}
	}
	/**
	 * Craetes scalar resources with shared instance ID
	 * @param compInstObjList List of components
	 * @param resNames List of resource's names
	 * @param moPath mo path
	 * @param initializedResList List of initialized resources
	 */
	public static void createScalarSharedResources(List<EObject> compInstObjList, List<String> resNames,
			String moPath, List<String> initializedResList) {
		if(compInstObjList != null && compInstObjList.size() > 0) {
			EClass eClass = (EClass) compInstObjList.get(0).eClass().getEPackage().getEClassifier("resourceType");
			for (int i = 0; i < compInstObjList.size(); i++) {
				EObject compInstObj = compInstObjList.get(i);
				BasicEList<EObject> resourceList = (BasicEList) EcoreUtils.getValue(
						compInstObj, "resources");
				List<String> moIDs = getAllMoIDs(resourceList);
				ArrayList list = new ArrayList(resNames.size());
				for (int j = 0; j < resNames.size(); j++) {
					String resName = resNames.get(j);
					boolean autoCreate = !initializedResList.contains(resName);
					String moID =  moPath + "\\"+ resName + ":" + 0;
					if(!moIDs.contains(moID)) {
						EObject resObj = EcoreUtils.createEObject(eClass, true);
						EcoreUtils.setValue(resObj, "moID", moID);
						EcoreUtils.setValue(resObj, "autoCreate", String.valueOf(autoCreate));
						list.add(resObj);
					}
				}
				resourceList.grow(list.size());
				resourceList.addAllUnique(list);
			}
		}
	}
	/**
	 * Returns all mo ids for the resources
	 * @param resources List of resources
	 * @return mo ids
	 */
	public static List<String> getAllMoIDs(BasicEList<EObject> resources) {
		List<String> moIds = new ArrayList<String>(resources.size());
		for(int i = 0; i < resources.size(); i++) {
			EObject resObj = resources.get(i);
			moIds.add((String)EcoreUtils.getValue(resObj, "moID"));
		}
		return moIds;
	}
	/**
	 * Returns all the mo ids for the components
	 * @param compInstObjList List of components
	 * @return mo ids
	 */
	public static List<String> getAllMoIDsForType(List<EObject> compInstObjList) {
		BasicEList<String> moIds = new BasicEList<String>();
		for (int j = 0; j < compInstObjList.size(); j++) {
			EObject compInstObj = compInstObjList.get(j);
			List<EObject> resourceList = (EList) EcoreUtils.getValue(
					compInstObj, "resources");
			moIds.grow(resourceList.size());
			for (int i = 0; i < resourceList.size(); i++) {
				EObject resObj = resourceList.get(i);
				moIds.add((String) EcoreUtils.getValue(resObj, "moID"));
			}
		}
		return moIds;
	}
	/**
	 * Finds and returns available mo id for the particular resource
	 * @param moIDs List of mo ids
	 * @param moPath mo path
	 * @param resName Resource name
	 * @param startID start instance id
	 * @return
	 */
	public static int getAvailableMoID(List<String> moIDs, String moPath, String resName, int startID) {
		for(int i = startID; i < moIDs.size(); i++) {
			String moID = moPath + "\\" + resName + ":" + i;
			if(!moIDs.contains(moID))
				return i;
		}
		return  moIDs.size();
	}
	/**
	 * Check the initialized array Attributes in the Resource
	 * @param mibResObj
	 * @return
	 */
	public static boolean isInitializedArrayAttrResource(EObject mibResObj) {
		EReference provRef = (EReference) mibResObj.eClass().getEStructuralFeature(
				ClassEditorConstants.RESOURCE_PROVISIONING);
		EObject provObj = (EObject) mibResObj.eGet(provRef);
		if (provObj == null) {
			return false;
		}
		EReference attrRef = (EReference) provObj.eClass().getEStructuralFeature(
				ClassEditorConstants.CLASS_ATTRIBUTES);
		List<EObject> attrList = (List) provObj.eGet(attrRef);
		for(int i = 0; i < attrList.size(); i++) {
			EObject attrObj = attrList.get(i);
			if(((Boolean)EcoreUtils.getValue(attrObj, "initialized")).booleanValue() && ((Integer)EcoreUtils.getValue(attrObj, "multiplicity")) > 1)
				return true; 
		}
		return false;
	}
	/**
	 * Create and Returns Initialized Array Attribute's Resource List
	 */
	public static List getInitializedArrayAttrResList(List list) {
		EObject rootObject = (EObject) list.get(0);
		List<EObject> mibList = (EList)rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.MIB_RESOURCE_REF_NAME));
		List<EObject> nhwList = (EList)rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME));
		List<EObject> swList = (EList)rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME));
		List<String> initializedResList = new ArrayList<String>(mibList.size() + nhwList.size() + swList.size());
		for (int i = 0; i < mibList.size(); i++) {
			EObject resObj = mibList.get(i);
			String name = EcoreUtils.getName(resObj);
			if(isInitializedArrayAttrResource(resObj)) {
				initializedResList.add(name);
			}
		}
		for (int i = 0; i < nhwList.size(); i++) {
			EObject resObj = nhwList.get(i);
			String name = EcoreUtils.getName(resObj);
			if(isInitializedArrayAttrResource(resObj)) {
				initializedResList.add(name);
			}
		}
		for (int i = 0; i < swList.size(); i++) {
			EObject resObj = swList.get(i);
			String name = EcoreUtils.getName(resObj);
			if(isInitializedArrayAttrResource(resObj)) {
				initializedResList.add(name);
			}
		}
		return initializedResList;
	}
	/**
	 * Return ResourceType for the moID
	 * @param id resource moID
	 * @return String Resource type
	 */
	public static String getResourceTypeFromInstanceID(String id) {
    	if (id.contains(":") && id.contains("\\")) {
			String paths[] = id.split(":");
			StringTokenizer tokenizer = new StringTokenizer(
					paths[paths.length - 2], "\\");
			tokenizer.nextToken();
			String resName = tokenizer.nextToken();
			return resName;
		}
		return id;
    }
	/**
	 * Returns the instance id
	 * @param moID Resource mo ID
	 * @return
	 */
	public static int getInstanceNumberFromMoID(String moID) {
    	if (moID.contains(":") && moID.contains("\\") && !moID.endsWith("*")) {
			String paths[] = moID.split(":");
			return Integer.parseInt(paths[paths.length - 1]);
		}
		return 0;
    }
}