package com.clovis.cw.workspace.handler;

import java.util.HashMap;
import java.util.List;
import java.util.Properties;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.workspace.project.migration.ResourceCopyHandler;

/**
 * 
 * @author shubhada
 * 
 * Specific alarm rule migration handler to migrate the alarm rule
 * information to a different file 
 */
public class AlarmRuleMigrationHandler
{
	/**
	 * Migrates the resource
	 * 
	 * @param project - IProject
	 * @param newResource - New resource
	 * @param oldResource - Old resource
	 * @param changeInfoObj - ChangeInfoObj specified in track changes migration xmi file
	 * @param ePack - New ecore EPackage
	 * @param oldVersion - Old ecore/resource version
	 */
	public static void migrateResource(IProject project, Resource newResource, Resource oldResource,
			EObject changeInfoObj, EPackage ePack, String oldVersion)
	{
		List retList = new Vector();
		EClass objInfoClass = (EClass) ePack.getEClassifier(
			"alarmInformation");
		EObject infoObj = EcoreUtils.createEObject(
				objInfoClass, true);
		newResource.getContents().add(infoObj);
		HashMap destSrcMap = new HashMap();
		EObject oldInfoObj = (EObject) oldResource.getContents().get(0);
		destSrcMap.put(infoObj, oldInfoObj);
		ResourceCopyHandler copyHandler = new ResourceCopyHandler(changeInfoObj);
        copyHandler.copyResourceInfo(destSrcMap, retList);
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
		Model caModel = pdm.getCAModel();
		List resourceList = ResourceDataUtils.
			getResourcesList(caModel.getEList());
		HashMap alarmIDGenRuleMap = createAlarmIDGenRuleMap(
				oldInfoObj);
		HashMap alarmIDSuppRuleMap = createAlarmIDSuppRuleMap(
				oldInfoObj);
		Model alarmRuleModel = pdm.getAlarmRules();
		EObject alarmInfoObj = (EObject) alarmRuleModel.getEList().get(0);
		if (alarmInfoObj != null) {
			EReference resourceRef = (EReference) alarmInfoObj.eClass().
				getEStructuralFeature(ClassEditorConstants.ALARM_RESOURCE);
			List resList = (List) alarmInfoObj.eGet(resourceRef);
			for (int i = 0; i < resourceList.size(); i++) {
				EObject resObj = (EObject) resourceList.get(i);
				List associatedAlarmList = ResourceDataUtils.
					getAssociatedAlarms(project, resObj);
				if (associatedAlarmList != null &&
						!associatedAlarmList.isEmpty()) {
					EObject resource = EcoreUtils.createEObject(
							resourceRef.getEReferenceType(), true);
					EcoreUtils.setValue(resource, ClassEditorConstants.
							ALARM_RESOURCE_NAME, EcoreUtils.getName(resObj));
					resList.add(resource);
					EReference alarmRef = (EReference) resource.eClass().
						getEStructuralFeature(ClassEditorConstants.ALARM_ALARMOBJ);
					List alarmList = (List) resource.eGet(alarmRef);
					for (int j = 0; j < associatedAlarmList.size(); j++) {
						String associatedAlarm = (String) associatedAlarmList.get(j);
						EObject alarmObj = EcoreUtils.createEObject(
								alarmRef.getEReferenceType(), true);
						EcoreUtils.setValue(alarmObj, ClassEditorConstants.ALARM_ID, associatedAlarm);
						alarmList.add(alarmObj);
						EObject genRuleObj = (EObject) alarmIDGenRuleMap.get(associatedAlarm);
						EObject suppRuleObj = (EObject) alarmIDSuppRuleMap.get(associatedAlarm);
						Properties map = new Properties();
						map.put("Relation", "relationType");
						map.put("MaxAlarm", "maxAlarm");
						map.put("AlarmIDs", "alarmIDs");
						
						if (genRuleObj != null) {
							EReference genRuleRef = (EReference) alarmObj.eClass().getEStructuralFeature(
									ClassEditorConstants.ALARM_GENERATIONRULE);
							EObject genRule = EcoreUtils.createEObject(
									genRuleRef.getEReferenceType(), true);
							EcoreUtils.copyValues(genRuleObj, genRule, map);
							alarmObj.eSet(genRuleRef, genRule);
						}
						
						if (suppRuleObj != null) {
							EReference suppRuleRef = (EReference) alarmObj.eClass().getEStructuralFeature(
									ClassEditorConstants.ALARM_SUPPRESSIONRULE);
							EObject suppRule = EcoreUtils.createEObject(
									suppRuleRef.getEReferenceType(), true);
							EcoreUtils.copyValues(suppRuleObj, suppRule, map);
							alarmObj.eSet(suppRuleRef, suppRule);
						}
					}
				}
			}
			// Save the alarmRule model
			alarmRuleModel.save(true);
		}
	}
	/**
	 * Creates alarmID to generationRule map
	 * 
	 * @param alarmInfoObj - AlarmInformation object
	 * @return the alarmID to generationRule map
	 */
	private static HashMap createAlarmIDGenRuleMap(EObject alarmInfoObj)
	{
		HashMap alarmIDGenRuleMap = new HashMap();
		List alarmProfileList = (List) EcoreUtils.getValue(alarmInfoObj, "AlarmProfile");
		for (int i = 0; i < alarmProfileList.size(); i++) {
			EObject alarmObj = (EObject) alarmProfileList.get(i);
			String alarmID = EcoreUtils.getValue(
					alarmObj, "AlarmID").toString();
			EObject genRuleObj = (EObject) EcoreUtils.getValue(
					alarmObj, "GenerationRule");
			if (genRuleObj != null) {
				alarmIDGenRuleMap.put(alarmID, genRuleObj);
			}
			
		}
		return alarmIDGenRuleMap;
	}
	/**
	 * Creates alarmID to SuppressionRule map
	 * 
	 * @param alarmInfoObj - AlarmInformation object
	 * @return the alarmID to generationRule map
	 */
	private static HashMap createAlarmIDSuppRuleMap(EObject alarmInfoObj)
	{
		HashMap alarmIDSuppRuleMap = new HashMap();
		List alarmProfileList = (List) EcoreUtils.getValue(alarmInfoObj, "AlarmProfile");
		for (int i = 0; i < alarmProfileList.size(); i++) {
			EObject alarmObj = (EObject) alarmProfileList.get(i);
			String alarmID = EcoreUtils.getValue(
					alarmObj, "AlarmID").toString();
			EObject genRuleObj = (EObject) EcoreUtils.getValue(
					alarmObj, "SuppressionRule");
			if (genRuleObj != null) {
				alarmIDSuppRuleMap.put(alarmID, genRuleObj);
			}
			
		}
		return alarmIDSuppRuleMap;
	}

}
