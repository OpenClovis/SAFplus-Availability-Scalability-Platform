/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/MappingModelTrackListener.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.project.data;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.resource.Resource;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;

/**
 * Listener to handle mapping model changes
 * @author pushparaj
 *
 */
public abstract class MappingModelTrackListener extends AdapterImpl {

	private static final String LINK_SOURCE = "linkSource";
	protected EPackage _package;
	protected Resource _resource;
	protected NotifyingList _trackList;
	protected List _addList, _removeList, _modifyList;
	protected ProjectDataModel _dataModel;
	protected List _resourcesList = new ArrayList();
	
	/**
	 * Constructor
	 * 
	 * @param track
	 */
	public MappingModelTrackListener(ProjectDataModel dataModel) {
		_dataModel = dataModel;
		TrackingModel trackingModel = dataModel.getTrackingModel();
		Model model = trackingModel.getTrackModel();
		_package = model.getEPackage();
		_resource = model.getResource();
		_trackList = model.getEList();
		_addList = trackingModel.getAddedList();
		_removeList = trackingModel.getRemovedList();
		_modifyList = trackingModel.getModifiedList();
	}
	protected abstract void createResourcesList();
	
	/** Update modelchanges list
	 * @param name
	 */
	protected void trackModifiedObject(String name) {
		for (int i = 0; i < _resourcesList.size(); i++) {
			EObject obj = (EObject) _resourcesList.get(i);
			String resName = (String) EcoreUtils.getName(obj);
			String rdn = (String) EcoreUtils.getValue(obj, ModelConstants.RDN_FEATURE_NAME);
			if(resName.equals(name)) {
				if(/*!_addList.contains(rdn) && */!_modifyList.contains(rdn)) {
					EObject newObj = EcoreUtils.createEObject((EClass) _package
							.getEClassifier(obj.eClass().getName()), false);
					EcoreUtils.setValue(newObj, TrackingModelConstants.OLD_NAME, name);
					EcoreUtils.setValue(newObj, TrackingModelConstants.NEW_NAME, name);
					EcoreUtils.setValue(newObj, ModelConstants.RDN_FEATURE_NAME, rdn);
					EcoreUtils.setValue(newObj, TrackingModelConstants.CHANGE_TYPE, TrackingModelConstants.MODIFY_TYPE);
					_trackList.add(newObj);
					_modifyList.add(rdn);
					saveResource();
					return;
				}
			}
		}
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
				
			}
			break;
		case Notification.ADD:
			if (notification.getNewValue() instanceof EObject) {
				EObject obj = (EObject) notification.getNewValue();
				String name = (String) EcoreUtils.getValue(obj, LINK_SOURCE);
				trackModifiedObject(name);
			} else if(notification.getNewValue() instanceof String) {
				EObject obj = (EObject) notification.getNotifier();
				String name = (String) EcoreUtils.getValue(obj, LINK_SOURCE);
				trackModifiedObject(name);
			}
			break;
		case Notification.REMOVE:
			if (notification.getOldValue() instanceof EObject) {
				EObject obj = (EObject) notification.getOldValue();
				String name = (String) EcoreUtils.getValue(obj, LINK_SOURCE);
				trackModifiedObject(name);
			} else if(notification.getOldValue() instanceof String) {
				EObject obj = (EObject) notification.getNotifier();
				String name = (String) EcoreUtils.getValue(obj, LINK_SOURCE);
				trackModifiedObject(name);
			}
			break;
		case Notification.ADD_MANY:
			List objs = (List) notification.getNewValue();
			for (int i = 0; i < objs.size(); i++) {
				if (objs.get(i) instanceof EObject) {
					EObject obj = (EObject) objs;
					String name = (String) EcoreUtils.getValue(obj, LINK_SOURCE);
					trackModifiedObject(name);
				} else if(objs.get(i) instanceof String) {
					EObject obj = (EObject) notification.getNotifier();
					String name = (String) EcoreUtils.getValue(obj, LINK_SOURCE);
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
						String name = (String) EcoreUtils.getValue(obj, LINK_SOURCE);
						trackModifiedObject(name);
					} else if(objects.get(i) instanceof String) {
						EObject obj = (EObject) notification.getNotifier();
						String name = (String) EcoreUtils.getValue(obj, LINK_SOURCE);
						trackModifiedObject(name);
					}
				}
			}
			break;
		}
	}
	/**
	 * Saves Resource
	 */
	protected void saveResource() {
		try {
			_resource.save(null);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}
