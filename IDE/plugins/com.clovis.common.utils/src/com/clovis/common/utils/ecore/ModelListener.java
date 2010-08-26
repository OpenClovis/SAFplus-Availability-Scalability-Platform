/*
 * @(#) $RCSfile: ModelListener.java,v $
 * $Revision: #2 $ $Date: 2007/01/03 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.common.utils.ecore;

import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;

/**
 * 
 * @author shubhada
 * 
 * Model Listener class to listen to add/remove in the editor model
 * list and update on Actual Model/View Model List.
 * 
 * This class is introduced only because of the extra heirarchy introduced
 * in editor schema
 * 
 */
public class ModelListener extends AdapterImpl
{
    private Model _model = null;
    /**
     * Constructor
     * 
     * @param model - Model to be updated upon
     * the change (addition/deletion) in list
     */
    public ModelListener(Model model)
    {
        _model = model;
    }
    /**
     * @param notification - Notification
     */
    public void notifyChanged(Notification notification)
    {
        switch (notification.getEventType()) {
        case Notification.REMOVING_ADAPTER:
            break;
        case Notification.ADD:
        	// Here we have assumption that there wont be two EReferences 
        	// referring same EClass
            Object newVal = notification.getNewValue();
            if (newVal instanceof EObject) {
                EObject eobj = (EObject) newVal;
                EObject infoObj = (EObject) _model.getEList().get(0);
                List refList = infoObj.eClass().getEAllReferences();
                for (int i = 0; i < refList.size(); i++) {
                    EReference ref = (EReference) refList.get(i);
                    if (eobj.eClass().getName().equals(ref.
                            getEReferenceType().getName())) {
                        Object val = infoObj.eGet(ref);
                        if (ref.getUpperBound() == -1 || ref.getUpperBound() > 1) {
                        	if (val != null && !((List) val).contains(eobj)) {
                        		((List) val).add(eobj);
                        	}
                        } else if (ref.getUpperBound() == 1) {
                            infoObj.eSet(ref, eobj);
                        }
                        
                    }
                }
            }
            break;

        case Notification.ADD_MANY:
            List objs = (List) notification.getNewValue();
            for (int i = 0; i < objs.size(); i++) {
                if (objs.get(i) instanceof EObject) {
                    EObject eobj = (EObject) objs.get(i);
                    EObject infoObj = (EObject) _model.getEList().get(0);
                    List refList = infoObj.eClass().getEAllReferences();
                    for (int j = 0; j < refList.size(); j++) {
                        EReference ref = (EReference) refList.get(j);
                        if (eobj.eClass().getName().equals(ref.
                                getEReferenceType().getName())) {
                            Object val = infoObj.eGet(ref);
                            if (ref.getUpperBound() == -1 || ref.getUpperBound() > 1) {
                            	if (val != null && !((List) val).contains(eobj)) {
                            		((List) val).add(eobj);
                            	}
                            } else if (ref.getUpperBound() == 1) {
                                infoObj.eSet(ref, eobj);
                            }
                        }
                    }
                }
            }
            break;
        case Notification.REMOVE:
            Object obj = notification.getOldValue();
            if (obj instanceof EObject) {
                EObject eobj = (EObject) obj;
                EObject infoObj = (EObject) _model.getEList().get(0);
                List refList = infoObj.eClass().getEAllReferences();
                for (int i = 0; i < refList.size(); i++) {
                    EReference ref = (EReference) refList.get(i);
                    if (eobj.eClass().getName().equals(ref.
                            getEReferenceType().getName())) {
                        Object val = infoObj.eGet(ref);
                        if (val != null) {
                            if (val instanceof List) {
                                ((List) val).remove(eobj);
                            } else if (val instanceof EObject) {
                                infoObj.eUnset(ref);
                            }
                        }
                    }
                }
            }
            break;
        case Notification.REMOVE_MANY:
            objs = (List) notification.getOldValue();
            for (int i = 0; i < objs.size(); i++) {
                if (objs.get(i) instanceof EObject) {
                    EObject eobj = (EObject) objs.get(i);
                    EObject infoObj = (EObject) _model.getEList().get(0);
                    List refList = infoObj.eClass().getEAllReferences();
                    for (int j = 0; j < refList.size(); j++) {
                        EReference ref = (EReference) refList.get(j);
                        if (eobj.eClass().getName().equals(ref.
                                getEReferenceType().getName())) {
                            Object val = infoObj.eGet(ref);
                            if (val != null) {
                                if (val instanceof List) {
                                    ((List) val).remove(eobj);
                                } else if (val instanceof EObject) {
                                    infoObj.eUnset(ref);
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
    }
    
}
