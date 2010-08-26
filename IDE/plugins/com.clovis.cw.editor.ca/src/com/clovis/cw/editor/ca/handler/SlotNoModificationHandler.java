/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/handler/SlotNoModificationHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.handler;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.MessageDialog;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * Handler to update the dependent values when the Number of slots
 * field in the Chassis Resource changes
 */
public class SlotNoModificationHandler extends NotificationHandler
{
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
            Object dependentObj, String [] featureNames,
            String [] referencePaths, ProjectDataModel pdm)
    {
        //dependenObj is always a list, So take the first element from it.
        if (((List) dependentObj).isEmpty()) {
            return;
        }
        try {
        EObject dependent = (EObject) ((List) dependentObj).get(0);
        if (dependent == null)
        {
            return;
        }
        int maxSlots = ((Integer) EcoreUtils.getValue((EObject) changedObj,
                ClassEditorConstants.CHASSIS_NUM_SLOTS)).intValue();
        if (dependent.eClass().getName().equals("slot")) {
            EReference slotRef = (EReference) dependent.eClass().
                getEStructuralFeature(featureNames[0]);
            List slotList = (List) dependent.eGet(slotRef);
            HashMap SlotObjectNumberMap = new HashMap();
            
            boolean isDependentsToBeDeleted = false;
            String msg = "";
            for (int i = 0; i < slotList.size(); i++) {
                EObject slotObj = (EObject) slotList.get(i);
                int slot = ((Integer) EcoreUtils.getValue(slotObj, "slotNumber")).
                    intValue();
                if (maxSlots < slot) {
                    isDependentsToBeDeleted = true;
                    msg = msg + String.valueOf(slot) + " ";
                }
            }
            if (isDependentsToBeDeleted) {
                MessageDialog.openWarning(null,
                        "Slot Validations", "Configuration for slot number " + msg
                        + "in Node Admission Control will be deleted");
            }
            
            Iterator iterator = slotList.iterator();
            while (iterator.hasNext()) {
                EObject slot = (EObject) iterator.next();
                int slotNumber = ((Integer) EcoreUtils.getValue(
                        slot, "slotNumber")).intValue();
                if (slotNumber > maxSlots) {
                    iterator.remove();
                    SlotObjectNumberMap.remove(slot);
                } else {
                    SlotObjectNumberMap.put(slot, String.valueOf(slotNumber));
                }
            }
            for (int i = 0; i < maxSlots; i++) {
                EObject slotObj = EcoreUtils.createEObject(
                        slotRef.getEReferenceType(), true);
                EcoreUtils.setValue(slotObj, "slotNumber", String.valueOf(i + 1));
                if (!SlotObjectNumberMap.containsValue(String.valueOf(i + 1))) {
                    slotList.add(slotObj);
    //                  Select all the node types for this slot initially
                    EReference slotsRef = (EReference) slotObj.eClass().
                        getEStructuralFeature("classTypes");
                    EObject nodeTypesObj = (EObject) slotObj.eGet(slotsRef);
                    if (nodeTypesObj == null) {
                        nodeTypesObj = EcoreUtils.createEObject(slotsRef.getEReferenceType(), true);
                        slotObj.eSet(slotsRef, nodeTypesObj);
                    }
                    EReference classTypeRef = (EReference) nodeTypesObj.eClass().
                        getEStructuralFeature("classType");
                    List nodeTypeList = (List) nodeTypesObj.eGet(classTypeRef);
                    List nodesList = ComponentDataUtils.getNodesList(pdm.getComponentModel().getEList()); 
                    for (int j = 0; j < nodesList.size(); j++) {
                        EObject nodeObj = (EObject) nodesList.get(j);
                        EObject eobj = EcoreUtils.createEObject(
                                classTypeRef.getEReferenceType(), true);
                        EcoreUtils.setValue(eobj, "name", EcoreUtils.getName(nodeObj));
                        nodeTypeList.add(eobj);
                    }
                } 
            }
        } else if (dependent.eClass().getName().equals("LinkType")) {
            boolean isDependentsToBeDeleted = false;
            String msg = "";
            List locationList = (List) EcoreUtils.getValue(dependent, featureNames[0]);
            if(locationList == null)
            	return;

            for (int i = 0; i < locationList.size(); i++) {
                EObject locationObj = (EObject) locationList.get(i);
                int slot = ((Integer) EcoreUtils.getValue(locationObj, "slot")).
                    intValue();
                if (maxSlots < slot) {
                    isDependentsToBeDeleted = true;
                    msg = msg + String.valueOf(slot) + " ";
                }
            }
            if (isDependentsToBeDeleted) {
                MessageDialog.openWarning(null,
                        "Slot Validations", "Configuration for slot number " + msg
                        + "in IOC will be deleted");
            }
            
            Iterator iterator = locationList.iterator();
            while (iterator.hasNext()) {
                EObject locationObj = (EObject) iterator.next();
                int slot = ((Integer) EcoreUtils.getValue(locationObj, "slot")).
                    intValue();
                if (maxSlots < slot) {
                    iterator.remove();
                }
            }
           
        }
        } catch (Exception e) {
            LOG.error("Error while invoking handler", e);
        }
    }
}
