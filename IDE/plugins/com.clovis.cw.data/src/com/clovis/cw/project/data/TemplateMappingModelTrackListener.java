/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/TemplateMappingModelTrackListener.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.project.data;

import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;

import com.clovis.common.utils.ecore.EcoreUtils;

public class TemplateMappingModelTrackListener extends MappingModelTrackListener{

	private static final String TEMPLATE_GROUP_NAME = "templateGroupName";
	public TemplateMappingModelTrackListener(ProjectDataModel dataModel) {
		super(dataModel);
	}
	/**
	 * @see com.clovis.cw.project.data.MappingModelTrackListener#createResourcesList(com.clovis.cw.project.data.ProjectDataModel)
	 */
	protected void createResourcesList() {
		List caList = _dataModel.getComponentModel().getEList();
		EObject rootObject = (EObject) caList.get(0);
		List resources = (List) rootObject.eGet(rootObject.eClass().getEStructuralFeature("safComponent"));
		_resourcesList.addAll(resources);
		/*resources = (List) rootObject.eGet(rootObject.eClass().getEStructuralFeature("nonSAFComponent"));
		_resourcesList.addAll(resources);*/
	}
	/**
	 * @param notification
	 *            Notofication event
	 */
	public void notifyChanged(Notification notification) {
		if (notification.isTouch())
			return;
		createResourcesList();
		switch (notification.getEventType()) {
		case Notification.SET:
			Object notifier = notification.getNotifier();
			if (notifier instanceof EObject) {
				EStructuralFeature feature = (EStructuralFeature) notification.getFeature();
				if(feature.getName().equals(TEMPLATE_GROUP_NAME)) {
					String name = (String) EcoreUtils.getName((EObject) notifier);
					trackModifiedObject(name);
				}
			}
			break;
		case Notification.ADD:
			if (notification.getNewValue() instanceof EObject) {
				EObject obj = (EObject) notification.getNewValue();
				String name = (String) EcoreUtils.getName(obj);
				trackModifiedObject(name);
			}
			break;
		case Notification.REMOVE:
			if (notification.getOldValue() instanceof EObject) {
				EObject obj = (EObject) notification.getOldValue();
				String name = (String) EcoreUtils.getName(obj);
				trackModifiedObject(name);
			}
			break;
		case Notification.ADD_MANY:
			List objs = (List) notification.getNewValue();
			for (int i = 0; i < objs.size(); i++) {
				if (objs.get(i) instanceof EObject) {
					EObject obj = (EObject) objs;
					String name = (String) EcoreUtils.getName(obj);
					trackModifiedObject(name);
				}
			}
			break;
		case Notification.REMOVE_MANY:
			List objects = (List) notification.getOldValue();
			if (objects != null) {
				for (int i = 0; i < objects.size(); i++) {
					if (objects.get(i) instanceof EObject) {
						EObject obj = (EObject) objects.get(i);
						String name = (String) EcoreUtils.getName(obj);
						trackModifiedObject(name);
					}
				}
			}
			break;
		}
	}
}
