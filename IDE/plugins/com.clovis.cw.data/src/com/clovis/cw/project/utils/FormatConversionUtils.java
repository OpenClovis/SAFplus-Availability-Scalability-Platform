package com.clovis.cw.project.utils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.emf.common.notify.Adapter;
import org.eclipse.emf.ecore.ENamedElement;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

import com.clovis.common.utils.ClovisUtils;
import com.clovis.common.utils.constants.AnnotationConstants;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreCloneUtils;
import com.clovis.common.utils.ecore.EcoreUtils;



public class FormatConversionUtils
{
	public static final String COMPONENT_EDITOR = "Component Editor";
    public static final String RESOURCE_EDITOR = "Resource Editor";
	 /**
     * Return whether named element is a Editor Connection object or not.
     *  Uses isConnection key in CwAnnotation to find this.
     * @param element Element
     * @return true if feature refers to a connection object.
     */
    public static boolean isConnectionObject(ENamedElement element)
    {
        String modifyStr = EcoreUtils.getAnnotationVal(element, null,
        		AnnotationConstants.IS_CONNECTION_REFERENCE);
        return modifyStr != null ? Boolean.parseBoolean(modifyStr) : false;
    }
    
    /**
     * Converts all project xml objects to internal editor EObjects
     * which have internal information like rdn
     * @param topObj - top level xml object in the project xml file
     * @param infoObj - Editor Top Level Object
     * @param editorType - Editor Type
     * @return - Return the resource objects
     */
    public static void convertToEditorSupportedData(EObject topObj, EObject infoObj, String editorType)
    {
        List list = new Vector();
        List refList = topObj.eClass().getEAllReferences();
        for (int i = 0; i < refList.size(); i++) {
            EReference ref = (EReference) refList.get(i);
            Object val = topObj.eGet(ref);
            if (val != null && val instanceof List) {
                List valList = (List) val;
                for (int j = 0; j < valList.size(); j++) {
                    list.add(valList.get(j));
                   
                }
                    
            }
        }
        List edgeList = new Vector();
        List mibObjList = new Vector();
        List mibList = new ArrayList();
        
        ClovisUtils.setKey(list);
        if (editorType.equals(RESOURCE_EDITOR)) {
            Iterator iterator = list.iterator();
            while (iterator.hasNext()) {
                EObject eobj = (EObject) iterator.next();
                
                List containedObjList = (List) EcoreUtils.getValue(
                        eobj, "contains");
                processConnectionObjects(containedObjList, edgeList, eobj,
                		"source", null);
                
                
                List associatedObjList = (List) EcoreUtils.getValue(
                        eobj, "associatedTo"); 
                processConnectionObjects(associatedObjList, edgeList, eobj,
                		"source", null);
                
                List inheritedObjList = (List) EcoreUtils.getValue(
                        eobj, "inherits");
                processConnectionObjects(inheritedObjList, edgeList, eobj,
                        "source", null);
                
                if (eobj.eClass().getName().equals("MibResource")) {
                	String mibName = EcoreUtils.getValue(eobj, "mibName").toString();
                	if (!mibList.contains(mibName)) {
                		mibList.add(mibName);
                	}
                }
            }
            for (int i = 0; i < mibList.size(); i++) {
            	String mibName = (String) mibList.get(i); 
            
            	EReference mibRef = (EReference) topObj.
            		eClass().getEStructuralFeature("mib");
            	EObject mibObj = EcoreUtils.
        			createEObject(mibRef.getEReferenceType(), true);
            	EcoreUtils.setValue(mibObj, "name", mibName);
            	mibObjList.add(mibObj);
            	// add the connection from chassis to mib
            	EObject chassisObj = (EObject) ((List) EcoreUtils.getValue(
    					topObj, "chassisResource")).get(0);
    	        EReference compositionRef = (EReference) topObj.eClass().
    				getEStructuralFeature("composition");
            	EObject compositionObj = EcoreUtils.createEObject(
    					compositionRef.getEReferenceType(), true);
	        	EcoreUtils.setValue(compositionObj, "source", (String) EcoreUtils.getName(
							chassisObj));
	        	EcoreUtils.setValue(compositionObj, "target", (String) EcoreUtils.getValue(
							mibObj, "name"));
	        	edgeList.add(compositionObj);
            }
            list.addAll(mibObjList);
            
        } else if (editorType.equals(COMPONENT_EDITOR)) {
           
            Iterator iterator = list.iterator();
            while (iterator.hasNext()) {
                EObject eobj = (EObject) iterator.next();
                List containedObjList = (List) EcoreUtils.getValue(
                        eobj, "contains");
                processConnectionObjects(containedObjList, edgeList, eobj,
                		"source",
                        "Containment");
                
                List proxiesList = (List) EcoreUtils.getValue(
                        eobj, "proxies");
                processConnectionObjects(proxiesList, edgeList, eobj,
                		"source",
                        "Proxy_Proxied");
                
                List associatedObjList = (List) EcoreUtils.getValue(
                        eobj, "associatedTo");
                processConnectionObjects(associatedObjList, edgeList, eobj,
                		"source",
                        "Association");
                
            }
            
        }
        
        for (int i = 0; i < edgeList.size(); i++) {
            EObject connObj = (EObject) edgeList.get(i);
            
            String start = (String) EcoreUtils.getValue(connObj,
                    "source");
            String end = (String) EcoreUtils.getValue(connObj,
                    "target");
            if (start != null && end != null) {
                
                EObject srcObj = ClovisUtils.getObjectFrmName(list, start);
                String startKey = (String) EcoreUtils.getValue(srcObj,
                		ModelConstants.RDN_FEATURE_NAME);
                EcoreUtils.setValue(connObj, "source", startKey);
                
                EObject targetObj = ClovisUtils.getObjectFrmName(list, end);
                String endKey = (String) EcoreUtils.getValue(targetObj,
                		ModelConstants.RDN_FEATURE_NAME);
                EcoreUtils.setValue(connObj, "target", endKey);
                    
            }
            
        }
        
        List adapterList = new Vector();
        adapterList.addAll(infoObj.eAdapters());
        for (int i = 0; i < adapterList.size(); i++) {
        	EcoreUtils.removeListener(infoObj, (Adapter) adapterList.get(i), 2);
        }
        
        List referenceList = infoObj.eClass().getEAllReferences();
        for (int i = 0; i < referenceList.size(); i++) {
            EReference ref = (EReference) referenceList.get(i);
            infoObj.eUnset(ref);
        }
        
        ClovisUtils.addObjectsToModel(list, infoObj);
        ClovisUtils.addObjectsToModel(edgeList, infoObj);
        //ClovisUtils.addObjectsToModel(mibObjList, infoObj);
        
        for (int i = 0; i < adapterList.size(); i++) {
            EcoreUtils.addListener(infoObj, (Adapter) adapterList.get(i), 2);
        }
        
    }
    /**
     * Adds the relevent information to the connection
     * objects contained inside resources 
     * @param connList - List of connection objects contained in node
     * @param currentEdgeList - Edge List for the editor format
     * @param eobj - Resource Object
     * @param connSrcFeature - Connection Source Feature Name
     * @param connType - Connection Type
     */
    private static void processConnectionObjects(List connList, List currentEdgeList,
    		EObject eobj, String connSrcFeature, String connType)
    {
        if (connList != null) {
        	
            for (int i = 0; i < connList.size(); i++) {
                EObject connObj = (EObject) connList.get(i);
                EcoreUtils.setValue(connObj,
                		connSrcFeature,
                        EcoreUtils.getName(eobj));
                if (connType != null) {
                	EcoreUtils.setValue(connObj, "type", connType);
                }
            }
            currentEdgeList.addAll(connList);
            connList.clear();
        }
    }
    
    /**
     * Converts all project xml objects to internal editor EObjects
     * which have internal information like rdn
     * @param topObj - top level xml object in the project xml file
     * @param editorType - Editor Type
     * @return - Return the resource objects
     */
    public static void convertToResourceFormat(EObject topObj, String editorType)
    {
    	HashMap keyObjectMap = new HashMap();
    	HashMap objAdaptersMap = new HashMap();
    	List adapterList = new Vector();
        adapterList.addAll(topObj.eAdapters());
        for (int i = 0; i < adapterList.size(); i++) {
        	EcoreUtils.removeListener(topObj, (Adapter) adapterList.get(i), 2);
        }
       
        List refList = topObj.eClass().getEAllReferences();
        
        
        
        for (int i = 0; i < refList.size(); i++) {
            EReference ref = (EReference) refList.get(i);
            Object val = topObj.eGet(ref);
            if (val != null && val instanceof List) {
                
                List valList = (List) val;
                for (int j = 0; j < valList.size(); j++) {
                	EObject valObj = (EObject) valList.get(j);
                	String key = (String) EcoreUtils.getValue(valObj,
                			ModelConstants.RDN_FEATURE_NAME);
                	keyObjectMap.put(key, valObj);
                	List adapters = new Vector();
                	adapters.addAll(valObj.eAdapters());
                	objAdaptersMap.put(valObj, adapters);
                    valObj.eAdapters().clear();
                	if (editorType.equals(RESOURCE_EDITOR)) {
                		List containsList = (List) EcoreUtils.getValue(
                    			valObj, "contains");
                    	if (containsList != null) {
                    		containsList.clear();
                    	}
                    	
                    	List associatedList = (List) EcoreUtils.getValue(
                    			valObj, "associatedTo");
                    	if (associatedList != null) {
                    		associatedList.clear();
                    	}
                    	
                    	List inheritedList = (List) EcoreUtils.getValue(
                    			valObj, "inherits");
                    	if (inheritedList != null) {
                    		inheritedList.clear();
                    	}
                    } else if (editorType.equals(COMPONENT_EDITOR)) {
                    	List containsList = (List) EcoreUtils.getValue(
                    			valObj, "contains");
                    	if (containsList != null) {
                    		containsList.clear();
                    	}
                    	
                    	List associatedList = (List) EcoreUtils.getValue(
                    			valObj, "associatedTo");
                    	if (associatedList != null) {
                    		associatedList.clear();
                    	}
                    	
                    	List proxiesList = (List) EcoreUtils.getValue(
                    			valObj, "proxies");
                    	if (proxiesList != null) {
                    		proxiesList.clear();
                    	}
                    }
                }
            }
        }
       
        for (int i = 0; i < refList.size(); i++) {
            EReference ref = (EReference) refList.get(i);
            boolean isConnRef = isConnectionObject(ref);
            if (isConnRef) {
	            Object val = topObj.eGet(ref);
	            if (val != null && val instanceof List) {
	                List valList = (List) val;
	                for (int j = 0; j < valList.size(); j++) {
	                	EObject connObj = EcoreCloneUtils.cloneEObject((EObject) valList.get(j));
	                	
	                	String srcKey = (String) EcoreUtils.getValue(connObj, "source");
	                    EObject srcObj = (EObject) keyObjectMap.get(srcKey);
	                    
	                    String targetKey = (String) EcoreUtils.getValue(connObj, "target");
	                    EObject targetObj = (EObject) keyObjectMap.get(targetKey);
	                    // Handle the case wherein target of the connection is already stored as name
	                    if (targetObj != null) {
	                    	EcoreUtils.setValue(connObj, "target", EcoreUtils.getName(targetObj));
	                    }
	                    if (editorType.equals(RESOURCE_EDITOR)) {
	                    	if (connObj.eClass().getName().equals("Composition")) {
	                    		List containsList = (List) EcoreUtils.getValue(srcObj, "contains");
	                    		if (!targetObj.eClass().getName().equals("Mib")) {
	                    			containsList.add(connObj);
	                    		}
	                    	} else if (connObj.eClass().getName().equals("Association")) {
	                    		List associatedList = (List) EcoreUtils.getValue(srcObj, "associatedTo");
	                    		associatedList.add(connObj);
	                    	} else if (connObj.eClass().getName().equals("Inheritence")) {
	                    		List inheritedList = (List) EcoreUtils.getValue(srcObj, "inherits");
	                    		inheritedList.add(connObj);
	                    	}
	                    } else if (editorType.equals(COMPONENT_EDITOR)) {
	                    	String connType = (String) EcoreUtils.getValue(connObj, "type");
	                    	if (connType.equals("Containment")) {
	                    		List containsList = (List) EcoreUtils.getValue(srcObj, "contains");
	                    		containsList.add(connObj);
	                    	} else if (connType.equals("Association")) {
	                    		List associatedList = (List) EcoreUtils.getValue(srcObj, "associatedTo");
	                    		associatedList.add(connObj);
	                    	} else if (connType.equals("Proxy_Proxied")) {
	                    		List proxiesList = (List) EcoreUtils.getValue(srcObj, "proxies");
	                    		proxiesList.add(connObj);
	                    	}
	                    }
	                }
	                
	            }
            }
        }
        
        for (int i = 0; i < adapterList.size(); i++) {
            EcoreUtils.addListener(topObj, (Adapter) adapterList.get(i), 2);
        }
        
        for (int i = 0; i < refList.size(); i++) {
            EReference ref = (EReference) refList.get(i);
            Object val = topObj.eGet(ref);
            if (val != null && val instanceof List) {
               
                List valList = (List) val;
                for (int j = 0; j < valList.size(); j++) {
                	EObject valObj = (EObject) valList.get(j);
                	List adapters = (List) objAdaptersMap.get(valObj);
                	for (int k = 0; k < adapters.size(); k++) {
                		EcoreUtils.addListener(valObj, (Adapter) adapters.get(k), 1);
                	}
                	
                }    
                
            }
        }
    }

}
