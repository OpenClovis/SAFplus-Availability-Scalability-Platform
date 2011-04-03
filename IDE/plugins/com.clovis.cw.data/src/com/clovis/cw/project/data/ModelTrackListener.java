/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.data/src/com/clovis/cw/project/data/ModelTrackListener.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.project.data;

import java.io.IOException;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.emf.ecore.xmi.impl.XMLResourceImpl;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;

public class ModelTrackListener extends AdapterImpl {
	private EPackage _package;

	private Resource _resource;

	private NotifyingList _trackList;

	private List _addList, _removeList/*, _modifyList*/;
	
    private TrackingModel _trackingModel;
    
    private ProjectDataModel _projectDataModel;

	/**
	 * Constructor
	 * 
	 * @param pack
	 * @param resource
	 * @param list
	 */
	public ModelTrackListener(TrackingModel track, ProjectDataModel dataModel) {
		_trackingModel = track;
		_projectDataModel = dataModel;
		Model model = _trackingModel.getTrackModel();
		_package = model.getEPackage();
		_resource = model.getResource();
		_trackList = model.getEList();
		_addList = _trackingModel.getAddedList();
		_removeList = _trackingModel.getRemovedList();
		//_modifyList = _trackingModel.getModifiedList();
		readTrackList();
	}
	/**
	 * Reads track model
	 */
	private void readTrackList() {
		for (int i = 0; i < _trackList.size(); i++) {
			EObject obj = (EObject) _trackList.get(i);
			String changeType = (String) EcoreUtils.getValue(obj, TrackingModelConstants.CHANGE_TYPE);
			String rdn = (String) EcoreUtils.getValue(obj, ModelConstants.RDN_FEATURE_NAME);
			if(changeType.equals(TrackingModelConstants.ADD_TYPE)) {
				_addList.add(rdn);
			} else if(changeType.equals(TrackingModelConstants.REMOVE_TYPE)) {
				_removeList.add(rdn);
			} else if(changeType.equals(TrackingModelConstants.MODIFY_TYPE)) {
				//_modifyList.add(rdn);
			}
		}
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
			Object notifier = notification.getNotifier();
			if (notifier instanceof EObject) {
				if (!(notification.getOldValue() == notification.getNewValue())) {
					EObject obj = (EObject) notifier;
					boolean bol = Boolean.parseBoolean(EcoreUtils
							.getAnnotationVal(obj.eClass(), null,
									TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
					if (bol) {
						if(getRootObjectForTrack(obj) == obj) {
							String feature = ((EStructuralFeature)notification.getFeature()).getName();
							//if(feature.equalsIgnoreCase("name")) {
							if(feature.equals(EcoreUtils.getNameField(obj.eClass()))) {
								EObject root = getRootObjectForTrack(obj);
								String key = (String) EcoreUtils.getValue(root,
										ModelConstants.RDN_FEATURE_NAME);
								EObject newObj = EcoreUtils.createEObject((EClass) _package
										.getEClassifier(obj.eClass().getName()), false);
								EcoreUtils.setValue(newObj, TrackingModelConstants.OLD_NAME, notification.getOldStringValue());
								EcoreUtils.setValue(newObj, TrackingModelConstants.NEW_NAME, notification.getNewStringValue());
								EcoreUtils.setValue(newObj, ModelConstants.RDN_FEATURE_NAME, key);
								EcoreUtils.setValue(newObj, TrackingModelConstants.CHANGE_TYPE, TrackingModelConstants.MODIFY_TYPE);
								if(newObj.eClass().getEStructuralFeature("parent") != null) {
									EcoreUtils.setValue(newObj, "parent", EcoreUtils.getName(root.eContainer()));
								}
								_trackList.add(newObj);
								//if(!_modifyList.contains(key))
									//_modifyList.add(key);
								saveResource();
								
							} else {
								trackModifiedObject(obj);
							}
						} else {
							trackModifiedObject(obj);
						}
					}
				}
			}
			_projectDataModel.setModified(true);
			break;
		case Notification.ADD:
			if (notification.getNewValue() instanceof EObject) {
				EObject obj = (EObject) notification.getNewValue();
				EcoreUtils.addListener(obj, this, -1);
				boolean bol = Boolean.parseBoolean(EcoreUtils.getAnnotationVal(
						obj.eClass(), null, TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
				if (bol) {
					trackNewObject(obj);
				}
			} else if(notification.getNewValue() instanceof String) {
				if(notification.getNotifier() instanceof EObject) {
					EObject obj = (EObject) notification.getNotifier();
					boolean bol = Boolean.parseBoolean(EcoreUtils.getAnnotationVal(
							obj.eClass(), null, TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
					if (bol) {
						trackModifiedObject(obj);
					}
				}
			}
			_projectDataModel.setModified(true);
			break;
		case Notification.REMOVE:
			if (notification.getOldValue() instanceof EObject) {
				EObject obj = (EObject) notification.getOldValue();
				EcoreUtils.removeListener(obj, this, -1);
				boolean bol = Boolean.parseBoolean(EcoreUtils.getAnnotationVal(
						obj.eClass(), null, TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
				if (bol) {
					trackOldObject(notification);
				}
			} else if(notification.getOldValue() instanceof String) {
				if(notification.getNotifier() instanceof EObject) {
					EObject obj = (EObject) notification.getNotifier();
					boolean bol = Boolean.parseBoolean(EcoreUtils.getAnnotationVal(
							obj.eClass(), null, TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
					if (bol) {
						trackModifiedObject(obj);
					}
				}
			}
			_projectDataModel.setModified(true);
			break;

		case Notification.ADD_MANY:
			List objs = (List) notification.getNewValue();
			for (int i = 0; i < objs.size(); i++) {
				if (objs.get(i) instanceof EObject) {
					EObject obj = (EObject) objs.get(i);
					EcoreUtils.addListener(obj, this, -1);
					boolean bol = Boolean.parseBoolean(EcoreUtils
							.getAnnotationVal(obj.eClass(), null,
									TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
					if (bol) {
						trackNewObject(obj);
					}
				} else if(objs.get(i) instanceof String) {
					if(notification.getNotifier() instanceof EObject) {
						EObject obj = (EObject) notification.getNotifier();
						boolean bol = Boolean.parseBoolean(EcoreUtils.getAnnotationVal(
								obj.eClass(), null, TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
						if (bol) {
							trackModifiedObject(obj);
						}
					}
				}
			}
			_projectDataModel.setModified(true);
			break;
		case Notification.REMOVE_MANY:
			List objects = (List) notification.getOldValue();
			if (objects != null) {
				for (int i = 0; i < objects.size(); i++) {
					if (objects.get(i) instanceof EObject) {
						EObject obj = (EObject) objects.get(i);
						EcoreUtils.removeListener(obj, this, -1);
						boolean bol = Boolean.parseBoolean(EcoreUtils
								.getAnnotationVal(obj.eClass(), null,
										TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
						if (bol) {
							trackOldObject(notification);
						}
					} else if(objects.get(i) instanceof String) {
						if(notification.getNotifier() instanceof EObject) {
							EObject obj = (EObject) notification.getNotifier();
							boolean bol = Boolean.parseBoolean(EcoreUtils.getAnnotationVal(
									obj.eClass(), null, TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
							if (bol) {
								trackModifiedObject(obj);
							}
						}
					}
				}
			}
			_projectDataModel.setModified(true);
			break;
		}
	}

	/**
	 * Update changes list
	 * 
	 * @param obj
	 *            EObject
	 */
	private void trackNewObject(EObject obj) {
		EObject root = getRootObjectForTrack(obj);
		if (root == obj) {
			String key = (String) EcoreUtils.getValue(root,
					ModelConstants.RDN_FEATURE_NAME);
			if (!_addList.contains(key)) {
				_trackList.add(createTrackObject(root, key, TrackingModelConstants.ADD_TYPE));
				_addList.add(key);
				saveResource();
			}
		} else {
			trackModifiedObject(obj);
		}
	}

	/**
	 * Update changes list
	 * 
	 * @param obj
	 *            EObject
	 */
	private void trackOldObject(Notification notification) {
		if (notification.getNotifier() instanceof EObject) {
			EObject notifier = (EObject) notification.getNotifier();
			EObject obj = null;
			if (notification.getOldValue() instanceof EObject) {
				obj = (EObject) notification.getOldValue();
			} else {
				obj = notifier;
			}
			boolean bol = Boolean.parseBoolean(EcoreUtils.getAnnotationVal(
					notifier.eClass(), null, TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
			// EObject root = null;
			if (bol) {
				trackModifiedObject(notifier);
			} else {
				String key = (String) EcoreUtils.getValue(obj,
						ModelConstants.RDN_FEATURE_NAME);
				if (!_removeList.contains(key)) {
					_trackList.add(createTrackObject(obj, key, TrackingModelConstants.REMOVE_TYPE));
					_removeList.add(key);
					saveResource();
				}
			}
		} else if (notification.getOldValue() instanceof EObject) {
			EObject obj = (EObject) notification.getOldValue();
			String key = (String) EcoreUtils.getValue(obj,
					ModelConstants.RDN_FEATURE_NAME);
			if (!_removeList.contains(key)) {
				_trackList.add(createTrackObject(obj, key, TrackingModelConstants.REMOVE_TYPE));
				_removeList.add(key);
				saveResource();
			}
		}
	}
	/**
	 * Update changes list
	 * 
	 * @param obj
	 *            EObject
	 */
	private void trackModifiedObject(EObject obj) {
		EObject root = getRootObjectForTrack(obj);
		String key = (String) EcoreUtils.getValue(root,
				ModelConstants.RDN_FEATURE_NAME);
		//if (!_modifyList.contains(key)) {
			_trackList.add(createTrackObject(root, key, TrackingModelConstants.MODIFY_TYPE));
			//_modifyList.add(key);
			saveResource();
		//}
	}
	/**
	 * Creates and returns track Object
	 * @param root EObject
	 * @param key CWKey
	 * @param type modification Type
	 * @return EObject
	 */
	private EObject createTrackObject(EObject obj, String key, String type) {
		String name = EcoreUtils.getName(obj);
		EObject newObj = EcoreUtils.createEObject((EClass) _package
				.getEClassifier(obj.eClass().getName()), false);
		EcoreUtils.setValue(newObj, TrackingModelConstants.OLD_NAME, name);
		EcoreUtils.setValue(newObj, TrackingModelConstants.NEW_NAME, name);
		EcoreUtils.setValue(newObj, ModelConstants.RDN_FEATURE_NAME, key);
		EcoreUtils.setValue(newObj, TrackingModelConstants.CHANGE_TYPE, type);
		if(newObj.eClass().getEStructuralFeature("parent") != null) {
			EcoreUtils.setValue(newObj, "parent", EcoreUtils.getName(obj.eContainer()));
		}
		return newObj;
	}
	/**
	 * Saves Resource
	 *
	 */
	private void saveResource() {
		try {
			_resource.save(null);
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	/**
	 * Returns root objec twhich is required for maintaining model changes
	 * 
	 * @param obj
	 *            EObject
	 * @return root object
	 */
	private EObject getRootObjectForTrack(EObject obj) {
		EObject root = null;
		boolean bol = false;
		do {
			bol = false;
			root = obj;
			obj = obj.eContainer();
			if (obj != null) {
				bol = Boolean.parseBoolean(EcoreUtils.getAnnotationVal(obj
						.eClass(), null, TrackingModelConstants.CHANGE_TRACK_ANNOTATION));
				if (bol
						&& _package.getEClassifier(root.eClass().getName()) != null) {
					return root;
				}
			}
		} while (bol);
		return root;
	}
}
