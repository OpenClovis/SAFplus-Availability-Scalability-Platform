/*
 * @(#) $RCSfile: AlarmRuleDialog.java,v $
 * $Revision: #4 $ $Date: 2007/04/30 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/AlarmRuleDialog.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * @(#) $RCSfile: AlarmRuleDialog.java,v $
 * $Revision: #4 $ $Date: 2007/04/30 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.cw.editor.ca.dialog;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.factory.WidgetProviderFactory;
import com.clovis.common.utils.ui.list.SelectionListView;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.project.data.ProjectDataModel;
/**
 * @author pushparaj
 *
 * This dialog is used to capture Generation and Suppression Rule
 */
public class AlarmRuleDialog extends TitleAreaDialog
{
    private List  _selAlarmObjsList;
    private List  _alarmObjList;
    private Model _viewModel;
    
    private Model _alarmRuleViewModel;
    private SelectionListView _alarmView;
    
    private String _selectedAlarmName;
    private boolean _needDefaultSuppressionRule;
    private List _tempResList;
    private CCombo _ruleCombo;
    /**
     * Constructor
     * @param shell Parent Shell
     * @param obj EObject
     * @param alarmsList Alarms List
     */
    public AlarmRuleDialog(Shell shell, String alarmName, Model alarmRuleViewModel, EObject obj, List alarmObjList)
    {
        super(shell);
        super.setShellStyle(SWT.CLOSE|SWT.MIN|SWT.MAX|SWT.RESIZE);
        
        _alarmRuleViewModel = alarmRuleViewModel;
        _alarmObjList = alarmObjList;
        _viewModel = new Model(obj.eResource(), obj).getViewModel();
        _selectedAlarmName = alarmName;
        
         List alarmNameList = (List) EcoreUtils.getValue(obj, ClassEditorConstants.ASSOCIATED_ALARM_IDS);
         _selAlarmObjsList = getAlarmObjectsFrmName(alarmNameList);       
    }
    
    /**
     * Constructor
     * @param shell Parent Shell
     * @param obj EObject
     * @param alarmsList Alarms List
     */
    public AlarmRuleDialog(Shell shell, String alarmName, Model alarmRuleViewModel, EObject obj, List alarmObjList, String resName, IProject project)
    {
        super(shell);
        super.setShellStyle(SWT.CLOSE|SWT.MIN|SWT.MAX|SWT.RESIZE);
        
        _alarmRuleViewModel = alarmRuleViewModel;
        _alarmObjList = alarmObjList;
        _viewModel = new Model(obj.eResource(), obj).getViewModel();
        _selectedAlarmName = alarmName;
        
         List alarmNameList = (List) EcoreUtils.getValue(obj, ClassEditorConstants.ASSOCIATED_ALARM_IDS);
                
        _needDefaultSuppressionRule = (alarmNameList.size() == 0) && (_alarmObjList.size() > 1);
        if(_needDefaultSuppressionRule) {
        	_tempResList = new ArrayList();
        	_selAlarmObjsList = getAlarmObjectsFrmName(getDefaultSuppRuleAlarms(project, resName));
        	for(int i = 0; i < _selAlarmObjsList.size(); i++) {
        		_tempResList.add(_selAlarmObjsList.get(i));
        	}
        } else {
        	_selAlarmObjsList = getAlarmObjectsFrmName(alarmNameList);
        }
    }
    public void enableOKButton(boolean status) {
		getButton(IDialogConstants.OK_ID).setEnabled(status);
	}
    /**
     * see org.eclipse.jface.dialogs.Dialog#okPressed()
     */
    protected void okPressed()
    {
    	String selectedRule = _ruleCombo.getItem(_ruleCombo.getSelectionIndex());
    	if(_needDefaultSuppressionRule && _selAlarmObjsList.size() > 0 && selectedRule.equals("CL_ALARM_RULE_LOGICAL_OR")) {
    		if ((_selAlarmObjsList.size() == _tempResList.size()) && _selAlarmObjsList.containsAll(_tempResList)) {
    			super.okPressed();
    			return;
    		}
    	}
    	if (_selAlarmObjsList.size() > 4)
    	{
			setErrorMessage("A maximum of four alarms can be selected for an alarm rule.");
			return;
    	}
    	EObject eObject = _viewModel.getEObject();
    	List alarmIDList = (List) EcoreUtils.getValue(eObject,
				ClassEditorConstants.ASSOCIATED_ALARM_IDS);
    	if (_selAlarmObjsList.size() > 0) {
			try {
				alarmIDList.clear();
				if(selectedRule.trim().equals("") && _selAlarmObjsList.size() > 0) {
					setErrorMessage("Dont select any alarams If you don't specified relation");
					return;
				}
				for (int i = 0; i < _selAlarmObjsList.size(); i++) {
					EObject selAlarmObj = (EObject) _selAlarmObjsList.get(i);
					String alarmID = (String) EcoreUtils.getValue(selAlarmObj,
							ClassEditorConstants.ALARM_ID);
					if(selectedRule.equals("CL_ALARM_RULE_NO_RELATION")) {
						if(!alarmID.equals(_selectedAlarmName)) {
							setErrorMessage("Select only " + _selectedAlarmName + " for NO Relation.");
							return;
						}
					}
					if(selectedRule.equals("CL_ALARM_RULE_LOGICAL_OR") || selectedRule.equals("CL_ALARM_RULE_LOGICAL_AND")) {
						if(alarmID.equals(_selectedAlarmName)) {
							setErrorMessage("Dont select " + _selectedAlarmName + " for AND/OR Relation.");
							return;
						}
					}
					alarmIDList.add(alarmID);
				}
				_viewModel.save(true);
				// Save the rule info in the view model only
				_alarmRuleViewModel.save(false);
			} catch (Exception e) {
				CaPlugin.LOG.error("Error in saving Alarm Rule", e);
			}
			super.okPressed();
		} else if(selectedRule.trim().equals("")){
			alarmIDList.clear();
			_viewModel.save(true);
			_alarmRuleViewModel.save(false);
			super.okPressed();
		} else {
			setErrorMessage("Select atleast one alarm for rule or don't select any relation");
		}
    }
    /**
	 * @see org.eclipse.jface.window.Window#close()
	 */
    public boolean close()
    {
        if (_viewModel != null) {
            _viewModel.dispose();
            _viewModel = null;
        }
        return super.close();
    }
    /**
     * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(
     *      org.eclipse.swt.widgets.Composite)
     */
    protected Control createDialogArea(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 2;
        container.setLayout(containerLayout);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));

        EObject eObject = _viewModel.getEObject();
        EClass eClass  = eObject.eClass();
        WidgetProviderFactory factory = new WidgetProviderFactory(eObject);

        Label relLabel = new Label(container, SWT.NONE);
        relLabel.setText("Relation:");
        relLabel.setLayoutData(new GridData(GridData.BEGINNING));
        _ruleCombo = factory.getComboBox(container, SWT.BORDER | SWT.READ_ONLY,
            (EStructuralFeature) eClass.getEStructuralFeature(ClassEditorConstants.ALARM_RELATIONTYPE));
        _ruleCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        _ruleCombo.add("", 0);
        if(_needDefaultSuppressionRule && _selAlarmObjsList.size() > 0) {
        	_ruleCombo.select(2);
        } else if(_alarmObjList.size() == 1) {
        	_ruleCombo.remove(3); //removing AND and OR Relation.
        	_ruleCombo.remove(2);
        }
        if(_ruleCombo.getSelectionIndex() == -1) {
        	_ruleCombo.select(0);
        }
        
        /*Label maxLabel = new Label(container, SWT.NONE);
        maxLabel.setText("Maximum number of participants:");
        maxLabel.setLayoutData(new GridData(GridData.BEGINNING));
        Text text = factory.getTextBox(container, SWT.BORDER,
            (EStructuralFeature) eClass.getEStructuralFeature("maxAlarm"));
        text.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
*/
        // Create Alarm Seletion Group
        Group selGroup     = new Group(container, SWT.BORDER);
        GridData groupData = new GridData(GridData.FILL_BOTH);
        groupData.horizontalSpan = 2;
        selGroup.setLayoutData(groupData);
        selGroup.setLayout(new GridLayout());
        selGroup.setText("Select alarms for this rule");
        _alarmView = new SelectionListView(selGroup,
                SWT.NONE, _selAlarmObjsList, _alarmObjList, null, true);
        _alarmView.setLayoutData(new GridData(GridData.FILL_BOTH));
        setTitle("Alarm Rule");
        parent.getShell().setText("Alarm Rule");
        setMessage("Configure Alarm Rule", IMessageProvider.INFORMATION);
        final String contextid = "com.clovis.cw.help.alarm_rule";
		container.addHelpListener(new HelpListener() {

			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem().displayHelp(
						contextid);
			}
		});
        return container;
    }
    /**
     * 
     * @param keyList List of AlarmProfile Keys
     * @return
     */
    private List getAlarmObjectsFrmName(List nameList)
    {
        List alarmIds = new ClovisNotifyingListImpl();
        for (int i = 0; i < _alarmObjList.size(); i++) {
            EObject alarmObj = (EObject) _alarmObjList.get(i);
            String name = (String) EcoreUtils.getName(alarmObj);
            if (nameList.contains(name)) {
                alarmIds.add(alarmObj);
            } else {
                String cwkey = (String) EcoreUtils.getValue(alarmObj,
                		ModelConstants.RDN_FEATURE_NAME);
                if (nameList.contains(cwkey)) {
                    alarmIds.add(alarmObj);
                }
            }
        }
        return alarmIds;
    }
    /**
     * Find and Returns alarm ids for default supp rule
     * @param project
     * @param resName
     * @return
     */
    private List getDefaultSuppRuleAlarms(IProject project, String resName) {
    	ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
    	Model alarmAssociationModel = pdm.getAlarmAssociationModel();
    	List<EObject> resourceList = (List<EObject>) EcoreUtils.getValue(
				(EObject) alarmAssociationModel.getEList().get(0), "resource");
    	List alarmIDs = new ArrayList();
    	for(int i = 0; i < resourceList.size(); i++) {
    		EObject resObject = resourceList.get(i);
    		if (resName.equals(EcoreUtils.getName(resObject))) {
				List<EObject> attributeList = (List<EObject>) EcoreUtils
						.getValue(resObject, "attribute");
				int severity = getSeverityForSelectedAlarm(attributeList);
				if(severity == 0) {
					return alarmIDs;
				} 
				for (int j = 0; j < attributeList.size(); j++) {
					EObject attrObj = attributeList.get(j);
					List<EObject> alarmList = (List<EObject>) EcoreUtils
					.getValue(attrObj, "alarm");
					for(int k = 0; k < alarmList.size(); k++) {
						EObject alarmObj = alarmList.get(k);
						String alarmID = (String) EcoreUtils.getValue(alarmObj, "alarmID");
						if(!_selectedAlarmName.equals(alarmID)) {
							if (getNumericValueForSeverity((String)EcoreUtils.getValue(alarmObj, "severity")) > severity) {
								alarmIDs.add(alarmID);
							}
						}
					}
				}
			}
    	}
    	return alarmIDs;
    }
    /**
     * Find and Returns severity value for the alarm
     * @param attributeList
     * @return
     */
    private int getSeverityForSelectedAlarm(List attributeList) {
    	for (int j = 0; j < attributeList.size(); j++) {
			EObject attrObj = (EObject)attributeList.get(j);
			List<EObject> alarmList = (List<EObject>) EcoreUtils
			.getValue(attrObj, "alarm");
			for(int k = 0; k < alarmList.size(); k++) {
				EObject alarmObj = alarmList.get(k);
				if(_selectedAlarmName.equals(EcoreUtils.getValue(alarmObj, "alarmID"))) {
					return getNumericValueForSeverity((String)EcoreUtils.getValue(alarmObj, "severity"));
				}
			}
		}
    	return 0;
    }
    /**
     * Returns the numeric value for the severity
     * @param severity
     * @return
     */
    private int getNumericValueForSeverity(String severity) {
    	if(severity.equals("Critical"))	{ return 4;	} 
    	else if(severity.equals("Major")) { return 3; } 
    	else if(severity.equals("Minor")) { return 2; } 
    	else if(severity.equals("Warning")) { return 1; }
    	return 0;
    }
}
