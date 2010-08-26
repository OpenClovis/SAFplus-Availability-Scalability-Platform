/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/action/TableDoubleClickListener.java $
 * $Author: swapnesh $
 * $Date: 2007/03/01 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.action;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.action.SNMPCodeGenPropertiesAction;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.model.NodeModel;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * 
 * @author shubhada
 *
 * TableDoubleClickListener listenes to double clicks
 * happening on the table objects 
 */
public class TableDoubleClickListener implements IDoubleClickListener
{
    /**
     * @param event - DoubleClickEvent
     */
    public void doubleClick(DoubleClickEvent event)
    {
        TableViewer viewer = (TableViewer) event.getViewer();
        Shell shell = viewer.getTable().getShell();
        EObject selObj = (EObject) ((IStructuredSelection) event.
                getSelection()).getFirstElement();
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
        GenericEditorInput caInput = (GenericEditorInput) pdm.
            getCAEditorInput();
        GenericEditorInput compInput = (GenericEditorInput) pdm.
            getComponentEditorInput();
        int problemNumber = ((Integer) EcoreUtils.
                getValue(selObj, "problemNumber")).intValue();
        EObject source = (EObject) EcoreUtils.getValue(
                selObj, "source");
        switch(problemNumber) {
        case 1: // Resource having 2 alarms of same probable cause
        case 2: // Resource has a associated alarm which is 
                //deleted from Alarm Profile configuration
        case 3: //Alarm Service object has a associated DO which is 
                //deleted from DO List of resource
        case 4: // Provisioning Service object has a associated DO 
                //which is deleted from DO List of resource
        case 5: // Provisioning enabled on the resource 
                //without having any attributes to be provisioned.
        case 6: // Alarm enabled on the resource without having any alarms
                //associated to the resource.
        case 7: // Isolated resource in the Resource Editor
        		selectEditorProblemSource(ClassEditorConstants.EDITOR_TYPE, caInput, source);
                break;
        case 8: // Invalid connection due to dependency on other connection(s)
        case 9: // Component image name is empty
        case 10: // Duplicate component image name
        case 11: // Component does not have associated resource(s)
        case 12: // Associated resource does not exist in 
                //list of resources of resource editor
        		selectEditorProblemSource(ComponentEditorConstants.EDITOR_TYPE, compInput, source);
                break;
        case 13: // Proxied component doesn't belongs to proper SG and Node hierarchy
                selectEditorProblemSource(ComponentEditorConstants.EDITOR_TYPE, compInput, source);
                break;
        case 14: // One of associated resource has provisioning/alarm
                 //enabled with associated device object(s) but HAL
                 //library is not selected
                
        case 15: // One of associated resources has provisioning /alarm
                 //enabled but OM library is not selected
                
        case 16: // HAL Library is selected when component does not have
                 //associated hardware resource with prov/alarm enabled
                
        case 17: // PROV library for component is selected but none of 
                 //associated resource has provisioning enabled
               
        case 18: // PROV library for component is not selected but 
                 //associated resource has provisioning enabled
                
        case 19: // ALARM library for component is selected but none of
                 //associated resources has alarm as enabled
                
        case 20: // ALARM library for component is not selected but
                 //associated resource has alarm as enabled
                
        case 21: // HAL Library is selected when none of the associated
                 //resources have associated device objects for provisioning/alarm
                
        case 22: // ServiceGroup/CSI is not associated to any of  the 
                 //ServiceUnit/Component
                
        case 23: //ServiceUnit/Component is not associated to any of the ServiceGroup/CSI
                
        case 24: //Component has duplicate EO name
               
        case 25: //Single CSI type shared by a Proxy and Proxied is not valid
                
        case 26: //'Active SUs' field value should be 1 when 
                 //redundancy model is 'NO_REDUNDANCY''
                
        case 27: //'Standby SUs' field value should be 0 when 
                 //redundancy model is 'NO_REDUNDANCY'
                
        case 28: //'Inservice SUs' field value should be1 when 
                 //redundancy model is 'NO_REDUNDANCY'
                
        case 29: //'Active SUs' field value should be1 when 
                //redundancy model is 'TWO_N'
            
        case 30: //'Standby SUs' field value should be1 when
                 //redundancy model is 'TWO_N'
            
        case 31: //'Inservice SUs' field value should be >=2 when
                 //redundancy model is 'TWO_N'
            
        case 32: //'Assigned SUs' field value should be 2 when
                 //redundancy model is 'TWO_N'
        		selectEditorProblemSource(ComponentEditorConstants.EDITOR_TYPE, compInput, source);
                break;
        case 33: //Component/SU/Node instance Configuration has
                 //reference to Invalid Component/SU/Node type
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);

                break;
        case 34: //ServiceUnit instance does not have any 
                 //Component instances defined
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);

                break;
        case 35: //ServiceInstance instance has more number of CSI
                //instances configured than what is specified in SI type definition
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);

                break;
        case 36: //ServiceGroup instance does not have any 
                 //ServiceInstance instances defined
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);

                break;
        case 37: //ServiceInstance does not have any CSI instance defined
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);

                break;
        case 38: //ServiceGroup instance should have atleast two ServiceUnit
                 //instances associated if it's redundancy model is TWO_N
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);

                break;
        case 39: //There are no Node instances defined in AMF Configuration
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);

                break;
        case 40: //There are no ServiceGroup instances defined in AMF Configuration
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);
                break;
        case 41: //Node/SU/Component/SG/SI/CSI instance name is duplicate
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);
                break;
        case 42: //ServiceUnit instance is not associated to any ServiceGroup instance
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);
                break;
        case 43: //Component Editor Object does not have relationship to any other object
    			selectEditorProblemSource(ComponentEditorConstants.EDITOR_TYPE, compInput, source);
                break;
        case 44: //Atleast one node of type 'Class A' or 'Class B' should
                 //be present in the component editor
                OpenComponentEditorAction.openComponentEditor(_project);
                break;
        case 45: //Mismatch in 'Number of additional clients' defined in RMD
                 //and 'Number of additional clients' specified in EOProperties
                IDLAction.openRMDDialog(shell, _project);
                break;
        case 46: //AMF Configuration not done
                NodeProfileAction.openNodeProfileDialog(shell, _project);
                break;
        case 47: //ComponentInstances Configuration has Invalid Resource
                NodeProfileAction.openNodeProfileDialog(shell, _project, source);
                break;
        case 48: //There is no service defined for the client in RMD
            	IDLAction.openRMDDialog(shell, _project);
            	break;
        case 49: //There are missing associated resources for the component in AMF
        		NodeProfileAction.openNodeProfileDialog(shell, _project,source);
            	break;
        case 50: //The Slot Configuration is not done.
        		//OpenBootConfigurationAction.openBootConfigurationDialog(shell, _project);
        		break;
        case 51: ////Either of the provisioning or alarm is not enabled 
    			//for the resources.
        		selectEditorProblemSource(ClassEditorConstants.EDITOR_TYPE, caInput, source);
        		break;
        case 52: ////There cant be more than one snmp subagent
        		OpenComponentEditorAction.openComponentEditor(_project);
        		break;
        case 53: //SNMP subagent cannot be proxy
        		selectEditorProblemSource(ComponentEditorConstants.EDITOR_TYPE, compInput, source);
                break;
        case 54: // Transaction lib is mandatory if prov lib for the component is selected
        	    break;
        case 55: 
        		selectEditorProblemSource(ComponentEditorConstants.EDITOR_TYPE, compInput, source);
        		break;
        case 61: //	Associated Service Units validation
				NodeProfileAction.openNodeProfileDialog(shell, _project, source);
				break;		
        case 62: //	Primary OI is true for the same moId for more than one component
				NodeProfileAction.openNodeProfileDialog(shell, _project, source);
				break;
        case 69: // MO ID Validation
				NodeProfileAction.openNodeProfileDialog(shell, _project, source);
				break;
        case 70: //	There should be atleast one Primary OI for the Resource
			//NodeProfileAction.openNodeProfileDialog(shell, _project, source);
        		break;
        case 71: //The IOC Configuration is not done.
        		OpenBootConfigurationAction.openBootConfigurationDialog(shell, _project);
        		break;
        case 72: //The Link Configuration is not done.
        		OpenBootConfigurationAction.openBootConfigurationDialog(shell, _project);
        		break;
        case 76: // SU type is configured as restartable but the constituent component is not restartable.	
        	
        case 78: // Invalid Redundancy Model
        	        	
        case 79: // Invalid Loading Strategy
        	        	
        case 80: // Active SUs Per SI should be 1 for NO_REDUNDANCY
        	
        case 81: // Active SUs Per SI should be 1 for 2N_REDUNDANCY
        	
        case 82: // Active SUs Per SI should be 1 for M+N REDUNDANCY
        	
        case 83: // MaxActiveSIsPerSU's value should be 1 when Associated ServiceUnit is Non-Preinstantiable
        
        case 84: // MaxStandbySIsPerSU's value should be 1 when Associated ServiceUnit is Non-Preinstantiable
        
        case 85: // Number of components per SU should be >=1
        
        case 86: // Number of CSI per SI should be >=1
        		selectEditorProblemSource(ComponentEditorConstants.EDITOR_TYPE, compInput, source);
        		break;

        case 87: // Service Instances have circular dependency
        
        case 88: // Node Instances have circular dependency
        	
        case 89: //CSI should contain atleast one component in the SU list for the SI, 
        	     //which has a supported CSI type same as the  type for the CSI in the CSI list
        	
        case 90: // CSI list should contain one instance of each CSI type(SI contaned CSI)
        		NodeProfileAction.openNodeProfileDialog(shell, _project,source);
        		break;        
        
        case 91: // Associted CSI types should be equal to Number of CSIs which is captured in SI
        
        case 92: // Number of Maximum Active CSIs and Number of Maximum Standby CSIs value should be 1 when Capability Model is 'One Active or One Standby'
        	
        case 93: // Number of Maximum Standby CSIs value should be > 0 when Capability Model is 'X Active or Y Standby'
        
        case 94: // Number of Maximum Active CSIs should be 1 when Capability Model is 'One Active or X Standby'
        	
        case 95: // Number of Maximum Standby CSIs should be > 0 when Capability Model is 'One Active or X Standby'
        
        case 96: // Number of Maximum Standby CSIs should be 0 when Capability Model is 'X Active'
        	
        case 97: // Number of Maximum Active CSIs should be 1 when Capability Model is 'One Active'
        
        case 98: // Number of Maximum Standby CSIs should be 0 when Capability Model is 'One Active'
        
        case 99: // Number of Maximum Active CSIs should be 1 when Capability Model is 'Non Preinstantiable'
        	
        case 100: // Number of Maximum Standby CSIs should be 0 when Capability Model is 'Non Preinstantiable'
        	
        case 101: // Number of Maximum Standby CSIs should be > 0 when Capability Model is 'X Active Y Standby'
        	selectEditorProblemSource(ComponentEditorConstants.EDITOR_TYPE, compInput, source);
        	break;
        	
        case 103: // module name is not configured for SNMP sub agent
        case 104: // SNMP sub agent not configured with mib path	
        case 105: // SNMP sub agent not configured with mib info
        case 106: // SNMP sub agent configured with invalid module identity.
        case 109: // SNMP sub agent configured with node name.	
           		selectEditorProblemSource(ComponentEditorConstants.EDITOR_TYPE, compInput, source);
           		SNMPCodeGenPropertiesAction.openSNMPConfigDialog(source, _project, null);
        		break;
        		
        default:
        	break;
        }
    
    }
    /**
     * sets the selected project
     * @param project - IProject
     */
    public void setProject(IProject project)
    {
        _project = project;
    }
	private IProject _project = null;

	/**
	 * Selects the problem source for the editor.
	 * 
	 * @param type
	 * @param editorInput
	 * @param source
	 */
	private void selectEditorProblemSource(String type,
			GenericEditorInput editorInput, EObject source) {

		if (type.equals(ClassEditorConstants.EDITOR_TYPE)) {
			OpenResourceEditorAction.openResourceEditor(_project);
			if(editorInput == null) {
				ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		        editorInput = (GenericEditorInput) pdm.getCAEditorInput();
			}
		} else if (type.equals(ComponentEditorConstants.EDITOR_TYPE)) {
			OpenComponentEditorAction.openComponentEditor(_project);
			if(editorInput == null) {
				ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		        editorInput = (GenericEditorInput) pdm.getComponentEditorInput();
			}
		}

		if (editorInput != null && editorInput.getEditor() != null
				&& editorInput.getEditor().getEditorModel() != null) {

			NodeModel nodeModel = editorInput.getEditor().getEditorModel()
					.getNodeModel(EcoreUtils.getName(source));

			if (nodeModel != null) {
				nodeModel.setSelection();
				nodeModel.setFocus();
			}
		}
	}
}
