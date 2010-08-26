package com.clovis.cw.editor.ca.manageability.ui;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;

import org.eclipse.draw2d.ColorConstants;
import org.eclipse.emf.common.util.BasicEList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.CheckboxTreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.BusyIndicator;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.events.VerifyListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeItem;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.editor.ca.ResourceDataUtils;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.manageability.common.ResourceTreeNode;
import com.clovis.cw.editor.ca.manageability.common.StatusLineManagerUtils;
import com.clovis.cw.editor.ca.snmp.MibImportManager;
import com.clovis.cw.project.data.ProjectDataModel;
import com.ireasoning.util.MibTreeNode;

/**
 * 
 * @author Pushparaj
 * 
 */
public class InstancesTreeComposite extends Composite {

	private Button _createBtn, _sharedResBtn, _instanceBtn, _arrayInstBtn, _rangeBtn, _assResBtn/*, _rmResBtn, _deleteResBtn*/;
	private Text _sharedInstText, _arrayInstText, _fromText, _toText;
	private CheckboxTreeViewer _availableResViewer;
	private Tree _instancesTree;
	private Tree _createdResTree;
	private ManageabilityEditor _editor;
	private List<EObject> _nodeList;
	private List<EObject> _componentList;
	private Map<String, ArrayList<EObject>> _compTypeMap = new HashMap<String, ArrayList<EObject>>();
	private ProjectDataModel _pdm;
	private EObject _resourceAssociationObj;
	private Map<String, BasicEList<String>> _createdResourceMoIDsMap= new HashMap<String, BasicEList<String>>();
	private Map<String, Map<String, BasicEList<EObject>>> _compResourcesMap = new HashMap<String, Map<String, BasicEList<EObject>>>();
	private BasicEList<String> _previouslyAssociatedCompType = new BasicEList<String>();
	private BasicEList<String> _previouslyAssociatedResourceType = new BasicEList<String>();
	//private Map _compResourcesMap = null;
	
	public InstancesTreeComposite(ManageabilityEditor editor, Composite parent,
			CheckboxTreeViewer viewer, Tree createdResTree,
			List<EObject> compList, List<EObject> nodeList, ProjectDataModel pdm, Map compResourcesMap, EObject associationObj) {
		super(parent, SWT.NONE);
		_editor = editor;
		_nodeList = nodeList;
		_availableResViewer = viewer;
		_createdResTree = createdResTree;
		_componentList = compList;
		_pdm = pdm;
		_resourceAssociationObj = associationObj;
		//_compResourcesMap = compResourcesMap;
		populateCompTypeMap();
		updatePrimaryOIForSharedResources();
		createControls();
		selectAssociatedComponents();
		selectAssociatedResources();
	}

	private void createControls() {
		setBackground(ColorConstants.white);
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = 15;
		layout.numColumns = 5;
		setLayout(layout);
			        
		VerifyHandler verifyHandler = new VerifyHandler();
		TextModifyHandler modifyHandler = new TextModifyHandler();
		RadioButtonSelectionHandler radioHandler = new RadioButtonSelectionHandler();
		
		int style = SWT.MULTI | SWT.BORDER | SWT.CHECK | SWT.H_SCROLL | SWT.V_SCROLL
				| SWT.FULL_SELECTION | SWT.HIDE_SELECTION;

		_instancesTree = new Tree(this, style);
		GridData gridData1 = new GridData();
		gridData1.horizontalSpan = 5;
		gridData1.horizontalAlignment = GridData.FILL;
		gridData1.grabExcessHorizontalSpace = true;
		gridData1.grabExcessVerticalSpace = true;
		gridData1.verticalAlignment = GridData.FILL;
		gridData1.heightHint = getDisplay().getClientArea().height / 5;
		_instancesTree.setLayoutData(gridData1);
		createTreeItems();
		_instancesTree.addSelectionListener(new SelectionListener(){
			public void widgetDefaultSelected(SelectionEvent e) {}
			public void widgetSelected(SelectionEvent e) {
				TreeItem item = (TreeItem) e.item;
				setChildItemSelection(item, item.getChecked());
				updateButtonStatus();
			}});
		_instancesTree.setBackground(ColorConstants.white);
		Composite instanceGroup = new Composite(this, SWT.NONE);
		GridData btnData = new GridData(GridData.FILL_HORIZONTAL);
		btnData.horizontalSpan = 5;
		instanceGroup.setLayoutData(btnData);
		GridLayout layout2 = new GridLayout();
		layout2.numColumns = 5;
		layout2.verticalSpacing = 10;
		instanceGroup.setLayout(layout2);
		instanceGroup.setBackground(ColorConstants.white);
			
		_sharedResBtn = new Button(instanceGroup, SWT.RADIO);
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_BEGINNING);
        btnData.horizontalSpan = 4;
        _sharedResBtn.setBackground(ColorConstants.white);
        _sharedResBtn.setText("Create just one shared instance");
        _sharedResBtn.setLayoutData(btnData);
        _sharedResBtn.addSelectionListener(radioHandler);
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_CENTER);
        btnData.horizontalSpan = 1;
        _sharedInstText = new Text(instanceGroup, SWT.BORDER);
        _sharedInstText.setText("	");
        _sharedInstText.setLayoutData(btnData);
        _sharedInstText.addVerifyListener(verifyHandler);
        _sharedInstText.addModifyListener(modifyHandler);       
        _instanceBtn = new Button(instanceGroup, SWT.RADIO);
        _instanceBtn.setText("Create one instance per entity");
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_BEGINNING);
        btnData.horizontalSpan = 5;
        _instanceBtn.setLayoutData(btnData);
        _instanceBtn.setBackground(ColorConstants.white);
        _instanceBtn.addSelectionListener(radioHandler);
        
        _arrayInstBtn = new Button(instanceGroup, SWT.RADIO);
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_BEGINNING);
        btnData.horizontalSpan = 4;
        _arrayInstBtn.setText("Create an array of instances per entity");
        _arrayInstBtn.setLayoutData(btnData);
        _arrayInstBtn.setBackground(ColorConstants.white);
        _arrayInstText = new Text(instanceGroup, SWT.BORDER);
        _arrayInstText.setText("	");
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_CENTER);
        btnData.horizontalSpan = 1;
        _arrayInstText.setLayoutData(btnData);
        _arrayInstText.addVerifyListener(verifyHandler);
        _arrayInstText.addModifyListener(modifyHandler);
        _arrayInstBtn.addSelectionListener(radioHandler);
        
        _rangeBtn = new Button(instanceGroup, SWT.RADIO);
        _rangeBtn.setText("Create array of instances");
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_BEGINNING);
        btnData.horizontalSpan = 4;
        _rangeBtn.setLayoutData(btnData);
        _rangeBtn.setBackground(ColorConstants.white);
        _rangeBtn.addSelectionListener(radioHandler);
        Composite rangeGroup = new Composite(instanceGroup, SWT.NONE);
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_END);
        btnData.horizontalSpan = 1;
        rangeGroup.setLayoutData(btnData);
        rangeGroup.setBackground(ColorConstants.white);
        GridLayout layout1 = new GridLayout();
        layout1.numColumns = 4;
        rangeGroup.setLayout(layout1);
        Label fromLabel = new Label(rangeGroup, SWT.NONE);
        fromLabel.setText("From");
        fromLabel.setBackground(ColorConstants.white);
        _fromText = new Text(rangeGroup, SWT.BORDER);
        _fromText.setText("	");
        _fromText.addVerifyListener(verifyHandler);
        _fromText.addModifyListener(modifyHandler); 
        Label toLabel = new Label(rangeGroup, SWT.NONE);
        toLabel.setText("To");
        toLabel.setBackground(ColorConstants.white);
        _toText = new Text(rangeGroup, SWT.BORDER);
        _toText.setText("	");
        _toText.addVerifyListener(verifyHandler);
        _toText.addModifyListener(modifyHandler);
        
        _createBtn = new Button(this, SWT.BORDER);
        _createBtn.setText("Create Instance(s)");
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_BEGINNING);
        btnData.horizontalSpan = 3;
        _createBtn.setLayoutData(btnData);
        _createBtn.addSelectionListener(new CreateButtonHandler());
        _createBtn.setEnabled(false);
        /*_deleteResBtn = new Button(this, SWT.BORDER);
        _deleteResBtn.setText("Delete Instance(s)");
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_END);
        btnData.horizontalSpan = 2;
        _deleteResBtn.setLayoutData(btnData);
        _deleteResBtn.addSelectionListener(new DeleteButtonHandler());
        _deleteResBtn.setEnabled(false);*/
        _assResBtn = new Button(this, SWT.BORDER);
        _assResBtn.setText("Associate Resource to CSI");
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_BEGINNING);
        btnData.horizontalSpan = 2;
        _assResBtn.setLayoutData(btnData);
        _assResBtn.setEnabled(false);
        /*_rmResBtn = new Button(this, SWT.BORDER);
        _rmResBtn.setText("Resmove CSI Association");
        btnData = new GridData(GridData.HORIZONTAL_ALIGN_END);
        btnData.horizontalSpan = 2;
        _rmResBtn.setLayoutData(btnData);
        _rmResBtn.setEnabled(false);*/
    }
	
	public Tree getComponentsTree() {
		return _instancesTree;
	}
	
	/**
	 * Creates Component type Instances Map
	 * @param nodesList
	 */
	public void populateCompTypeMap() {
		_compTypeMap.clear();
		for (int i = 0; i < _nodeList.size(); i++) {
			EObject nodeObj = _nodeList.get(i);
			EObject serviceUnitInstsObj = (EObject) EcoreUtils.getValue(
					nodeObj, SafConstants.SERVICEUNIT_INSTANCES_NAME);
			if (serviceUnitInstsObj != null) {
				List suInstList = (List) EcoreUtils.getValue(serviceUnitInstsObj,
						SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
				for (int j = 0; j < suInstList.size(); j++) {
					EObject suInstObj = (EObject) suInstList.get(j);
					EObject compInstsObj = (EObject) EcoreUtils.getValue(suInstObj,
							SafConstants.COMPONENT_INSTANCES_NAME);
					if (compInstsObj != null) {
						List compInstList = (List) EcoreUtils.getValue(
								compInstsObj,
								SafConstants.COMPONENT_INSTANCELIST_NAME);
						for (int k = 0; k < compInstList.size(); k++) {
							EObject compInstObj = (EObject) compInstList.get(k);
							String compType = (String)EcoreUtils.getValue(compInstObj, "type");
							List<EObject> resourceList = (EList) EcoreUtils.getValue(
									compInstObj, "resources");
							if(resourceList.size() > 0) {
								if(!_previouslyAssociatedCompType.contains(compType))
									_previouslyAssociatedCompType.addUnique(compType);
							}
							ArrayList<EObject> instList = _compTypeMap.get(compType);
							if(instList == null) {
								instList = new ArrayList<EObject>();
								_compTypeMap.put(compType, instList);
							}
							instList.add(compInstObj);
						}
					}
				}
			}
		}
	}
	private void updatePrimaryOIForSharedResources() {
		Map<String, EObject> createdMoIDResourceMap = new HashMap<String, EObject>();
		Iterator<String> keys = _compTypeMap.keySet().iterator();
		while (keys.hasNext()) {
			List<EObject> compInstList = _compTypeMap.get(keys.next());
			for (int k = 0; k < compInstList.size(); k++) {
				EObject compInstObj = (EObject) compInstList.get(k);
				List<EObject> resourceList = (EList) EcoreUtils.getValue(
						compInstObj, "resources");
				for (int l = 0; l < resourceList.size(); l++) {
					EObject resObj = resourceList.get(l);
					String moID = (String) EcoreUtils.getValue(resObj,"moID");
					String resName = getResourceTypeForInstanceID(moID);
					if(!_previouslyAssociatedResourceType.contains(resName))
						_previouslyAssociatedResourceType.addUnique(resName);
					EObject obj = createdMoIDResourceMap.get(moID);
					if(obj != null) {
						EcoreUtils.setValue(obj,"primaryOI", "false");
						EcoreUtils.setValue(resObj,"primaryOI", "false");
					} else {
						EcoreUtils.setValue(resObj,"primaryOI", "true");
						createdMoIDResourceMap.put(moID, resObj);
					}
				}
			}
		}
	}
	private void populateMoIDsList(){
		_createdResourceMoIDsMap.clear();
		Iterator<String> keys = _compTypeMap.keySet().iterator();
		while (keys.hasNext()) {
			List<EObject> compInstList = _compTypeMap.get(keys.next());
			for (int k = 0; k < compInstList.size(); k++) {
				EObject compInstObj = (EObject) compInstList.get(k);
				List<EObject> resourceList = (EList) EcoreUtils.getValue(
						compInstObj, "resources");
				for (int l = 0; l < resourceList.size(); l++) {
					EObject resObj = resourceList.get(l);
					String moID = (String) EcoreUtils.getValue(resObj,"moID");
					String resName = getResourceTypeForInstanceID(moID);
					BasicEList<String> list =_createdResourceMoIDsMap.get(resName);
					if(list == null) {
						list = new BasicEList<String>();
						_createdResourceMoIDsMap.put(resName, list);
					}
					list.addUnique(moID);
				}
			}
		}
	}
	private void populateCompResourcesMap() {
		_compResourcesMap.clear();
		Iterator<String> keys = _compTypeMap.keySet().iterator();
		while (keys.hasNext()) {
			List<EObject> compInstList = _compTypeMap.get(keys.next());
			for (int k = 0; k < compInstList.size(); k++) {
				EObject compInstObj = (EObject) compInstList.get(k);
				String compName = EcoreUtils.getName(compInstObj);
				Map<String, BasicEList<EObject>> resNameListMap = _compResourcesMap.get(compName);
				if(resNameListMap == null) {
					resNameListMap = new HashMap<String, BasicEList<EObject>>();
					_compResourcesMap.put(compName, resNameListMap);
				}
				List<EObject> resourceList = (EList) EcoreUtils.getValue(
						compInstObj, "resources");
				for (int l = 0; l < resourceList.size(); l++) {
					EObject resObj = resourceList.get(l);
					String moID = (String) EcoreUtils.getValue(resObj,"moID");
					String resName = getResourceTypeForInstanceID(moID);
					BasicEList<EObject> list = resNameListMap.get(resName);
					if(list == null) {
						list = new BasicEList<EObject>();
						resNameListMap.put(resName, list);
					}
					list.addUnique(resObj);
				}
			}
		}
	}
	/**
	 * Create Tree Items for component and csi types
	 */
	public void createTreeItems() {
		_instancesTree.removeAll();
		EObject rootObject = (EObject) _componentList.get(0);
		EList<EObject> compList = (EList)rootObject.eGet(rootObject.eClass().getEStructuralFeature("safComponent"));
		TreeItem compItem = new TreeItem(_instancesTree, SWT.NONE);
		compItem.setText("Applications (SAF Components)");
		for (int i = 0; i< compList.size(); i++) {
			EObject obj = compList.get(i);
			TreeItem item = new TreeItem(compItem, SWT.NONE);
			item.setText(EcoreUtils.getName(obj));
			item.setData("obj", obj);
			item.setData("type", "component");
		}
		EList<EObject> csiList = (EList)rootObject.eGet(rootObject.eClass().getEStructuralFeature("componentServiceInstance"));
		TreeItem csiItem = new TreeItem(_instancesTree, SWT.NONE);
		csiItem.setText("Workloads (SAF Component Service Instances)");
		for (int i = 0; i< csiList.size(); i++) {
			EObject obj = csiList.get(i);
			TreeItem item = new TreeItem(csiItem, SWT.NONE);
			item.setText(EcoreUtils.getName(obj));
			item.setData("obj", obj);
			item.setData("type", "csi");
		}
	}
	/**
	 * Selects the previously selected component types
	 */
	private void selectAssociatedComponents(){
		TreeItem applicationItem = _instancesTree.getItem(0);
		applicationItem.setExpanded(true);
		TreeItem items[] = applicationItem.getItems();
		for (int i = 0; i < items.length; i++) {
			TreeItem item = items[i];
			if(_previouslyAssociatedCompType.contains(item.getText())) {
				item.setChecked(true);
			}
		}
	}
	/**
	 * Selects the previously associated resource types
	 */
	private void selectAssociatedResources(){
		TreeItem rootItem = _availableResViewer.getTree().getItem(0);
		_availableResViewer.setExpandedState(rootItem.getData(), true);
		TreeItem items[] = rootItem.getItems();
		for (int i = 0; i < items.length; i++) {
			TreeItem item = items[i];
			/*if(_previouslyAssociatedResourceType.contains(item.getText())) {
				item.setChecked(true);
			}*/
			ArrayList<TreeItem> expandedItems = new ArrayList<TreeItem>();
    		selectAssociatedResources(item, expandedItems);
    		if(expandedItems.size() == 0) {
				_availableResViewer.setExpandedState(item.getData(), false);
			}
		}
		/*if(_availableResViewer.getCheckedElements().length == 0) {
			_availableResViewer.setExpandedState(rootItem.getData(), false);
		}*/
	}
	/**
	 * Selects the previously associated resource types
	 */
	private void selectAssociatedResources(TreeItem rootItem, ArrayList expandedItems) {
		_availableResViewer.setExpandedState(rootItem.getData(), true);
		TreeItem items[] = rootItem.getItems();
		for (int i = 0; i < items.length; i++) {
			TreeItem item = items[i];
			if(_previouslyAssociatedResourceType.contains(item.getText())) {
				item.setChecked(true);
				expandedItems.add(item);
			}
			selectAssociatedResources(item, expandedItems);
		}
	}
	/**
	 * Select all childrens under in the TreeItem 
	 * @param root
	 */
	private void setChildItemSelection(TreeItem root, boolean checked) {
		TreeItem items[] = root.getItems();
		for (int i = 0; i < items.length; i++) {
			TreeItem item = items[i];
			item.setChecked(checked);
			setChildItemSelection(item, checked);
		}
	}
	
	class VerifyHandler implements VerifyListener {
		public void verifyText(VerifyEvent e) {
			// only allow numbers, delete, backspace key
			switch (e.keyCode) {
			case SWT.DEL:
			case SWT.BS:
			case SWT.NONE:
				return;
			}
			if (!("" + e.text).matches("[0-9]"))
				e.doit = false;
		}
	}
	
	class CreateButtonHandler extends SelectionAdapter {
		public void widgetSelected(SelectionEvent e) {
			Map<String, ArrayList<MibTreeNode>> fileNameNodesMap = new HashMap<String, ArrayList<MibTreeNode>>();
			Object items[] = _availableResViewer.getCheckedElements();
			final ArrayList<String> editorResourceNames = new ArrayList<String>();
			final ArrayList<String> selectedResNames = new ArrayList<String>();
			final ArrayList<String> selectedScalarNames = new ArrayList<String>();
			if(items.length > 0){
				populateResourceNames(editorResourceNames);
			}
			for (int i = 0; i < items.length; i++) {
				ResourceTreeNode node = (ResourceTreeNode) items[i];
				if(node.getNode() != null) {
					MibTreeNode mibNode =  node.getNode();
					String mibName = new File(node.getMibFileName()).getName();
					ArrayList<MibTreeNode> nodeList = fileNameNodesMap.get(mibName);
					if(nodeList == null) {
						nodeList = new ArrayList<MibTreeNode>();
						fileNameNodesMap.put(mibName, nodeList);
					}
					if(mibNode.isTableNode()) {
						selectedResNames.add((String)mibNode.getName());
						if(!editorResourceNames.contains((String)mibNode.getName()))
							nodeList.add(mibNode);
					} else if((mibNode.isGroupNode() && isGroupWithScalar(mibNode))) {
						selectedScalarNames.add((String)mibNode.getName());
						if(!editorResourceNames.contains((String)mibNode.getName()))
							nodeList.add(mibNode);
					}
				}
			}
			if(selectedScalarNames.size() == 0 && selectedResNames.size() == 0){
				StatusLineManagerUtils.setErrorMessage("Resource association failed. " + "Please select Mib resource(s) from Resource Type Browser");
				return;
			}
			List<String> needsToBeSelected = new ArrayList<String>(selectedScalarNames.size() + selectedResNames.size());
			needsToBeSelected.addAll(selectedResNames);
			needsToBeSelected.addAll(selectedScalarNames);
			List assList = (EList) EcoreUtils.getValue(_resourceAssociationObj, "association");
			EClass assClass = (EClass)_resourceAssociationObj.eClass().getEPackage().getEClassifier("association");
			final EObject newObj = EcoreUtils.createEObject(assClass, true);
			assList.add(newObj);
			List<String> selectedCompTypes = new ArrayList<String>();
			TreeItem compItem = _instancesTree.getItems()[0];
			TreeItem childs [] = compItem.getItems();
			final List<EObject> compInstObjList = new ArrayList<EObject>();
			for (int i = 0; i < childs.length; i++) {
				TreeItem child = childs[i];
				if(child.getChecked()) {
					String compType = child.getText();
					((EList)EcoreUtils.getValue(newObj, "compName")).add(compType);
					if(_compTypeMap.get(child.getText()) != null) {
						compInstObjList.addAll(_compTypeMap.get(child.getText()));
						selectedCompTypes.add(compType);
					}
				}
			}
			if(compInstObjList.size() == 0) {
				StatusLineManagerUtils
						.setMessage("There are no component instances to associate. Create component instances in the 'Clovis->AMF Configuration' dialog");
			}
			boolean isImportNeeded = false;
			((EList)EcoreUtils.getValue(newObj, "tableName")).addAll(selectedResNames);
			((EList)EcoreUtils.getValue(newObj, "scalarName")).addAll(selectedScalarNames);
			if (_sharedResBtn.getSelection()) {
				isImportNeeded = true;
				EcoreUtils.setValue(newObj, "type", AssociateResourceConstants.SHARED_INSTANCE);
			} else if (_instanceBtn.getSelection()) {
				isImportNeeded = true;
				EcoreUtils.setValue(newObj, "type", AssociateResourceConstants.INSTANCE);
			} else if (_arrayInstBtn.getSelection()) {
				isImportNeeded = true;
				EcoreUtils.setValue(newObj, "type", AssociateResourceConstants.ARRAY_INSTANCE);
			} else if (_rangeBtn.getSelection()) {
				String fromValue = _fromText.getText().trim();
				String toValue = _toText.getText().trim();
				if (fromValue.length() > 0 && toValue.length() > 0) {
					int from = 0;
					int to = 1;
					try {
						from = Integer.parseInt(fromValue);
					} catch (NumberFormatException ex) {
						StatusLineManagerUtils.setErrorMessage("Invalid Value. " + _fromText.getText().trim() + " is not a valid number");
						return;
					}
					try {
						to = Integer.parseInt(toValue);
					} catch (NumberFormatException ex) {
						StatusLineManagerUtils.setErrorMessage("Invalid Value. " + _toText.getText().trim() + " is not a valid number");
						return;
					}
					if (from <= to) {
						isImportNeeded = true;
					} else {
						StatusLineManagerUtils.setErrorMessage("Invalid Range. " + "'To' value should be greated than 'From' value");
						return;
					}
				}
				EcoreUtils.setValue(newObj, "type", AssociateResourceConstants.ARRAY_SHARED_INSTANCE);
			}

			if (isImportNeeded) {
				Iterator<String> iterator = fileNameNodesMap.keySet()
						.iterator();
				while (iterator.hasNext()) {
					final String mibName = iterator.next();
					final ArrayList<MibTreeNode> nodelist = (ArrayList) fileNameNodesMap
							.get(mibName);
					if (nodelist.size() > 0) {
						BusyIndicator.showWhile(Display.getCurrent(), new Runnable(){
							public void run() {
								MibImportManager manager = new MibImportManager(_pdm.getProject(), null);
								manager.convertMibObjToClovisObj(mibName, nodelist);								
							}});
					}
				}
			}
			
			final List<String> initializedResList = AssociateResourceUtils.getInitializedArrayAttrResList(_editor.getResourceList());
			if (_sharedResBtn.getSelection()) {
					try {
						BusyIndicator.showWhile(Display.getCurrent(), new Runnable() {
							public void run() {
								if (_sharedInstText.getText().trim().length() > 0) {
									final int id = Integer.parseInt(_sharedInstText
											.getText().trim());
										String message = "";
									    List existingResources = AssociateResourceUtils.filterExistingSharedResources(id, selectedResNames, "\\" + _editor.getChassisName() + ":0", compInstObjList);
									    for(int i = 0; i < existingResources.size(); i++) {
									    	if(message.equals("")) {
									    		message += ", "; 
									    	}
									    	message += existingResources.get(i);
									    }
									    selectedResNames.removeAll(existingResources);
										AssociateResourceUtils
										.createSharedResourceForComponents(
												compInstObjList,
												selectedResNames,
												"\\" + _editor.getChassisName() + ":0", id, initializedResList);
										if (id == 0) {
											List existingScalarResources = AssociateResourceUtils.filterExistingSharedResources(id, selectedScalarNames, "\\" + _editor.getChassisName() + ":0", compInstObjList);
											for(int i = 0; i < existingScalarResources.size(); i++) {
										    	if(message.equals("")) {
										    		message += ", "; 
										    	}
										    	message += existingScalarResources.get(i);
										    }
											selectedScalarNames.removeAll(existingScalarResources);
											AssociateResourceUtils
												.createScalarSharedResources(
														compInstObjList,
														selectedScalarNames,
														"\\" + _editor.getChassisName() + ":0", initializedResList);
										}
										EcoreUtils.setValue(newObj, AssociateResourceConstants.VALUE , String.valueOf(id));
										if(!message.equals("")) {
											StatusLineManagerUtils.setErrorMessage("Unable to create the instances for the resource(s) " + message.toString() + ". Because instances are already exist.");
										} else {
											StatusLineManagerUtils.setMessage("Shared resource(s) created for selected entities");
										}
								} else {
									populateMoIDsList();
									StringBuffer message = new StringBuffer("Resources '");
									AssociateResourceUtils
									.createSharedResourceForComponents(
											compInstObjList,
											selectedResNames,
											"\\" + _editor.getChassisName() + ":0", _createdResourceMoIDsMap, initializedResList, message);
									message.append("' are created under entity ");
									for( int i = 0; i < compInstObjList.size(); i++) {
										message.append(EcoreUtils.getName(compInstObjList.get(i)) + " ");
									}
									StatusLineManagerUtils.setMessage(message.toString());
								}
							}
						});
					} catch (NumberFormatException ex) {
						StatusLineManagerUtils.setErrorMessage("Invalid Value. " + _sharedInstText.getText().trim() + " is not a valid number");
						return;
					}
			} else if (_instanceBtn.getSelection()) {
				BusyIndicator.showWhile(Display.getCurrent(), new Runnable() {
					public void run() {
						populateMoIDsList();
						AssociateResourceUtils.createOneResourceForComponents(
								compInstObjList, selectedResNames, "\\" + _editor.getChassisName() + ":0",
								_createdResourceMoIDsMap, initializedResList);
						AssociateResourceUtils.createScalarResources(compInstObjList,
								selectedScalarNames, "\\" + _editor.getChassisName() + ":0", initializedResList);
						StatusLineManagerUtils.setMessage("Resources created for the selected entities");
					}});
				
			} else if (_arrayInstBtn.getSelection()) {
				final String value = _arrayInstText.getText().trim();
				
					BusyIndicator.showWhile(Display.getCurrent(), new Runnable(){
						public void run() {
							try {
								int size = 0;
								if (value.length() > 0) {
									size = Integer.parseInt(value);
								} else {
									size = 1;
								}
								populateMoIDsList();
								AssociateResourceUtils
										.createArrayOfResourcesForComponents(
												compInstObjList, selectedResNames,
												"\\" + _editor.getChassisName() + ":0", size, _createdResourceMoIDsMap, initializedResList);
								
								AssociateResourceUtils.createScalarResources(
											compInstObjList, selectedScalarNames,
											"\\" + _editor.getChassisName() + ":0", initializedResList);
								EcoreUtils.setValue(newObj, AssociateResourceConstants.VALUE, String.valueOf(size));
								StringBuffer message = new StringBuffer(size + " instance(s) created for the resources ");
								for (int i = 0; i < selectedResNames.size(); i++) {
									message.append(selectedResNames.get(i) + " ");
								}
								for (int i = 0; i < selectedScalarNames.size(); i++) {
									message.append(selectedScalarNames.get(i) + " ");
								}
								message.append("under entity ");
								for( int i = 0; i < compInstObjList.size(); i++) {
									message.append(EcoreUtils.getName(compInstObjList.get(i)) + " ");
								}
								StatusLineManagerUtils.setMessage(message.toString());
							} catch (NumberFormatException ex) {
								StatusLineManagerUtils.setErrorMessage("Invalid Value. " + _arrayInstText.getText().trim() + " is not a valid number");
								return;
							}
							
						}});
			
			} else if (_rangeBtn.getSelection()) {
				String fromValue = _fromText.getText().trim();
				String toValue = _toText.getText().trim();
				if (fromValue.length() > 0 && toValue.length() > 0) {
					int from = 0;
					int to = 1;
					try {
						from = Integer.parseInt(fromValue);
					} catch (NumberFormatException ex) {
						StatusLineManagerUtils.setErrorMessage("Invalid Value. " + _fromText.getText().trim() + " is not a valid number");
						return;
					}
					try {
						to = Integer.parseInt(toValue);
					} catch (NumberFormatException ex) {
						StatusLineManagerUtils.setErrorMessage("Invalid Value. " + _toText.getText().trim()	+ " is not a valid number");
						return;
					}
					if (from <= to) {
						final int start = from;
						final int end = to;
						BusyIndicator.showWhile(Display.getCurrent(),
								new Runnable() {

									public void run() {
										AssociateResourceUtils
												.createResourcesForComponentsWithInRange(
														compInstObjList,
														selectedResNames,
														"\\" + _editor.getChassisName() + ":0", start,
														end, initializedResList);
										if (start == 0) {
											AssociateResourceUtils
													.createScalarSharedResources(
															compInstObjList,
															selectedScalarNames,
															"\\" + _editor.getChassisName() + ":0", initializedResList);
										}
										StatusLineManagerUtils.setMessage("Resource(s) created for selected entities");
										EcoreUtils.setValue(newObj, AssociateResourceConstants.VALUE, String.valueOf(start + "-" + end));
									}
								});
					} else {
						StatusLineManagerUtils.setErrorMessage("Invalid Range. " + "'To' value should be greated than 'From' value");
						return;
					}
				}
			}
			if (isImportNeeded) {
				boolean expanded = _createdResTree.getItem(0).getExpanded();
				_createdResTree.removeAll();
				_createdResTree.setFocus();
				_editor.updateCreatedResourcesTreeItem(_createdResTree,
						needsToBeSelected, selectedCompTypes, expanded);
				_editor.propertyChange(null);
			}
		}
		/**
		 * Creates resource name list
		 * 
		 * @param nameList
		 */
		private void populateResourceNames(List<String> nameList) {
			Model caModel = _pdm.getCAModel();
			List<EObject> resourceList = ResourceDataUtils.getMoList(caModel.getEList());
			for (int i = 0; i < resourceList.size(); i++) {
				EObject resource = (EObject) resourceList.get(i);
				nameList.add(EcoreUtils.getName(resource));
			}
		}
	}
	/**
	 * Check if the group node contains scalar node
	 * @param node Group Node
	 * @return boolean
	 */
	private boolean isGroupWithScalar(MibTreeNode node){
		List children = node.getChildNodes();
		for (int i = 0; i < children.size(); i++) {
			MibTreeNode child = (MibTreeNode) children.get(i);
			if (child.isScalarNode()) {
				return true;
			}
		}
		return false;
	}
	class DeleteButtonHandler extends SelectionAdapter {
		public void widgetSelected(SelectionEvent e) {
			//deAssociateResources();
		}
	}
	/**
	 * De-Associate Resources
	 */
	/*public void deAssociateResources() {
		Object items[] = _availableResViewer.getCheckedElements();
		final List<String> selectedResNames = new ArrayList<String>();
		for (int i = 0; i < items.length; i++) {
			ResourceTreeNode node = (ResourceTreeNode) items[i];
			if(node.getNode() != null) {
				MibTreeNode mibNode =  node.getNode();
				if(mibNode.isTableNode() || (mibNode.isGroupNode() && isGroupWithScalar(mibNode))) {
					selectedResNames.add((String)mibNode.getName());
				}
			}
		}
		List<String> selectedCompTypes = new ArrayList<String>();
		TreeItem compItem = _instancesTree.getItems()[0];
		TreeItem childs [] = compItem.getItems();
		final List<EObject> compInstObjList = new ArrayList<EObject>();
		for (int i = 0; i < childs.length; i++) {
			TreeItem child = childs[i];
			if(child.getChecked()) {
				String compType = child.getText();
				if(_compTypeMap.get(child.getText()) != null) {
					compInstObjList.addAll(_compTypeMap.get(child.getText()));
					selectedCompTypes.add(compType);
				}
			}
		}
		if(compInstObjList.size() > 0 && selectedResNames.size() > 0) {
			populateCompResourcesMap();
			for (int i = 0; i < compInstObjList.size(); i++) {
				EObject compInstObj = compInstObjList.get(i);
				Map<String, BasicEList<EObject>> nameResourcesMap = _compResourcesMap.get(EcoreUtils.getName(compInstObj));
				List<EObject> resourceList = (EList) EcoreUtils.getValue(
						compInstObj, "resources");
				for (int j = 0; j < selectedResNames.size(); j++) {
					String resName = selectedResNames.get(j);
					List<EObject> list = nameResourcesMap.get(resName);
					if(list != null) {
						resourceList.removeAll(list);
					}
				}
			}
			boolean expanded = _createdResTree.getItem(0).getExpanded();
			_createdResTree.removeAll();
			_createdResTree.setFocus();
			_editor.updateCreatedResourcesTreeItem(_createdResTree, selectedCompTypes, expanded);
			_editor.propertyChange(null);
		}
	}*/
	/**
	 * De-Associate Resources
	 */
	public boolean deAssociateResources(final List<String> selectedResNames) {
		final List<String> selectedComponents = new ArrayList<String>();
		TreeItem compItem = _instancesTree.getItems()[0];
		TreeItem childs[] = compItem.getItems();
		final List<EObject> compInstObjList = new ArrayList<EObject>();
		for (int i = 0; i < childs.length; i++) {
			TreeItem child = childs[i];
			String compType = child.getText();
			if (_compTypeMap.get(child.getText()) != null) {
				compInstObjList.addAll(_compTypeMap.get(child.getText()));
			}
		}
		if (compInstObjList.size() > 0 && selectedResNames.size() > 0) {
			UnLoadMessageDialog messageDialog = new UnLoadMessageDialog(getShell(), "Delete All Resources", MessageDialog.getDefaultImage(), "This will delete all resources which are created from selected mib(s)"
					+ ". Do you still want to continue?", 3, new String[]{"Ok", "Cancel"}, 0);
			if(messageDialog.open() == Dialog.OK) {
				boolean unLoadMIB = messageDialog.needUnLoadMIB();
				BusyIndicator.showWhile(Display.getDefault(), new Runnable() {
					public void run() {
						populateCompResourcesMap();
						for (int i = 0; i < compInstObjList.size(); i++) {
							EObject compInstObj = compInstObjList.get(i);
							Map<String, BasicEList<EObject>> nameResourcesMap = _compResourcesMap
									.get(EcoreUtils.getName(compInstObj));
							List<EObject> resourceList = (EList) EcoreUtils.getValue(
									compInstObj, "resources");
							boolean needsToBeAdded = false;
							for (int j = 0; j < selectedResNames.size(); j++) {
								String resName = selectedResNames.get(j);
								List<EObject> list = nameResourcesMap.get(resName);
								if (list != null && list.size() > 0) {
									resourceList.removeAll(list);
									needsToBeAdded = true;
								}
							}
							if (needsToBeAdded) {
								selectedComponents.add(EcoreUtils.getName(compInstObj));
							}
						}
						if (selectedComponents.size() > 0) {
							boolean expanded = _createdResTree.getItem(0).getExpanded();
							_createdResTree.removeAll();
							_createdResTree.setFocus();
							_editor.updateCreatedResourcesTreeItem(_createdResTree,
									selectedComponents, expanded);
							_editor.propertyChange(null);
						}
					}});
				return unLoadMIB;
			} else {
				return false;
			}
		} else {
			return false;
		}
	}
	/**
	 * Return ResourceType for the moID
	 * @param id resource moID
	 * @return String Resource type
	 */
	private String getResourceTypeForInstanceID(String id) {
    	String paths[] = id.split(":");
		StringTokenizer tokenizer = new StringTokenizer(
					paths[paths.length - 2], "\\");
		tokenizer.nextToken();
		String resName = tokenizer.nextToken();
		return resName;
	}
	/**
	 * Updates Create Button State
	 */
	private void updateButtonStatus() {
		_createBtn.setEnabled(false);
		//_deleteResBtn.setEnabled(false);
		boolean compSelected = false;
		TreeItem root = _instancesTree.getItem(0);
		TreeItem items[] = root.getItems();
		for (int i = 0; i < items.length; i++) {
			TreeItem item = items[i];
			if (item.getChecked()) {
				compSelected = true;
				break;
			}
		}
		if(!compSelected)
			return;
		//_deleteResBtn.setEnabled(true);
		//try {
			if (_sharedResBtn.getSelection()) {
				_createBtn.setEnabled(true);
			} else if (_instanceBtn.getSelection()) {
				_createBtn.setEnabled(true);
			} else if (_arrayInstBtn.getSelection()) {
				_createBtn.setEnabled(true);
			} else if (_rangeBtn.getSelection()) {
				String value1 = _fromText.getText().trim();
				String value2 = _toText.getText().trim();
				if (value1.length() > 0 && value2.length() > 0) {
					//int insNo1 = Integer.parseInt(value1);
					//int insNo2 = Integer.parseInt(value2);
					_createBtn.setEnabled(true);
				}
			}
		//} catch (NumberFormatException e) {
			//return;
		//}
	}

	class RadioButtonSelectionHandler extends SelectionAdapter {
		public void widgetSelected(SelectionEvent e) {
			updateButtonStatus();
		}
	}

	class TextModifyHandler implements ModifyListener {
		public void modifyText(ModifyEvent e) {
			updateButtonStatus();
		}
	}
	
	class UnLoadMessageDialog extends MessageDialog {
		Button deleteResBtn, unLoadBtn;
		boolean unLoadMIB;
		public UnLoadMessageDialog(Shell parentShell, String dialogTitle,
				Image dialogTitleImage, String dialogMessage,
				int dialogImageType, String[] dialogButtonLabels,
				int defaultIndex) {
			super(parentShell, dialogTitle, dialogTitleImage, dialogMessage,
					dialogImageType, dialogButtonLabels, defaultIndex);
		}
		
		protected Control createCustomArea(Composite composite) {
			unLoadBtn = new Button(composite, SWT.RADIO);
			unLoadBtn.setText("Also unload selected mib(s)");
			unLoadBtn.addSelectionListener(new SelectionListener(){

				public void widgetDefaultSelected(SelectionEvent e) {}
				public void widgetSelected(SelectionEvent e) {
					if(unLoadBtn.getSelection()) {
						unLoadMIB = true;
					} else {
						unLoadMIB = false;
					}
				}});
			deleteResBtn = new Button(composite, SWT.RADIO);
			deleteResBtn.setText("Do not unload mib(s)");
			deleteResBtn.setSelection(true);
			deleteResBtn.addSelectionListener(new SelectionListener(){
				public void widgetDefaultSelected(SelectionEvent e) {}
				public void widgetSelected(SelectionEvent e) {
					if(deleteResBtn.getSelection()) {
						unLoadMIB = false;
					} else {
						unLoadMIB = true;
					}
				}});
			return composite;
	    }
		
		public boolean needUnLoadMIB() {
			return unLoadMIB;
		}
	}
}

