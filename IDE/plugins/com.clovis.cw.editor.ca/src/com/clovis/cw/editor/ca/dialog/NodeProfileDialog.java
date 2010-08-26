/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/NodeProfileDialog.java $
 * $Author: pushparaj $
 * $Date: 2007/03/21 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.io.File;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.Vector;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.common.util.BasicEList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EEnumLiteral;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.preference.IPreferenceNode;
import org.eclipse.jface.preference.IPreferencePage;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.BusyIndicator;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Link;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.ClovisProjectUtils;
import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreModels;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.log.Log;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.editor.ca.CaPlugin;
import com.clovis.cw.editor.ca.ComponentDataUtils;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.constants.SafConstants;
import com.clovis.cw.editor.ca.manageability.ui.AssociateResourceConstants;
import com.clovis.cw.editor.ca.manageability.ui.AssociateResourceUtils;
import com.clovis.cw.editor.ca.manageability.ui.ManageabilityEditorInput;
import com.clovis.cw.project.data.DependencyListener;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * Preference page for the Node List item in the AMF Configuration tree.
 * This page allows the user to create a number of node instances with
 * one action.
 * 
 * @author matt
 */
public class NodeProfileDialog extends GenericPreferenceDialog
{
    private Model _nodeProfiles;
    private String            _dialogDescription = "AMF Configuration";
    private HashMap           _nodeSuMap         = new HashMap();
    private HashMap           _suCompMap         = new HashMap();
    private HashMap           _sgSiMap         = new HashMap();
    private HashMap           _siCsiMap         = new HashMap();
    private EClass            _amfClass          = null;
    private List      _listOfNodes;
    private List      _resourceList;
    private List 	  _compEditorObjects = new Vector();
    private List      _compList = new Vector();
    private List      _serviceGrpList = new Vector();
    private List      _connectionList = new Vector();
    private HashMap _nodeTypeCpmObjMap = new HashMap();
    private HashMap _nodeInstanceMap = new HashMap();
    private HashMap _cwkeyObjectMap = new HashMap();
    private static final Log  LOG = Log.getLog(CaPlugin.getDefault());
    private DependencyListener _dependencyListener;
    private String 			  _contextHelpId = "com.clovis.cw.help.node_profile";
    private EObject _sourceObj = null; // This object is used. when this dialog is open from Problems View.
    public String _selectablePageID = null;// This id is used to select prefence node. if explicit selection required. 
    public IPreferenceNode _selectableNode;
    private static NodeProfileDialog instance    = null;
    private static int APPLY_BUTTON_ID = -1;
    private Button _applyButton;
    private PreferenceNode _nodeInstancesPrefNode;
    private PreferenceNode _sgInstancesPrefNode;
    private Model _pmConfigModel;
    private ModelListener _modelListener;
    
    /**
     * Gets current instance of Dialog
     * @return current instance of Dialog
     */
    public static NodeProfileDialog getInstance()
    {
        return instance;
    }
    /**
     * Close the dialog.
     * Remove static instance
     * @return super.close()
     */
    public boolean close()
    {
        if (_viewModel != null) {
            EcoreUtils.removeListener(_viewModel.getEList(), _dependencyListener, -1);
            EcoreUtils.removeListener(_viewModel.getEList(), _modelListener, -1);
            _viewModel.dispose();
            _viewModel = null;
        }
        instance = null;
        return super.close();
    }
    /**
     * Open the dialog.
     * Set static instance to itself
     * @return super.open()
     */
    public int open()
    {
        instance = this;
        return super.open();
    }
    /**
     *
     * @param parentShell
     *            parent shell
     * @param pManager
     *            preference manager
     * @param pdm
     *            ProjectDataModel
     * @param resource -
     *            Project Resource
     */
    public NodeProfileDialog(Shell parentShell, PreferenceManager pManager,
			IResource resource) {
		super(parentShell, pManager, (IProject) resource);
		BusyIndicator.showWhile(Display.getCurrent(), new Runnable() {
			public void run() {
				ProjectDataModel pdm = ProjectDataModel
						.getProjectDataModel((IContainer) _project);
				_compEditorObjects = pdm.getComponentModel().getEList();
				_resourceList = pdm.getCAModel().getEList();
				_listOfNodes = ComponentDataUtils.getNodesList(pdm
						.getComponentModel().getEList());
				_nodeProfiles = pdm.getNodeProfiles();
				_pmConfigModel = pdm.getPMConfigModel().getViewModel();
				_amfClass = (EClass) _nodeProfiles.getEPackage()
						.getEClassifier("amfConfig");
				initMaps(_compEditorObjects);
				initServiceGroupsList(_compEditorObjects);
				initConnectionList(_compEditorObjects);
				addPreferenceNodes();
			}
		});
	}
    /**
	 * 
	 * @param parentShell
	 *            parent shell
	 * @param pManager
	 *            preference manager
	 * @param pdm
	 *            ProjectDataModel
	 * @param resource -
	 *            Project Resource
	 * @param sourceObj
	 *            object needs to be opened
	 */
    public NodeProfileDialog(Shell parentShell, PreferenceManager pManager,
			IResource resource, final EObject sourceObj) {
		super(parentShell, pManager, (IProject) resource);
		BusyIndicator.showWhile(Display.getCurrent(), new Runnable() {
			public void run() {
				_sourceObj = sourceObj;
				ProjectDataModel pdm = ProjectDataModel
						.getProjectDataModel((IContainer) _project);
				_compEditorObjects = pdm.getComponentModel().getEList();
				_resourceList = pdm.getCAModel().getEList();
				_listOfNodes = ComponentDataUtils.getNodesList(pdm
						.getComponentModel().getEList());
				_nodeProfiles = pdm.getNodeProfiles();
				_pmConfigModel = pdm.getPMConfigModel().getViewModel();
				_amfClass = (EClass) _nodeProfiles.getEPackage()
						.getEClassifier("amfConfig");
				initMaps(_compEditorObjects);
				initServiceGroupsList(_compEditorObjects);
				initConnectionList(_compEditorObjects);
				addPreferenceNodes();
			}
		});
	}

	/**
	 * Override to add Apply button to buttonbar.
	 */
    protected void createButtonsForButtonBar(Composite parent)
    {
		_applyButton = createButton(parent, APPLY_BUTTON_ID, "Apply", false);
		
		super.createButtonsForButtonBar(parent);
	}
    
	/**
	 * Override to take action when Apply button pressed.
	 */
    protected void buttonPressed(int buttonId)
    {
		if (buttonId == APPLY_BUTTON_ID) {
	        save();
		} else {
			super.buttonPressed(buttonId);
		}
	}

	/**
	 * Override to enable/disable the Apply button.
	 */
    public void updateButtons()
	{
		_applyButton.setEnabled(isCurrentPageValid());
		super.updateButtons();
	}

	/**
     * Returns Nodes Model List
     *@param compList List of Components in editor
     *
     */
    public void initServiceGroupsList(List compList) {
		EObject rootObject = (EObject) compList.get(0);
		_serviceGrpList.addAll((EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.SERVICEGROUP_REF_NAME)));
	}
    /**
	 * 
	 * @param inputList
	 *            Resource Editor Input
	 */
   public void initConnectionList(List inputList) {
		EObject rootObject = (EObject) inputList.get(0);
		EList list = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(
						ComponentEditorConstants.AUTO_REF_NAME));
		for (int i = 0; i < list.size(); i++) {
			EObject eobj = (EObject) list.get(i);
			String type = (String) EcoreUtils.getValue(eobj,
					ComponentEditorConstants.CONNECTION_TYPE);
			if (type.equals(ComponentEditorConstants.CONTAINMENT_NAME)
					|| type.equals(ComponentEditorConstants.ASSOCIATION_NAME)) {
				_connectionList.add(eobj);
			}
		}
	}
    /**
	 * 
	 * @param list -
	 *            List of all EObjects in component editor
	 */
    private void initMaps(List compsList) {
		EObject rootObject = (EObject) compsList.get(0);
		List refList = rootObject.eClass().getEAllReferences();
		for (int i = 0; i < refList.size(); i++) {
			EList list = (EList) rootObject.eGet((EReference) refList.get(i));
			for (int j = 0; j < list.size(); j++) {
				EObject eObject = (EObject) list.get(j);
				String cwkey = (String) EcoreUtils.getValue(eObject,
						ModelConstants.RDN_FEATURE_NAME);
				_cwkeyObjectMap.put(cwkey, eObject);
			}
		}
		List connsList = (EList) rootObject.eGet(rootObject.eClass()
				.getEStructuralFeature(ComponentEditorConstants.AUTO_REF_NAME));
		for (int i = 0; i < connsList.size(); i++) {
			EObject eObject = (EObject) connsList.get(i);
			String type = (String) EcoreUtils.getValue(eObject,
					ComponentEditorConstants.CONNECTION_TYPE);
			if (type.equals(ComponentEditorConstants.CONTAINMENT_NAME)
					|| type.equals(ComponentEditorConstants.ASSOCIATION_NAME)) {
				String src = (String) EcoreUtils.getValue(eObject,
						ComponentEditorConstants.CONNECTION_START);
				EObject srcObj = (EObject) _cwkeyObjectMap.get(src);
				if (srcObj.eClass().getName().equals(
						ComponentEditorConstants.NODE_NAME)) {
					String target = (String) EcoreUtils.getValue(eObject,
							ComponentEditorConstants.CONNECTION_END);
					EObject targetObj = (EObject) _cwkeyObjectMap.get(target);
					if (targetObj.eClass().getName().equals(
							ComponentEditorConstants.SERVICEUNIT_NAME)) {
						List suList = (List) _nodeSuMap.get(EcoreUtils
								.getName(srcObj));
						if (suList == null) {
							suList = new Vector();
							_nodeSuMap.put(EcoreUtils.getName(srcObj), suList);
						}
						suList.add(targetObj);
					}
				} else if (srcObj.eClass().getName().equals(
						ComponentEditorConstants.SERVICEUNIT_NAME)) {
					String target = (String) EcoreUtils.getValue(eObject,
							ComponentEditorConstants.CONNECTION_END);
					EObject targetObj = (EObject) _cwkeyObjectMap.get(target);
					if (targetObj.eClass().getName().equals(
							ComponentEditorConstants.NONSAFCOMPONENT_NAME)
							|| targetObj.eClass().getName().equals(
									ComponentEditorConstants.SAFCOMPONENT_NAME)) {
						List compList = (List) _suCompMap.get(EcoreUtils
								.getName(srcObj));
						if (compList == null) {
							compList = new Vector();
							_suCompMap
									.put(EcoreUtils.getName(srcObj), compList);
						}
						compList.add(targetObj);

						if (!_compList.contains(targetObj))
							_compList.add(targetObj);
					}
				} else if (srcObj.eClass().getName().equals(
						ComponentEditorConstants.SERVICEGROUP_NAME)) {
					String target = (String) EcoreUtils.getValue(eObject,
							ComponentEditorConstants.CONNECTION_END);
					EObject targetObj = (EObject) _cwkeyObjectMap.get(target);
					if (targetObj.eClass().getName().equals(
							ComponentEditorConstants.SERVICEINSTANCE_NAME)) {
						List siList = (List) _sgSiMap.get(EcoreUtils
								.getName(srcObj));
						if (siList == null) {
							siList = new Vector();
							_sgSiMap.put(EcoreUtils.getName(srcObj), siList);
						}
						siList.add(targetObj);
					}
				} else if (srcObj.eClass().getName().equals(
						ComponentEditorConstants.SERVICEINSTANCE_NAME)) {
					String target = (String) EcoreUtils.getValue(eObject,
							ComponentEditorConstants.CONNECTION_END);
					EObject targetObj = (EObject) _cwkeyObjectMap.get(target);
					if (targetObj
							.eClass()
							.getName()
							.equals(
									ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME)) {
						List csiList = (List) _siCsiMap.get(EcoreUtils
								.getName(srcObj));
						if (csiList == null) {
							csiList = new Vector();
							_siCsiMap.put(EcoreUtils.getName(srcObj), csiList);
						}
						csiList.add(targetObj);
					}
				}
			}
		}
	}
    /**
	 * Adds the nodes to tree on the left side of the dialog
	 * 
	 */
    protected void addPreferenceNodes()
    {
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel((IContainer) _project);

        _viewModel = _nodeProfiles.getViewModel();
        _modelListener = new ModelListener();
        EcoreUtils.addListener(_viewModel.getEList(), _modelListener, -1);
		_dependencyListener = new DependencyListener(ProjectDataModel
				.getProjectDataModel((IContainer) _project), DependencyListener.VIEWMODEL_OBJECT);
		EcoreUtils.addListener(_viewModel.getEList(), _dependencyListener, -1);
		EObject amfObj = _viewModel.getEObject();

		PreferenceNode amfNode = new PreferenceNode("AMFNode",
                new AMFInstructionsPage("AMF Configuration"));
        _preferenceManager.addToRoot(amfNode);
		
		if(_sourceObj != null && amfObj.eClass().getName().equals(_sourceObj.eClass().getName())){
			_selectablePageID = amfNode.getId();
		}
        EReference nodeInstsRef = (EReference) _amfClass
                .getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
        EObject nodeInstancesObj = (EObject) amfObj.eGet(nodeInstsRef);

        updateCompResValues(nodeInstancesObj);
        loadIOCConfiguration(nodeInstancesObj);

        PreferenceNode nodeInstancesPrefNode = new PreferenceNode("Node Instance List",
        		new AMFNodeWizardPage("Node Instance List", pdm, amfObj, this));
        amfNode.add(nodeInstancesPrefNode);
        if(_sourceObj != null && nodeInstancesObj.eClass().getName().equals(_sourceObj.eClass().getName())){
			_selectablePageID = nodeInstancesPrefNode.getId();
		}

        _nodeInstancesPrefNode = nodeInstancesPrefNode;

    	EStructuralFeature nodeFeature = (EStructuralFeature) nodeInstancesObj
    		.eClass().getEStructuralFeature(SafConstants.NODE_INSTANCELIST_NAME);
    	EList nodeList = (EList) nodeInstancesObj.eGet(nodeFeature);
        Iterator nodeItr = nodeList.iterator();
        while(nodeItr.hasNext()) {
        	EObject nodeObj = (EObject) nodeItr.next();
        	String nodeName = (String) EcoreUtils.getValue(nodeObj, "name");
        	PreferenceNode node = new PreferenceNode(nodeName, 
        			new GenericFormPage(nodeName, nodeObj));
        	nodeInstancesPrefNode.add(node);
        	if(_sourceObj != null && nodeObj.eClass().getName().equals(_sourceObj.eClass().getName()) && nodeName.equals(EcoreUtils.getName(_sourceObj))){
    			_selectablePageID = node.getId();
    		}
            PreferenceUtils.createChildTree(node, nodeObj);
            updateSelectablePageID(node);
        }

        EReference serviceGrpsRef = (EReference) _amfClass
            .getEStructuralFeature(SafConstants.SERVICEGROUP_INSTANCES_NAME);
        EObject serviceGrpsObj = (EObject) amfObj.eGet(serviceGrpsRef);

        PreferenceNode serviceGrpsPrefNode = new PreferenceNode("Service Group List",
        		new AMFServiceGroupWizardPage("Service Group List", pdm, amfObj, this));
        amfNode.add(serviceGrpsPrefNode);
        if(_sourceObj != null && serviceGrpsObj.eClass().getName().equals(_sourceObj.eClass().getName())){
			_selectablePageID = serviceGrpsPrefNode.getId();
		}

        _sgInstancesPrefNode = serviceGrpsPrefNode;
              	
        EStructuralFeature SGFeature = (EStructuralFeature) serviceGrpsObj
        	.eClass().getEStructuralFeature(SafConstants.SERVICEGROUP_INSTANCELIST_NAME);
        EList objList = (EList) serviceGrpsObj.eGet(SGFeature);
        Iterator SGItr = objList.iterator();
        while(SGItr.hasNext()) {
        	EObject nodeObj = (EObject) SGItr.next();
        	String nodeName = (String) EcoreUtils.getValue(nodeObj, "name");
        	PreferenceNode node = new PreferenceNode(nodeName, 
        			new GenericFormPage(nodeName, nodeObj));
        	serviceGrpsPrefNode.add(node);
        	if(_sourceObj != null && nodeObj.eClass().getName().equals(_sourceObj.eClass().getName()) && nodeName.equals(EcoreUtils.getName(_sourceObj))){
    			_selectablePageID = node.getId();
    		}
        	PreferenceUtils.createChildTree(node, nodeObj);
        	updateSelectablePageID(node);
        }
        

        PreferenceNode cpmConfigNode = new PreferenceNode("CPMConfigNode",
                new BlankPreferencePage("CPM Configuration"));
        if (ClovisProjectUtils.getCodeGenMode(_project).equals("openclovis")) {
			_preferenceManager.addToRoot(cpmConfigNode);
		}
        EReference cpmConfsRef = (EReference) _amfClass
            .getEStructuralFeature(SafConstants.CPM_CONFIGS_NAME);
        EReference cpmConfRef = (EReference) cpmConfsRef
            .getEReferenceType().getEStructuralFeature(SafConstants.
                    CPM_CONFIGLIST_NAME);
        EObject cpmConfigsObj = (EObject) amfObj.eGet(cpmConfsRef);
        List cpmConfigList = (List) cpmConfigsObj.eGet(cpmConfRef);
        List nodeTypeList = new Vector();
        for (int i = 0; i < _listOfNodes.size(); i++) {
            EObject eobj = (EObject) _listOfNodes.get(i);
            String nodeType = EcoreUtils.getName(eobj);
            nodeTypeList.add(nodeType);
        }
        Iterator iterator = cpmConfigList.iterator();
        while (iterator.hasNext()) {
            EObject cpmConfObj = (EObject) iterator.next();
            String nodeType = (String) EcoreUtils.
                getValue(cpmConfObj, "nodeType");
            // If node with nodeType of cpmConfigObj exists in the 
            //editor, then only proceed, else remove the cpmObj from cmpConfigList.
            if (nodeTypeList.contains(nodeType)) {
                _nodeTypeCpmObjMap.put(nodeType, cpmConfObj);
            } else {
                iterator.remove(); 
            } 
        }
        
        for (int i = 0; i < _listOfNodes.size(); i++) {
            EObject eobj = (EObject) _listOfNodes.get(i);
            String nodeType = EcoreUtils.getName(eobj);
            EEnumLiteral nodeClass = (EEnumLiteral)EcoreUtils.getValue(eobj,
                    ComponentEditorConstants.NODE_CLASS_TYPE);
            
            EObject cpmConfObj = (EObject) _nodeTypeCpmObjMap.get(nodeType);
            
            if (cpmConfObj == null) {
                cpmConfObj = EcoreUtils.createEObject(cpmConfRef.
                        getEReferenceType(), true);
                EcoreUtils.setValue(cpmConfObj, "nodeType", nodeType);
                if( nodeClass.getName().equals("CL_AMS_NODE_CLASS_A") || nodeClass.getName().equals("CL_AMS_NODE_CLASS_B"))
            		EcoreUtils.setValue(cpmConfObj, "cpmType", "GLOBAL");
                else
                	EcoreUtils.setValue(cpmConfObj, "cpmType", "LOCAL");
                
                // Select all the ASP service units by default.
                EReference aspSUsRef = (EReference) cpmConfObj.eClass().
                getEStructuralFeature(SafConstants.ASP_SERVICE_UNITS);
                EObject aspSUsObj = (EObject) cpmConfObj.eGet(aspSUsRef);
                if (aspSUsObj == null) {
                    aspSUsObj = EcoreUtils.createEObject(
                            aspSUsRef.getEReferenceType(), true);
                    cpmConfObj.eSet(aspSUsRef, aspSUsObj);
                }
                EReference aspSuRef = (EReference) aspSUsRef.getEReferenceType().
                    getEStructuralFeature(SafConstants.ASP_SERVICE_UNITS_LIST);
                List aspSuList = (List) aspSUsObj.eGet(aspSuRef);
                List suList = getAspSUs(nodeClass.toString());
                for (int j = 0; j < suList.size(); j++) {
                    EObject su = (EObject) suList.get(j);
                    EObject suObj = EcoreUtils.createEObject(aspSuRef.
                            getEReferenceType(), true);
                    aspSuList.add(suObj);
                    EcoreUtils.setValue(suObj, "name", (String) EcoreUtils.
                            getValue(su, "name"));
                }
                
                cpmConfigList.add(cpmConfObj);
            }
           
            PreferenceNode profileNode = new PreferenceNode("Node" + i,
                    new CpmConfigPage(EcoreUtils.getName(eobj), cpmConfObj));
            cpmConfigNode.add(profileNode);
        }
    }

	/**
	 * Loads the IOC configuration for the nodes.
	 * 
	 * @param nodeInstancesObj
	 */
	private void loadIOCConfiguration(EObject nodeInstancesObj) {
		ProjectDataModel pdm = ProjectDataModel
				.getProjectDataModel((IContainer) _project);
		EObject bootConfigObject = (EObject) pdm.getIOCConfigList().get(0);
		EObject iocObject = (EObject) EcoreUtils.getValue(bootConfigObject,
				SafConstants.IOC);

		EObject iocNodeInstancesObj = (EObject) EcoreUtils.getValue(iocObject,
				SafConstants.NODE_INSTANCES_NAME);
		List iocNodeList = (List) EcoreUtils.getValue(iocNodeInstancesObj,
				SafConstants.NODE_INSTANCELIST_NAME);
		if (iocNodeList == null || iocNodeList.size() == 0)
			return;

		HashMap<String, EObject> nodeMap = new HashMap<String, EObject>();
		Iterator<EObject> itr = iocNodeList.iterator();
		while (itr.hasNext()) {
			EObject nodeObj = itr.next();
			nodeMap.put(EcoreUtils.getName(nodeObj), nodeObj);
		}

		EClass iocConfigurationClass = (EClass) nodeInstancesObj.eClass()
				.getEPackage().getEClassifier(
						SafConstants.IOC_CONFIGURATION_ECLASS);

		List nodeList = (List) EcoreUtils.getValue(nodeInstancesObj,
				SafConstants.NODE_INSTANCELIST_NAME);
		itr = nodeList.iterator();
		while (itr.hasNext()) {
			EObject nodeObj = itr.next();
			EObject iocNodeObj = nodeMap.get(EcoreUtils.getName(nodeObj));

			EObject iocConfigObj = EcoreUtils.createEObject(iocConfigurationClass, true);

			copyEObject((EObject) EcoreUtils.getValue(iocNodeObj,
					SafConstants.SEND_QUEUE), (EObject) EcoreUtils.getValue(
					iocConfigObj, SafConstants.SEND_QUEUE));
			copyEObject((EObject) EcoreUtils.getValue(iocNodeObj,
					SafConstants.RECEIVE_QUEUE), (EObject) EcoreUtils.getValue(
					iocConfigObj, SafConstants.RECEIVE_QUEUE));

			EStructuralFeature iocConfigRef = nodeObj.eClass()
					.getEStructuralFeature(SafConstants.IOC_CONFIGURATION);
			nodeObj.eSet(iocConfigRef, iocConfigObj);
		}
	}

	/**
     * read the xmi file
     * @param nodeClassType 
     *@return the list of asp sus based on the Node Type
     */
    public static List getAspSUs(String nodeClassType)
    {
        try {
            URL url = DataPlugin.getDefault().getBundle().getEntry("/");
            url = Platform.resolve(url);
            String fileName = url.getFile() + "xml" + File.separator
                + "aspSUs.xmi";
            URI uri = URI.createFileURI(fileName);
            List list = EcoreModels.read(uri);
            for (int i = 0; i < list.size(); i++) {
                EObject aspSusObj = (EObject) list.get(i);
                String classType = EcoreUtils.getValue(aspSusObj, "NodeClassType").toString();
                if (classType.equals(nodeClassType)) {
                    List aspSuList = (List) EcoreUtils.getValue(aspSusObj, "aspSu");
                    return aspSuList;
                }
            }
            
        } catch (Exception exception) {
            LOG.error("Error while reading aspSU data file", exception);
        }
        return null;
    }
    /**
     * Updates the Component Instance to Associated Resource 
     * Values after reading from the Resource. 
     * @param nodeInstancesObj - Node Instances Object
     */
    private void updateCompResValues(EObject nodeInstancesObj)
    {
		List nodeInstList = (List) EcoreUtils.getValue(nodeInstancesObj,
                SafConstants.NODE_INSTANCELIST_NAME);
		HashMap compInstMap = new HashMap();
		for (int i = 0; i < nodeInstList.size(); i++) {
			EObject nodeInstObj = (EObject) nodeInstList.get(i);
			EObject serviceUnitInstsObj = (EObject) EcoreUtils.getValue(nodeInstObj,
                    SafConstants.SERVICEUNIT_INSTANCES_NAME);
			if (serviceUnitInstsObj != null) {
				List suInstList = (List) EcoreUtils.getValue(serviceUnitInstsObj,
                        SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
				for (int j = 0; j < suInstList.size(); j++) {
					EObject suInstObj = (EObject) suInstList.get(j);
					EObject compInstsObj = (EObject) EcoreUtils.getValue(suInstObj,
                            SafConstants.COMPONENT_INSTANCES_NAME);
					if (compInstsObj != null) {
						List compInstList = (List) EcoreUtils.getValue(compInstsObj,
                                SafConstants.COMPONENT_INSTANCELIST_NAME);
						for (int k = 0; k < compInstList.size(); k++) {
							EObject compInstObj = (EObject) compInstList.get(k);
							compInstMap.put(EcoreUtils.getName(compInstObj),
									compInstObj);
						}
					}
				}
			}
		}
		for (int i = 0; i < nodeInstList.size(); i++) {
			EObject nodeInstObj = (EObject) nodeInstList.get(i);
			Resource rtResource = getCompResResource(EcoreUtils
					.getName(nodeInstObj), false, ProjectDataModel
					.getProjectDataModel((IContainer) _project));
			if (rtResource != null && rtResource.getContents().size() > 0) {
				EObject compInstancesObj = (EObject) rtResource.
					getContents().get(0);
				List compInstList = (List) EcoreUtils.getValue(compInstancesObj,
						"compInst");
				for (int j = 0; compInstList != null && j < compInstList.size(); j++) {
					EObject compObj = (EObject) compInstList.get(j);
					String compInstName = EcoreUtils.getName(compObj);
					EObject compInstObj = (EObject) compInstMap.
						get(compInstName);
					if (compInstObj != null) {
						BasicEList compResList = (BasicEList) EcoreUtils.
							getValue(compInstObj, SafConstants.RESOURCES_NAME);
						List resList = (List) EcoreUtils.
							getValue(compObj, SafConstants.RESOURCELIST_NAME);
						compResList.clear();
						compResList.grow(resList.size());
						compResList.addAllUnique(resList);
						//resList.clear();
					}
					
				}
			}
		}
	}
	/**
     * @param parent Composite
     * @return Control
     */
    protected Control createContents(Composite parent)
    {
    	//updatePage();
    	Control control = super.createContents(parent);
        List prefNodes = getPreferenceManager().getElements(
        		PreferenceManager.PRE_ORDER);
        for (int i = 0; i < prefNodes.size(); i++)
        {
        	PreferenceNode prefNode = (PreferenceNode) prefNodes.get(i);
        		IPreferencePage prefPage =  prefNode.getPage();
        		if (prefPage instanceof GenericFormPage) {
	        		GenericFormPage page = (GenericFormPage) prefPage;
	        		page.getValidator().setOKButton(
	        				getButton(IDialogConstants.OK_ID));
	        		page.getValidator().setApplyButton(getButton(3457));
	        	}
        		
        }
        control.addHelpListener(new HelpListener(){
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
			}});
        PreferenceUtils.addListenerForTree(this, getTreeViewer());
        if (_selectablePageID != null) {
			getTreeViewer().expandAll();
			getTreeViewer().setSelection(
					new StructuredSelection(findNodeMatching(_selectablePageID)));
		}
        return control;
    }

	/**
     * @param shell -
     *            shell
     */
    protected void configureShell(Shell shell)
    {
        super.configureShell(shell);
        shell.setText(_project.getName() + " - " + _dialogDescription);
        //this.setMinimumPageSize(400, 185);
    }
    /**
     * OK pressed, so save the original model and write the resources to the
     * file
     */
    protected void okPressed()
    {
    	save();
    	ProjectDataModel pdm = ProjectDataModel
		.getProjectDataModel((IContainer) _project);
    	if (pdm.getManageabilityEditorInput() != null) {
			final ManageabilityEditorInput input = (ManageabilityEditorInput) pdm
					.getManageabilityEditorInput();
			if (input.getEditor() != null) {
				BusyIndicator.showWhile(Display.getDefault(), new Runnable(){
					public void run() {
						input.getEditor().refresh();
					}});
				
			}
		}
    	super.okPressed();
    }

    /**
     * save the model
     */
    protected void save() {
		BusyIndicator.showWhile(Display.getCurrent(), new Runnable() {
			public void run() {
				deleteRTResources();
				removeNonASPNodeParam();
				ProjectDataModel pdm = ProjectDataModel
						.getProjectDataModel((IContainer) _project);
				writeCompToResMap();
				_pmConfigModel.save(true);
				_viewModel.save(true);
				storeIOCConfiguration(pdm);
			}
		});
	}

    /**
	 * Remove the non ASP node fields.
	 */
	private void removeNonASPNodeParam() {
		EObject amfObj = _viewModel.getEObject();
		EReference nodeInstsRef = (EReference) amfObj.eClass()
				.getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
		EObject nodeInstsObj = (EObject) amfObj.eGet(nodeInstsRef);

		EReference nodeInstRef = (EReference) nodeInstsObj.eClass()
				.getEStructuralFeature(SafConstants.NODE_INSTANCELIST_NAME);
		Iterator<EObject> nodeItr = ((List<EObject>) nodeInstsObj
				.eGet(nodeInstRef)).iterator();

		EObject nodeObj, nodeType;
		while (nodeItr.hasNext()) {
			nodeObj = nodeItr.next();
			nodeType = getObjectFrmName(EcoreUtils
					.getValue(nodeObj, "type").toString());

			if (!EcoreUtils.getValue(nodeType, "classType").toString().equals(
					"CL_AMS_NODE_CLASS_D")) {
				EcoreUtils.clearValue(nodeObj, "chassisId");
				EcoreUtils.clearValue(nodeObj, "slotId");
			}
		}
	}

    /**
	 * Stores the IOC Configuration.
	 * 
	 * @param pdm
	 *            the ProjectDataModel
	 */
    public static void storeIOCConfiguration(ProjectDataModel pdm) {
		EObject amfObj = (EObject) pdm.getNodeProfiles().getEList().get(0);
		EObject nodeInstsObj = (EObject) EcoreUtils.getValue(amfObj,
				SafConstants.NODE_INSTANCES_NAME);

		if (nodeInstsObj == null)
			return;
		List nodeList = (List) EcoreUtils.getValue(nodeInstsObj,
				SafConstants.NODE_INSTANCELIST_NAME);

		EObject bootConfigObject = (EObject) pdm.getIOCConfigList().get(0);
		EObject iocObject = (EObject) EcoreUtils.getValue(bootConfigObject,
				SafConstants.IOC);

		EObject iocNodeInstancesObj = (EObject) EcoreUtils.getValue(iocObject,
				SafConstants.NODE_INSTANCES_NAME);
		EClass iocNodeInstanceClass = (EClass) iocNodeInstancesObj.eClass()
				.getEPackage().getEClassifier(
						SafConstants.NODE_INSTANCELIST_ECLASS);
		List iocNodeList = (List) EcoreUtils.getValue(iocNodeInstancesObj,
				SafConstants.NODE_INSTANCELIST_NAME);
		iocNodeList.clear();

		Iterator<EObject> itr = nodeList.iterator();
		while (itr.hasNext()) {
			EObject nodeObj = itr.next();

			EObject iocNodeObj = EcoreUtils.createEObject(iocNodeInstanceClass,
					true);
			EcoreUtils
					.setValue(iocNodeObj, "name", EcoreUtils.getName(nodeObj));

			EObject iocConfigObj = (EObject) EcoreUtils.getValue(nodeObj,
					SafConstants.IOC_CONFIGURATION);
			if (iocConfigObj == null) {
				EClass iocConfigurationClass = (EClass) nodeObj.eClass()
						.getEPackage().getEClassifier(
								SafConstants.IOC_CONFIGURATION_ECLASS);
				iocConfigObj = EcoreUtils.createEObject(iocConfigurationClass,
						true);
			}

			copyEObject((EObject) EcoreUtils.getValue(iocConfigObj,
					SafConstants.SEND_QUEUE), (EObject) EcoreUtils.getValue(
					iocNodeObj, SafConstants.SEND_QUEUE));
			copyEObject((EObject) EcoreUtils.getValue(iocConfigObj,
					SafConstants.RECEIVE_QUEUE), (EObject) EcoreUtils.getValue(
					iocNodeObj, SafConstants.RECEIVE_QUEUE));

			iocNodeList.add(iocNodeObj);
		}
		EcoreModels.saveResource(bootConfigObject.eResource());
	}

    /**
	 * Copies the values from srcObj to destObj.
	 * 
	 * @param srcObj
	 *            the Source EObject
	 * @param destObj
	 *            the Destination EObject
	 */
    private static void copyEObject(EObject srcObj, EObject destObj) {
    	Iterator<EAttribute> attrIterator = srcObj.eClass().getEAllAttributes().iterator();
    	while(attrIterator.hasNext()) {
    		EAttribute attr = attrIterator.next();

    		EcoreUtils.setValue(destObj, attr.getName(), EcoreUtils.getValue(
					srcObj, attr.getName()).toString());
    	}

    	List<EReference> srcRefList = srcObj.eClass().getEAllReferences();
    	List<EReference> destRefList = destObj.eClass().getEAllReferences();

    	for(int i=0 ; i<srcRefList.size() ; i++) {
    		EReference srcRef = srcRefList.get(i);
    		EReference destRef = destRefList.get(i);

    		Object srcRefObj = srcObj.eGet(srcRef);
    		Object destRefObj = destObj.eGet(destRef);

    		if(srcRefObj == null || destRefObj == null)		continue;

    		if(srcRefObj instanceof List) {
    			List srcList = (List) srcRefObj;
    			List destList = (List) destRefObj;

    			for(int j=0 ; j<srcRefList.size() ; j++) {
    				copyEObject((EObject) srcList.get(j), (EObject) destList.get(j));
    			}
    			
    		} else {
    			copyEObject((EObject) srcRefObj, (EObject) destRefObj);
    		}
    	}
    }

    /**
     * Delete the RT resources those are not required.
     */
    private void deleteRTResources() {
		Resource amfResource = _nodeProfiles.getResource();
		List amfList = amfResource.getContents();
		EObject resAmfObj = (EObject) amfList.get(0);

		Model model = new Model(resAmfObj.eResource(), resAmfObj);
		EObject amfObjModel = model.getEObject();

		EReference nodeInstsRef = (EReference) _amfClass
				.getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
		EObject nodeInstancesObjModel = (EObject) amfObjModel
				.eGet(nodeInstsRef);

		EStructuralFeature nodeFeature = (EStructuralFeature) nodeInstancesObjModel
				.eClass().getEStructuralFeature(
						SafConstants.NODE_INSTANCELIST_NAME);
		EList nodeList = (EList) nodeInstancesObjModel.eGet(nodeFeature);

		EObject amfObjVM = _viewModel.getEObject();
		EObject nodeInstancesObjVM = (EObject) amfObjVM.eGet(nodeInstsRef);
		EList nodeListVM = (EList) nodeInstancesObjVM.eGet(nodeFeature);

		Iterator nodeItr = nodeList.iterator();
		while (nodeItr.hasNext()) {
			String nodeName = (String) EcoreUtils.getValue((EObject) nodeItr.next(), "name");
			boolean delete = true;

			for (int i = 0; i < nodeListVM.size(); i++) {
				if (nodeName.equals(EcoreUtils.getValue((EObject) nodeListVM
						.get(i), "name"))) {
					delete = false;
					break;
				}
			}

			if (delete) {
				try {
					String dataFilePath = _project.getLocation().toOSString()
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

    /**
     * initialize the node name - corresponding Instances Map
     *
     */
    public void initNodeInstanceMap()
    {
    	_nodeInstanceMap.clear();
    	EObject amfObj = _viewModel.getEObject();
        EReference nodeInstsRef = (EReference) amfObj.eClass()
            .getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
        EReference nodeInstanceRef = (EReference) nodeInstsRef
            .getEReferenceType().getEStructuralFeature(SafConstants.
                    NODE_INSTANCELIST_NAME);
        EObject nodeInstancesObj = (EObject) amfObj.eGet(nodeInstsRef);
        NotifyingList nodeInstList = (NotifyingList) nodeInstancesObj.
                eGet(nodeInstanceRef);
        for (int i = 0; i < nodeInstList.size(); i++) {
            EObject eobj = (EObject) nodeInstList.get(i);
            String nodeType = (String) EcoreUtils.getValue(eobj, "type");
            List instList = (List) _nodeInstanceMap.get(nodeType);
            if (instList == null) {
                instList = new Vector();
                _nodeInstanceMap.put(nodeType, instList);
            }
            if (!instList.contains(eobj)) {
                instList.add(eobj);
            }
        }

    }
    /**
    *
    * Write the Component to Resource File
    */
    public static void writeCompToResMap(ProjectDataModel pdm) {
		EObject amfObj = (EObject) pdm.getNodeProfiles().getEList().get(0);
		EReference nodeInstsRef = (EReference) amfObj.eClass()
				.getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
		EObject nodeInstsObj = (EObject) amfObj.eGet(nodeInstsRef);
		List nodeList = null;
		if (nodeInstsObj != null) {
			EReference nodeInstRef = (EReference) nodeInstsObj.eClass()
					.getEStructuralFeature(SafConstants.NODE_INSTANCELIST_NAME);
			nodeList = (List) nodeInstsObj.eGet(nodeInstRef);
		}

		EClass mapClass = (EClass) pdm.getNodeProfiles().getEPackage()
				.getEClassifier("compInstances");
		EReference compInstRef = (EReference) mapClass
				.getEStructuralFeature("compInst");
		EObject mapCompInstsObj = EcoreUtils.createEObject(mapClass, true);

		Iterator iterator = nodeList.iterator();
		while (iterator.hasNext()) {
			EObject nodeInstObj = (EObject) iterator.next();
			EObject suInstsObj = (EObject) EcoreUtils.getValue(nodeInstObj,
					SafConstants.SERVICEUNIT_INSTANCES_NAME);
			List mapCompInstList = (List) mapCompInstsObj.eGet(compInstRef);
			mapCompInstList.clear();
			if (suInstsObj != null) {
				List suInstList = (List) EcoreUtils.getValue(suInstsObj,
						SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
				for (int j = 0; j < suInstList.size(); j++) {
					EObject suInstObj = (EObject) suInstList.get(j);
					EObject compInstsObj = (EObject) EcoreUtils.getValue(
							suInstObj, SafConstants.COMPONENT_INSTANCES_NAME);
					if (compInstsObj != null) {
						List compInstList = (List) EcoreUtils.getValue(
								compInstsObj,
								SafConstants.COMPONENT_INSTANCELIST_NAME);
						for (int k = 0; k < compInstList.size(); k++) {
							EObject compInstObj = (EObject) compInstList.get(k);
							EObject mapCompObj = EcoreUtils.createEObject(
									compInstRef.getEReferenceType(), true);
							mapCompInstList.add(mapCompObj);
							EcoreUtils.setValue(mapCompObj, "compName",
									(String) EcoreUtils.getValue(compInstObj,
											"name"));
							List compResList = (List) EcoreUtils.getValue(
									compInstObj, SafConstants.RESOURCES_NAME);
							BasicEList mapResList = (BasicEList) EcoreUtils.getValue(
									mapCompObj, SafConstants.RESOURCELIST_NAME);
							mapResList.clear();
							mapResList.grow(compResList.size());
							mapResList.addAllUnique(compResList);
							//compResList.clear();
						}
					}
				}
				Resource rtResource = getCompResResource(EcoreUtils
						.getName(nodeInstObj), true, pdm);
				rtResource.getContents().clear();
				rtResource.getContents().add(mapCompInstsObj);
				EcoreModels.saveResource(rtResource);
			}
		}
	}

    /**
     *
     * @param nodeInstName NodeInstance Name
     * @param toBeCreated - to indicate if whether the file
     * has to be created or not if it does not exist.
     * @return the rt resource corresponding to Node Instance
     */
    public static Resource getCompResResource(String nodeInstName, boolean toBeCreated, ProjectDataModel pdm)
    {
        Resource nodeInstResource = null;
        try {
            // Now get the compRes xml file for each node Instance
            // defined from the config dir under project.
            String dataFilePath = pdm.getProject().getLocation().toOSString()
                + File.separator
                + ICWProject.CW_PROJECT_CONFIG_DIR_NAME
                + File.separator
                + nodeInstName + "_" + SafConstants.RT_SUFFIX_NAME;
            URI uri = URI.createFileURI(dataFilePath);
            File xmlFile = new File(dataFilePath);
            if (toBeCreated) {
            nodeInstResource = xmlFile.exists()
                ? EcoreModels.getUpdatedResource(uri)
                : EcoreModels.create(uri);
            } else {
            	nodeInstResource = xmlFile.exists()
                ? EcoreModels.getUpdatedResource(uri)
                : null;
            }

        } catch (Exception exc) {
            LOG.error("Error Reading CompRes Resource.", exc);
        }
        return nodeInstResource;
    }
    /**
    *
    * @param resourceObj EObject
    * @return the Parent EObject of resourceObj
    */
   public List getParent(EObject resourceObj)
   {
       List parentList = new Vector();
       String cwkey = (String) EcoreUtils.getValue(resourceObj,
    		   ModelConstants.RDN_FEATURE_NAME);
       for (int i = 0; i < _connectionList.size(); i++) {
           EObject compositionObj = (EObject) _connectionList.get(i);
           String targetKey = (String) EcoreUtils.getValue(
                   compositionObj, ComponentEditorConstants.CONNECTION_END);
           if (targetKey.equals(cwkey)) {
               parentList.add((EObject) _cwkeyObjectMap.get(EcoreUtils.
                       getValue(compositionObj, ComponentEditorConstants.CONNECTION_START)));
           }
       }
       return parentList;
   }
   /**
    *
    * @param resourceObj EObject
    * @return the Children of the resourceObj
    */
   public List getChildren(EObject resourceObj) {
		List children = new Vector();
		EObject rootObject = (EObject) _compEditorObjects.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
		for (int k = 0; k < refList.length; k++) {
			EList list = (EList) rootObject.eGet(rootObject.eClass()
					.getEStructuralFeature(refList[k]));
			for (int i = 0; i < list.size(); i++) {
				EObject nodeObj = (EObject) list.get(i);
				List parentList = getParent(nodeObj);
				for (int j = 0; j < parentList.size(); j++) {
					EObject parentObj = (EObject) parentList.get(j);
					if (parentObj != null && parentObj.equals(resourceObj)) {
						children.add(nodeObj);
					}
				}
			}
		}
		return children;
	}
    /**
     *
     * @return the list of nodes
     */
    public List getNodeList()
    {
        return _listOfNodes;
    }
    /**
    *
    * @return the list of Service Groups
    */
   public List getServiceGroupsList()
   {
       return _serviceGrpList;
   }
    /**
     *
     * @return nodeSUMap
     */
    public Map getNodeSuMap()
    {
        return _nodeSuMap;
    }
    /**
    *
    * @return SUCompMap
    */
   public Map getSuCompMap()
   {
       return _suCompMap;
   }
   /**
   *
   * @return SGSiMap
   */
  public Map getSgSiMap()
  {
      return _sgSiMap;
  }
  /**
  *
  * @return SiCsiMap
  */
 public Map getSiCsiMap()
 {
     return _siCsiMap;
 }
   /**
   *
   * @return nodeInstanceMap
   */
  public Map getNodeTypeInstancesMap()
  {
      return _nodeInstanceMap;
  }
   /**
    *
    * @return Resource Editor EObjects
    */
   public List getResourceList()
   {
       return _resourceList;
   }
   /**
   *
   * @return Component EObjects
   */
  public List getComponentsList()
  {
      return _compList;
  }
  /**
   * Goes thru all the Editor Objects to find out the Object with the
   * given name and returns the Object
   * @param name Name of the Object to be fetched
   * @return the Object corresponding to Name
   */
  public EObject getObjectFrmName(String name)
  {	  
	  EObject rootObject = (EObject) _compEditorObjects.get(0);
		String refList[] = ComponentEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[i]);
			EList list = (EList) rootObject.eGet(ref);
			for (int j = 0; j < list.size(); j++) {
				EObject obj = (EObject) list.get(j);
				String objName = EcoreUtils.getName(obj);
				if (objName != null && objName.equals(name)) {
					return obj;
				}
			}
		}
		return null;
  }
  
  	/**
  	 * Add the given node instance object to the object instance tree. The
  	 * node will be added directly under the "Node Instance List" node.
  	 * @param nodeObj
  	 */
  	public void addNodeInstanceToTree(EObject nodeObj)
  	{
		String nodeName = EcoreUtils.getName(nodeObj);
		PreferenceNode node = new PreferenceNode(nodeName, new GenericFormPage(nodeName, nodeObj));
		_nodeInstancesPrefNode.add(node);
		if(_sourceObj != null
			&& nodeObj.eClass().getName().equals(_sourceObj.eClass().getName())
			&& nodeName.equals(EcoreUtils.getName(_sourceObj)))
		{
			_selectablePageID = node.getId();
		}
		PreferenceUtils.createChildTree(node, nodeObj);
		updateSelectablePageID(node);
  	}
  
  	/**
  	 * Add the given service group instance object to the object instance tree.
  	 * The service group will be added directly under the "Service Group List" node.
  	 * @param nodeObj
  	 */
	public void addSGInstanceToTree(EObject sgObj)
	{
		String sgName = EcoreUtils.getName(sgObj);
		PreferenceNode node = new PreferenceNode(sgName, new GenericFormPage(sgName, sgObj));
		_sgInstancesPrefNode.add(node);
		if(_sourceObj != null 
			&& sgObj.eClass().getName().equals(_sourceObj.eClass().getName()) 
			&& sgName.equals(EcoreUtils.getName(_sourceObj)))
		{
			_selectablePageID = node.getId();
		}
		PreferenceUtils.createChildTree(node, sgObj);
		updateSelectablePageID(node);
	}
  
  	/**
  	 * Add the given service unit instance object to the object instance tree. The
  	 * service unit will be added under the "SU Instance List" node that is under
  	 * the node representing the passed in node instance.
  	 * @param nodeInstance
  	 * @param serviceUnitInstance
  	 */
	public void addSUInstanceToTree(EObject nodeInstance, EObject serviceUnitInstance)
  	{
  		// find the node instance node
  		String nodeInstanceName = EcoreUtils.getName(nodeInstance);
  		IPreferenceNode parentNode = _nodeInstancesPrefNode.findSubNode(nodeInstanceName);
  		
  		// get the single node beneath the node instance node
  		//  this should the the "SU Instance List" node
  		IPreferenceNode suListNode = parentNode.getSubNodes()[0];

  		// create the su instance node and add to to the node instance node
  		String serviceUnitInstanceName = EcoreUtils.getName(serviceUnitInstance);
  		PreferenceNode node = new PreferenceNode(serviceUnitInstanceName, new GenericFormPage(serviceUnitInstanceName, serviceUnitInstance));
  		suListNode.add(node);
  		
  		if(_sourceObj != null 
  			&& serviceUnitInstance.eClass().getName().equals(_sourceObj.eClass().getName()) 
  			&& serviceUnitInstanceName.equals(EcoreUtils.getName(_sourceObj)))
  		{
  			_selectablePageID = node.getId();
  		}
  	    PreferenceUtils.createChildTree(node, serviceUnitInstance);
  	    updateSelectablePageID(node);
  	}
  
  /***** This code needs to be removed in future ******/
  protected Control createHelpControl(Composite parent) {
		Image helpImage = JFaceResources.getImage(DLG_IMG_HELP);
		if (helpImage != null) {
			return createHelpImageButton(parent, helpImage);
		}
		return createHelpLink(parent);
  }
	private ToolBar createHelpImageButton(Composite parent, Image image) {
      ToolBar toolBar = new ToolBar(parent, SWT.FLAT | SWT.NO_FOCUS);
      ((GridLayout) parent.getLayout()).numColumns++;
		toolBar.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		final Cursor cursor = new Cursor(parent.getDisplay(), SWT.CURSOR_HAND);
		toolBar.setCursor(cursor);
		toolBar.addDisposeListener(new DisposeListener() {
			public void widgetDisposed(DisposeEvent e) {
				cursor.dispose();
			}
		});		
      ToolItem item = new ToolItem(toolBar, SWT.NONE);
		item.setImage(image);
		item.setToolTipText(JFaceResources.getString("helpToolTip")); //$NON-NLS-1$
		item.addSelectionListener(new SelectionAdapter() {
          public void widgetSelected(SelectionEvent e) {
          	PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
          }
      });
		return toolBar;
	}
	private Link createHelpLink(Composite parent) {
		Link link = new Link(parent, SWT.WRAP | SWT.NO_FOCUS);
      ((GridLayout) parent.getLayout()).numColumns++;
		link.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		link.setText("<a>"+IDialogConstants.HELP_LABEL+"</a>"); //$NON-NLS-1$ //$NON-NLS-2$
		link.setToolTipText(IDialogConstants.HELP_LABEL);
		link.addSelectionListener(new SelectionAdapter() {
          public void widgetSelected(SelectionEvent e) {
          	PlatformUI.getWorkbench().getHelpSystem().displayHelp(_contextHelpId);
          }
      });
		return link;
	}
	/***** This code needs to be removed in future ******/
	
	private void updateSelectablePageID(IPreferenceNode root) {
		IPreferenceNode nodes[] = root.getSubNodes();
		for(int i = 0; i < nodes.length; i++) {
			IPreferenceNode node = nodes[i];
			IPreferencePage page = node.getPage();
			if (page instanceof GenericFormPage) {
				GenericFormPage page1 = (GenericFormPage) node.getPage();
				EObject obj = page1.getEObject();
				if (_sourceObj != null && _sourceObj.eClass().getName()
						.equals(obj.eClass().getName())
						&& EcoreUtils.getName(_sourceObj).equals(
								EcoreUtils.getName(obj))) {
					_selectablePageID = node.getId();
					_selectableNode = node;
					return;
				}
			} else if(page instanceof BlankPreferencePage) {
			}
			updateSelectablePageID(node);
		}
	}
	public void updatePage() {
		if(_selectablePageID != null) {
	    	   super.setSelectedNode(_selectablePageID);
	       }
	}

	/**
	 * Retruns the list of all the resources for the component.
	 * 
	 * @param rtResource
	 * @return
	 */
	public static List<EObject> getCompResListFromRTRes(Resource rtResource) {
		List<EObject> resList = new ArrayList<EObject>();

		if (rtResource != null && rtResource.getContents().size() > 0) {
			EObject compInstancesObj = (EObject) rtResource.getContents()
					.get(0);
			List compInstList = (List) EcoreUtils.getValue(compInstancesObj,
					"compInst");

			for (int j = 0; compInstList != null && j < compInstList.size(); j++) {
				resList.addAll((List) EcoreUtils.getValue(
						(EObject) compInstList.get(j),
						SafConstants.RESOURCELIST_NAME));
			}
		}
		return resList;
	}
	/**
    *
    * Write the Component to Resource File
    */
    public void writeCompToResMap() {
    	Map<String, EObject> createdMoIDResourceMap = new HashMap<String, EObject>();
    	ProjectDataModel pdm = ProjectDataModel.getProjectDataModel((IContainer) _project);
		EObject amfObj = (EObject) _viewModel.getEList().get(0);
		EReference nodeInstsRef = (EReference) amfObj.eClass()
				.getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
		EObject nodeInstsObj = (EObject) amfObj.eGet(nodeInstsRef);
		List nodeList = null;
		if (nodeInstsObj != null) {
			EReference nodeInstRef = (EReference) nodeInstsObj.eClass()
					.getEStructuralFeature(SafConstants.NODE_INSTANCELIST_NAME);
			nodeList = (List) nodeInstsObj.eGet(nodeInstRef);
		}

		EClass mapClass = (EClass) _viewModel.getEPackage()
				.getEClassifier("compInstances");
		EReference compInstRef = (EReference) mapClass
				.getEStructuralFeature("compInst");
		Iterator iterator = nodeList.iterator();
		Map<EObject, EObject> nodeCompMap = new HashMap<EObject, EObject>();
		while (iterator.hasNext()) {
			EObject nodeInstObj = (EObject) iterator.next();
			EObject suInstsObj = (EObject) EcoreUtils.getValue(nodeInstObj,
					SafConstants.SERVICEUNIT_INSTANCES_NAME);
			EObject mapCompInstsObj = EcoreUtils.createEObject(mapClass, true);
			nodeCompMap.put(nodeInstObj, mapCompInstsObj);
			List mapCompInstList = (List) mapCompInstsObj.eGet(compInstRef);
			mapCompInstList.clear();
			if (suInstsObj != null) {
				List suInstList = (List) EcoreUtils.getValue(suInstsObj,
						SafConstants.SERVICEUNIT_INSTANCELIST_NAME);
				for (int j = 0; j < suInstList.size(); j++) {
					EObject suInstObj = (EObject) suInstList.get(j);
					EObject compInstsObj = (EObject) EcoreUtils.getValue(
							suInstObj, SafConstants.COMPONENT_INSTANCES_NAME);
					if (compInstsObj != null) {
						List compInstList = (List) EcoreUtils.getValue(
								compInstsObj,
								SafConstants.COMPONENT_INSTANCELIST_NAME);
						for (int k = 0; k < compInstList.size(); k++) {
							EObject compInstObj = (EObject) compInstList.get(k);
							EObject mapCompObj = EcoreUtils.createEObject(
									compInstRef.getEReferenceType(), true);
							mapCompInstList.add(mapCompObj);
							EcoreUtils.setValue(mapCompObj, "compName",
									(String) EcoreUtils.getValue(compInstObj,
											"name"));
							List compResList = (List) EcoreUtils.getValue(
									compInstObj, SafConstants.RESOURCES_NAME);
							for (int l = 0; l < compResList.size(); l++) {
								EObject resObj = (EObject)compResList.get(l);
								String moID = (String) EcoreUtils.getValue(resObj,"moID");
								EObject obj = createdMoIDResourceMap.get(moID);
								if(obj != null) {
									EcoreUtils.setValue(obj,"primaryOI", "false");
									EcoreUtils.setValue(resObj,"primaryOI", "false");
								} else {
									EcoreUtils.setValue(resObj,"primaryOI", "true");
									createdMoIDResourceMap.put(moID, resObj);
								}
							}
							BasicEList mapResList = (BasicEList) EcoreUtils.getValue(
									mapCompObj, SafConstants.RESOURCELIST_NAME);
							mapResList.clear();
							mapResList.grow(compResList.size());
							mapResList.addAllUnique(compResList);
							//compResList.clear();
						}
					}
				}
			}
		}
		iterator = nodeList.iterator();
		while (iterator.hasNext()) {
			EObject nodeInstObj = (EObject) iterator.next();
			EObject mapCompInstsObj = nodeCompMap.get(nodeInstObj);
			Resource rtResource = getCompResResource(EcoreUtils
					.getName(nodeInstObj), true, pdm);
			rtResource.getContents().clear();
			rtResource.getContents().add(mapCompInstsObj);
			EcoreModels.saveResource(rtResource);
		}
	}
    
    /**
     * 
     * @author Pushparaj
     * listener class to listen for componentInstance additions in the object list
     * and update PM config
     */
    public class ModelListener extends AdapterImpl
    {
    	Map<String, EObject> _pmMap = new HashMap<String, EObject>();
    	EList<EObject> _intervalList;
    	EClass _configClass;
    	EObject _resourceAssociationObj;
    	List _associationList;
    	List<String> _initializedResList;
    	String _chassisName;
    	List _compInstList;
    	Map<String, BasicEList<String>> _createdResourceMoIDsMap= new HashMap<String, BasicEList<String>>();
    	public ModelListener() {
    		 _configClass = (EClass) _pmConfigModel.getEObject().eClass().getEPackage().getEClassifier("CompConfigIntervalType");
    		 populatePMConfig();
    		 ProjectDataModel pdm = ProjectDataModel
				.getProjectDataModel((IContainer) _project);
    		 _resourceAssociationObj = (EObject)pdm.getResourceAssociationModel().getEList().get(0);
    		 _associationList = (EList) EcoreUtils.getValue(_resourceAssociationObj, "association");
    		 if(_associationList.size() > 0 ) {
    			 _initializedResList = AssociateResourceUtils.getInitializedArrayAttrResList(_resourceList);
    			 _chassisName = getChassisName();
    		 }
    		 _compInstList = getComponentInstanceList();
    	}
    	/**
    	 * Returns Chassis Name
    	 * @return
    	 */
    	public String getChassisName() {
    		EObject rootObject = (EObject) _resourceList.get(0);
    		EReference ref = (EReference) rootObject.eClass()
    						.getEStructuralFeature(ClassEditorConstants.CHASSIS_RESOURCE_REF_NAME);
    		EList<EObject> chassisList = (EList) rootObject.eGet(ref);

    		if (chassisList.size() != 1) return "Chassis";

    		EObject eobj = (EObject) chassisList.get(0);
    		return EcoreUtils.getName(eobj);
    	}
    	/**
    	 * Returns the component list
    	 * @return
    	 */
    	public List getComponentInstanceList() {
    		List instList = new ArrayList();
    		List nodeList = new ArrayList();
    		EObject amfObj = (EObject) _viewModel.getEList().get(0);
    		EReference nodeInstsRef = (EReference) amfObj.eClass()
    				.getEStructuralFeature(SafConstants.NODE_INSTANCES_NAME);
    		EObject nodeInstsObj = (EObject) amfObj.eGet(nodeInstsRef);
    		if (nodeInstsObj != null) {
    			EReference nodeInstRef = (EReference) nodeInstsObj.eClass()
    					.getEStructuralFeature(SafConstants.NODE_INSTANCELIST_NAME);
    			nodeList = (List) nodeInstsObj.eGet(nodeInstRef);
    		}
    		for (int i = 0; i < nodeList.size(); i++) {
    			EObject nodeObj = (EObject) nodeList.get(i);
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
    							instList.add(compInstObj);
    						}
    					}
    				}
    			}
    		}
    		return instList;
    	}
    	/**
    	 * Creates map for resource - created moIDs
    	 */
    	private void populateMoIDsList(){
    		_createdResourceMoIDsMap.clear();
			for (int k = 0; k < _compInstList.size(); k++) {
				EObject compInstObj = (EObject) _compInstList.get(k);
				List<EObject> resourceList = (EList) EcoreUtils.getValue(
						compInstObj, "resources");
				for (int l = 0; l < resourceList.size(); l++) {
					EObject resObj = resourceList.get(l);
					String moID = (String) EcoreUtils.getValue(resObj, "moID");
					String resName = getResourceTypeForInstanceID(moID);
					BasicEList<String> list = _createdResourceMoIDsMap
							.get(resName);
					if (list == null) {
						list = new BasicEList<String>();
						_createdResourceMoIDsMap.put(resName, list);
					}
					list.addUnique(moID);
				}
			}
		}
    	/**
    	 * Associated resources for the newly created component
    	 * @param compObj
    	 */
    	private void associateResourcesForComponent(EObject compObj) {
    		String compType = (String) EcoreUtils.getValue(compObj, "type");
    		List compInstObjList = new ArrayList();
			compInstObjList.add(compObj);
    		for(int i = 0; i < _associationList.size(); i++) {
    			EObject assObj = (EObject) _associationList.get(i);
    			List compNames = (List)EcoreUtils.getValue(assObj, "compName");
    			List selectedResNames = (List)EcoreUtils.getValue(assObj, "tableName");
    			List selectedScalarNames  = (List)EcoreUtils.getValue(assObj, "scalarName");
    								  
    			if(compNames.contains(compType)) {
    				String assType = EcoreUtils.getValue(assObj, "type").toString();
    				String value = (String) EcoreUtils.getValue(assObj, "value");
    				if(assType.equals(AssociateResourceConstants.INSTANCE)) {
    					populateMoIDsList();
						AssociateResourceUtils.createOneResourceForComponents(
								compInstObjList, selectedResNames, "\\" + _chassisName + ":0",
								_createdResourceMoIDsMap, _initializedResList);
						AssociateResourceUtils.createScalarResources(compInstObjList,
								selectedScalarNames, "\\" + _chassisName + ":0", _initializedResList);    					
    				} else if(assType.equals(AssociateResourceConstants.SHARED_INSTANCE)) {
    					int instance = Integer.parseInt(value);
    					if(instance != -1) {
    						AssociateResourceUtils
							.createSharedResourceForComponents(
									compInstObjList,
									selectedResNames,
									"\\" + _chassisName + ":0", instance, _initializedResList);
							if (instance == 0) {
								AssociateResourceUtils
									.createScalarSharedResources(
											compInstObjList,
											selectedScalarNames,
											"\\" + _chassisName + ":0", _initializedResList);
							}
    					} else {
    						populateMoIDsList();
    						StringBuffer message = new StringBuffer("Resources '");
							AssociateResourceUtils
							.createSharedResourceForComponents(
									compInstObjList,
									selectedResNames,
									"\\" + _chassisName + ":0", _createdResourceMoIDsMap, _initializedResList, message);
    					}
    				} else if(assType.equals(AssociateResourceConstants.ARRAY_INSTANCE)) {
    					int size = Integer.parseInt(value);
    					populateMoIDsList();
						AssociateResourceUtils
								.createArrayOfResourcesForComponents(
										compInstObjList, selectedResNames,
										"\\" + _chassisName + ":0", size, _createdResourceMoIDsMap, _initializedResList);
						
						AssociateResourceUtils.createScalarResources(
									compInstObjList, selectedScalarNames,
									"\\" + _chassisName + ":0", _initializedResList);
    				} else if (assType
							.equals(AssociateResourceConstants.ARRAY_SHARED_INSTANCE)) {
						String values[] = value.split("-");
						int from = Integer.parseInt(values[0]);
						int to = Integer.parseInt(values[1]);
						AssociateResourceUtils
								.createResourcesForComponentsWithInRange(
										compInstObjList, selectedResNames, "\\"
												+ _chassisName + ":0", from,
										to, _initializedResList);
						if (from == 0) {
							AssociateResourceUtils.createScalarSharedResources(
									compInstObjList, selectedScalarNames, "\\"
											+ _chassisName + ":0",
									_initializedResList);
						}
					}
    			}
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
		 * Creates application config object map
		 */
    	private void populatePMConfig() {
    		EObject pmObj = _pmConfigModel.getEObject();
    		_intervalList = (EList<EObject>)EcoreUtils.getValue(pmObj, "compConfigInterval");
    		for (int i = 0; i < _intervalList.size(); i++) {
    			EObject obj = _intervalList.get(i);
    			_pmMap.put((String)EcoreUtils.getValue(obj, "componentName"), obj);
    		}
    	}
    	/**
    	 * Adds Component in PM config
    	 * @param obj
    	 */
    	private void addComponent(EObject obj) {
			String compName = EcoreUtils.getName(obj);
			String type = (String) EcoreUtils.getValue(obj, "type");
			EObject configObj = EcoreUtils.createEObject(_configClass, true);
			EcoreUtils.setValue(configObj, "componentName", compName);
			EcoreUtils.setValue(configObj, "type", type);
			_intervalList.add(configObj);
			_pmMap.put(compName, configObj);
			if(!type.equals("") && _associationList.size() > 0 ) {
 				_compInstList.add(obj);
 				associateResourcesForComponent(obj);
 			}
		}
    	/**
    	 * Parse SU obj and add Component in PM config
    	 * @param obj
    	 */
		private void addComponentsFromSU(EObject obj) {
			EObject compInstancesObj = (EObject) EcoreUtils.getValue(obj,
					"componentInstances");
			if (compInstancesObj != null) {
				List ciList = (List) EcoreUtils.getValue(compInstancesObj,
						"componentInstance");
				for (int j = 0; j < ciList.size(); j++) {
					addComponent((EObject) ciList.get(j));
				}
			}
		}
		/**
		 * Parse Node obj and add Component in PM config
		 * @param obj
		 */
		private void addComponentsFromNode(EObject obj) {
			EObject suInstancesObj = (EObject) EcoreUtils.getValue(obj,
					"serviceUnitInstances");
			if (suInstancesObj != null) {
				List suList = (EList) EcoreUtils.getValue(suInstancesObj,
						"serviceUnitInstance");
				for (int i = 0; i < suList.size(); i++) {
					EObject suObj = (EObject) suList.get(i);
					addComponentsFromSU(suObj);
				}
			}
		}
		/**
		 * Removes Component fro PM config
		 * @param obj
		 */
		private void removeComponent(EObject obj){
			String compName = EcoreUtils.getName(obj);
 			EObject configObj = _pmMap.get(compName);
 			_intervalList.remove(configObj);
 			_pmMap.remove(compName);
 			_compInstList.remove(obj);
		}
		/**
		 * Parse SU obj and removes Component from PM config
		 * @param obj
		 */
		private void removeComponentsFromSU(EObject obj) {
			EObject compInstancesObj = (EObject) EcoreUtils.getValue(obj,
					"componentInstances");
			if (compInstancesObj != null) {
				List ciList = (List) EcoreUtils.getValue(compInstancesObj,
						"componentInstance");
				for (int j = 0; j < ciList.size(); j++) {
					removeComponent((EObject) ciList.get(j));
				}
			}
		}
		/**
		 * Parse Node obj and removes Component from PM config
		 * @param obj
		 */
		private void removeComponentsFromNode(EObject obj) {
			EObject suInstancesObj = (EObject) EcoreUtils.getValue(obj,
					"serviceUnitInstances");
			if (suInstancesObj != null) {
				List suList = (EList) EcoreUtils.getValue(suInstancesObj,
						"serviceUnitInstance");
				for (int i = 0; i < suList.size(); i++) {
					EObject suObj = (EObject) suList.get(i);
					removeComponentsFromSU(suObj);
				}
			}
		}
		/**
		 * Remove associated service units for SG
		 * @param serviceUnits
		 */
		private void removeAssociatedServiceUnits(List serviceUnits) {
			TreeViewer viewer = getTreeViewer();
			IPreferenceNode[] nodes = getPreferenceManager().getRootSubNodes()[0].getSubNodes()[0].getSubNodes();
			for (int i = 0; i < nodes.length; i++) {
				IPreferenceNode[] subNodes = nodes[i].getSubNodes()[0].getSubNodes();
				for (int j = 0; j < subNodes.length; j++) {
					IPreferenceNode subNode = subNodes[j];
					if(serviceUnits.contains(subNode.getLabelText())) {
						EObject nodeObj = ((GenericFormPage)subNode.getPage()).getEObject();
						EList nodeList = PreferenceUtils.getContainerEList(nodeObj);
						nodeList.remove(nodeObj);
						nodes[i].getSubNodes()[0].remove(subNode);
					}
				}
			}
		}
    	/**
		 * @param msg -
		 *            Notification
		 */
    	public void notifyChanged(Notification notification)
    	{
    		switch (notification.getEventType()) {
            case Notification.REMOVING_ADAPTER:
                break;
                
            case Notification.SET:
                Object object = notification.getNotifier();
                if (!notification.isTouch()
                        && object instanceof EObject) {
                	EObject eobj = (EObject) object;
                	EStructuralFeature feature = (EStructuralFeature) notification.getFeature();
                	if(eobj.eClass().getName().equals("ComponentInstance")) {
                		if (feature.getName().equals(ModelConstants.NAME_FEATURE_NAME)) {
                			String oldName = notification.getOldStringValue();
                			String newName = notification.getNewStringValue();
                			EObject configObj = _pmMap.get(oldName);
                			EcoreUtils.setValue(configObj, "componentName", newName);
                			_pmMap.remove(oldName);
                			_pmMap.put(newName, configObj);
                		} else if(feature.getName().equals("type")) {
                			String oldType = notification.getOldStringValue();
                			String newType = notification.getNewStringValue();
                			EObject configObj = _pmMap.get(EcoreUtils.getName((EObject) notification.getNotifier()));
                			EcoreUtils.setValue(configObj, "type", newType);
                			if (oldType.equals("") && !newType.equals("")
									&& _associationList.size() > 0) {
								_compInstList.add(eobj);
								associateResourcesForComponent(eobj);
							}
                		}
                	}
                }
                break;
            case Notification.ADD:
            	 Object newVal = notification.getNewValue();
            	 if (newVal instanceof EObject) {
            		 EObject newObj = (EObject) newVal;
            		 EcoreUtils.addListener(newObj, _modelListener, -1);
            		 if(newObj.eClass().getName().equals("ComponentInstance")) {
             			addComponent(newObj);
             		 } else if(newObj.eClass().getName().equals("SUInstance")) {
             			addComponentsFromSU(newObj);
             		 } else if(newObj.eClass().getName().equals("NodeInstance")) {
             			addComponentsFromNode(newObj);
             		 }
            	 }
                 break;
            case Notification.ADD_MANY:
	           	 List newVals = (List) notification.getNewValue();
	           	 for (int i = 0; i < newVals.size(); i++) {
	           		 newVal = newVals.get(i);
		           	 if (newVal instanceof EObject) {
		           		EObject newObj = (EObject) newVal;
		           		 EcoreUtils.addListener(newObj, _modelListener, -1);
		           		 if(newObj.eClass().getName().equals("ComponentInstance")) {
		           			 addComponent(newObj);
		           		 } else if(newObj.eClass().getName().equals("SUInstance")) {
		             		 addComponentsFromSU(newObj);
		             	 } else if(newObj.eClass().getName().equals("NodeInstance")) {
		             		 addComponentsFromNode(newObj);
		             	 }
		           	 }
	           	 }
                break;
            case Notification.REMOVE:
                Object oldVal = notification.getOldValue();
                if (oldVal instanceof EObject) {
                    EObject eobj = (EObject) oldVal;
                    EcoreUtils.removeListener(eobj, _modelListener, -1);
        			if(eobj.eClass().getName().equals("ComponentInstance")) {
             			removeComponent((EObject)eobj);
             		} else if(eobj.eClass().getName().equals("SUInstance")) {
             			removeComponentsFromSU(eobj);
             		} else if(eobj.eClass().getName().equals("NodeInstance")) {
             			removeComponentsFromNode(eobj);
             		} else if(eobj.eClass().getName().equals("ServiceGroupInstance")) {
             			EObject serviceUnitsObj = (EObject)EcoreUtils.getValue(eobj, SafConstants.ASSOCIATED_SERVICEUNITS_NAME);
             			List serviceUnits = (List) EcoreUtils.getValue(serviceUnitsObj, SafConstants.ASSOCIATED_SERVICEUNIT_LIST);
             			if(serviceUnits.size() > 0 && MessageDialog.openQuestion(getShell(), "", "Do you want to remove Service Units which are associated with " + EcoreUtils.getName(eobj)+ " ?")) {
             				removeAssociatedServiceUnits(serviceUnits);
             			}
             		}
        		}
                break;

            case Notification.REMOVE_MANY:
                List objs = (List) notification.getOldValue();
                for (int i = 0; i < objs.size(); i++) {
                    if (objs.get(i) instanceof EObject) {
                        EObject eobj = (EObject) objs.get(i);
                        EcoreUtils.removeListener(eobj, _modelListener, -1);
                        if(eobj.eClass().getName().equals("ComponentInstance")) {
		           			 removeComponent(eobj);
		           		 } else if(eobj.eClass().getName().equals("SUInstance")) {
		             		 removeComponentsFromSU(eobj);
		             	 } else if(eobj.eClass().getName().equals("NodeInstance")) {
		             		 removeComponentsFromNode(eobj);
		             	 } else if(eobj.eClass().getName().equals("ServiceGroupInstance")) {
		             		EObject serviceUnitsObj = (EObject)EcoreUtils.getValue(eobj, SafConstants.ASSOCIATED_SERVICEUNITS_NAME);
		             		List serviceUnits = (List) EcoreUtils.getValue(serviceUnitsObj, SafConstants.ASSOCIATED_SERVICEUNIT_LIST);
		             		if(serviceUnits.size() > 0 && MessageDialog.openQuestion(getShell(), "", "Do you want to remove Service Units which are associated with " + EcoreUtils.getName(eobj)+ " ?")) {
		             			removeAssociatedServiceUnits(serviceUnits);
		             		}
		             	}
                    }
                }
                break;
           }
    		
    	}
    	
    }
}