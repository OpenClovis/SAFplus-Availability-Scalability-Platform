/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/AutoCorrectAction.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.ActionContributionItem;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.MenuItem;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Widget;
import org.eclipse.ui.IWorkbenchPage;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.editor.EditorUtils;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.data.SubModelMapReader;
import com.clovis.cw.project.utils.FormatConversionUtils;
import com.clovis.cw.workspace.ProblemsView;
import com.clovis.cw.workspace.WorkspacePlugin;
import com.clovis.cw.workspace.utils.CombinationGenerator;
/**
 * 
 * @author shubhada
 * Auto Correct Action Class which provides various
 * options for auto correcting the validation problem
 */
public class AutoCorrectAction extends Action
implements ISelectionChangedListener
{
	private EObject _selProblemObj = null;
	private static IProject project = null;
	private static IMenuManager menuManager = null;
    private static Shell shell = null;
    private static HashMap actionObjectsMap = new HashMap();
    private static final Log  LOG = Log.getLog(WorkspacePlugin.getDefault());
    /**
     * Constructor
     * @param selObj
     */
    public AutoCorrectAction(EObject selObj)
    {
        _selProblemObj = selObj;
    }
	/**
	 * @param event - SelectionChangedEvent
	 */
	public void selectionChanged(SelectionChangedEvent event)
	{
        //System.out.println("source" + event.getSource());
		IMenuManager subMenuMgr = (IMenuManager) menuManager.find(
				ProblemsView.AUTOCORRECT_MENU_TEXT);
		ISelection selection = event.getSelection();
		_selProblemObj = (EObject) ((IStructuredSelection) selection).
			getFirstElement();
		subMenuMgr.removeAll();
		fillAutoCorrectionMenu(subMenuMgr);
		
	}
	/**
	 * Fills the auto correction options to the menu
	 * @param menuMgr - IMenuManager 
	 */
	private void fillAutoCorrectionMenu(IMenuManager menuMgr)
	{
		if (_selProblemObj != null) {
            List relatedObjects = (List) EcoreUtils.getValue(
                    _selProblemObj,"relatedObjects");
            EObject source = (EObject) EcoreUtils.getValue(
                    _selProblemObj, "source");
			int problemNumber = ((Integer) EcoreUtils.
					getValue(_selProblemObj, "problemNumber")).intValue();
			switch(problemNumber) {
			case 1: // Resource having 2 alarms of same probable cause
					List alarmList = (List) relatedObjects.get(1);
                    CombinationGenerator generator = new CombinationGenerator(
                            alarmList.size(), alarmList.size() - 1);
                    int [] indices;
                    while (generator.hasMore()) {
                        indices = generator.getNext();
                        Action action = getAction(alarmList, indices, "Dissociate the alarm ", "");
                        action.setId("a");
                        action.setToolTipText("Dissociate the other alarms which have duplicate probable cause, from associated alarms list of resource.");
                        menuMgr.add(action);
                    }
                    menuMgr.add(new Separator());
                    Action action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Open the Alarm Configuration");
                    action.setId("b");
                    action.setToolTipText("Opens the Alarm Configuration dialog");
                    menuMgr.add(action);
					break;
			case 2: // Resource has a associated alarm which is 
					//deleted from Alarm Profile configuration
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Dissociate invalid alarm");
                    action.setId("a");
                    action.setToolTipText("Dissociate invalid alarm");
                    menuMgr.add(action);
					break;
			case 3: //Alarm/Prov Service object has a associated DO which is 
				    //deleted from DO List of resource
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Dissociate invalid Device Object");
                    action.setId("a");
                    action.setToolTipText("Dissociate invalid Device Object");
                    menuMgr.add(action);
					break;
			case 4: 
					break;
			case 5: // Provisioning enabled on the resource 
					//without having any attributes to be provisioned.
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Disable Provisioning");
                    action.setId("a");
                    action.setToolTipText("Disables the provisioning on the resource");
                    menuMgr.add(action);
					break;
			case 6: // Alarm enabled on the resource without having any alarms
					//associated to the resource.
                    
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Disable Alarm Management");
                    action.setId("a");
                    action.setToolTipText("Disables the Alarm Management on the resource");
                    menuMgr.add(action);
                    
					break;
			case 7: // Isolated resource in the Resource Editor
                    if (!source.eClass().getName().equals(
                            ClassEditorConstants.CHASSIS_RESOURCE_NAME)) {
                        action = new AutoCorrectAction(_selProblemObj);
                        action.setText("Delete '" + EcoreUtils.getName(source)
                                + "'");
                        action.setId("a");
                        action.setToolTipText("Deletes the isolated resource");
                        menuMgr.add(action);
                    }
					break;
			case 8: // Invalid connection due to dependency on other connection(s)
                    EObject targetObj = (EObject) relatedObjects.get(0);
                    EObject connObj = (EObject) relatedObjects.get(1);
                    String connType = (String) EcoreUtils.getValue(connObj,
                            ComponentEditorConstants.CONNECTION_TYPE);
                    
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Delete " + connType + " from "
                            + EcoreUtils.getName(source) + " to "
                            + EcoreUtils.getName(targetObj));
                    action.setId("a");
                    action.setToolTipText("Deletes the invalid connection");
                    menuMgr.add(action);
					break;
			case 9: // Component image name is empty
                    String newImageName = (String) relatedObjects.get(0);
                    
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Change imageName to '" + newImageName + "'" );
                    action.setId("a");
                    action.setToolTipText("Changes the component image name to a unique name");
                    menuMgr.add(action);
					break;
			case 10: // Duplicate component image name
                    newImageName = (String) relatedObjects.get(0);
                    
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Change imageName to '" + newImageName + "'" );
                    action.setId("a");
                    action.setToolTipText("Changes the component image name to a unique name");
                    menuMgr.add(action);
					break;
			case 11: // Component does not have associated resource(s)
                    /*action = new AutoCorrectAction();
                    action.setText("Change imageName to '" + newImageName + "'" );
                    action.setId("a");
                    action.setToolTipText("Changes the component image name to a unique name");
                    menuMgr.add(action);*/
					break;
			case 12: // Associated resource does not exist in 
					//list of resources of resource editor
                    String key = (String) relatedObjects.get(0);
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Dissociate the resource with key '" + key + "'");
                    action.setId("a");
                    action.setToolTipText("Dissociates the invalid resource from component");
                    menuMgr.add(action);
					break;
			case 13: // Proxied component doesn't belongs to proper SG and Node hierarchy
                    break;
			case 14: // One of associated resource has provisioning/alarm
					 //enabled with associated device object(s) but HAL
					 //library is not selected
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Enable the HAL library");
                    action.setId("a");
                    action.setToolTipText("Enables the HAL library on the component EO");
                    menuMgr.add(action);
					break;
			case 15: // One of associated resources has provisioning /alarm
					 //enabled but OM library is not selected
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Enable the OM library");
                    action.setId("a");
                    action.setToolTipText("Enables the OM library");
                    menuMgr.add(action);
					break;
			case 16: // HAL Library is selected when component does not have
					 //associated hardware resource with prov/alarm enabled
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Disable the HAL library");
                    action.setId("a");
                    action.setToolTipText("Disables the HAL library");
                    menuMgr.add(action);
					break;
			case 17: // PROV library for component is selected but none of 
					 //associated resource has provisioning enabled
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Disable the PROV library");
                    action.setId("a");
                    action.setToolTipText("Disables the PROV library");
                    menuMgr.add(action);
					break;
			case 18: // PROV library for component is not selected but 
					 //associated resource has provisioning enabled
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Enable the PROV library");
                    action.setId("a");
                    action.setToolTipText("Enables the PROV library");
                    menuMgr.add(action);
					break;
			case 19: // ALARM library for component is selected but none of
					 //associated resources has alarm as enabled
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Disable the ALARM library");
                    action.setId("a");
                    action.setToolTipText("Disables the ALARM library");
                    menuMgr.add(action);
					break;
			case 20: // ALARM library for component is not selected but
					 //associated resource has alarm as enabled
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Enable the ALARM library");
                    action.setId("a");
                    action.setToolTipText("Enables the ALARM library");
                    menuMgr.add(action);
					break;	
			case 21: // HAL Library is selected when none of the associated
					 //resources have associated device objects for provisioning/alarm
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Disable the HAL library");
                    action.setId("a");
                    action.setToolTipText("Disables the HAL library");
                    menuMgr.add(action);
					break;
			case 22: // ServiceGroup/CSI is not associated to any of  the 
					 //ServiceUnit/Component
					break;
			case 23: //ServiceUnit/Component is not associated to any of the ServiceGroup/CSI
					break;
			case 24: //Component has duplicate EO name
                    String uniqueEOName = (String) relatedObjects.get(0); 
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Change the EO name to '"
                            + uniqueEOName + "'");
                    action.setId("a");
                    action.setToolTipText("Changes the EO name to a unique name"
                            + "so that it does not clash with other EO names");
                    menuMgr.add(action);
					break;
			case 25: //Single CSI type shared by a Proxy and Proxied is not valid
                    targetObj = source;
                    EObject sourceObj = (EObject) relatedObjects.get(0);
                    connObj = (EObject) relatedObjects.get(1);
                    connType = (String) EcoreUtils.getValue(connObj,
                            ComponentEditorConstants.CONNECTION_TYPE);
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Delete " + connType + " from "
                            + EcoreUtils.getName(sourceObj) + " to "
                            + EcoreUtils.getName(targetObj));
                    action.setId("a");
                    action.setToolTipText("Deletes the invalid connection");
                    menuMgr.add(action);
                    
					break;
			case 26: //'Active SUs' field value should be 1 when 
					 //redundancy model is 'NO_REDUNDANCY''
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Set 'Active SUs' field to 1");
                    action.setId("a");
                    action.setToolTipText("Sets 'Active SUs' field to 1");
                    menuMgr.add(action);
					break;
			case 27: //'Standby SUs' field value should be 0 when 
					 //redundancy model is 'NO_REDUNDANCY'
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Set 'Standby SUs' field to 0");
                    action.setId("a");
                    action.setToolTipText("Sets 'Standby SUs' field to 0");
                    menuMgr.add(action);
					break;
			case 28: //'Inservice SUs' field value should be 1 when 
					 //redundancy model is 'NO_REDUNDANCY'
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Set 'Inservice SUs' field to 1");
                    action.setId("a");
                    action.setToolTipText("Sets 'Inservice SUs' field to 1");
                    menuMgr.add(action);

					break;
			case 29: //'Active SUs' field value should be 1 when 
					//redundancy model is 'TWO_N'
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Set 'Active SUs' field to 1");
                    action.setId("a");
                    action.setToolTipText("Sets 'Active SUs' field to 1");
                    menuMgr.add(action);
					break;
			case 30: //'Standby SUs' field value should be 1 when
					 //redundancy model is 'TWO_N'
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Set 'Standby SUs' field to 1");
                    action.setId("a");
                    action.setToolTipText("Sets 'Standby SUs' field to 1");
                    menuMgr.add(action);

					break;
			case 31: //'Inservice SUs' field value should be >=2 when
					 //redundancy model is 'TWO_N'
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Set 'Inservice SUs' field to 2");
                    action.setId("a");
                    action.setToolTipText("Sets 'Inservice SUs' field to 2");
                    menuMgr.add(action);

					break;
			case 32: //'Assigned SUs' field value should be 2 when
					 //redundancy model is 'TWO_N'
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Set 'Assigned SUs' field to 2");
                    action.setId("a");
                    action.setToolTipText("Sets 'Assigned SUs' field to 2");
                    menuMgr.add(action);


					break;
			case 33: //Component/SU/Node instance Configuration has
					 //reference to Invalid Component/SU/Node type
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Delete instance '" + EcoreUtils.
                            getName(source) + "'");
                    action.setId("a");
                    action.setToolTipText("Deletes the invalid instance");
                    menuMgr.add(action);


					break;
			case 34: //ServiceUnit instance does not have any 
					 //Component instances defined
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Delete instance '" + EcoreUtils.
                            getName(source) + "'");
                    action.setId("a");
                    action.setToolTipText("Deletes the invalid ServiceUnit instance");
                    menuMgr.add(action);

					break;
            case 35: //ServiceInstance instance has more number of CSI
                    //instances configured than what is specified in SI type definition
                    EObject siTypeObj = (EObject) relatedObjects.get(0);
                    List csiList = (List) relatedObjects.get(2);
                    
                    int numCSIs = ((Integer) EcoreUtils.getValue(siTypeObj,
						"numCSIs")).intValue();

					if (csiList.size() > numCSIs) {
						generator = new CombinationGenerator(csiList.size(),
								csiList.size() - numCSIs);
	
						while (generator.hasMore()) {
							indices = generator.getNext();
							action = getAction(csiList, indices, "Delete the CSI ",
									"");
							action.setId("a");
							action
									.setToolTipText("Deletes the extra CSI's configured in AMF configuration");
							menuMgr.add(action);
						}
						menuMgr.add(new Separator());
					}

					action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Change 'Number of CSIs' of SI type to '"
                            + csiList.size() + "'");
                    action.setId("b");
                    action.setToolTipText("Changes the 'Number of CSIs' defined in SI type definition");
                    menuMgr.add(action);
                    break;
            case 36: //ServiceGroup instance does not have any 
                     //ServiceInstance instances defined
                    source = (EObject) EcoreUtils.getValue(
                        _selProblemObj, "source");
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Delete instance '" + EcoreUtils.
                            getName(source) + "'");
                    action.setId("a");
                    action.setToolTipText("Deletes the invalid ServiceGroup instance");
                    menuMgr.add(action);
                    break;
            case 37: //ServiceInstance does not have any CSI instance defined
    
                    source = (EObject) EcoreUtils.getValue(
                        _selProblemObj, "source");
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Delete instance '" + EcoreUtils.
                            getName(source) + "'");
                    action.setId("a");
                    action.setToolTipText("Deletes the invalid ServiceInstance instance");
                    menuMgr.add(action);
                    break;
            case 38: //ServiceGroup instance should have atleast two ServiceUnit
                     //instances associated if it's redundancy model is TWO_N
            		if(relatedObjects.size() > 1) {
                    List suInstList = (List) relatedObjects.get(0);
                    List associatedSUs = (List) relatedObjects.get(1);
                    
                    generator = new CombinationGenerator(suInstList.size(),
                            2 - associatedSUs.size());
                    
                    while (generator.hasMore()) {
                        indices = generator.getNext();
                        action = getAction(suInstList, indices, "Associate SU instance ", "");
                        action.setId("a");
                        action.setToolTipText("Associates the balance ServiceUnit instances");
                        menuMgr.add(action);
                    }
            		}
                    break;
            case 39: //There are no Node instances defined in AMF Configuration
    
    
                    break;
            case 40: //There are no ServiceGroup instances defined in AMF Configuration
    
                    break;
			case 41: //Node/SU/Component/SG/SI/CSI instance name is duplicate
                    int index = ((Integer) relatedObjects.get(0)).intValue();
                    List amfInstList = (List) relatedObjects.get(1);
                    String newName = EcoreUtils.getNextValue(
                            source.eClass().getName() + String.valueOf(index),
                            amfInstList, EcoreUtils.getNameField(source.eClass()));
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText("Change instance name to '" + newName + "'");
                    action.setId("a");
                    action.setToolTipText("Changes the instance name to solve the conflicts");
                    menuMgr.add(action);

					break;
			case 42: //ServiceUnit instance is not associated to any ServiceGroup instance
			        List sgInstList = (List) relatedObjects.get(0);
                    for (int i = 0; i < sgInstList.size(); i++) {
                        EObject sg = (EObject) sgInstList.get(i);
                        action = new AutoCorrectAction(_selProblemObj);
                        action.setText("Associate to SG instance '"
                                + EcoreUtils.getName(sg) + "'");
                        action.setId("a");
                        action.setToolTipText("Associates the SU instance to available SG instance");
                        menuMgr.add(action);
                        actionObjectsMap.put(action, sg);
                    }
					break;
			case 43: //Component Editor Object does not have relationship to any other object
                    if (!source.eClass().getName().equals(
                            ComponentEditorConstants.CLUSTER_NAME)) {
                        action = new AutoCorrectAction(_selProblemObj);
                        action.setText("Delete '" + EcoreUtils.getName(source) + "'");
                        action.setId("a");
                        action.setToolTipText(
                                "Deletes the isolated component from editor");
                        menuMgr.add(action);
                    }
					break;
			case 44: //Atleast one node of type 'Class A' or 'Class B' should
					 //be present in the component editor
			        List nodes = (List) relatedObjects.get(0);
                    for (int i = 0; i < nodes.size(); i++) {
                        EObject node = (EObject) nodes.get(i);
                        action = new AutoCorrectAction(_selProblemObj);
                        action.setText("Change the class of '"
                                + EcoreUtils.getName(node) + "' to 'Class A'");
                        action.setId("a");
                        action.setToolTipText("Changes the node class to 'Class A:System Controller'");
                        menuMgr.add(action);
                        actionObjectsMap.put(action, node);
                    }
					break;
			case 45: //Mismatch in 'Number of additional clients' defined in RMD
					 //and 'Number of additional clients' specified in EOProperties
			        List clientList = (List) relatedObjects.get(0);
                    int maxClients = ((Integer) relatedObjects.get(1)).intValue();
                    EObject eoObj = (EObject) relatedObjects.get(2);
                    if (clientList.size() > maxClients) {
                        generator = new CombinationGenerator(clientList.size(),
                            clientList.size() - maxClients);
                        while (generator.hasMore()) {
                            indices = generator.getNext();
                            action = getAction(clientList, indices, "Delete Client ", "");
                            action.setId("a");
                            action.setToolTipText("Deletes the extra clients defined in RMD configuration");
                            menuMgr.add(action);
                        }
                        
                    }
                    menuMgr.add(new Separator());
                    action = new AutoCorrectAction(_selProblemObj);
                    action.setText(
                            "Change 'Number of additional clients' defined in EO '"
                            + EcoreUtils.getName(eoObj) + "' to '"
                            + String.valueOf(clientList.size()) + "'");
                    action.setId("b");
                    action.setToolTipText("Changes 'Number of additional clients' defined in Component Editor EO");
                    menuMgr.add(action);
					break;
			case 46: //AMF Configuration not done

					break;
            case 47: //ComponentInstances Configuration has Invalid Resource
                     EObject resObj = (EObject) relatedObjects.get(0);
                     String moID = (String) EcoreUtils.getValue(resObj, "moID");
                     action = new AutoCorrectAction(_selProblemObj);
                     action.setText("Dissociate the resource with MO ID '" + moID + "'");
                     action.setId("a");
                     action.setToolTipText("Dissociates the resource with invalid MO ID");
                     menuMgr.add(action);
                     break;
            case 48: //There is no service defined for the client in RMD
                
                	break;
            case 49: //There are missing associated resources for the component in AMF
                
                	break;
            case 50: // The Slot Configuration is not done.

					break;
			case 51: // Either of the provisioning or alarm is not enabled
					// for the resources.
					action = new AutoCorrectAction(_selProblemObj);
					action.setText("Enable Alarm for '"
							+ EcoreUtils.getName(source) + "'");
					action.setId("a");
					action.setToolTipText("Enables Alarm for the Resource.");
					menuMgr.add(action);

					action = new AutoCorrectAction(_selProblemObj);
					action.setText("Enable Provisioning for '"
							+ EcoreUtils.getName(source) + "'");
					action.setId("b");
					action.setToolTipText("Enables Provisioning for the Resource.");
					menuMgr.add(action);

					action = new AutoCorrectAction(_selProblemObj);
					action.setText("Enable Alarm and Provisioning for '"
							+ EcoreUtils.getName(source) + "'");
					action.setId("c");
					action.setToolTipText("Enables Alarm and Provisioning for the "
							+ "Resource.");
					menuMgr.add(action);
					break;
			case 52: //There cant be more than one snmp subagent
					Iterator<EObject> subAgentItr = ((List<EObject>) relatedObjects.get(0)).iterator();
					while(subAgentItr.hasNext()) {
						String subAgentName = EcoreUtils.getName(subAgentItr.next());
						action = new AutoCorrectAction(_selProblemObj);
						action.setText("Keep " + subAgentName + " as Snmp SubAgent. Disable others");
						action.setId("a");
						action.setToolTipText("Make only this component as Snmp SubAgent.");
						menuMgr.add(action);
                        actionObjectsMap.put(action, subAgentName);
					}
					break;
			case 53: //SNMP subagent cannot be proxy
					break;
			case 54: // Transaction lib is mandatory if prov lib for the component is selected
					action = new AutoCorrectAction(_selProblemObj);
	                action.setText("Enable the Transaction library");
	                action.setId("a");
	                action.setToolTipText("Enables the Transaction library");
	                menuMgr.add(action);
        			break;
			case 55: //Set correct number of standby assingnments for SI
					action = new AutoCorrectAction(_selProblemObj);
					action.setText("Set Correct Number Of Standby Assignments for '"
						+ EcoreUtils.getName(source) + "'");
					action.setId("a");
					action.setToolTipText("Set Number Of Standby Assignments.");
					menuMgr.add(action);
					break;		
			case 62: // Primary OI validation
					HashMap<String, EObject> compPathResMap62 = (HashMap<String, EObject>) relatedObjects.get(0);
					Iterator<String> pathItr62 = compPathResMap62.keySet().iterator();
					while(pathItr62.hasNext()) {
						String path = pathItr62.next();
						action = new AutoCorrectAction(_selProblemObj);
						action.setText("Make Primary OI true for '"
								+ path + "' and false for the rest");
						action.setId("a");
						action.setToolTipText("Make Primary OI true for the component.");
						menuMgr.add(action);
                        actionObjectsMap.put(action, path);
					}
					break;
			case 69: // Resource Instance ID Validation
					/*action = new AutoCorrectAction(_selProblemObj);
					action.setText("Change the MO ID");
					action.setId("a");
					action.setToolTipText("Instance id should be less than 'maxInstances' value of that particular resource type");
					menuMgr.add(action);*/
					break;
			case 73: // The Slot does not have any class types configured
					action = new AutoCorrectAction(_selProblemObj);
	                action.setText("Add all the node types to this slot");
	                action.setId("a");
	                action.setToolTipText("Adds all the available node types to this empty slot");
	                menuMgr.add(action);
					break;
			case 74: // Node is not configured in any of the available slots
				action = new AutoCorrectAction(_selProblemObj);
                action.setText("Add this node to all the slots");
                action.setId("a");
                action.setToolTipText("Adds this node to all the available slots");
                menuMgr.add(action);
				break;
			case 75: // Mismatch in 'Number of slots' specified in chassis and number of slots
					// configured in Node Admission Control
				EObject slotsObj =  (EObject) relatedObjects.get(0);
				List slotList = (List) EcoreUtils.getValue(slotsObj,
						ModelConstants.SLOT_LIST_REF);
				EObject chassisObj = (EObject) relatedObjects.get(1);
				int numSlots = ((Integer) EcoreUtils.getValue(chassisObj,
		        		   ClassEditorConstants.CHASSIS_NUM_SLOTS)).intValue();
				if (numSlots > slotList.size()) {
					action = new AutoCorrectAction(_selProblemObj);
	                action.setText("Change the field 'Number of slots' in chassis to "
	                		+ String.valueOf(slotList.size()));
	                action.setId("a");
	                action.setToolTipText("Changes the 'Number of slots' in chassis to be in sync with number of slots configured in slotmap");
	                menuMgr.add(action);
				} else if (numSlots < slotList.size()) {
					action = new AutoCorrectAction(_selProblemObj);
	                action.setText("Change the field 'Number of slots' in chassis to "
	                		+ String.valueOf(slotList.size()));
	                action.setId("a");
	                action.setToolTipText("Changes the 'Number of slots' in chassis to be in sync with number of slots configured in slotmap");
	                menuMgr.add(action);
	                
	                String extraSlots = "";
	                for (int i = numSlots + 1; i <= slotList.size(); i++) {
	                	extraSlots = extraSlots + " " + String.valueOf(i);
	                }
	                action = new AutoCorrectAction(_selProblemObj);
	                action.setText("Remove the configured slots:" + extraSlots + "from slotmap configuration");
	                action.setId("b");
	                action.setToolTipText("Removes the extra configured slots from slotmap configuration");
	                menuMgr.add(action);
				}
				break;
			case 76:
				action = new AutoCorrectAction(_selProblemObj);
                action.setText("Change " + EcoreUtils.getName(source) + " restartable value as true");
                action.setId("a");
                action.setToolTipText("Change restartable field as true");
                menuMgr.add(action);
				break;
			
				
			case 80:
			case 81:
			case 82:
				action = new AutoCorrectAction(_selProblemObj);
                action.setText("Change ActiveSUsPerSI's value to 1");
                action.setId("a");
                action.setToolTipText("Change ActiveSUsPerSI's value to 1");
                menuMgr.add(action);
                break;
                
			case 83:
				action = new AutoCorrectAction(_selProblemObj);
                action.setText("Change MaxActiveSIsPerSU's value to 1");
                action.setId("a");
                action.setToolTipText("Change MaxActiveSIsPerSU's value to 1");
                menuMgr.add(action);
                break;
                	
			case 84:
				action = new AutoCorrectAction(_selProblemObj);
                action.setText("Change MaxStandbySIsPerSU's value to 0");
                action.setId("a");
                action.setToolTipText("Change MaxStandbySIsPerSU's value to 0");
                menuMgr.add(action);
                break;
                	
			case 94:			
			case 97:
			case 99:
				action = new AutoCorrectAction(_selProblemObj);
                action.setText("Change Maximum Number of Active CSIs to 1");
                action.setId("a");
                action.setToolTipText("Change Maximum Number of Active CSIs");
                menuMgr.add(action);
				break;
				
			case 96:
			case 98:
			case 100:
				action = new AutoCorrectAction(_selProblemObj);
                action.setText("Change Maximum Number of StandBy CSIs to 0");
                action.setId("a");
                action.setToolTipText("Change Maximum Number of StandBy CSIs");
                menuMgr.add(action);
				break;
				
			case 101:
				break;
				
			default:
					break;
			}
		}
		
	}
    /**
	 * 
	 * @param objList
	 * @param indices
	 * @param prefix
	 * @param suffix
	 * @return
	 */
	private Action getAction(List objList, int[] indices, String prefix,
            String suffix)
    {
        AutoCorrectAction action = new AutoCorrectAction(_selProblemObj);
        String names = "";
        List relatedObjs;
        for (int i = 0; i < indices.length; i++) {
            EObject eobj = (EObject) objList.get(indices[i]);
            String objName = EcoreUtils.getName(eobj);
            if (objName == null) {
                objName = (String) EcoreUtils.getValue(eobj, "name");
            }
            names = names.concat(objName);
            if (i < indices.length - 1) {
                names = names.concat(", ");
            }
            relatedObjs = (List) actionObjectsMap.get(action);
            if (relatedObjs == null) {
                relatedObjs = new Vector();
                actionObjectsMap.put(action, relatedObjs);
            }
            relatedObjs.add(eobj);
        }
        
        
        action.setText(prefix + names + suffix);
        return action;
    }
    /**
	 * Implementation of run method
	 */
	public void runWithEvent(Event event)
	{
        IAction action = null;
        Widget widget = event.widget;
        if (widget instanceof MenuItem) {
            if (((MenuItem) widget).getData() 
                    instanceof ActionContributionItem) {
                ActionContributionItem item = (ActionContributionItem)
                    ((MenuItem) widget).getData();
                action = item.getAction();
            
            }
        }
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
        Model compModel = pdm.getComponentModel();
        Model caModel = pdm.getCAModel();
        GenericEditorInput caInput = (GenericEditorInput) pdm.
            getCAEditorInput();
        GenericEditorInput compInput = (GenericEditorInput) pdm.
            getComponentEditorInput();
        if (_selProblemObj != null && action != null) {
			int problemNumber = ((Integer) EcoreUtils.
					getValue(_selProblemObj, "problemNumber")).intValue();
            EObject source = (EObject) EcoreUtils.getValue(
                    _selProblemObj, "source");
            String id = action.getId();
            char  actionId = id.charAt(0);
            List relatedObjects = (List) EcoreUtils.getValue(
                    _selProblemObj,"relatedObjects");
            boolean editorDirty = false;
            boolean latestEditorDirty = false;
			switch(problemNumber) {
            
			case 1: // Resource having 2 alarms of same probable cause
                    EObject  resObj = (EObject) relatedObjects.get(0);
					switch (actionId) {
                    case 'a':
                            List objects = (List) actionObjectsMap.get(action);
                            Model resourceAlarmModel = pdm.getResourceAlarmMapModel();
                            EObject mapObj = resourceAlarmModel.getEObject();
                            EObject linkObj = SubModelMapReader.getLinkObject(mapObj,
                            		ClassEditorConstants.ASSOCIATED_ALARM_LINK);
                        	List associatedAlarms = (List) SubModelMapReader.getLinkTargetObjects(
                            		linkObj, EcoreUtils.getName(resObj));
                            for (int i = 0; i < objects.size(); i++) {
                                EObject alarm = (EObject) objects.get(i);
                                String alarmID = (String) EcoreUtils.getValue(
                                        alarm, ClassEditorConstants.ALARM_ID);
                                associatedAlarms.remove(alarmID);
                            }
                            
                            resourceAlarmModel.save(true);
                           
                            break;
                    case 'b':
                            AddAlarmProfileAction.openAlarmDialog(shell, project);
                            break;
                    }
					break;
			case 2: // Resource has a associated alarm which is 
					//deleted from Alarm Profile configuration
                    editorDirty = isEditorDirty(caInput);
                    resObj = (EObject) relatedObjects.get(0);
                    String alarmID = (String) relatedObjects.get(1);
                    Model resourceAlarmModel = pdm.getResourceAlarmMapModel();
                    EObject mapObj = resourceAlarmModel.getEObject();
                    EObject linkObj = SubModelMapReader.getLinkObject(mapObj,
                    		ClassEditorConstants.ASSOCIATED_ALARM_LINK);
                	List associatedAlarms = (List) SubModelMapReader.getLinkTargetObjects(
                    		linkObj, EcoreUtils.getName(resObj));
                    associatedAlarms.remove(alarmID);
                    
                    resourceAlarmModel.save(true);
                    
					break;
			case 3: // Alarm Service object has a associated DO which is
				// deleted from DO List of resource
				editorDirty = isEditorDirty(caInput);
				EObject alarmServiceObj = (EObject) relatedObjects.get(0);
				String doName = (String) relatedObjects.get(1);
				List associatedDos = (List) EcoreUtils.getValue(
						alarmServiceObj, ClassEditorConstants.ASSOCIATED_DO);
				Iterator iterator = associatedDos.iterator();
				while (iterator.hasNext()) {
					EObject associatedDO = (EObject) iterator.next();
					String name = (String) EcoreUtils.getValue(associatedDO,
							ClassEditorConstants.DEVICE_ID);
					if (name.equals(doName)) {
						iterator.remove();
					}
				}
				latestEditorDirty = isEditorDirty(caInput);
				FormatConversionUtils.convertToResourceFormat(
						(EObject) alarmServiceObj.eResource().getContents()
								.get(0), ClassEditorConstants.EDITOR_TYPE);
				saveResource(alarmServiceObj.eResource());
				if (latestEditorDirty && !editorDirty && caInput != null) {
					caInput.getEditor().doSave(null);
				}
				break;
			case 4: // Provisioning Service object has a associated DO
					// which is deleted from DO List of resource
                    editorDirty = isEditorDirty(caInput);
                    EObject provServiceObj = (EObject) relatedObjects.get(0);
                    doName = (String) relatedObjects.get(1);
                    associatedDos = (List) EcoreUtils.getValue(provServiceObj,
                            ClassEditorConstants.ASSOCIATED_DO);
                    iterator = associatedDos.iterator();
                    while (iterator.hasNext()) {
                        EObject associatedDO = (EObject) iterator.next();
                        String name = (String) EcoreUtils.getValue(
                            associatedDO, ClassEditorConstants.DEVICE_ID);
                        if (name.equals(doName)) {
                            iterator.remove();
                        }
                    }
                    latestEditorDirty = isEditorDirty(caInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) provServiceObj.eResource().getContents()
									.get(0), ClassEditorConstants.EDITOR_TYPE);
                    saveResource(provServiceObj.eResource());
                    if (latestEditorDirty && !editorDirty && caInput != null) {
                        caInput.getEditor().doSave(null);
                    }
					break;
			case 5: // Provisioning enabled on the resource 
					//without having any attributes to be provisioned.
                    editorDirty = isEditorDirty(caInput);
                    provServiceObj = (EObject) relatedObjects.get(0);
                    EcoreUtils.setValue(provServiceObj, "isEnabled",
                            String.valueOf(false));
                    latestEditorDirty = isEditorDirty(caInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) provServiceObj.eResource().getContents()
									.get(0), ClassEditorConstants.EDITOR_TYPE);
                    saveResource(provServiceObj.eResource());
                    if (latestEditorDirty && !editorDirty && caInput != null) {
                        caInput.getEditor().doSave(null);
                    }
					break;
			case 6: // Alarm enabled on the resource without having any alarms
					//associated to the resource.
                    editorDirty = isEditorDirty(caInput);
                    alarmServiceObj = (EObject) relatedObjects.get(0);
                    EcoreUtils.setValue(alarmServiceObj, "isEnabled",
                            String.valueOf(false));
                    latestEditorDirty = isEditorDirty(caInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) alarmServiceObj.eResource().getContents()
									.get(0), ClassEditorConstants.EDITOR_TYPE);
                    saveResource(alarmServiceObj.eResource());
                    if (latestEditorDirty && !editorDirty && caInput != null) {
                        caInput.getEditor().doSave(null);
                    }
					break;
			case 7: // Isolated resource in the Resource Editor
                    editorDirty = isEditorDirty(caInput);
                    EObject viewObj = EditorUtils.getObjectFromName(caModel.
                            getEList(), EcoreUtils.getName(source));
                    if (viewObj != null) {
                        EObject rootObject = (EObject) caModel.getEList().get(0);
                        String refList[] = ClassEditorConstants.NODES_REF_TYPES;
                		for (int i = 0; i < refList.length; i++) {
                			EReference ref = (EReference) rootObject.eClass()
                					.getEStructuralFeature(refList[i]);
                			EList list = (EList) rootObject.eGet(ref);
                			if(list.remove(viewObj))
                			{
                				FormatConversionUtils.convertToResourceFormat(
            							rootObject, ClassEditorConstants.EDITOR_TYPE);
                				caModel.save(true);
                			}
                		}
                	}
                    latestEditorDirty = isEditorDirty(caInput);
                    if (latestEditorDirty && !editorDirty && caInput != null) {
                        caInput.getEditor().doSave(null);
                    } 
					break;
			case 8: // Invalid connection due to dependency on other connection(s)
                    editorDirty = isEditorDirty(compInput);
                    EObject connObj = (EObject) relatedObjects.get(1);
                    if (connObj != null) {
                        EObject rootObject = (EObject) compModel.getEList().get(0);
                        EReference ref = (EReference) rootObject.eClass()
                					.getEStructuralFeature(ComponentEditorConstants.AUTO_REF_NAME);
                		EList list = (EList) rootObject.eGet(ref);
                		list.remove(connObj);
                		FormatConversionUtils.convertToResourceFormat(
    							rootObject, ComponentEditorConstants.EDITOR_TYPE);
                		compModel.save(true);
                    }
                    latestEditorDirty = isEditorDirty(compInput);
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    } 
					break;
			case 9: // Component image name is empty
                    editorDirty = isEditorDirty(compInput);
                    String newImageName = (String) relatedObjects.get(0);
                    EcoreUtils.setValue(source, ComponentEditorConstants.
                            INSTANTIATION_COMMAND, newImageName);
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 10: // Duplicate component image name
                    editorDirty = isEditorDirty(compInput);
                    newImageName = (String) relatedObjects.get(0);
                    EcoreUtils.setValue(source, ComponentEditorConstants.
                            INSTANTIATION_COMMAND, newImageName);
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 11: // Component does not have associated resource(s)
                    
					break;
			case 12: // Associated resource does not exist in 
					//list of resources of resource editor
                    String name = (String) relatedObjects.get(0);
                    Model componentResourceModel = pdm.getComponentResourceMapModel();
                    mapObj = componentResourceModel.getEObject();
                    /*linkObj = SubModelMapReader.getLinkObject(mapObj,
                    		ComponentEditorConstants.ASSOCIATE_RESOURCES_NAME);*/
                    SubModelMapReader reader = SubModelMapReader.getSubModelMappingReader(
                			project, "component", "resource");
                	List associatedResList = (List) reader.getLinkTargetObjects(
                			ComponentEditorConstants.ASSOCIATE_RESOURCES_NAME, EcoreUtils.getName(source));
                    associatedResList.remove(name);
                    /*if (!isEditorDirty(compInput)) {
                        compInput.getEditor().doSave(null);
                    } else {*/
                    componentResourceModel.save(true);
                    //}
					break;
			case 13: //Proxied component doesn't belongs to proper SG and Node hierarchy 
                    break;
			case 14: // One of associated resource has provisioning/alarm
					 //enabled with associated device object(s) but HAL
					 //library is not selected
                    editorDirty = isEditorDirty(compInput);
                    EObject aspLibObj = (EObject) relatedObjects.get(0);
                    EcoreUtils.setValue(aspLibObj, ComponentEditorConstants.
                            EO_HALLIB_NAME, "CL_TRUE");
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) aspLibObj.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                	saveResource(aspLibObj.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 15: // One of associated resources has provisioning /alarm
					 //enabled but OM library is not selected
                    editorDirty = isEditorDirty(compInput);
                    aspLibObj = (EObject) relatedObjects.get(0);
                    EcoreUtils.setValue(aspLibObj, ComponentEditorConstants.
                            EO_OMLIB_NAME, "CL_TRUE");
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) aspLibObj.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(aspLibObj.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 16: // HAL Library is selected when component does not have
					 //associated hardware resource with prov/alarm enabled
                    editorDirty = isEditorDirty(compInput);
                    aspLibObj = (EObject) relatedObjects.get(0);
                    EcoreUtils.setValue(aspLibObj, ComponentEditorConstants.
                            EO_HALLIB_NAME, "CL_FALSE");
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) aspLibObj.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(aspLibObj.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 17: // PROV library for component is selected but none of 
					 //associated resource has provisioning enabled
                    editorDirty = isEditorDirty(compInput);
                    aspLibObj = (EObject) relatedObjects.get(0);
                    EcoreUtils.setValue(aspLibObj, ComponentEditorConstants.
                            EO_PROVLIB_NAME, "CL_FALSE");
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) aspLibObj.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(aspLibObj.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 18: // PROV library for component is not selected but 
					 //associated resource has provisioning enabled
                    editorDirty = isEditorDirty(compInput);
                    aspLibObj = (EObject) relatedObjects.get(0);
                    EcoreUtils.setValue(aspLibObj, ComponentEditorConstants.
                            EO_PROVLIB_NAME, "CL_TRUE");
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) aspLibObj.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(aspLibObj.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 19: // ALARM library for component is selected but none of
					 //associated resources has alarm as enabled
                    editorDirty = isEditorDirty(compInput);
                    aspLibObj = (EObject) relatedObjects.get(0);
                    EcoreUtils.setValue(aspLibObj, ComponentEditorConstants.
                            EO_ALARMLIB_NAME, "CL_FALSE");
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) aspLibObj.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(aspLibObj.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 20: // ALARM library for component is not selected but
					 //associated resource has alarm as enabled
                    editorDirty = isEditorDirty(compInput);
                    aspLibObj = (EObject) relatedObjects.get(0);
                    EcoreUtils.setValue(aspLibObj, ComponentEditorConstants.
                            EO_ALARMLIB_NAME, "CL_TRUE");
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) aspLibObj.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(aspLibObj.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;	
			case 21: // HAL Library is selected when none of the associated
					 //resources have associated device objects for provisioning/alarm
                    editorDirty = isEditorDirty(compInput);
                    aspLibObj = (EObject) relatedObjects.get(0);
                    EcoreUtils.setValue(aspLibObj, ComponentEditorConstants.
                            EO_HALLIB_NAME, "CL_FALSE");
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) aspLibObj.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                	saveResource(aspLibObj.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
                    break;
			case 22: // ServiceGroup/CSI is not associated to any of  the 
					 //ServiceUnit/Component
					break;
			case 23: //ServiceUnit/Component is not associated to any of the ServiceGroup/CSI
					break;
			case 24: //Component has duplicate EO name
                    editorDirty = isEditorDirty(compInput);
                    EObject eoObj = (EObject) EcoreUtils.getValue(source,
                            ComponentEditorConstants.EO_PROPERTIES_NAME);
                    String uniqueEOName = (String) relatedObjects.get(0);
                    EcoreUtils.setValue(eoObj, ComponentEditorConstants.
                            EO_NAME, uniqueEOName);
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 25: //Single CSI type shared by a Proxy and Proxied is not valid
                    editorDirty = isEditorDirty(compInput);
                    connObj = (EObject) relatedObjects.get(1);
                    if (connObj != null) {
                        EObject rootObject = (EObject) compModel.getEList().get(0);
                        EReference ref = (EReference) rootObject.eClass()
                					.getEStructuralFeature(ComponentEditorConstants.AUTO_REF_NAME);
                		EList list = (EList) rootObject.eGet(ref);
                		if(list.remove(connObj)) {
                			FormatConversionUtils.convertToResourceFormat(
        							rootObject, ComponentEditorConstants.EDITOR_TYPE);
                            compModel.save(true);
                		}
                	}
                    latestEditorDirty = isEditorDirty(compInput);
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 26: //'Active SUs' field value should be 1 when 
					 //redundancy model is 'NO_REDUNDANCY''
                    editorDirty = isEditorDirty(compInput);
                    EcoreUtils.setValue(source, SafConstants.
                            SG_ACTIVE_SU_COUNT, String.valueOf(1));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 27: //'Standby SUs' field value should be 0 when 
					 //redundancy model is 'NO_REDUNDANCY'
                    editorDirty = isEditorDirty(compInput);
                    EcoreUtils.setValue(source, SafConstants.
                            SG_STANDBY_SU_COUNT, String.valueOf(0));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
                    break;
			case 28: //'Inservice SUs' field value should be1 when 
					 //redundancy model is 'NO_REDUNDANCY'
                    editorDirty = isEditorDirty(compInput);
                    EcoreUtils.setValue(source, SafConstants.
                            SG_INSERVICE_SU_COUNT, String.valueOf(1));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
                    break;
			case 29: //'Active SUs' field value should be1 when 
					//redundancy model is 'TWO_N'
                    editorDirty = isEditorDirty(compInput);
                    EcoreUtils.setValue(source, SafConstants.
                            SG_ACTIVE_SU_COUNT, String.valueOf(1));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 30: //'Standby SUs' field value should be1 when
					 //redundancy model is 'TWO_N'
                    editorDirty = isEditorDirty(compInput);
                    EcoreUtils.setValue(source, SafConstants.
                            SG_STANDBY_SU_COUNT, String.valueOf(1));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
                    break;
			case 31: //'Inservice SUs' field value should be >=2 when
					 //redundancy model is 'TWO_N'
                    editorDirty = isEditorDirty(compInput);
                    EcoreUtils.setValue(source, SafConstants.
                            SG_INSERVICE_SU_COUNT, String.valueOf(2));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 32: //'Assigned SUs' field value should be 2 when
					 //redundancy model is 'TWO_N'
                    editorDirty = isEditorDirty(compInput);
                    EcoreUtils.setValue(source, SafConstants.
                            SG_ASSIGNED_SU_COUNT, String.valueOf(2));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 33: //Component/SU/Node instance Configuration has
					 //reference to Invalid Component/SU/Node type
			        EObject instancesObj = (EObject) relatedObjects.get(0);
                    List instList = (List) relatedObjects.get(1);
			        instList.remove(source);
                    saveResource(instancesObj.eResource());
					break;
			case 34: //ServiceUnit instance does not have any 
					 //Component instances defined
                    instancesObj = (EObject) relatedObjects.get(0);
                    instList = (List) relatedObjects.get(1);
                    instList.remove(source);
                    saveResource(instancesObj.eResource());
					break;
			case 35: //ServiceInstance instance has more number of CSI
					//instances configured than what is specified in SI type definition
			        switch (actionId) {
                    case 'a':
                            List objList = (List) actionObjectsMap.get(action);
                            EObject csiInstsObj = (EObject) relatedObjects.get(1);
                            List csiInstList = (List) relatedObjects.get(2);
                            for (int i = 0; i < objList.size(); i++) {
                                EObject csi = (EObject) objList.get(i);
                                csiInstList.remove(csi);
                            }
                            saveResource(csiInstsObj.eResource());
                            break;
                    case 'b':
                            editorDirty = isEditorDirty(compInput);
                            EObject siTypeObj = (EObject) relatedObjects.get(0);
                            List csiList = (List) relatedObjects.get(2);
                            EcoreUtils.setValue(siTypeObj, "numCSIs", String.
                                    valueOf(csiList.size()));
                            latestEditorDirty = isEditorDirty(compInput);
                            FormatConversionUtils.convertToResourceFormat(
        							(EObject) siTypeObj.eResource().getContents()
        									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                            saveResource(siTypeObj.eResource());
                            if (latestEditorDirty && !editorDirty && compInput != null) {
                                compInput.getEditor().doSave(null);
                            }
                            break;
                    }
					break;
			case 36: //ServiceGroup instance does not have any 
					 //ServiceInstance instances defined
                    instancesObj = (EObject) relatedObjects.get(0);
                    instList = (List) relatedObjects.get(1);
                    instList.remove(source);
                    saveResource(instancesObj.eResource());
					break;
			case 37: //ServiceInstance does not have any CSI instance defined
                    instancesObj = (EObject) relatedObjects.get(0);
                    instList = (List) relatedObjects.get(1);
                    instList.remove(source);
                    saveResource(instancesObj.eResource());

					break;
			case 38: //ServiceGroup instance should have atleast two ServiceUnit
					 //instances associated if it's redundancy model is TWO_N
			        List objList = (List) actionObjectsMap.get(action);
                    List associatedSUList = (List) relatedObjects.get(1);
			        for ( int i = 0; i < objList.size(); i++) {
			            EObject suInstObj = (EObject) objList.get(i);
                        associatedSUList.add(EcoreUtils.getName(suInstObj));
                    }
                    saveResource(source.eResource());
					break;
			case 39: //There are no Node instances defined in AMF Configuration


					break;
			case 40: //There are no ServiceGroup instances defined in AMF Configuration

					break;
			case 41: //Node/SU/Component/SG/SI/CSI instance name is duplicate
                    int index = ((Integer) relatedObjects.get(0)).intValue();
                    List amfInstList = (List) relatedObjects.get(1);
                    String newName = EcoreUtils.getNextValue(
                            source.eClass().getName() + String.valueOf(index),
                            amfInstList, EcoreUtils.getNameField(source.eClass()));
                    EcoreUtils.setValue(source, EcoreUtils.getNameField(source.
                            eClass()), newName);
                    saveResource(source.eResource());
					break;
			case 42: //ServiceUnit instance is not associated to any ServiceGroup instance
			        EObject sg = (EObject) actionObjectsMap.get(action);
                    EReference suInstRef = (EReference) sg.eClass().getEStructuralFeature(
                            SafConstants.ASSOCIATED_SERVICEUNITS_NAME);
                    EObject associatedSUSObj = (EObject) sg.eGet(suInstRef);
                    if (associatedSUSObj == null) {
                        associatedSUSObj = EcoreUtils.createEObject(suInstRef.
                                getEReferenceType(), true);
                    }
                    List associatedSUs = (List) EcoreUtils.getValue(associatedSUSObj,
                            SafConstants.ASSOCIATED_SERVICEUNIT_LIST);
                    associatedSUs.add(EcoreUtils.getName(source));
                    saveResource(sg.eResource());
					break;
			case 43: //Component Editor Object does not have relationship to any other object
                    editorDirty = isEditorDirty(compInput);
                    viewObj = EditorUtils.getObjectFromName(compModel
                            .getEList(), EcoreUtils.getName(source));
                    if (viewObj != null) {
                    	EObject rootObject = (EObject) compModel.getEList().get(0);
                    	String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
                    	for (int i = 0; i < refList.length; i++) {
                    		EReference ref = (EReference) rootObject.eClass()
								.getEStructuralFeature(refList[i]);
                    		EList list = (EList) rootObject.eGet(ref);
                    		if(list.remove(viewObj)) {
                    			FormatConversionUtils.convertToResourceFormat(
            							rootObject, ComponentEditorConstants.EDITOR_TYPE);
                                compModel.save(true);
                    		}
                    	}
                    }
                    latestEditorDirty = isEditorDirty(compInput);
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    } 
					break;
			case 44: //Atleast one node of type 'Class A' or 'Class B' should
					 //be present in the component editor
                    editorDirty = isEditorDirty(compInput);
			        EObject node = (EObject) actionObjectsMap.get(action);
                    EcoreUtils.setValue(node, ComponentEditorConstants.
                            NODE_CLASS_TYPE, "CL_AMS_NODE_CLASS_A");
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) node.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(node.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
			case 45: //Mismatch in 'Number of additional clients' defined in RMD
					 //and 'Number of additional clients' specified in EOProperties
			        switch(actionId) {
                    case 'a':
                            List clientList = (List) relatedObjects.get(0);
                            int maxClients = ((Integer) relatedObjects.get(1)).intValue();
                            
                            objList = (List) actionObjectsMap.get(action);
                            if (clientList.size() > maxClients) {
                                for (int i = 0; i < objList.size(); i++) {
                                    EObject client = (EObject) objList.get(i);
                                    clientList.remove(client);
                                }
                            }
                            saveResource(source.eResource());
                            break;
                    case 'b':
                            editorDirty = isEditorDirty(compInput);
                            eoObj = (EObject) relatedObjects.get(2);
                            clientList = (List) relatedObjects.get(0);
                            EcoreUtils.setValue(eoObj, ComponentEditorConstants.
                                    EO_MAX_CLIENTS, String.valueOf(clientList.size()));
                            latestEditorDirty = isEditorDirty(compInput);
                            FormatConversionUtils.convertToResourceFormat(
        							(EObject) eoObj.eResource().getContents()
        									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                            saveResource(eoObj.eResource());
                            if (latestEditorDirty && !editorDirty && compInput != null) {
                                compInput.getEditor().doSave(null);
                            }
                            break;
                    }
					break;
			case 46: //AMF Configuration not done

					break;
            case 47: //ComponentInstances Configuration has Invalid Resource
                    EObject res = (EObject) relatedObjects.get(0);
                    List resList = (List) relatedObjects.get(1);
                    resList.remove(res);
                    saveResource(source.eResource());
                    break;
            case 48: //There is no service defined for the client in RMD
                
                	break;
            case 49: //There are missing associated resources for the component in AMF
                
                	break;
            case 50: //The Slot Configuration is not done.

                	break;
            case 51: //Either of the provisioning or alarm is not enabled 
            		//for the resources.
            		editorDirty = isEditorDirty(caInput);
            		EObject provObj = (EObject) EcoreUtils.getValue(source,
            				ClassEditorConstants.RESOURCE_PROVISIONING);
            		EObject alarmObj = (EObject) EcoreUtils.getValue(source,
            				ClassEditorConstants.RESOURCE_ALARM);
            		switch (actionId) {
            		case 'a':
            				if (alarmObj != null) {
            					EcoreUtils.setValue(alarmObj, "isEnabled", "true");
            				}
            				break;
            		case 'b':
            				if (provObj != null) {
            					EcoreUtils.setValue(provObj, "isEnabled", "true");
            				}
            				break;
            		case 'c':
            				if (alarmObj != null) {
            					EcoreUtils.setValue(alarmObj, "isEnabled", "true");
            				}
            				if (provObj != null) {
            					EcoreUtils.setValue(provObj, "isEnabled", "true");
            				}
            				break;
            		}

            		latestEditorDirty = isEditorDirty(caInput);
            		FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ClassEditorConstants.EDITOR_TYPE);
        			saveResource(source.eResource());
            		if (latestEditorDirty && !editorDirty && caInput != null) {
            			caInput.getEditor().doSave(null);
            		}
            		break;
            case 52: //There cant be more than one snmp subagent
	            	editorDirty = isEditorDirty(compInput);
					String subAgentName = actionObjectsMap.get(action).toString();
	                Iterator<EObject> snmpSubAgentItr = ((List<EObject>) relatedObjects.get(0)).iterator();
	                EObject comp = null;
					while (snmpSubAgentItr.hasNext()) {
						comp = snmpSubAgentItr.next();
						if (!EcoreUtils.getName(comp).equals(subAgentName)) {
							EcoreUtils.setValue(comp,
									ComponentEditorConstants.IS_SNMP_SUBAGENT, "false");
						}
					}
	                latestEditorDirty = isEditorDirty(compInput);
	                FormatConversionUtils.convertToResourceFormat(
							(EObject) comp.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
	                saveResource(comp.eResource());
	                if (latestEditorDirty && !editorDirty && compInput != null) {
	                    compInput.getEditor().doSave(null);
	                }
	        		break;
            case 53: //SNMP subagent cannot be proxy
					break;
            case 54: // Transaction lib is mandatory if prov lib for the component is selected
	            	editorDirty = isEditorDirty(compInput);
	                aspLibObj = (EObject) relatedObjects.get(0);
	                EcoreUtils.setValue(aspLibObj, ComponentEditorConstants.
	                        EO_TRANSACTIONLIB_NAME, "CL_TRUE");
	                latestEditorDirty = isEditorDirty(compInput);
	                FormatConversionUtils.convertToResourceFormat(
							(EObject) aspLibObj.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(aspLibObj.eResource());
	                if (latestEditorDirty && !editorDirty && compInput != null) {
	                    compInput.getEditor().doSave(null);
	                }
            		break;
            case 55://Set correct number of standby assingnments for SI  
        			editorDirty = isEditorDirty(compInput);
        			String redundancyModel = relatedObjects.get(0).toString();
        		
        			if(redundancyModel.equals(ComponentEditorConstants.TWO_N_REDUNDANCY_MODEL)
        				|| redundancyModel.equals(ComponentEditorConstants.M_PLUS_N_REDUNDANCY_MODEL)){
        				EcoreUtils.setValue(source, "numStandbyAssignments", "1");
        			}else if(redundancyModel.equals(ComponentEditorConstants.NO_REDUNDANCY_MODEL)){
        				EcoreUtils.setValue(source, "numStandbyAssignments", "0");
        			}
        			latestEditorDirty = isEditorDirty(compInput);
        			if (latestEditorDirty && !editorDirty && compInput != null) {
	                    compInput.getEditor().doSave(null);
	                }
        			break;		
				case 62: // Primary OI validation
					String path62 = actionObjectsMap.get(action).toString();
					HashMap<String, EObject> compPathResMap62 = (HashMap<String, EObject>) relatedObjects
							.get(0);
					Iterator<String> pathItr62 = compPathResMap62.keySet().iterator();
					while (pathItr62.hasNext()) {
						String compPath = pathItr62.next();
						if (!path62.equals(compPath)) {
							EcoreUtils.setValue(compPathResMap62.get(compPath),
									"primaryOI", "false");
						}
					}
					saveResource(pdm.getNodeProfiles().getResource());
					NodeProfileDialog.writeCompToResMap(pdm);
					break;
				case 69: // Resoiurce Instance ID Validation
					break;
				case 73: // The Slot does not have any class types configured
					List nodesList = (List) relatedObjects.get(0);
					EObject classTypesObj = (EObject) relatedObjects.get(1);
					EReference classTypeRef = (EReference) classTypesObj.eClass().
						getEStructuralFeature(ModelConstants.SLOT_CLASSTYPE_LIST_REF);
					List classTypesList = (List) classTypesObj.eGet(classTypeRef);
					
					for (int i = 0; i < nodesList.size(); i++) {
						EObject nodeObj = (EObject) nodesList.get(i);
						EObject classTypeObj = EcoreUtils.createEObject(
								classTypeRef.getEReferenceType(), true);
						EcoreUtils.setValue(classTypeObj, "name", EcoreUtils.getName(nodeObj));
						classTypesList.add(classTypeObj);
					}
					saveResource(classTypesObj.eResource());
					break;
				case 74: // Node is not configured in any of the available slots
					EObject slotsObj =  (EObject) relatedObjects.get(0);
					List slotList = (List) EcoreUtils.getValue(slotsObj,
							ModelConstants.SLOT_LIST_REF);
					for (int i = 0; i < slotList.size(); i++) {
			        	   classTypesObj = (EObject) EcoreUtils.getValue(
			        			   (EObject) slotList.get(i), ModelConstants.SLOT_CLASSTYPES_OBJECT_REF);
			        	   
			        	   classTypeRef = (EReference) classTypesObj.eClass().
								getEStructuralFeature(ModelConstants.SLOT_CLASSTYPE_LIST_REF);
						   classTypesList = (List) classTypesObj.eGet(classTypeRef);
						   EObject classTypeObj = EcoreUtils.createEObject(
									classTypeRef.getEReferenceType(), true);
							EcoreUtils.setValue(classTypeObj, "name", EcoreUtils.getName(source));
							classTypesList.add(classTypeObj);
					}
					saveResource(slotsObj.eResource());
					break;
				case 75: // Mismatch in 'Number of slots' specified in chassis and number of slots
					// configured in Node Admission Control
					slotsObj =  (EObject) relatedObjects.get(0);
					slotList = (List) EcoreUtils.getValue(slotsObj,
							ModelConstants.SLOT_LIST_REF);
					EObject chassisObj = (EObject) relatedObjects.get(1);
					int numSlots = ((Integer) EcoreUtils.getValue(chassisObj,
			        		 ClassEditorConstants.CHASSIS_NUM_SLOTS)).intValue();
					if (numSlots > slotList.size()) {
						editorDirty = isEditorDirty(caInput);
                        EcoreUtils.setValue(chassisObj, ClassEditorConstants.CHASSIS_NUM_SLOTS,
                                String.valueOf(slotList.size()));
                        latestEditorDirty = isEditorDirty(caInput);
                        FormatConversionUtils.convertToResourceFormat(
    							(EObject) chassisObj.eResource().getContents()
    									.get(0), ClassEditorConstants.EDITOR_TYPE);
                        saveResource(chassisObj.eResource());
                        if (latestEditorDirty && !editorDirty && caInput != null) {
                            caInput.getEditor().doSave(null);
                        }
					} else if (numSlots < slotList.size()) {
						switch (actionId) {
	            		case 'a':
	            			editorDirty = isEditorDirty(caInput);
	                        EcoreUtils.setValue(chassisObj, ClassEditorConstants.CHASSIS_NUM_SLOTS,
	                                String.valueOf(slotList.size()));
	                        latestEditorDirty = isEditorDirty(caInput);
	                        FormatConversionUtils.convertToResourceFormat(
	    							(EObject) chassisObj.eResource().getContents()
	    									.get(0), ClassEditorConstants.EDITOR_TYPE);
	                        saveResource(chassisObj.eResource());
	                        if (latestEditorDirty && !editorDirty && caInput != null) {
	                            caInput.getEditor().doSave(null);
	                        }
	            				break;
	            		case 'b':
	            			iterator = slotList.iterator();
	            			while (iterator.hasNext()) {
	            				EObject slotObj = (EObject) iterator.next();
	            				int slotNumber = ((Integer) EcoreUtils.getValue(slotObj,
	     	        				   ModelConstants.SLOT_NUMBER_FEATURE)).intValue();
	            				if (isExtraSlot(slotNumber, numSlots, slotList.size())) {
	            					iterator.remove();
	            				}
	            			}
	            				break;
						}
					}
				case 76:
					editorDirty = isEditorDirty(compInput);
					EcoreUtils.setValue(source, "isRestartable", "CL_TRUE");
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
					break;
					
				case 80:
				case 81:	
				case 82:
					editorDirty = isEditorDirty(compInput);
					EcoreUtils.setValue(source, "numPrefActiveSUsPerSI", String.valueOf(1));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
                    break;
				case 83:
					editorDirty = isEditorDirty(compInput);
					EcoreUtils.setValue(source, "maxActiveSIsPerSU", String.valueOf(1));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
                    break;
				case 84:
					editorDirty = isEditorDirty(compInput);
					EcoreUtils.setValue(source, "maxStandbySIsPerSU", String.valueOf(0));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
                    break;
				
				case 92:
					editorDirty = isEditorDirty(compInput);
                    EcoreUtils.setValue(source, "numMaxActiveCSIs", String.valueOf(1));
                    EcoreUtils.setValue(source, "numMaxStandbyCSIs", String.valueOf(1));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
                    break;
                    
				case 94:	
				case 97:	
				case 99:
					editorDirty = isEditorDirty(compInput);
                    EcoreUtils.setValue(source, "numMaxActiveCSIs", String.valueOf(1));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
                    break;
                    
				case 96:    
				case 98:    
				case 100:
					editorDirty = isEditorDirty(compInput);
                    EcoreUtils.setValue(source, "numMaxStandbyCSIs", String.valueOf(0));
                    latestEditorDirty = isEditorDirty(compInput);
                    FormatConversionUtils.convertToResourceFormat(
							(EObject) source.eResource().getContents()
									.get(0), ComponentEditorConstants.EDITOR_TYPE);
                    saveResource(source.eResource());
                    if (latestEditorDirty && !editorDirty && compInput != null) {
                        compInput.getEditor().doSave(null);
                    }
            default:
            		break;
			}
		}
		IWorkbenchPage page = WorkspacePlugin.getDefault().getWorkbench()
    	.getActiveWorkbenchWindow().getActivePage();
		if (page != null) {
			ProblemsView problemsView = ((ProblemsView) page
               .findView("com.clovis.cw.workspace.problemsView"));
			problemsView.refreshProblemsList();
		}
	}
	/**
	 * 
	 * @param slotNumber - Slot number to be checked
	 * @param numSlots - number of slots configured in chassis
	 * @param slotListSize - List of slots configured in slotmap
	 * @return true if it is extra slot else return false
	 */
     private boolean isExtraSlot(int slotNumber, int numSlots, int slotListSize)
     {
		for (int i = numSlots + 1; i <= slotListSize; i++) {
			if (i == slotNumber) {
				return true;
			}
		}
		return false;
	}
	/**
     * @param resource - Resource to be saved
     */
    private void saveResource(Resource resource)
    {
        try {
            EcoreModels.save(resource);
        } catch (IOException e) {
           LOG.warn("Resource cannot be written", e);
        }

    }
	/**
	 * Sets the menu manager
	 * @param manager - IMenuManager
	 */
	public static void setMenuManager(IMenuManager manager)
	{
		menuManager = manager;
	}
	/**
	 * sets the Project
	 * @param proj - IProject
	 */
	public static void setProject(IProject proj)
	{
		project = proj;
	}
    /**
     * sets the Shell
     * @param windowShell - Shell
     */
    public static void setShell(Shell windowShell)
    {
        shell = windowShell;
    }
    /**
     * Checks whether editor is dirty
     * @param geInput - Generic Editor Input
     */
    public static boolean isEditorDirty(GenericEditorInput geInput)
    {
        if (geInput != null && geInput.getEditor() != null
                && geInput.getEditor().getEditorModel() != null
                && geInput.getEditor().getEditorModel().isDirty()) {
            return true;
        }
        return false;
    }
}
