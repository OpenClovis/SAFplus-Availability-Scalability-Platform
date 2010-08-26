/*
 * @(#) $RCSfile: AssociateAlarmsPage.java,v $
 * $Revision: #8 $ $Date: 2007/05/09 $
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
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/AssociateAlarmsPage.java $
 * $Author: bkpavan $
 * $Date: 2007/05/09 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * @(#) $RCSfile: AssociateAlarmsPage.java,v $
 * $Revision: #8 $ $Date: 2007/05/09 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.cw.editor.ca.dialog;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Vector;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.List;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.list.ListView;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.data.SubModelMapReader;


/**
 * @author pushparaj
 *
 * Alarms Page
 */
public class AssociateAlarmsPage extends PreferencePage
{
    private EList    _associatedAlarmsList;

    private EList    _alarmsList;
    
    private EList    _alarmRulesIdList;
    
    private java.util.List   _resourceList;

    private EObject _resObj;
    
    private EObject _alarmRuleResObj;

    private Model    _alarmModel;

    private Model    _alarmRuleViewModel;
    
    private String 	 _selectedAlarmName;
    
    private IProject _project;
    
    private ListView _alarmListViewer;

    private ListView _associateAlarmListViewer;
    
    private Button _rightButton;
    
    private Button _genRuleButton;
    
    private Button _supressionButton;
    
    private HashMap _alarmIDObjMap = new HashMap();
     
    private HashMap _alarmRuleMap = new HashMap();
    
    private Model _mapViewModel;
    
    private static final String ALARM_GENERATION_RULE = "GenRule";
    
    private static final String ALARM_SUPPRESSION_RULE = "SuppressRule";
    
    /**
     * Constructor.
     * @param alarmsList Alarm profile list
     * @param associatedList Associated Alarm list
     * @param name Name of the Page
     */
    public AssociateAlarmsPage(Model alarmModel, Model alarmRuleViewModel,Model mapModel, EObject alarmRuleResObj, 
    		IProject project, EObject resObj, EObject linkObj,
            java.util.List resList, String name)
    {
        super(name);
        noDefaultAndApplyButton();
        _resourceList = resList;
        _alarmModel = alarmModel;
        
        _alarmRuleViewModel = alarmRuleViewModel;
        _alarmRuleResObj = alarmRuleResObj;
        _mapViewModel = mapModel;
        
        _project = project;
        _resObj = resObj;
        EObject alarmInfoObj = (EObject) _alarmModel.getEList().get(0);
        _alarmsList = (EList) EcoreUtils.getValue(alarmInfoObj, "AlarmProfile");
        
        initAlarmsMap();
        
        String rdn = (String) EcoreUtils.getValue(resObj, ModelConstants.RDN_FEATURE_NAME);
        _associatedAlarmsList = (NotifyingList) SubModelMapReader.
        	getLinkTargetObjects(linkObj, rdn);
        if (_associatedAlarmsList == null) {
        	_associatedAlarmsList = (NotifyingList) SubModelMapReader.createLinkTargets(
        			linkObj, EcoreUtils.getName(resObj), rdn);
        }
        Iterator iterator = _associatedAlarmsList.iterator();
        while (iterator.hasNext()) {
            String associatedAlmName = (String) iterator.next();
            EObject associatedAlmObj = (EObject) _alarmIDObjMap.get(associatedAlmName);
            if (associatedAlmObj == null)
            {
                iterator.remove();
            }
        }
        
        
        if(null == _alarmRuleResObj){
 		   return;
 	   }
        initAlarmRulesMap(); 
    }
    
    
    
    
    /**
	 * initializes the AlarmIDMap
	 * 
	 */
    private void initAlarmsMap()
    {
        for (int i = 0; i < _alarmsList.size(); i++)
        {
                EObject almObj = (EObject) _alarmsList.get(i);
                String almID = (String) EcoreUtils.getValue(almObj,
                        ClassEditorConstants.ALARM_ID);
                _alarmIDObjMap.put(almID, almObj);
        }
    }
    
    /**
     * Initializes resourse to alarm rules map 
     *
     */
    private void initAlarmRulesMap()
    {
    	if(null == _alarmRuleResObj)
    		return;
    	
    	      
    	EList alarmList = (EList) EcoreUtils.getValue(_alarmRuleResObj, 
    			ClassEditorConstants.ALARM_ALARMOBJ);
    	
    	if(null == alarmList)
    		return;
    	
    	for(int i = 0; i < alarmList.size(); i++){
        	
        	EObject almObj = (EObject) alarmList.get(i);
        	
            String almID = (String) EcoreUtils.getValue(almObj,
                        ClassEditorConstants.ALARM_ID);
        		
            _alarmRuleMap.put(almID, almObj);
        	}
        		
    }
    
    /**
     * @see org.eclipse.jface.preference.PreferencePage#createContents(
     * org.eclipse.swt.widgets.Composite)
     */
    protected Control createContents(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 5;
        container.setLayout(containerLayout);
        GridData data = new GridData(GridData.FILL_BOTH);
        container.setLayoutData(data);

        Group resourceGroup = new Group(container, SWT.BORDER);
        resourceGroup.setText("Available Alarms");
        GridLayout layout1 = new GridLayout();
        layout1.numColumns = 1;
        resourceGroup.setLayout(layout1);
        data = new GridData(GridData.FILL_HORIZONTAL | GridData.FILL_VERTICAL);
        data.horizontalSpan = 2;

        resourceGroup.setLayoutData(data);
        List resourceList = new List(resourceGroup, SWT.FULL_SELECTION
                                                    | SWT.HIDE_SELECTION
                                                    | SWT.V_SCROLL
                                                    | SWT.H_SCROLL | SWT.BORDER
                                                    | SWT.MULTI);
        _alarmListViewer = new ListView(resourceList);
        java.util.List availableAlarms = getAvailableAlarms(_resourceList, _alarmsList);
        _alarmListViewer.setInput(availableAlarms);
        //data = new GridData(GridData.FILL_BOTH);
        GridData listData = new GridData(SWT.FILL, SWT.FILL, true, true);
		Rectangle bounds = Display.getCurrent().getClientArea();
		listData.heightHint = (int) (1.5 * bounds.height / 9);
		listData.widthHint = bounds.width / 10;
        resourceList.setLayoutData(listData);

        Composite buttonControl = new Composite(container, SWT.NONE);
        GridLayout layout3 = new GridLayout();
        layout3.numColumns = 1;
        buttonControl.setLayout(layout3);
        data = new GridData(GridData.HORIZONTAL_ALIGN_CENTER
                            | GridData.FILL_VERTICAL);
        data.horizontalSpan = 1;
        buttonControl.setLayoutData(data);
        _rightButton = new Button(buttonControl, SWT.BORDER);
        _rightButton.setText("Add");
        data = new GridData(GridData.FILL_HORIZONTAL);
        _rightButton.setLayoutData(data);
        
         Button leftButton = new Button(buttonControl, SWT.BORDER);
         leftButton.setText("Delete");
        data = new GridData(GridData.FILL_HORIZONTAL);
        leftButton.setLayoutData(data);
        
        _genRuleButton = new Button(buttonControl, SWT.BORDER);
        _genRuleButton.setText(ALARM_GENERATION_RULE);
        data = new GridData(GridData.FILL_HORIZONTAL);
        _genRuleButton.setLayoutData(data);
       
        _supressionButton = new Button(buttonControl, SWT.BORDER);
        _supressionButton.setText(ALARM_SUPPRESSION_RULE);
        data = new GridData(GridData.FILL_HORIZONTAL);
        _supressionButton.setLayoutData(data);
       
        Group associateResourceGroup = new Group(container, SWT.BORDER);
        associateResourceGroup.setText("Selected Alarms");
        GridLayout layout2 = new GridLayout();
        layout2.numColumns = 1;
        associateResourceGroup.setLayout(layout2);
        data = new GridData(GridData.FILL_HORIZONTAL | GridData.FILL_VERTICAL);
        data.horizontalSpan = 2;

        associateResourceGroup.setLayoutData(data);
        List associateResourceList = new List(associateResourceGroup,
                SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.V_SCROLL
                                                                | SWT.H_SCROLL
                                                                | SWT.BORDER
                                                                | SWT.MULTI);
        _associateAlarmListViewer = new ListView(associateResourceList);
        _associateAlarmListViewer.setInput(_associatedAlarmsList);
        //data = new GridData(GridData.FILL_BOTH);
        listData = new GridData(SWT.FILL, SWT.FILL, true, true);
		bounds = Display.getCurrent().getClientArea();
		listData.heightHint = (int) (1.5 * bounds.height / 9);
		listData.widthHint = bounds.width / 10;
        associateResourceList.setLayoutData(listData);

        _rightButton.addSelectionListener(new AddListener());
        leftButton.addSelectionListener(new DeleteListener());
        _genRuleButton.addSelectionListener(new RuleListener());
        _supressionButton.addSelectionListener(new RuleListener());
        
        _alarmListViewer.refresh();
        _associateAlarmListViewer.addSelectionChangedListener(new ListSelectionListener());
        return container;
    }
    /**
     * 
     * @param resList - Resource Object List
     * @param alarmsList - List of all alarm profile objects
     * @return the list of alarms available for associating to resource 
     */
    public static java.util.List processAvailableAlarms(IProject project, java.util.List resList,
            java.util.List alarmsList)
    {
        java.util.List availableAlarms = new ClovisNotifyingListImpl();
        availableAlarms.addAll(alarmsList);
        
        HashMap alarmIDObjMap = new HashMap();
        for (int i = 0; i < alarmsList.size(); i++) {
            EObject alarmObj = (EObject) alarmsList.get(i);
            String alarmId = (String) EcoreUtils.getValue(alarmObj,
                    ClassEditorConstants.ALARM_ID);
            alarmIDObjMap.put(alarmId, alarmObj);
            
        }
        
       for (int i = 0; i < resList.size(); i++) {
           EObject resObj = (EObject) resList.get(i);
           //change this code
           java.util.List alarmIds = ResourceDataUtils.
               getAssociatedAlarms(project, resObj);
           if (alarmIds != null) {
	           for (int j = 0; j < alarmIds.size(); j++) {
	               String alarmID = (String) alarmIds.get(j);
	               EObject assoAlarmObj = (EObject) alarmIDObjMap.get(alarmID);
	               if (assoAlarmObj != null) {
	            	   availableAlarms.remove(assoAlarmObj);
	               }
	               
	           }
           }
           
       }
        return availableAlarms;
    }
    /**
     * 
     * @param resList - Resource Object List
     * @param alarmsList - List of all alarm profile objects
     * @return the list of alarms available for associating to resource 
     */
    private java.util.List getAvailableAlarms(java.util.List resList,
            java.util.List alarmsList)
    {
        java.util.List availableAlarms = new ClovisNotifyingListImpl();
        availableAlarms.addAll(alarmsList);

		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		EObject mapObj = _mapViewModel.getEObject();
		EObject linkObj = SubModelMapReader.getLinkObject(mapObj,
				ClassEditorConstants.ASSOCIATED_ALARM_LINK);
		String rdn = (String) EcoreUtils.getValue(_resObj,
				ModelConstants.RDN_FEATURE_NAME);
		java.util.List alarmIds = SubModelMapReader.getLinkTargetObjects(
				linkObj, rdn);
		if (alarmIds != null) {
			for (int j = 0; j < alarmIds.size(); j++) {
				String alarmID = (String) alarmIds.get(j);
				EObject assoAlarmObj = (EObject) _alarmIDObjMap.get(alarmID);
				if (assoAlarmObj != null) {
					availableAlarms.remove(assoAlarmObj);
				}
			}
		}

		return availableAlarms;
    }
    /**
	 * Checks the alarms associated with a resource to ensure that no
	 * two alarms have the same Probable Cause and Specific Problem.
	 * @param obj Alarm Profile EObject
	 * @return true if more than one alarm is found with the same probable
	 *              cause and specific problem
	 */
    private boolean checkForDuplicateAlarm(EObject obj)
    {
        Vector selAlarmObjs = new Vector();
        for (int i = 0; i < _alarmsList.size(); i++) {
            EObject alarmObj = (EObject) _alarmsList.get(i);
            String alarmID = EcoreUtils.getName(alarmObj);
            if (_associatedAlarmsList.contains(alarmID)) {
                selAlarmObjs.add(alarmObj);
            }
        }
        String probableCause = EcoreUtils.getValue(obj,
                ClassEditorConstants.ALARM_PROBABLE_CAUSE).toString();
        int specificProblem = Integer.parseInt((EcoreUtils.getValue(obj,
                ClassEditorConstants.ALARM_SPECIFIC_PROBLEM)).toString());
        
        for (int i = 0; i < selAlarmObjs.size(); i++) {
            EObject alarmObj = (EObject) selAlarmObjs.get(i);
            String pc = EcoreUtils.getValue(alarmObj,
                    ClassEditorConstants.ALARM_PROBABLE_CAUSE).toString();
            int sp = Integer.parseInt((EcoreUtils.getValue(alarmObj,
                    ClassEditorConstants.ALARM_SPECIFIC_PROBLEM)).toString());
            if (pc.equals(probableCause) && sp == specificProblem) {
                return true;
            }
        }
        return false;
    }
    /**
    *
    * @author pushparaj
    *
    * Listener for associating resources for component
    */
   class AddListener implements SelectionListener
   {
       /**
        * @see org.eclipse.swt.events.SelectionListener#widgetSelected(
        * org.eclipse.swt.events.SelectionEvent)
        */
       public void widgetSelected(SelectionEvent e)
       {
           StructuredSelection sel =
               (StructuredSelection) _alarmListViewer.getSelection();
           java.util.List list = sel.toList();
           for (int i = 0; i < list.size(); i++) {
               EObject obj = (EObject) list.get(i);
                                            
               boolean duplicate = checkForDuplicateAlarm(obj);
               String name = (String) EcoreUtils.getName(obj);
               if (!_associatedAlarmsList.contains(name)
                       && _associatedAlarmsList.size() < 64) {
                   if (!duplicate) {
                       _associatedAlarmsList.add(name);
                       ((java.util.List) _alarmListViewer.getInput()).remove(obj);
               
                       addAlarmIDEntryToAlarmRule(obj);
                       
                   } else {
                       setMessage("Cannot associate 2 alarms which have same probable cause and specific problem",
                               IMessageProvider.INFORMATION);
                   }
               }
           }
           if (_associatedAlarmsList.size() >= 64) {
               _rightButton.setEnabled(false);
               setMessage("Resource can have maximum of 64 alarms associated to it",
                  IMessageProvider.INFORMATION);
           } else {
               _rightButton.setEnabled(true);
           }
       }
       
       
    /**
        * @see org.eclipse.swt.events.SelectionListener#widgetDefaultSelected(
        * org.eclipse.swt.events.SelectionEvent)
        */
       public void widgetDefaultSelected(SelectionEvent e)
       {
       }
   }
   
   /**
    * Adds entry for alarmID object to alarm rule
    * @param alarmObj - alarm associated to the resource
    */
   private void addAlarmIDEntryToAlarmRule(EObject alarmObj)
   {
	   	if(null == _alarmRuleResObj){
	   		return;
	   	}
	   
	   	EReference alarmRef = (EReference) _alarmRuleResObj.eClass()
	   		.getEStructuralFeature(ClassEditorConstants.ALARM_ALARMOBJ);

		java.util.List alarmList = (java.util.List) _alarmRuleResObj
				.eGet(alarmRef);
	
		String alarmID = (String) EcoreUtils.getValue(alarmObj, 
				ClassEditorConstants.ALARM_ID);
			
		if(!_alarmRuleMap.containsKey(alarmID)){
						
			EObject newObj = EcoreUtils.createEObject(alarmRef
						.getEReferenceType(), true);
		    EObject genRule = (EObject)EcoreUtils.getValue(newObj, ClassEditorConstants.ALARM_GENERATIONRULE);
		    EList genIds = (EList)EcoreUtils.getValue(genRule, "alarmIDs");
		    genIds.add(alarmID);
			EcoreUtils.setValue(newObj, ClassEditorConstants.ALARM_ID,
						alarmID);
			alarmList.add(newObj);
			
			_alarmRuleMap.put(alarmID, newObj);
			
			//_alarmRuleViewModel.save(false);
			
		}
	   
   }
   
   
   
    /**
    *
    * @author pushparaj
    *
    * Listener for removing associated resources for component
    */
   class DeleteListener implements SelectionListener
   {
       /**
        * @see org.eclipse.swt.events.SelectionListener#widgetSelected(
        * org.eclipse.swt.events.SelectionEvent)
        */
       public void widgetSelected(SelectionEvent e)
       {
           StructuredSelection sel = (StructuredSelection)
           _associateAlarmListViewer.getSelection();
            _associatedAlarmsList.removeAll(sel.toList());
            java.util.List selList = sel.toList();
            for (int i = 0; i < selList.size(); i++) {
                String alarmID = (String) selList.get(i);
                                               
                EObject assoAlarmObj = (EObject) _alarmIDObjMap.get(alarmID);
                if (assoAlarmObj != null) {
                    ((java.util.List) _alarmListViewer.getInput()).add(
                            assoAlarmObj);
                    
                    removeAlarmIDEntryFromAlarmRule(alarmID);
                }
            }
            if (_associatedAlarmsList.size() >= 64) {
                _rightButton.setEnabled(false);
                setMessage("Resource can have maximum of 64 alarms associated to it",
                        IMessageProvider.INFORMATION);
            } else {
                _rightButton.setEnabled(true);
            }
       }
       /**
        * @see org.eclipse.swt.events.SelectionListener#widgetDefaultSelected(
        * org.eclipse.swt.events.SelectionEvent)
        */
       public void widgetDefaultSelected(SelectionEvent e)
       {
       }
   }
   
   /**
    * Removes entry for 'alarmId' from alarm rule
    * 
    * @param alarmID - alarm id
    */
   private void removeAlarmIDEntryFromAlarmRule(String alarmID) {

		if (null == _alarmRuleResObj) {
			return;
		}

		EReference alarmRef = (EReference) _alarmRuleResObj.eClass()
				.getEStructuralFeature(ClassEditorConstants.ALARM_ALARMOBJ);

		java.util.List alarmList = (java.util.List) _alarmRuleResObj
				.eGet(alarmRef);

		if (_alarmRuleMap.containsKey(alarmID)) {

			alarmList.remove(_alarmRuleMap.get(alarmID));
		
			_alarmRuleMap.remove(alarmID);

			//_alarmRuleViewModel.save(false);

		}

	}
   
   
   /**
	 * 
	 * @author pushparaj
	 * 
	 * Listener which will open AlarmRuleDialog \ for configuring generation
	 * rule and supression rule.
	 */
  class RuleListener implements SelectionListener
  {
      /**
       * @see org.eclipse.swt.events.SelectionListener#widgetSelected(
       * org.eclipse.swt.events.SelectionEvent)
       */
      public void widgetSelected(SelectionEvent e)
      {
          Button button = (Button) e.getSource();
          
          _selectedAlarmName = (String) ((StructuredSelection) _associateAlarmListViewer.
                  getSelection()).getFirstElement();
       
          if(null == _selectedAlarmName ||
        		  _selectedAlarmName.equals(""))
        	  return;
                                        
          EObject selAlarmObj = (EObject) _alarmRuleMap.get(_selectedAlarmName);
          
          if (null == selAlarmObj) {
        	  
        	  EObject alarmRuleInfoObj = (EObject) _alarmRuleViewModel.getEList().get(0);
                      
    		  EReference alarmRef = (EReference) _alarmRuleResObj.eClass().
    		 		getEStructuralFeature(ClassEditorConstants.ALARM_ALARMOBJ);

       		  java.util.List alarmList = (java.util.List) _alarmRuleResObj.eGet(alarmRef);
  		  
       		  selAlarmObj = EcoreUtils.createEObject(alarmRef.
        				  getEReferenceType(), true);
  		  
       		  alarmList.add(selAlarmObj);
  		  
       		  EcoreUtils.setValue(selAlarmObj, ClassEditorConstants.ALARM_ID, 
        				  _selectedAlarmName);
         
          }
          
          
          //EObject selAlarmObj = (EObject) _alarmIDObjMap.get(selectedAlarm);
          if (selAlarmObj != null) {
              EObject ruleObj = null;
              boolean isSuppRule = false;
              if (button.getText().equals(ALARM_GENERATION_RULE)) {
                  ruleObj = (EObject) EcoreUtils.getValue(selAlarmObj,
                         ClassEditorConstants.ALARM_GENERATIONRULE);
                  EReference genRuleRef = (EReference) selAlarmObj.eClass().
                  getEStructuralFeature(ClassEditorConstants.
                          ALARM_GENERATIONRULE);
                  if (ruleObj == null) {
                      ruleObj = EcoreUtils.createEObject(genRuleRef.
                              getEReferenceType(), true);
                      selAlarmObj.eSet(genRuleRef, ruleObj);
                  }
              } else if (button.getText().equals(ALARM_SUPPRESSION_RULE)) {
            	  isSuppRule = true;
                  ruleObj = (EObject) EcoreUtils.getValue(selAlarmObj,
                          ClassEditorConstants.ALARM_SUPPRESSIONRULE);
                  EReference supRuleRef = (EReference) selAlarmObj.eClass().
                  getEStructuralFeature(ClassEditorConstants.
                          ALARM_SUPPRESSIONRULE);
                  if (ruleObj == null) {
                      ruleObj = EcoreUtils.createEObject(supRuleRef.
                              getEReferenceType(), true);
                      selAlarmObj.eSet(supRuleRef, ruleObj);
                  }
              }
              java.util.List assoAlarmObjs = new Vector();
              for (int i = 0; i < _associatedAlarmsList.size(); i++) {
					String assoAlarmId = (String) _associatedAlarmsList.get(i);
					//if (!assoAlarmId.equals(_selectedAlarmName)) {
						EObject alarmObj = (EObject) _alarmIDObjMap
								.get(assoAlarmId);
						if (alarmObj != null) {
							assoAlarmObjs.add(alarmObj);
						}
					//}
              }
              if(!isSuppRule)
            	  new AlarmRuleDialog(getShell(), _selectedAlarmName, _alarmRuleViewModel, ruleObj, assoAlarmObjs).open();
              else
            	  new AlarmRuleDialog(getShell(), _selectedAlarmName, _alarmRuleViewModel, ruleObj, assoAlarmObjs, EcoreUtils.getName(_resObj), _project).open();
          }
          
          
      }
      /**
       * @see org.eclipse.swt.events.SelectionListener#widgetDefaultSelected(
       * org.eclipse.swt.events.SelectionEvent)
       */
      public void widgetDefaultSelected(SelectionEvent e)
      {
      }
  }
  /**
   * 
   * @author shubhada
   *
   * selection changed listener 
   */
  class ListSelectionListener implements ISelectionChangedListener
  {

    public void selectionChanged(SelectionChangedEvent event)
    {
        java.util.List selList = ((StructuredSelection) _associateAlarmListViewer.getSelection()).toList();
        if (selList.size() > 1) {
            _genRuleButton.setEnabled(false);
            _supressionButton.setEnabled(false);
        } else {
            _genRuleButton.setEnabled(true);
            _supressionButton.setEnabled(true);
        }
        
    }
      
  }
}
