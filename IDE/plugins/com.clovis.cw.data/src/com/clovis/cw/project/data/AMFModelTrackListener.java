package com.clovis.cw.project.data;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.xmi.impl.XMLResourceImpl;


/**
 * Listener to handle AMF config changes
 * @author pushparaj
 *
 */
public class AMFModelTrackListener extends AdapterImpl{

	ProjectDataModel _projectDataModel;
	public AMFModelTrackListener(ProjectDataModel dataModel) {
		_projectDataModel = dataModel;
	}
	/**
	 * @param notification
	 *            Notofication event
	 */
	public void notifyChanged(Notification notification) {
		if (notification.isTouch()
				|| notification.getNotifier() instanceof XMLResourceImpl)
			return;
		switch (notification.getEventType()) {
		case Notification.SET:
		case Notification.ADD:
		case Notification.REMOVE:
		case Notification.ADD_MANY:
		case Notification.REMOVE_MANY:
			_projectDataModel.setModified(true);
			break;
		}
	}
}
