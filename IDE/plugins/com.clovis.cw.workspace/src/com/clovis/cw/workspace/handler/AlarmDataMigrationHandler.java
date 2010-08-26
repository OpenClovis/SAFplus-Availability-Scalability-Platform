package com.clovis.cw.workspace.handler;

import java.util.HashMap;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.workspace.project.migration.ResourceCopyHandler;

/**
 * 
 * @author shubhada
 * Specific Resource Editor data migration handler to handle the change in heirarchy
 * (i.e all the flat objects in R2.2 have be made to be contained under a toplevel
 * object now)
 */
public class AlarmDataMigrationHandler
{
	public static void migrateResource(IProject project, Resource newResource, Resource oldResource,
			EObject changeInfoObj, EPackage ePack, String oldVersion)
	{
		
		List retList = new Vector();
		
		EClass objInfoClass = (EClass) ePack.getEClassifier(
				"alarmInformation");
		EObject infoObj = EcoreUtils.createEObject(
				objInfoClass, true);
		newResource.getContents().add(infoObj);
		HashMap destSrcMap = addObjectsToModel(oldResource.getContents(), infoObj);
		destSrcMap.put(infoObj, null);
		ResourceCopyHandler copyHandler = new ResourceCopyHandler(changeInfoObj);
        copyHandler.copyResourceInfo(destSrcMap, retList);
        List alarmProfileList = (List) EcoreUtils.getValue(infoObj, "AlarmProfile");
        HashMap keyAlarmMap = new HashMap();
        for (int i = 0; i < alarmProfileList.size(); i++) {
        	EObject alarmProfile = (EObject) alarmProfileList.get(i);
        	String rdn = EcoreUtils.getValue(alarmProfile, "rdn").toString();
        	keyAlarmMap.put(rdn, alarmProfile);
        }
        for (int i = 0; i < alarmProfileList.size(); i++) {
        	EObject alarmProfile = (EObject) alarmProfileList.get(i);
        	EObject genRule = (EObject) EcoreUtils.getValue(alarmProfile,
        			ClassEditorConstants.ALARM_GENERATIONRULE);
        	if (genRule != null) {
        		List alarmIds = (List) EcoreUtils.getValue(genRule, "AlarmIDs");
        		for (int j = 0; j < alarmIds.size(); j++) {
        			String alarmId = (String) alarmIds.get(j);
        			EObject alarm = (EObject) keyAlarmMap.get(alarmId);
        			if (alarm != null) {
        				alarmIds.set(j, EcoreUtils.getName(alarm));
        			}
        		}
        	}
        	EObject suppressRule = (EObject) EcoreUtils.getValue(alarmProfile,
        			ClassEditorConstants.ALARM_SUPPRESSIONRULE);
        	if (suppressRule != null) {
        		List alarmIds = (List) EcoreUtils.getValue(suppressRule, "AlarmIDs");
        		for (int j = 0; j < alarmIds.size(); j++) {
        			String alarmId = (String) alarmIds.get(j);
        			EObject alarm = (EObject) keyAlarmMap.get(alarmId);
        			if (alarm != null) {
        				alarmIds.set(j, EcoreUtils.getName(alarm));
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
    private static HashMap addObjectsToModel(List eObjects, EObject topObj)
    {
    	HashMap destSrcMap = new HashMap();
        for (int i = 0; i < eObjects.size(); i++) {
            EObject eobj = (EObject) eObjects.get(i);
            List refList = topObj.eClass().getEAllReferences();
            for (int j = 0; j < refList.size(); j++) {
                EReference ref = (EReference) refList.get(j);
                if (eobj.eClass().getName().equals(ref.
                        getEReferenceType().getName())) {
                    Object val = topObj.eGet(ref);
                    if (ref.getUpperBound() == 1) {
                    	EObject refObj = EcoreUtils.createEObject(ref.
                    			getEReferenceType(), true); 
                        topObj.eSet(ref, refObj);
                        destSrcMap.put(refObj, eobj);
                    } else if (ref.getUpperBound() == -1
                            || ref.getUpperBound() > 1) {
                    	EObject refObj = EcoreUtils.createEObject(ref.
                    			getEReferenceType(), true); 
                        ((List) val).add(refObj);
                        destSrcMap.put(refObj, eobj);
                    }
                
                }
            }
        }
        return destSrcMap;
    }

}
