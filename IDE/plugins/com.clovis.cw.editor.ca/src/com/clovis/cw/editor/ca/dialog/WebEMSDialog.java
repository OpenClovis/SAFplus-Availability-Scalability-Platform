/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/WebEMSDialog.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IResource;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.ecore.ClovisNotifyingListImpl;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.project.data.ProjectDataModel;

/**
 * @author abhay
 * 
 * Web EMS configuration Dialog
 */
public class WebEMSDialog extends PreferenceDialog {
	private PreferenceManager _preferenceManager;

	private Model _configViewModel;

	private Model _layoutViewModel;
	
	private List _categoriesViewModelList = new Vector();

	private String _dialogDescription = "Web EMS Configurations";

	private EClass _clusterConfigClass = null;

	private EClass _clusterLayoutClass = null;

	private ClusterConfigReader _configReader = null;

	private ClusterLayoutReader _layoutReader = null;

	// private ResourceAttrCategoryReader[] _resourceReaders = null;

	private List _resourceList;

	private IResource _project;
    
    private HashMap viewModelResMap = new HashMap(); 

	private static WebEMSDialog instance = null;

	/**
	 * Gets current instance of Dialog
	 * 
	 * @return current instance of Dialog
	 */
	public static WebEMSDialog getInstance() {
		return instance;
	}

	/**
	 * Close the dialog. Remove static instance
	 * 
	 * @return super.close()
	 */
	public boolean close() {
		if (_configViewModel != null) {
			_configViewModel.dispose();
			_configViewModel = null;
		}
		if (_layoutViewModel != null) {
			_layoutViewModel.dispose();
			_layoutViewModel = null;
		}
		
		for(int i=0; i<_categoriesViewModelList.size(); i++)
		{
			Model categoriesViewModel = (Model)_categoriesViewModelList.get(i);
			if( categoriesViewModel != null )
			{
				categoriesViewModel.dispose();
				categoriesViewModel = null;
			}
		}
		instance = null;
		return super.close();
	}

	/**
	 * Open the dialog. Set static instance to itself
	 * 
	 * @return super.open()
	 */
	public int open() {
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
	public WebEMSDialog(Shell parentShell, PreferenceManager pManager,
			IResource resource) {

		super(parentShell, pManager);
		_preferenceManager = pManager;
		_project = resource;
		ProjectDataModel pdm = ProjectDataModel
				.getProjectDataModel((IContainer) _project);

        _resourceList = pdm.getCAModel().getEList();

		_configReader = new ClusterConfigReader(_project);
		_clusterConfigClass = _configReader.getClusterConfigClass();

		_layoutReader = new ClusterLayoutReader(_project);
		_clusterLayoutClass = _layoutReader.getClusterLayoutClass();

		addPreferenceNodes();
	}

	/**
	 * Adds the nodes to tree on the left side of the dialog
	 * 
	 */
	private void addPreferenceNodes() {
		PreferenceNode clusterNode = new PreferenceNode("Cluster",
				new BlankPreferencePage("Cluster Configuration"));
		_preferenceManager.addToRoot(clusterNode);

		_clusterConfigClass = _configReader.getClusterConfigClass();

		Resource configResource = _configReader.getClusterConfigResource();
		List configList = configResource.getContents();
		Model configModel = new Model(configResource,
				(NotifyingList) configList, _clusterConfigClass.getEPackage());
		_configViewModel = configModel.getViewModel();

		clusterNode.add(new PreferenceNode("Config", new ClusterConfigPage(
				"Config", (EObject) _configViewModel.getEList().get(0))));

		_clusterLayoutClass = _layoutReader.getClusterLayoutClass();

		Resource layoutResource = _layoutReader.getClusterLayoutResource();
		List layoutList = layoutResource.getContents();
		Model layoutModel = new Model(layoutResource,
				(NotifyingList) layoutList, _clusterLayoutClass.getEPackage());
		_layoutViewModel = layoutModel.getViewModel();

		clusterNode.add(new PreferenceNode("Layout", new ClusterLayoutPage(
				"Layout", (EObject) _layoutViewModel.getEList().get(0))));

		PreferenceNode resourcesNode = new PreferenceNode("Resources",
				new BlankPreferencePage("Resource Attribute Category Configuration"));
		_preferenceManager.addToRoot(resourcesNode);
		
		EObject rootObject = (EObject) _resourceList.get(0);
		String refList[] = ClassEditorConstants.NODES_REF_TYPES;
		for (int i = 0; i < refList.length; i++) {
			List list = (EList) rootObject.eGet(rootObject.eClass()
					.getEStructuralFeature(refList[i]));
			for (int k = 0; k < list.size(); k++) {
				String resourceName = (String) EcoreUtils
						.getName((EObject) list.get(k));
				if (resourceName == null || resourceName.equals("Chassis"))
					continue;
				ResourceAttrCategoryReader categoriesReader = new ResourceAttrCategoryReader(
						_project, resourceName);
				Resource categoriesResource = categoriesReader
						.getCategoriesResource();
				EClass categoriesClass = categoriesReader.getCategoriesClass();
				List categoriesList = categoriesResource.getContents();

				Model categoriesModel = new Model(categoriesResource,
						(NotifyingList) categoriesList, categoriesClass
								.getEPackage());
				Model categoriesViewModel = categoriesModel.getViewModel();
				viewModelResMap.put(categoriesViewModel, resourceName);
				_categoriesViewModelList.add(categoriesViewModel);

				EReference categoryInstRef = (EReference) categoriesClass
						.getEStructuralFeature("CATEGORY");
				NotifyingList categoryInstObj = (NotifyingList) ((EObject) categoriesViewModel
						.getEList().get(0)).eGet(categoryInstRef);
				// if list of categories is empty, then add a
				// default category with all the attributes inside it
				if (categoryInstObj.isEmpty()) {
					EObject defaultCatObj = EcoreUtils.createEObject(
							categoryInstRef.getEReferenceType(), true);
					categoryInstObj.add(defaultCatObj);
					EcoreUtils.setValue(defaultCatObj, "NAME", "default");
					EReference attrRef = (EReference) defaultCatObj.eClass()
							.getEStructuralFeature("ATTRIBUTE");
					List catAttrList = (List) defaultCatObj.eGet(attrRef);
					List attrList = getAllAttributesOfResource(resourceName,
							_resourceList);
					for (int j = 0; j < attrList.size(); j++) {
						EObject attribute = (EObject) attrList.get(j);
						EObject catAttrObj = EcoreUtils.createEObject(attrRef
								.getEReferenceType(), true);
						catAttrList.add(catAttrObj);
						String attrType = EcoreUtils.getValue(attribute,
								ClassEditorConstants.ATTRIBUTE_TYPE).toString();
						int multiplicity = Integer
								.parseInt(EcoreUtils
										.getValue(
												attribute,
												ClassEditorConstants.ATTRIBUTE_MULTIPLICITY)
										.toString());
						EcoreUtils.setValue(catAttrObj, "NAME", EcoreUtils
								.getName(attribute));
						EcoreUtils.setValue(catAttrObj, "TYPE", getAttrType(
								attrType, multiplicity));
					}

				}
				resourcesNode.add(new PreferenceNode(resourceName,
						new GenericTablePage(resourceName, categoryInstRef
								.getEReferenceType(), categoryInstObj)));
			}
		}

	}

	/**
	 * @param parent
	 *            Composite
	 * @return Control
	 */
	protected Control createContents(Composite parent) {
		Control control = super.createContents(parent);
        PreferenceNode configNode = (PreferenceNode) findNodeMatching(
            "Config");
        if (configNode != null) {
            ClusterConfigPage page = (ClusterConfigPage) configNode.getPage();
            page.getValidator().setOKButton(getButton(IDialogConstants.OK_ID));
        }
		return control;
	}

	/**
	 * @param shell -
	 *            shell
	 */
	protected void configureShell(Shell shell) {
		super.configureShell(shell);
		shell.setText(_project.getName() + " - " + _dialogDescription);
		// this.setMinimumPageSize(400, 185);
	}

	/**
	 * OK pressed, so save the original model and write the resources to the
	 * file
	 */
	protected void okPressed() {
		_configViewModel.save(true);
		_layoutViewModel.save(true);

		for(int i=0; i<_categoriesViewModelList.size(); i++)
		{
			Model categoriesViewModel = (Model)_categoriesViewModelList.get(i);
            String resName = (String) viewModelResMap.get(categoriesViewModel);
            List attrList = getAllAttributesOfResource(resName, _resourceList);
            List categoryList = categoriesViewModel.getEList();
            modifyCategoryListForNonSelectedAttributes(attrList, categoryList);
			if( categoriesViewModel != null )
			{
				categoriesViewModel.save(true);		
			}
		}		
		super.okPressed();
	}
	/**
     * 
     * @param attrList - Attribute List
     * @param categoryList - All Category List
	 */
	private void modifyCategoryListForNonSelectedAttributes(List attrList,
            List categoryObjectList)
    {
        HashMap attrNameTypeMap = new HashMap();
        List nonSelAttrNames = new Vector();
        for (int i = 0; i < attrList.size(); i++) {
          EObject attrObj = (EObject) attrList.get(i);
          nonSelAttrNames.add(EcoreUtils.getName(attrObj));
          String attrType = EcoreUtils.getValue(attrObj, 
                  ClassEditorConstants.ATTRIBUTE_TYPE).toString();
          int multiplicity = Integer.parseInt(EcoreUtils.getValue(attrObj,
                  ClassEditorConstants.ATTRIBUTE_MULTIPLICITY).toString());
          attrNameTypeMap.put(EcoreUtils.getName(attrObj),
                  getAttrType(attrType, multiplicity));
        }
        if (!categoryObjectList.isEmpty()) {
            EObject categoriesObj = (EObject) categoryObjectList.get(0);
            EReference catRef = (EReference) categoriesObj.eClass().
                getEStructuralFeature("CATEGORY");
            List categoryList = (List) categoriesObj.eGet(catRef);
            Iterator iterator = categoryList.iterator();
            while (iterator.hasNext()) {
                EObject categoryObj = (EObject) iterator.next();
                List catAttrList = (List) EcoreUtils.getValue(categoryObj, "ATTRIBUTE");
                for (int j = 0; j < catAttrList.size(); j++) {
                    EObject catAttrObj = (EObject) catAttrList.get(j);
                    nonSelAttrNames.remove(EcoreUtils.getName(catAttrObj));
                    
                }
            }
            if (!nonSelAttrNames.isEmpty()) {
                EObject defaultCatObj = EcoreUtils.createEObject(catRef.
                        getEReferenceType(), true);
                categoryList.add(defaultCatObj);
                EcoreUtils.setValue(defaultCatObj, "NAME" , "default");
                EReference attrRef = (EReference) defaultCatObj.eClass().
                    getEStructuralFeature("ATTRIBUTE");
                List catAttrList = (List) defaultCatObj.eGet(attrRef);
                for (int i = 0; i < nonSelAttrNames.size(); i++) {
                    String attrName = (String) nonSelAttrNames.get(i);
                    EObject attrObj = EcoreUtils.createEObject(
                            attrRef.getEReferenceType(), true);
                    catAttrList.add(attrObj);
                    EcoreUtils.setValue(attrObj, "NAME", attrName);
                    if (attrNameTypeMap.get(attrName) != null) {
                    EcoreUtils.setValue(attrObj, "TYPE", (String)
                            attrNameTypeMap.get(attrName));
                    }
                }
            }
        }
        
    }

     /**
       * 
       * @param resName - Resource name whose attrobutes to fetched
       * @param resList - List of all resources
       * @return all the attributes of the specified resource
       */
      public static List getAllAttributesOfResource(String resName, List resList) {
		List attrList = new ClovisNotifyingListImpl();
		EObject rootObject = (EObject) resList.get(0);
		String refList[] = ClassEditorConstants.NODES_REF_TYPES;
		for (int j = 0; j < refList.length; j++) {
			EReference ref = (EReference) rootObject.eClass()
					.getEStructuralFeature(refList[j]);
			List list = (EList) rootObject.eGet(ref);
			for (int i = 0; i < list.size(); i++) {
				EObject obj = (EObject) list.get(i);
				String name = EcoreUtils.getName(obj);
				if (name != null && name.equals(resName)) {
					List attributes = (List) EcoreUtils.getValue(obj,
							ClassEditorConstants.CLASS_ATTRIBUTES);
					attrList.addAll(attributes);
					EObject provObj = (EObject) EcoreUtils.getValue(obj,
							ClassEditorConstants.RESOURCE_PROVISIONING);
					if (provObj != null) {
						List provAttributes = (List) EcoreUtils.getValue(
								provObj, ClassEditorConstants.CLASS_ATTRIBUTES);
						attrList.addAll(provAttributes);
					}
				}

			}
		}
		return attrList;
	}
      /**
		 * 
		 * @param type
		 *            -AttrType
		 * @param multiplicity -
		 *            Attr multiplicity
		 * @return the ARRAY or type
		 */
      public static String getAttrType(String type, int multiplicity)
      {   
          if( type.equals("OCTET STRING") || type.equals("DisplayString") || type.equals("UTF8String")
                  || type.equals("Opaque") || type.equals("IA5String") || type.equals("IpAddress") || type.equals("NetworkAddress"))
              return "STRING";    

          if( multiplicity > 1 )
          {
              return "ARRAY";
          }
          else
              return type;
      }
	/**
	 * 
	 * @return All the resource objects
	 */
	public List getResourceList()
	{
		return _resourceList;
	}
	/**
     * return the selected preference node name 
	 */
	public String getSelectedNodePreference()
	{
		return super.getSelectedNodePreference();
	}
	
}
