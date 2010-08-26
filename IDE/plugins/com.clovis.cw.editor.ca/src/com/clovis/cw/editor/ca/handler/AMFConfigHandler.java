/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/handler/AMFConfigHandler.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.handler;

import java.io.File;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.MessageDialog;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.project.data.NotificationHandler;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * Handler to handle the change in types of Instances. Types
 * are defined in component editor
 */
public class AMFConfigHandler extends NotificationHandler
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
    	try {
        List dependentObjs = (List) dependentObj;
        for (int i = 0; i < dependentObjs.size(); i++) {
             EObject dependent = (EObject) dependentObjs.get(i);
             EStructuralFeature eAttr = (EStructuralFeature) n.getFeature();
             String changedFeature = null;
             if( eAttr != null )
            	 changedFeature = eAttr.getName();
             
             if (n.getEventType() == Notification.SET)
             {      
                 if (changedFeature != null && changedFeature.equals(
                         ComponentEditorConstants.NODE_CLASS_TYPE)) {
                	 if( dependent.eClass().getName().equals(SafConstants.CPM_CONFIGLIST_NAME ))
                	 {            		 
                		 // check if this is for the correct node type
                         String nodeType = EcoreUtils.getName((EObject)changedObj);
                         String cpmNodeType = (String)EcoreUtils.getValue(dependent, "nodeType");
                         if( nodeType.equals(cpmNodeType))
                         {
                        	 EEnumLiteral nodeClass = (EEnumLiteral)EcoreUtils.
                                 getValue((EObject)changedObj,
                                         ComponentEditorConstants.NODE_CLASS_TYPE);
    		            	 if( nodeClass.getName().equals("CL_AMS_NODE_CLASS_A") || nodeClass.getName().equals("CL_AMS_NODE_CLASS_B"))
    		             		EcoreUtils.setValue(dependent, "cpmType", "GLOBAL");
    		                 else
    		                 	EcoreUtils.setValue(dependent, "cpmType", "LOCAL");
                         }
                	 }
                 } else {
                     String type = (String) EcoreUtils.getValue(dependent, featureNames[0]);
                     if (n.getOldValue() != null && n.getOldValue().equals(type)) {
                         String typeName = EcoreUtils.getName((EObject) changedObj);
                         EcoreUtils.setValue(dependent, featureNames[0], typeName);
                     }
                 }
             }
             // check if notification is for delete, then warn
             //the user that all the AMF configuration which will
             // refer to deleted object will be deleted.
             else if (n.getEventType() == Notification.REMOVE
                     || n.getEventType() == Notification.REMOVE_MANY)
             {
                Object oldVal = n.getOldValue();
                if (oldVal != null && oldVal instanceof EObject) {
                    EObject eobj = (EObject) oldVal;
                    String changedType = EcoreUtils.getName(eobj);
                    String type = EcoreUtils.getValue(dependent, featureNames[0]).toString();
                    String dependentName = EcoreUtils.getName(dependent);
                    if (dependentName == null) {
                        String label = EcoreUtils.getAnnotationVal(dependent.eClass(), null,
                                "label");
                        if (label != null) {
                            dependentName = label;
                        } else {
                            dependentName = dependent.eClass().getName();
                        }
                    }
                    if (changedType.equals(type)) {
                        MessageDialog.openWarning(null, "Updation warning",
                                "Configuration done for '" + dependentName
                                + "' referring object '"+ changedType + "' will be deleted");
                        EObject container = dependent.eContainer();
                        Object val = container.eGet(dependent.eContainingFeature());
                        if (val instanceof List) {
                            ((List) val).remove(dependent);
                        } else if (val instanceof EObject) {
                            ((EObject) val).eUnset(dependent.eContainingFeature());
                        }
                    }
                    if (dependent.eClass().getName().equals(
                    		SafConstants.NODE_INSTANCELIST_ECLASS)) {
                    	try {
                    		String nodeName = EcoreUtils.getName(dependent);
        					String dataFilePath = pdm.getProject().getLocation().toOSString()
        							+ File.separator
        							+ ICWProject.CW_PROJECT_CONFIG_DIR_NAME
        							+ File.separator + nodeName + "_"
        							+ SafConstants.RT_SUFFIX_NAME;
        					File file = new File(dataFilePath);
        					file.delete();
        				} catch (Exception e) {
        					LOG.error("Error Deleting Node RT Resource.", e);
        				}
                    }
                    
                }
                
             }
        }
    	
        } catch (Exception e) {
            LOG.error("Error while invoking handler", e);
        }
    }

}
