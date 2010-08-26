package com.clovis.cw.editor.ca.dialog;

import java.util.HashMap;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.events.VerifyEvent;
import org.eclipse.swt.events.VerifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.validator.AMFListUniquenessValidator;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Preference page for the Node Instances List item in the AMF Configuration tree.
 * This page allows the user to create a number of node with one action.
 * 
 * @author matt
 */
public class AMFNodeWizardPage extends GenericPreferencePage {
	
	private ProjectDataModel _pdm;
	private CCombo _nodeTypeCombo;
	private CCombo _bladeTypeCombo;
	private Text _nodeCount;
	private Button _startButton;
	private EObject _amfObject;
	private NodeProfileDialog _npd;
	//private HashMap<String, Integer> _bladeTypesMaxInstance = new HashMap<String, Integer>();
	private String _systemCtrNodeName = null;
	private static String MOID_ATTR = "nodeMoId";
	  
	/**
	 * Constructor
	 * @param label Title for the page.
	 * @param obj EObject for the FormView.
	 * @param nodeList 
	 */
	public AMFNodeWizardPage(String label, ProjectDataModel pdm, EObject amfObject, NodeProfileDialog npd) {
		  super(label);
		  noDefaultAndApplyButton();
		  _pdm = pdm;
		  _amfObject = amfObject;
		  _npd = npd;
	}
	  
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createContents(Composite parent)
	{
		/* Create container for dialog controls */		
		Composite container = new Composite(parent, SWT.NONE);
        GridLayout glayout = new GridLayout();
        glayout.numColumns = 2;
        container.setLayout(glayout);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));
        
        /******************************************************/
    	/* Create Instruction Label                           */
    	/******************************************************/
        GridData gridData = new GridData(GridData.HORIZONTAL_ALIGN_FILL);
        gridData.horizontalSpan = 2;
        Label instructions = new Label(container, SWT.WRAP);
        instructions.setText("Use this page to rapidly create Node Instances. Select the type of"
        		+ " node to be created along with the type of blade on which the node will be run."
        		+ " Enter the number of node instances to create in the 'Node Count' field and then"
        		+ " click the 'Create Instance Tree' button. Note that the total number of Node"
        		+ " Instances (existing and about to be created) associated with a given Blade Type"
        		+ " cannot exceed the 'Max Instances' value defined for that Blade Type in the"
        		+ " resource editor.\n");
        instructions.setLayoutData(gridData);
        
        /******************************************************/
    	/* Create Node Type Selector                          */
    	/******************************************************/
    	Label nodeTypeLabel = new Label(container, SWT.NONE);
    	nodeTypeLabel.setText("Node Type: ");

    	_nodeTypeCombo = new CCombo(container, SWT.DROP_DOWN | SWT.READ_ONLY | SWT.BORDER);
    	_nodeTypeCombo.setItems(getNodeTypes());
    	_nodeTypeCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	_nodeTypeCombo.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				if (_nodeTypeCombo.getText().length() > 0 
						&& _bladeTypeCombo.getText().length() > 0 
						&& _nodeCount.getText().length() > 0) 
				{
					_startButton.setEnabled(true);
				} else {
					_startButton.setEnabled(false);
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
    	});

    	/******************************************************/
    	/* Create Blade Type Selector                         */
    	/******************************************************/
    	Label bladeTypeLabel = new Label(container, SWT.NONE);
    	bladeTypeLabel.setText("Blade Type: ");

    	_bladeTypeCombo = new CCombo(container, SWT.DROP_DOWN | SWT.READ_ONLY | SWT.BORDER);
    	_bladeTypeCombo.setItems(getBladeTypes());
    	_bladeTypeCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	_bladeTypeCombo.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				if (_nodeTypeCombo.getText().length() > 0 
						&& _bladeTypeCombo.getText().length() > 0 
						&& _nodeCount.getText().length() > 0)
				{
					_startButton.setEnabled(true);
				} else {
					_startButton.setEnabled(false);
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
    	});

    	/******************************************************/
    	/* Create Node Count Text Box                         */
    	/******************************************************/
    	Label nodeCountLabel = new Label(container, SWT.NONE);
    	nodeCountLabel.setText("Node Count: ");

    	_nodeCount = new Text(container, SWT.BORDER);
    	_nodeCount.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	_nodeCount.addVerifyListener(new VerifyListener() {
    		public void verifyText(VerifyEvent e) {
				// only allow numbers, delete, backspace key
    			switch (e.keyCode)
				{
					case SWT.DEL:
					case SWT.BS:
					case SWT.NONE:
					return;
				}
				if ( !("" + e.text).matches("[0-9]")) e.doit = false;
			}
    	});
    	/*_nodeCount.addKeyListener(new KeyListener() {
			public void keyPressed(KeyEvent e) {
			}
			public void keyReleased(KeyEvent e) {
				if (_nodeTypeCombo.getText().length() > 0
						&& _bladeTypeCombo.getText().length() > 0
						&& _nodeCount.getText().length() > 0)
				{
					_startButton.setEnabled(true);
				} else {
					_startButton.setEnabled(false);
				}
			}
    	});*/
    	_nodeCount.addModifyListener(new ModifyListener() {
			public void modifyText(ModifyEvent e) {
				if (_nodeTypeCombo.getText().length() > 0
						&& _bladeTypeCombo.getText().length() > 0
						&& _nodeCount.getText().length() > 0)
				{
					_startButton.setEnabled(true);
				} else {
					_startButton.setEnabled(false);
				}
			}});
        /******************************************************/
    	/* Create Instruction Label for Table                 */
    	/******************************************************/
        GridData gridData2 = new GridData(GridData.HORIZONTAL_ALIGN_FILL);
        gridData2.horizontalSpan = 2;
    	Label tableInstruct = new Label(container, SWT.WRAP);
    	tableInstruct.setText("(Use the table below to help determine the number of nodes to create)");
    	tableInstruct.setLayoutData(gridData2);

        /******************************************************/
    	/* Create Service Group Info Table                    */
    	/******************************************************/
        GridData tableGridData = new GridData(GridData.HORIZONTAL_ALIGN_FILL);
        tableGridData.horizontalSpan = 2;
    	Table serviceGroupInfo = new Table(container, SWT.SINGLE | SWT.HIDE_SELECTION
    										| SWT.V_SCROLL | SWT.H_SCROLL | SWT.BORDER);
    	tableGridData.heightHint = 90;
    	tableGridData.minimumHeight = 50;
    	
    	serviceGroupInfo.setHeaderVisible(true);
    	serviceGroupInfo.setLinesVisible(true);

    	TableColumn tc = new TableColumn(serviceGroupInfo, SWT.LEFT);
        tc.setText("Service Group");
        tc.setResizable(true);
        tc.setWidth(200);

    	TableColumn tc2 = new TableColumn(serviceGroupInfo, SWT.CENTER);
        tc2.setText("Node Count Options");
        tc2.setResizable(true);
        tc2.setWidth(80);
    	
        serviceGroupInfo.setLayoutData(tableGridData);
    	
        Object[] tableData = getServiceGroupData();
        
        for (int i=0; i<tableData.length; i++)
        {
        	String[] rowData = (String[])tableData[i];
            TableItem ti = new TableItem(serviceGroupInfo, SWT.NONE);
            ti.setText(rowData);
        }
    	
    	/******************************************************/
    	/* Create Button to Initiate Work                     */
    	/******************************************************/
    	_startButton = new Button(container, SWT.PUSH);
    	_startButton.setText("Create Instance Tree");
    	_startButton.addSelectionListener(new SelectionListener()
    	{
			public void widgetSelected(SelectionEvent e)
			{
				if (buildAMFInstanceTree())
				{
					_nodeTypeCombo.setText("");
					_bladeTypeCombo.setText("");
					_nodeCount.setText("");
					_nodeCount.clearSelection();
					_startButton.setEnabled(false);
					_npd.getTreeViewer().refresh();
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) { }
		});
    	_startButton.setEnabled(false);

    	return container;
	}

	/**
	 * Retrieves the list of all node types defined in the component editor.
	 * 
	 * @return the list of node types
	 */
	private String[] getNodeTypes()
	{
		List nodeTypeList = ComponentDataUtils.getNodesList(_pdm.getComponentModel().getEList());

		String[] nodeTypes = new String[nodeTypeList.size() + 1];

		nodeTypes[0] = "";
		
		for (int i = 0; i < nodeTypeList.size(); i++) {
            EObject eobj = (EObject) nodeTypeList.get(i);
            String nodeType = EcoreUtils.getName(eobj);
            nodeTypes[i+1] = nodeType;
        }

		return nodeTypes;
	}
	
	/**
	 * Retrieves the list of all blade types defined in the resource editor.
	 * 
	 * @return the list of blade types
	 */
	private String[] getBladeTypes()
	{
		List resObjects = _pdm.getResourceModel().getEList();
    	EObject rootObject = (EObject) resObjects.get(0);
		EReference ref = (EReference) rootObject.eClass()
						.getEStructuralFeature(ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME);
		EList bladeTypeList = (EList) rootObject.eGet(ref);

		String[] bladeTypes = new String[bladeTypeList.size() + 1];

		bladeTypes[0] = "";

		for (int i = 0; i < bladeTypeList.size(); i++) 
		{
			EObject eobj = (EObject) bladeTypeList.get(i);
			String bladeType = EcoreUtils.getName(eobj);
			Integer maxCount = (Integer)EcoreUtils.getValue(eobj, ClassEditorConstants.CLASS_MAX_INSTANCES);
			bladeTypes[i+1] = bladeType;
			//_bladeTypesMaxInstance.put(bladeType, maxCount);
		}
		
		return bladeTypes;
	}

	/**
	 * Builds the required object instances in the instance tree. In this case it is
	 *  only node instances. There is no saving involved. Saving of the instances is
	 *  handled by the Save on the AMF Configuration dialog.
	 *  
	 * @return success or failure
	 */
	private boolean buildAMFInstanceTree()
	{
		String nodeType = _nodeTypeCombo.getText();
		String bladeType = _bladeTypeCombo.getText();
		Integer nodeCount = Integer.parseInt(_nodeCount.getText());
		
		EObject iocNodeInstancesObj = (EObject) EcoreUtils.getValue(_amfObject,
				SafConstants.NODE_INSTANCES_NAME);
		EClass iocNodeInstanceClass = (EClass) iocNodeInstancesObj.eClass()
				.getEPackage().getEClassifier(
						SafConstants.NODE_INSTANCELIST_ECLASS);
		List iocNodeList = (List) EcoreUtils.getValue(iocNodeInstancesObj,
				SafConstants.NODE_INSTANCELIST_NAME);
		
		//********************************************************
		// make sure that moid count does not exceed blade type max
		//********************************************************
		/*int currCount = getCurrentInstanceCount(iocNodeList, bladeType);
		int maxCount = _bladeTypesMaxInstance.get(bladeType).intValue();
		if (currCount + nodeCount > maxCount)
		{
			String message = "A node instance cannot be created for blade type " + bladeType
				+ ". The number of node instances already meets or exceeds the maximum"
				+ " number of instances defined for this blade type. Either increase the"
				+ " maximum number of instances for " + bladeType + " or delete an existing"
				+ " node instance that is associated with this blade type.";

			MessageDialog.openError(getShell(), "AMF Configuration Errors", message);
			return false;
		}*/
		
		if(nodeType.equals(getSystemControllerNodeName())) {
			if((nodeCount + getSCNodeCount()) > 2) {
				String message = "More than two node instances cannot be created for System Controller node type " + nodeType;
				MessageDialog.openError(getShell(), "AMF Configuration Errors", message);
				return false;
			}
		}
		// create the nodes
		for (int i=0; i<nodeCount; i++)
		{
			EObject newNodeInst = EcoreUtils.createEObject(iocNodeInstanceClass, true);
			EcoreUtils.setValue(newNodeInst, EcoreUtils.getNameField(newNodeInst.eClass()), getNextNodeName(nodeType));
			EcoreUtils.setValue(newNodeInst, "type", nodeType);
			
			String moid = getNextNodeInstanceMoId(bladeType);
			// if we didn't get a moid then bail out
			if (moid.length() == 0)
			{
				String message = "A valid MOID could not be generated for the node instance being created.";
				MessageDialog.openError(getShell(), "AMF Configuration Errors", message);
				return false;
			}
			EcoreUtils.setValue(newNodeInst, MOID_ATTR, moid);
			EcoreUtils.setValue(newNodeInst, "chassisId", "0");
			EcoreUtils.setValue(newNodeInst, "slotId", "0");

			iocNodeList.add(newNodeInst);

			_npd.addNodeInstanceToTree(newNodeInst);
		}
		
		return true;
	}
	
	/**
	 * Return the number of node instances already created on the given
	 *  blade type.
	 *  
	 * @param nodeInstances
	 * @param bladeType
	 * @return
	 */
	/*private int getCurrentInstanceCount(List nodeInstances, String bladeType)
	{
		int currCount = 0;
		Pattern bladePattern = Pattern.compile("\\\\.+\\\\" + bladeType + ":.+");
		
		for (int i=0; i<nodeInstances.size(); i++)
		{
			EObject nodeInst = (EObject)nodeInstances.get(i);
			String moid = (String) EcoreUtils.getValue(nodeInst, MOID_ATTR);
			Matcher matcher = bladePattern.matcher(moid);
			if (matcher.matches()) currCount++;
		}
		
		return currCount;
	}*/
	
	/**
	 * Compute the next available node instance name based on comparing a possible
	 *  name against the existing object instance names.
	 *  
	 * @param nameRoot - base string that will be used at the beginning of the name
	 * @return the valid, unique name
	 */
	private String getNextNodeName(String nameRoot)
	{
		int count=0;
		String nextName = new String();
		boolean keepLooking = true;
		
		List objectList = AMFListUniquenessValidator.getAllAMFObjects(_npd.getViewModel().getEList());
		
		while (keepLooking)
		{
			keepLooking = false;
			
			nextName = nameRoot + "I" + count;
			
			for (int i=0; i<objectList.size(); i++)
			{
				String existingName = EcoreUtils.getName((EObject)objectList.get(i));
				if (existingName.toLowerCase().equals(nextName.toLowerCase()))
				{
					keepLooking = true;
				}
			}
			
			count++;
		}
		
		return nextName;
	}
	
	/**
	 * Computes the next available MOID for the node instance based on the MOID's
	 *  that already exists.
	 *   
	 * @param bladeType
	 * @return the valid moid value
	 */
	private String getNextNodeInstanceMoId(String bladeType)
	{
		int count=0;
		String nextName = new String();
		boolean keepLooking = true;
		
		// get the list of chassis in the resource model...there should be only one
		List resObjects = _pdm.getResourceModel().getEList();
    	EObject rootObject = (EObject) resObjects.get(0);
		EReference ref = (EReference) rootObject.eClass()
						.getEStructuralFeature(ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME);
		EList chassisList = (EList) rootObject.eGet(ref);

		if (chassisList.size() != 1) return "";

		// the the name of the one and only chassis
		EObject eobj = (EObject) chassisList.get(0);
		String chassisName = EcoreUtils.getName(eobj);

		// get a list of all the amf objects in the model so we can check moid uniqueness
		List objectList = AMFListUniquenessValidator.getAllAMFObjects(_npd.getViewModel().getEList());
		
		while (keepLooking)
		{
			keepLooking = false;
			
			nextName = "\\" + chassisName + ":0\\" + bladeType + ":" + count;
			
			for (int i=0; i<objectList.size(); i++)
			{
				EObject obj = (EObject)objectList.get(i);
				if (obj.eClass().getName().equals(SafConstants.NODE_INSTANCELIST_ECLASS))
				{
					String existingMoId = (String)EcoreUtils.getValue(obj, MOID_ATTR);
					if (existingMoId.toLowerCase().equals(nextName.toLowerCase()))
					{
						keepLooking = true;
					}
				}
			}
			
			count++;
		}
		
		return nextName;
	}

	/**
	 * Returns the data required to fill the service group info table.
	 * @return
	 */
	private Object[] getServiceGroupData()
	{
		List componentObjects = _pdm.getComponentModel().getEList();
    	EObject rootObject = (EObject) componentObjects.get(0);
		EReference ref = (EReference) rootObject.eClass()
						.getEStructuralFeature(ComponentEditorConstants.SERVICEGROUP_REF_NAME);
		EList serviceGroupTypeList = (EList) rootObject.eGet(ref);

		Object[] serviceGroupData = new Object[serviceGroupTypeList.size()];
		
		for (int i = 0; i < serviceGroupTypeList.size(); i++) {
            EObject eobj = (EObject) serviceGroupTypeList.get(i);
            String serviceGroupType = EcoreUtils.getName(eobj);
            String[] sgData = new String[2];
            Integer active = (Integer)EcoreUtils.getValue(eobj, SafConstants.SG_ACTIVE_SU_COUNT);
            Integer standby = (Integer)EcoreUtils.getValue(eobj, SafConstants.SG_STANDBY_SU_COUNT);
            sgData[0] = serviceGroupType + " (" + active.toString() + " + " + standby.toString() + ")";
            
            int suCount = new Integer(active + standby).intValue();
            String nodeOptions = "1";
            if (suCount > 1)
            {
            	nodeOptions = nodeOptions + " or " + suCount;
            }
            sgData[1] = nodeOptions;
            serviceGroupData[i] = sgData;
        }

		return serviceGroupData;
	}
	
	/**
	 * Returns SC Node count
	 * @return
	 */
	private int getSCNodeCount() {
		int count = 0;
		EObject iocNodeInstancesObj = (EObject) EcoreUtils.getValue(_amfObject,
				SafConstants.NODE_INSTANCES_NAME);
		EClass iocNodeInstanceClass = (EClass) iocNodeInstancesObj.eClass()
				.getEPackage().getEClassifier(
						SafConstants.NODE_INSTANCELIST_ECLASS);
		List iocNodeList = (List) EcoreUtils.getValue(iocNodeInstancesObj,
				SafConstants.NODE_INSTANCELIST_NAME);
		for (int i = 0; i < iocNodeList.size(); i++) {
			EObject eobj = (EObject) iocNodeList.get(i);
			String type = String.valueOf(EcoreUtils.getValue(eobj, "type"));
			if (type.equals(getSystemControllerNodeName())) {
				count++;
			}
		}
		return count;
	}
	/**
	 * Find System Controller Node Name 
	 * @return SC name
	 */
	private String getSystemControllerNodeName() {
		if(_systemCtrNodeName == null) {
			List nodeTypeList = ComponentDataUtils.getNodesList(_pdm.getComponentModel().getEList());
			for (int i = 0; i < nodeTypeList.size(); i++) {
	            EObject eobj = (EObject) nodeTypeList.get(i);
	            String classType = String.valueOf(EcoreUtils.getValue(eobj, "classType"));
	            if(classType.equals("CL_AMS_NODE_CLASS_B") || classType.equals("CL_AMS_NODE_CLASS_A")){
	            	_systemCtrNodeName = EcoreUtils.getName(eobj);
	            	return _systemCtrNodeName;
	            }
	        }
		}
		return _systemCtrNodeName;
	}
}
