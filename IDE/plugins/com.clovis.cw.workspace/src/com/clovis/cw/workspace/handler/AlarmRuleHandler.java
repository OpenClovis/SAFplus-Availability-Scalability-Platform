package com.clovis.cw.workspace.handler;

import java.util.Iterator;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;


public class AlarmRuleHandler extends NotificationHandler {

	private static final Log LOG = Log.getLog(CaPlugin.getDefault());

	/**
	 * 
	 * @param n - Notification Object
	 * @param changedObj - Object which is changed
	 * @param dependentObj - The dependent object(s) which has to be 
	 * updated
	 * @param the features to updated in dependent object(s)
	 */
	public static void processNotifications(Notification n, Object changedObj,
			Object dependentObj, String[] featureNames,
			String[] referencePaths, ProjectDataModel pdm) {

		if (n.getEventType() == Notification.ADD
				|| n.getEventType() == Notification.ADD_MANY) {
			return;
		}

		try {
			List dependentList = (List) dependentObj;

			EStructuralFeature eAttr = (EStructuralFeature) n.getFeature();
			String changedFeature = null;

			if (eAttr != null)
				changedFeature = eAttr.getName();
			
			if(null == changedFeature)
				return;

			for (int i = 0; i < dependentList.size() ; i++) {
				
				EObject dependent = (EObject) dependentList.get(i);

				String dependentType = dependent.eClass().getName();

				switch (n.getEventType()) {

				case Notification.SET:

					if (dependentType.equals(ClassEditorConstants.ALARM_RESOURCE)
							&& changedFeature.equals(ClassEditorConstants.ALARM_RESOURCE_NAME)) {

						String oldResName = n.getOldStringValue();

						String resName = (String) EcoreUtils.getValue(
									dependent, featureNames[0]);

							if (resName.equals(oldResName)) {

								EcoreUtils.setValue(dependent, featureNames[0], n
										.getNewStringValue());

							}
					}

					else if (dependentType.equals(ClassEditorConstants.ALARM_ALARMRULE)) {

						if (changedFeature.equals(ClassEditorConstants.ALARM_ID)) {

							List associatedIDs = (List) EcoreUtils.getValue(
									dependent, featureNames[0]);

							String oldAlarmID = n.getOldStringValue();
							
							if (associatedIDs.contains(oldAlarmID)) {
								
								int index = associatedIDs.indexOf(oldAlarmID);
								associatedIDs.set(index, n.getNewStringValue());

							}
						}

					}
					
					else if(dependentType.equals(ClassEditorConstants.ALARM_ALARMOBJ)){
						
						if (changedFeature.equals(ClassEditorConstants.ALARM_ID)) {
							
							String oldAlarmID = n.getOldStringValue();
							
							String alarmID	= (String) EcoreUtils.getValue(dependent, featureNames[0]);
							
							if(oldAlarmID.equals(alarmID)){
								
								EcoreUtils.setValue(dependent, featureNames[0], n.getNewStringValue());
							}
						}
						
					}

					break;

				case Notification.REMOVE:

					if (dependentType.equals(ClassEditorConstants.ALARM_RESOURCE)) {
						
						boolean isRes = changedFeature.equals(ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME) 
									||changedFeature.equals(ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME)
									||changedFeature.equals(ClassEditorConstants.HARDWARE_RESOURCE_REF_NAME)
									||changedFeature.equals(ClassEditorConstants.MIB_RESOURCE_REF_NAME);
			
						if(!isRes)
							continue;
						
						List resObjList = (List) EcoreUtils.getValue(dependent.eContainer(), 
											ClassEditorConstants.ALARM_RESOURCE);
						
						EObject oldResObj = (EObject) n.getOldValue();
						
						String oldResName = (String) EcoreUtils.getValue(oldResObj, ClassEditorConstants.ALARM_RESOURCE_NAME);
						
						String resName = (String) EcoreUtils.getValue(dependent, ClassEditorConstants.ALARM_RESOURCE_NAME);
						
						if(resName.equals(oldResName)){
						
							resObjList.remove(dependent);
						}
					}
					else if (dependentType.equals(ClassEditorConstants.ALARM_ALARMRULE)) {

							if (changedFeature.equals(ClassEditorConstants.ALARM_ALARMPROFILE)) {
	
								String deletedAlarmID = (String) EcoreUtils.getValue((EObject) n.getOldValue(),
										ClassEditorConstants.ALARM_ID);
	
								List associatedIDs = (List) EcoreUtils
											.getValue(dependent, featureNames[0]);
	
								if (associatedIDs.contains(deletedAlarmID)) {
	
									int index = associatedIDs.indexOf(deletedAlarmID);
								
									associatedIDs.remove(index);
	
								}
							}
					}
					
					else if(dependentType.equals(ClassEditorConstants.ALARM_ALARMOBJ)){
						
						if (changedFeature.equals(ClassEditorConstants.ALARM_ALARMPROFILE)) {
							
							String deletedAlarmID = (String) EcoreUtils.getValue((EObject) n.getOldValue(),
									featureNames[0]);
				
							String alarmID	= (String) EcoreUtils.getValue(dependent, 
									featureNames[0]);
							
							if(deletedAlarmID.equals(alarmID)){
								
								List alarmList = (List) EcoreUtils.getValue(dependent.eContainer(), 
														dependentType);;
								
								alarmList.remove(dependent);
								
							}
						}
						
					}
					
					break;
				
				case Notification.REMOVE_MANY:
					
					List delObjList = (List) n.getOldValue();
					
					for(int objIndex = 0; objIndex < delObjList.size(); objIndex++){
						
						if (dependentType.equals(ClassEditorConstants.ALARM_RESOURCE)) {
							
							boolean isRes = changedFeature.equals(ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME) 
										||changedFeature.equals(ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME)
										||changedFeature.equals(ClassEditorConstants.HARDWARE_RESOURCE_REF_NAME)
										||changedFeature.equals(ClassEditorConstants.MIB_RESOURCE_REF_NAME);
							
							if(!isRes)
								continue;
							
							List resObjList = (List) EcoreUtils.getValue(dependent.eContainer(), 
												ClassEditorConstants.ALARM_RESOURCE);
							
							EObject oldResObj = (EObject) delObjList.get(objIndex);
							
							String oldResName = (String) EcoreUtils.getValue(oldResObj, ClassEditorConstants.ALARM_RESOURCE_NAME);
							
							String resName = (String) EcoreUtils.getValue(dependent, ClassEditorConstants.ALARM_RESOURCE_NAME);
							
							if(resName.equals(oldResName)){
							
								resObjList.remove(dependent);
							}
						}
						else if (dependentType.equals(ClassEditorConstants.ALARM_ALARMRULE)) {

								if (changedFeature.equals(ClassEditorConstants.ALARM_ALARMPROFILE)) {
		
									String deletedAlarmID = (String) EcoreUtils.getValue(
											(EObject) delObjList.get(objIndex),
											ClassEditorConstants.ALARM_ID);
		
									List associatedIDs = (List) EcoreUtils
												.getValue(dependent, featureNames[0]);
		
									if (associatedIDs.contains(deletedAlarmID)) {
		
										int index = associatedIDs.indexOf(deletedAlarmID);
									
										associatedIDs.remove(index);
		
									}
								}
						}
						
						else if(dependentType.equals(ClassEditorConstants.ALARM_ALARMOBJ)){
							
							if (changedFeature.equals(ClassEditorConstants.ALARM_ALARMPROFILE)) {
								
								String deletedAlarmID = (String) EcoreUtils.getValue(
										(EObject) delObjList.get(objIndex),
										featureNames[0]);
					
								String alarmID	= (String) EcoreUtils.getValue(dependent, 
										featureNames[0]);
								
								if(deletedAlarmID.equals(alarmID)){
									
									List alarmList = (List) EcoreUtils.getValue(dependent.eContainer(), 
															dependentType);;
									
									alarmList.remove(dependent);
									
								}
							}
							
						}
					}
					
					break;
				
				}// End of Switch for notification type
			}
			
		} catch (Exception e) {
			LOG.error("Error while invoking handler", e);

		}
	}
}
