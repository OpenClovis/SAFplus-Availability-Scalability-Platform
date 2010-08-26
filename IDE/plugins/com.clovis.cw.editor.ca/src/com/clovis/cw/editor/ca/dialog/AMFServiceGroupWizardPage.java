package com.clovis.cw.editor.ca.dialog;

import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.validator.AMFListUniquenessValidator;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Preference page for the Service Group List item in the AMF Configuration tree.
 * This page allows the user to create a number of service group instances with
 * one action. Along with creating the service group instance it will also create
 * an associated service instance and component service instance.
 * 
 * For each node instance selected it will create a service unit instance and
 * component instance and associate the service unit instance with the newly
 * created service group instance.
 * 
 * @author matt
 */
public class AMFServiceGroupWizardPage extends GenericPreferencePage {
	
	private ProjectDataModel _pdm;
	private CCombo _serviceGroupTypeCombo;
	private org.eclipse.swt.widgets.List _nodeInstancesList;
	private Button _startButton;
	private EObject _amfObject;
	private NodeProfileDialog _npd;
	
	private static String TYPE_ATTR = "type";
	
	// Hashtable to hold the service group type names and the maximum number of nodes the
	//  type can have. This value is determined by adding the active SU's and the
	//  standby SU's for the type.
	private Hashtable<String, Object> _sgRedundancyNodes = new Hashtable<String, Object>();

	/* Hashtable to hold the service group type names and the maximum number of nodes the
	 * type can have considering the inservice SU configuration for the service group type.
	 */
	private Hashtable<String, Object> _sgInserviceNodes = new Hashtable<String, Object>();

	/**
	 * Constructor
	 * @param label Title for the page.
	 * @param obj EObject for the FormView.
	 * @param nodeList 
	 */
	public AMFServiceGroupWizardPage(String label, ProjectDataModel pdm, EObject amfObject, NodeProfileDialog npd) {
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
        instructions.setText("Use this page to rapidly fill out the object instance tree."
        		+ " From this page you can create Service Group Instances and define on which"
        		+ " Node Instances they will run. After clicking the 'Create Object Tree' button"
        		+ " the wizard will create the entire instance tree for this association and"
        		+ " link the newly created Service Group with the appropriate Service Instance(s)"
        		+ " under the Node Instance.\n");
        instructions.setLayoutData(gridData);
        
    	/******************************************************/
    	/* Create Service Group Type Selector                 */
    	/******************************************************/
    	Label serviceGroupTypeLabel = new Label(container, SWT.NONE);
    	serviceGroupTypeLabel.setText("Service Group Type: ");

    	_serviceGroupTypeCombo = new CCombo(container, SWT.DROP_DOWN | SWT.READ_ONLY | SWT.BORDER);
    	_serviceGroupTypeCombo.setItems(getServiceGroupTypes());
    	_serviceGroupTypeCombo.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	_serviceGroupTypeCombo.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				if (_serviceGroupTypeCombo.getText().length() > 0)
				{
					_nodeInstancesList.setItems(getNodeInstances(_serviceGroupTypeCombo.getText()));
				} else {
					_nodeInstancesList.removeAll();
				}
				_startButton.setEnabled(false);
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
    	});
        
    	/******************************************************/
    	/* Create Node Instance List                          */
    	/******************************************************/
    	Label nodeInstancesLabel = new Label(container, SWT.None);
    	nodeInstancesLabel.setText("Associated Node Instances: ");
    	nodeInstancesLabel.setLayoutData(new GridData(GridData.VERTICAL_ALIGN_BEGINNING));
    	
    	_nodeInstancesList = new org.eclipse.swt.widgets.List(container, SWT.MULTI | SWT.BORDER | SWT.V_SCROLL);

    	GridData listGridData = new GridData(GridData.HORIZONTAL_ALIGN_FILL | GridData.VERTICAL_ALIGN_FILL);
        int listHeight = _nodeInstancesList.getItemHeight() * 4;
        Rectangle trim = _nodeInstancesList.computeTrim(0, 0, 0, listHeight);
        listGridData.heightHint = trim.height;
        _nodeInstancesList.setLayoutData(listGridData);
    	_nodeInstancesList.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				if (_serviceGroupTypeCombo.getText().length() > 0 && _nodeInstancesList.getSelectionCount() > 0)
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
					_serviceGroupTypeCombo.setText("");
					_nodeInstancesList.removeAll();
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
	 * Get a list of node instances that can be selected based on the service
	 *  group type that is passed in. A node instance is considered available for
	 *  selection if the type that it is based on shares a service unit type with
	 *  the given service group type.
	 * @param serviceGroupType
	 * @return list of valid node instance names
	 */
	private String[] getNodeInstances(String serviceGroupType)
	{
		ArrayList<String> validServiceUnitTypes = new ArrayList<String>();
		ArrayList<String> validNodeTypes = new ArrayList<String>();
		ArrayList<String> validNodeInstances = new ArrayList<String>();
		
		// get the list of all service unit types under the passed service group type
		EObject serviceGroupTypeObject = _npd.getObjectFrmName(serviceGroupType);
		List serviceGroupChildren = _npd.getChildren(serviceGroupTypeObject);
		for (int i=0; i<serviceGroupChildren.size(); i++)
		{
			EObject childObject = (EObject)serviceGroupChildren.get(i);
			String childType = childObject.eClass().getName();
			if (childType.equals(ComponentEditorConstants.SERVICEUNIT_NAME))
			{
				validServiceUnitTypes.add((String)EcoreUtils.getName(childObject));
			}
		}
		
		// get a list of all node types that contain any of the service unit types
		//  that are also under the passed service group type
		List nodeTypeList = ComponentDataUtils.getNodesList(_pdm.getComponentModel().getEList());
		for (int i=0; i<nodeTypeList.size(); i++)
		{
			EObject nodeTypeObject = (EObject)nodeTypeList.get(i);
			List nodeTypeChildren = _npd.getChildren(nodeTypeObject);
			for (int j=0; j<nodeTypeChildren.size(); j++)
			{
				EObject childObject = (EObject)nodeTypeChildren.get(j);
				String childType = childObject.eClass().getName();
				if (childType.equals(ComponentEditorConstants.SERVICEUNIT_NAME))
				{
					if (validServiceUnitTypes.contains((String)EcoreUtils.getName(childObject)))
					{
						validNodeTypes.add((String)EcoreUtils.getName(nodeTypeObject));
					}
				}
			}
			
		}
		
		EObject iocNodeInstancesObj = (EObject) EcoreUtils.getValue(_amfObject,
				SafConstants.NODE_INSTANCES_NAME);
		List iocNodeList = (List) EcoreUtils.getValue(iocNodeInstancesObj,
				SafConstants.NODE_INSTANCELIST_NAME);
		
		for (int i=0; i<iocNodeList.size(); i++)
		{
			EObject nodeInstance = (EObject)iocNodeList.get(i);
			String nodeType = (String)EcoreUtils.getValue(nodeInstance, TYPE_ATTR);
			if (validNodeTypes.contains(nodeType))
			{
				validNodeInstances.add((String)EcoreUtils.getName(nodeInstance));
			}
		}

		String[] nodeInstances = new String[validNodeInstances.size()];
		for (int i=0; i<validNodeInstances.size(); i++)
		{
			nodeInstances[i] = (String)validNodeInstances.get(i);
		}
		
		return nodeInstances;
	}

	/**
	 * Gets a list of service group types that are defined in the component editor.
	 * 
	 * @return list of service group type names
	 */
	private String[] getServiceGroupTypes()
	{
		List componentObjects = _pdm.getComponentModel().getEList();
    	EObject rootObject = (EObject) componentObjects.get(0);
		EReference ref = (EReference) rootObject.eClass()
						.getEStructuralFeature(ComponentEditorConstants.SERVICEGROUP_REF_NAME);
		EList serviceGroupTypeList = (EList) rootObject.eGet(ref);

		String[] serviceGroupTypes = new String[serviceGroupTypeList.size() + 1];

		serviceGroupTypes[0] = "";
		
		for (int i = 0; i < serviceGroupTypeList.size(); i++) {
            EObject eobj = (EObject) serviceGroupTypeList.get(i);
            String serviceGroupType = EcoreUtils.getName(eobj);
            Integer maxNodes = (Integer)EcoreUtils.getValue(eobj, "numPrefActiveSUs")
            				+ (Integer)EcoreUtils.getValue(eobj, "numPrefStandbySUs");
            _sgRedundancyNodes.put(serviceGroupType, maxNodes);
            _sgInserviceNodes.put(serviceGroupType, (Integer)EcoreUtils.getValue(eobj, "numPrefInserviceSUs"));
            serviceGroupTypes[i+1] = serviceGroupType;
        }

		return serviceGroupTypes;
	}

	/**
	 * Builds the required object instances in the instance tree. In this case it is
	 *  service group, service instance, component service instance, service unit, and
	 *  component instances. There is no saving involved. Saving of the instances is
	 *  handled by the Save on the AMF Configuration dialog.
	 *  
	 * @return success or failure
	 */
	private boolean buildAMFInstanceTree()
	{
		String serviceGroupType = _serviceGroupTypeCombo.getText();
		String[] nodeInstances = _nodeInstancesList.getSelection();

		// now create service groups and their children
		if (serviceGroupType.length() > 0)
		{
			EObject iocSGInstancesObj = (EObject) EcoreUtils.getValue(_amfObject,
					SafConstants.SERVICEGROUP_INSTANCES_NAME);
			EClass iocSGInstanceClass = (EClass) iocSGInstancesObj.eClass()
					.getEPackage().getEClassifier(
							SafConstants.SG_INSTANCELIST_ECLASS);
			List iocSGList = (List) EcoreUtils.getValue(iocSGInstancesObj,
					SafConstants.SERVICEGROUP_INSTANCELIST_NAME);
			
			//*******************************************************************************
			// Here we make sure that the user selected either one node instance or a number
			//  equal to the maximum SU's/inservice SU's for the service group type. These 
			//  are the only three configurations we support through the wizard. If the user
			//  selected one node instance all SU instances will be created under that node
			//  instance. If the user selected the max number of node instances we will
			//  create one SU instance under each of the node instances.
			//*******************************************************************************
			int selectedNodeCount = nodeInstances.length;
			int maxNodeCount = ((Integer)_sgRedundancyNodes.get(serviceGroupType)).intValue();
			int inserviceNodeCount = ((Integer) _sgInserviceNodes.get(serviceGroupType)).intValue();
			if (selectedNodeCount != 1 && selectedNodeCount != maxNodeCount && selectedNodeCount != inserviceNodeCount)
			{
				String message = "You have selected an invalid number of node instances. You must"
					+ " either select one node instance (in which case all generated service unit"
					+ " instances will be placed under that one node instance) or the maximum number"
					+ " of node instances for the service group type's redundancy model. In the"
					+ " case of service group type " + serviceGroupType + " this would be "
					+ inserviceNodeCount + " node instance(s).";

				
				MessageDialog.openError(getShell(), "AMF Configuration Errors", message);
				return false;
			}

			EObject newSGInst = EcoreUtils.createEObject(iocSGInstanceClass, true);
			EcoreUtils.setValue(newSGInst, EcoreUtils.getNameField(newSGInst.eClass()), getNextInstanceName(serviceGroupType));
			EcoreUtils.setValue(newSGInst, TYPE_ATTR, serviceGroupType);

			iocSGList.add(newSGInst);

			createServiceInstances(newSGInst, serviceGroupType);
			
			_npd.addSGInstanceToTree(newSGInst);
			
			//now create service units for each node instance selected
			for (int i=0; i<nodeInstances.length; i++)
			{
				String nodeInstanceName = nodeInstances[i];
				createServiceUnitInstances(serviceGroupType, nodeInstanceName, newSGInst);
			}
		}
		
		return true;
	}

	/**
	 * Create service unit instances under the node instance specified by the given node
	 *  instance name. These service unit instances are also associated with the given
	 *  service group instance.
	 *  
	 * @param serviceGroupType
	 * @param nodeInstanceName
	 * @param serviceGroupInstance
	 */
	private void createServiceUnitInstances(String serviceGroupType, String nodeInstanceName, EObject serviceGroupInstance)
	{
		// get an ArrayList of all service unit types under the given service group type
		EObject serviceGroupTypeObject = _npd.getObjectFrmName(serviceGroupType);
		List serviceGroupChildren = _npd.getChildren(serviceGroupTypeObject);
		ArrayList<String> validServiceUnits = new ArrayList<String>();
		
		for (int i=0; i<serviceGroupChildren.size(); i++)
		{
			EObject child = (EObject)serviceGroupChildren.get(i);
			String childType = child.eClass().getName();
			if (childType.equals(ComponentEditorConstants.SERVICEUNIT_NAME))
			{
				validServiceUnits.add((String)EcoreUtils.getName(child));
			}
		}

		// get the list of all node instances
		EObject iocNodeInstancesObj = (EObject) EcoreUtils.getValue(_amfObject,
				SafConstants.NODE_INSTANCES_NAME);
		List iocNodeList = (List) EcoreUtils.getValue(iocNodeInstancesObj,
				SafConstants.NODE_INSTANCELIST_NAME);
		
		// go over the list of node instances looking for the one whose name
		//  matches the node instance name that was passed in
		for (int i=0; i<iocNodeList.size(); i++)
		{
			EObject nodeInstance = (EObject)iocNodeList.get(i);
			String nodeName = EcoreUtils.getName(nodeInstance);
			if (nodeName.equals(nodeInstanceName))
			{
				// now that we have found the matching node instance figure out what
				//  type of node it is and get all children of that type from the
				//  component model
				String nodeType = (String)EcoreUtils.getValue(nodeInstance, TYPE_ATTR);
				EObject nodeTypeObject = _npd.getObjectFrmName(nodeType);
				List children = _npd.getChildren(nodeTypeObject);
				
				for (int j=0; j<children.size(); j++)
				{
					// check to see if the child object is of a type listed in the valid
					//  service units that we got from the service group type
					String suType = EcoreUtils.getName((EObject)children.get(j));
					if (validServiceUnits.contains(suType))
					{
						// this service unit type is valid so create a new service unit instance
						//  of this type and add it to the list of service unit instances for
						//  this node instance
						EObject iocSUInstancesObj = (EObject) EcoreUtils.getValue(nodeInstance,
								SafConstants.SERVICEUNIT_INSTANCES_NAME);
						EClass nodeInstanceClass = (EClass) iocSUInstancesObj.eClass()
								.getEPackage().getEClassifier(
										SafConstants.SERVICEUNIT_INSTANCELIST_ECLASS);
						List iocSUList = (List) EcoreUtils.getValue(iocSUInstancesObj,
								SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
	
						//*******************************************************************************
						// If the user selected only one node instance then we will create a number of
						//  service unit instances under the single node instance. The number created
						//  will be equal to the max number of SU's allowed by the service group type.
						//  If the user selected more than one node instance then we will create one
						//  service unit instance under each of the node instances.
						//*******************************************************************************
						int selectedNodeCount = _nodeInstancesList.getSelection().length;
						int suCount = selectedNodeCount > 1 ? 1 : ((Integer)_sgInserviceNodes.get(serviceGroupType)).intValue();

						for (int k=0; k<suCount; k++)
						{
							EObject newSUInst = EcoreUtils.createEObject(nodeInstanceClass, true);
							EcoreUtils.setValue(newSUInst, EcoreUtils.getNameField(newSUInst.eClass()), getNextInstanceName(suType));
							EcoreUtils.setValue(newSUInst, TYPE_ATTR, suType);
							
							// associate created service unit with the passed in service group
							associateSUwithSG(serviceGroupInstance, newSUInst);
							
							createComponentInstances(newSUInst, suType);
							
							iocSUList.add(newSUInst);
							
							_npd.addSUInstanceToTree(nodeInstance, newSUInst);
						}
					}

				}
			}
		}

		
	}
	
	/**
	 * Associate the given service unit instance with the given service group instance.
	 * 
	 * @param serviceGroup
	 * @param serviceUnit
	 */
	private void associateSUwithSG(EObject serviceGroup, EObject serviceUnit)
	{
		EReference suInstRef = (EReference) serviceGroup.eClass().getEStructuralFeature(
								SafConstants.ASSOCIATED_SERVICEUNITS_NAME);
		EObject associatedSUSObj = (EObject) serviceGroup.eGet(suInstRef);
		if (associatedSUSObj == null)
		{
			associatedSUSObj = EcoreUtils.createEObject(suInstRef.getEReferenceType(), true);
		}
		List associatedSUs = (List) EcoreUtils.getValue(associatedSUSObj,
								SafConstants.ASSOCIATED_SERVICEUNIT_LIST);
		associatedSUs.add(EcoreUtils.getName(serviceUnit));
	}

	/**
	 * Create an instance of each component type that is associated with the service unit
	 *  type represented by the given name and associate them with the given service unit instance.
	 * 
	 * @param serviceUnit
	 * @param serviceUnitType
	 */
	private void createComponentInstances(EObject serviceUnit, String serviceUnitType)
	{
		EObject serviceUnitTypeObject = _npd.getObjectFrmName(serviceUnitType);
		List children = _npd.getChildren(serviceUnitTypeObject);
		
		for (int i=0; i<children.size(); i++)
		{
			String compType = EcoreUtils.getName((EObject)children.get(i));
			EObject iocCompInstancesObj = (EObject) EcoreUtils.getValue(serviceUnit,
					SafConstants.COMPONENT_INSTANCES_NAME);
			EClass compInstanceClass = (EClass) iocCompInstancesObj.eClass()
					.getEPackage().getEClassifier(
							SafConstants.COMPONENT_INSTANCELIST_ECLASS);
			List iocCompList = (List) EcoreUtils.getValue(iocCompInstancesObj,
					SafConstants.COMPONENT_INSTANCELIST_NAME);

			EObject newCompInst = EcoreUtils.createEObject(compInstanceClass, true);
			EcoreUtils.setValue(newCompInst, EcoreUtils.getNameField(newCompInst.eClass()), getNextInstanceName(compType));
			EcoreUtils.setValue(newCompInst, TYPE_ATTR, compType);
			
			iocCompList.add(newCompInst);

		}
		
	}

	/**
	 * Create Service Instance (SI) instances under the given Service Group (SG) instance. The number
	 *  of SIs created is determined by the characteristics of the SG type. It is calculated by dividing
	 *  the number of SG type's number of active SUs by its number of active SUs per SI.
	 *  
	 *  After creating an SI instance a method will be called to create the Component Service Instance (CSI)
	 *  instances belonging to that SI. It is a complex calculation to determine how many CSI instances
	 *  should be created under each SI instance. This calculation is based on the following rules.
	 *  
	 *  For the set of SI instances under a SG instance...
	 *  1. Each SI instance must have at least one CSI instance.
	 *  2. As CSIs are created we look at the 'numMaxActiveCSIs' value on the CSI types component type. This
	 *     is the number of CSI instances that will be created and attached to to SI instance. Once a CSI
	 *     type is created we keep track of it and the next one created will be of the next CSI type.
	 *  3. After all of the CSI types defined under an SI type have been created, if new SI instances are
	 *     created we will force the creation of a single CSI instance (based on teh the first CSI type)
	 *     on these new SI instances.
	 *  4. If the number of CSI types exceeds the number needed to 'fill out' the SI instances then the
	 *     remaining CSI types will not have instances created under any of the SI instances.
	 * 
	 * @param serviceGroup
	 * @param serviceGroupType
	 */
	private void createServiceInstances(EObject serviceGroup, String serviceGroupType)
	{
		EObject serviceGroupTypeObject = _npd.getObjectFrmName(serviceGroupType);
		
		// Compute the number of Service Instances to create under the Service Group. This number is
		//  determined by dividing the number of active service units for the SG by the number of
		//  active service units per SI for the SG.
		int activeSUs = ((Integer)EcoreUtils.getValue(serviceGroupTypeObject, "numPrefActiveSUs")).intValue();
		if (activeSUs <1) activeSUs = 1;
		int activeSUsPerSI = ((Integer)EcoreUtils.getValue(serviceGroupTypeObject, "numPrefActiveSUsPerSI")).intValue();
		if (activeSUsPerSI <1) activeSUsPerSI = 1;
		int numberSIsToCreate = activeSUs/activeSUsPerSI;
		
		List children = _npd.getChildren(serviceGroupTypeObject);
		
		for (int i=0; i<children.size(); i++)
		{
			EObject child = (EObject)children.get(i);
			if (child.eClass().getName().equals(SafConstants.SI_INSTANCELIST_ECLASS))
			{
				// get the number of CSIs to create
				int csiCount = getCSICount(child);
				
				// arraylist to keep track of the CSI types that have been created
				ArrayList createdCSIs = new ArrayList();
				
				for (int j=0; j<numberSIsToCreate; j++)
				{
					String siType = EcoreUtils.getName(child);
					EObject iocSIInstancesObj = (EObject) EcoreUtils.getValue(serviceGroup,
							SafConstants.SERVICE_INSTANCES_NAME);
					EClass siInstanceClass = (EClass) iocSIInstancesObj.eClass()
							.getEPackage().getEClassifier(
									SafConstants.SI_INSTANCELIST_ECLASS);
					List iocSIList = (List) EcoreUtils.getValue(iocSIInstancesObj,
							SafConstants.SERVICE_INSTANCELIST_NAME);
		
					EObject newSIInst = EcoreUtils.createEObject(siInstanceClass, true);
					EcoreUtils.setValue(newSIInst, EcoreUtils.getNameField(newSIInst.eClass()), getNextInstanceName(siType));
					EcoreUtils.setValue(newSIInst, TYPE_ATTR, siType);
					
					iocSIList.add(newSIInst);
					
					// if we have already created an instance of each CSI type then force the creation
					//  of a CSI instance on the SI instance
					boolean forceCreate = createdCSIs.size() >= csiCount ? true : false;

					createComponentServiceInstances(newSIInst, siType, createdCSIs, forceCreate);
				}
			}

		}
		
	}
	
	/**
	 * Return the number of Component Service Instance types under the given Service Instance
	 *  type as defined in the component editor.
	 * @param serviceInstanceTypeObject
	 * @return
	 */
	private int getCSICount(EObject serviceInstanceTypeObject)
	{
		int csiCount = 0;
		
		List children = _npd.getChildren(serviceInstanceTypeObject);
		
		for (int i=0; i<children.size(); i++)
		{
			EObject child = (EObject)children.get(i);
			if (child.eClass().getName().equals(SafConstants.CSI_INSTANCELIST_ECLASS))
			{
				csiCount++;
			}

		}

		return csiCount;
	}

	/**
	 * Create Component Service Instance (CSI) instances under the given Service Insance (SI) instance. The
	 *  CSI instance created will typically be of the next available CSI type (that has not yet been created).
	 *  This behavior can be overridden by the frontLoadCount parameter (which forces the next N number of
	 *  CSI types to be created under the given SI) and the forceCreate parameter (which will force the
	 *  creation of CSI instance of the first type under the given SI).
	 * 
	 * @param serviceInstance
	 * @param serviceInstanceType
	 * @param createdCSI
	 * @param frontLoadCount
	 * @param forceCreate
	 */
	private void createComponentServiceInstances(EObject serviceInstance, String serviceInstanceType,
												 ArrayList createdCSI, boolean forceCreate)
	{
		EObject serviceInstanceTypeObject = _npd.getObjectFrmName(serviceInstanceType);
		List children = _npd.getChildren(serviceInstanceTypeObject);
		
		int numActiveCSI = 0;
		int createdCount = 0;

		for (int i=0; i<children.size(); i++)
		{
			EObject child = (EObject)children.get(i);
			String csiType = EcoreUtils.getName(child);
			
			// create the instance if it is of the correct class type, and either
			//  - we have not created an instance of this CSI type yet
			//  - we have been told to force create a CSI intance
			if (child.eClass().getName().equals(SafConstants.CSI_INSTANCELIST_ECLASS)
					&& (!createdCSI.contains(csiType) || forceCreate))
			{
				
				if (numActiveCSI == 0) numActiveCSI = getComponentActiveCSICount(child);

				EObject iocCSIInstancesObj = (EObject) EcoreUtils.getValue(serviceInstance,
						SafConstants.CSI_INSTANCES_NAME);
				EClass csiInstanceClass = (EClass) iocCSIInstancesObj.eClass()
						.getEPackage().getEClassifier(
								SafConstants.CSI_INSTANCELIST_ECLASS);
				List iocCSIList = (List) EcoreUtils.getValue(iocCSIInstancesObj,
						SafConstants.CSI_INSTANCELIST_NAME);
				
				EObject newCSIInst = EcoreUtils.createEObject(csiInstanceClass, true);
				EcoreUtils.setValue(newCSIInst, EcoreUtils.getNameField(newCSIInst.eClass()), getNextInstanceName(csiType));
				EcoreUtils.setValue(newCSIInst, TYPE_ATTR, csiType);
				
				iocCSIList.add(newCSIInst);
				
				createdCSI.add(csiType);
				
				createdCount++;
				
				// if we have hit the number of active CSIs or we are force creating a CSI
				//  then make sure we exit the loop
				if ((createdCount >= numActiveCSI) || forceCreate)
				{
					i = children.size();
				}
			}

		}
		
	}
	
	/**
	 * Find the first Component type under the given CSI type and return its number
	 *  of active CSIs value.
	 * @param csiTypeObject
	 * @return
	 */
	private int getComponentActiveCSICount(EObject csiTypeObject)
	{
		int activeCSICount = 0;
		
		List children = _npd.getChildren(csiTypeObject);
		for (int i=0; i<children.size(); i++)
		{
			EObject child = (EObject)children.get(i);
			
			if (child.eClass().getName().equals(ComponentEditorConstants.SAFCOMPONENT_NAME))
			{
				activeCSICount = ((Integer)EcoreUtils.getValue(child, "numMaxActiveCSIs")).intValue();
			}
		}

		return activeCSICount;
	}
	
	/**
	 * Compute the next available object instance name based on comparing a possible
	 *  name against the existing object instance names.
	 *  
	 * @param nameRoot - base string that will be used at the beginning of the name
	 * @return the valid, unique name
	 */
	private String getNextInstanceName(String nameRoot)
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
}
