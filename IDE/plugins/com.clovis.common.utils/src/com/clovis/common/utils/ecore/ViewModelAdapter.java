/*
 * @(#) $RCSfile: Log.java,v $
 * $Revision: #3 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2002 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ecore/ViewModelAdapter.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * @(#) $RCSfile: Log.java,v $
 * $Revision: #3 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2002 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.common.utils.ecore;

import java.util.Map;
import java.util.HashSet;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.xmi.impl.XMIResourceImpl;

/**
 * Listener for changes to actual Model.
 */
class ViewModelAdapter extends AdapterImpl
{
    private final ViewModel _viewModel;
    private HashSet _notificationsProcessed = new HashSet();
    /**
     * Constructor.
     * @param vm ViewModel
     */
    public ViewModelAdapter(ViewModel vm)
    {
        _viewModel = vm;
    }
    /**
     * Its a callback method
     * @param msg Notification object.
     */
    public void notifyChanged(Notification msg)
    {
        Map modelToViewMap = _viewModel.getModelToViewMap();
        Map viewToModelMap = _viewModel.getViewToModelMap();
        /*
         *
         * 1. keeping notificationsProcessed for the time being
         *    to handle duplicate Notifications
         * 2. Instead of "_isViewModelSaveTriggered" flag, use
         *    object comparison to reject the notification.
         *    ViewModel save() results in Notification from Model,
         *    which needs to be rejected by the view model
         *    instance that got saved.
         */
        try {
            //Event Type
            int eventType = msg.getEventType();
            //Position of change IF feature is List based
            int position = msg.getPosition();
            // Obj representing the feature of the Notifier that changed
            EStructuralFeature feature = (EStructuralFeature) msg.getFeature();
            // Notifier Object
            Object notifier = msg.getNotifier();
            // Value of Notifier's feature after change
            Object newValue = msg.getNewValue();
            //Value of Notifier's feature before change
            Object oldValue = msg.getOldValue();
            // Add/Remove the listener to the newly added/removed object(s)
            switch (eventType) {

            case Notification.ADD:
            case Notification.REMOVE:
            case Notification.ADD_MANY:
            case Notification.REMOVE_MANY:
                
                
                if (eventType == Notification.ADD) {
                    if (newValue instanceof EObject) {
                    	EcoreUtils.addListener(newValue, this, -1);
                       
                    } 

                } else if (eventType == Notification.REMOVE) {
                	if (oldValue instanceof EObject) {
                		EcoreUtils.removeListener(oldValue, this, -1);
                		
                	}
                } else if (eventType == Notification.ADD_MANY) {
                    List newObjects = (List) newValue;
                    for (int i = 0; i < newObjects.size(); i++) {
                       if (newObjects.get(i) instanceof EObject) {
                    	   EcoreUtils.addListener(newObjects.get(i), this, -1); 
                       }
                    }
                } else if (eventType == Notification.REMOVE_MANY) {
                    List removedObjects = (List) oldValue;
                    for (int i = 0; i < removedObjects.size(); i++) {
                       if (removedObjects.get(i) instanceof EObject) {
                        EObject modelObj = (EObject) removedObjects.get(i);
                        EcoreUtils.removeListener(modelObj, this, -1);
                       }
                    }
                }
                break;
            }
            // Save the model changes to the corresponding view model objects
            if (!_viewModel.isSaving()
                && !_notificationsProcessed.contains(msg)) {
                switch (eventType) {
                case Notification.SET:
                    if (notifier instanceof EObject) {
                        EObject vmObject = (EObject) modelToViewMap
                                .get(notifier);
                        if (vmObject != null) {
	                        Object value = vmObject.eGet(feature);
	                        if (value instanceof EList) {
	                            ((EList) value).set(position, newValue);
	                        } else {
                               if (newValue instanceof EEnumLiteral) {
                                   vmObject.eSet(feature, newValue);
                               } else if (newValue instanceof EObject) {
                                   vmObject.eSet(feature, EcoreCloneUtils.cloneEObject((EObject) newValue,
                                        _viewModel.getViewToModelMap(), _viewModel.getModelToViewMap()));
                               } else {
                                   vmObject.eSet(feature, newValue);
                               }
                        }
                        }
                    }
                    break;

                case Notification.ADD:
                case Notification.REMOVE:
                case Notification.ADD_MANY:
                case Notification.REMOVE_MANY:
                    EList vmList = null;
                    if (notifier instanceof EObject) {
                        EObject notifierObj = (EObject) notifier;
                        EObject vmObj = (EObject) modelToViewMap
                                .get(notifierObj);
                        vmList = (EList) vmObj.eGet(feature);
                    } else if (notifier instanceof XMIResourceImpl) {
                        // We need to act on the top level List
                        vmList = _viewModel.getEList();
                    }

                    if (vmList != null) {
	                    if (eventType == Notification.ADD) {
	                        if (newValue instanceof EObject) {
	                        	EcoreUtils.addListener(newValue, this, -1);
	                            vmList.add(EcoreCloneUtils.cloneEObject(
	                                    (EObject) newValue, viewToModelMap,
	                                    modelToViewMap));
	                        } else {
	                            vmList.add(newValue);
	                        }
	
	                    } else if (eventType == Notification.REMOVE) {
	                    	if (oldValue instanceof EObject) {
	                    		EcoreUtils.removeListener(oldValue, this, -1);
	                    		vmList.remove(modelToViewMap.get(oldValue));
	                    	} else{
	                    		vmList.remove(oldValue);
	                    	}
	                    } else if (eventType == Notification.ADD_MANY) {
	                        List newObjects = (List) newValue;
	                        for (int i = 0; i < newObjects.size(); i++) {
	                           if (newObjects.get(i) instanceof EObject) {
	                        	   EcoreUtils.addListener(newObjects.get(i), this, -1); 
	                        	   vmList.add(EcoreCloneUtils.cloneEObject(
	                                    (EObject) newObjects.get(i),
	                                    viewToModelMap, modelToViewMap));
	                           } else {
	                        	   vmList.add(newObjects.get(i));
	                           }
	                        }
	                    } else if (eventType == Notification.REMOVE_MANY) {
	                        List removedObjects = (List) oldValue;
	                        for (int i = 0; i < removedObjects.size(); i++) {
	                           if (removedObjects.get(i) instanceof EObject) {
	                            EObject modelObj = (EObject) removedObjects.get(i);
	                            EcoreUtils.removeListener(modelObj, this, -1);
	                            vmList.remove(modelToViewMap.get(modelObj));
	                           } else{
	                       			vmList.remove(removedObjects.get(i));
	                       		}
	                        }
	                    }
                    }
                    break;
                }
                _notificationsProcessed.add(msg);
            }

        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }
}
