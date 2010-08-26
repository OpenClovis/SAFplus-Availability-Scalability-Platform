/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/ClassPropertiesDialog.java $
 * $Author: pushparaj $
 * $Date: 2007/05/17 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.manageability.ui;

import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IProject;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.preference.IPreferenceNode;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Link;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.ToolBar;
import org.eclipse.swt.widgets.ToolItem;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.dialog.AssociateAlarmsPage;
import com.clovis.cw.editor.ca.dialog.AttributesPage;
import com.clovis.cw.editor.ca.dialog.ClassPropertiesPage;
import com.clovis.cw.editor.ca.dialog.DataStructurePropertiesPage;
import com.clovis.cw.editor.ca.dialog.GenericTablePage;
import com.clovis.cw.editor.ca.dialog.ServicePropertiesPage;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.data.SubModelMapReader;
/**
 * @author shubhada
 * Dialog to show class properties.
 */
public class ResourcePropertiesDialog extends PreferenceDialog
    implements ClassEditorConstants
{
	private ManageabilityEditor _editor;
	private String            _dialogDescription = " Details";
    private PreferenceManager _preferenceManager;
    private EObject           _classObject;
    private Model             _viewModel;
    private Model             _alarmModel;
    private Model			  _alarmRuleViewModel;
    private Model              _alarmLinkViewModel;
    private List              _resourceList;
    private IProject          _project;
    private EObject 		  _alarmRuleResObj = null;
    private String 			  _contextHelpId = null;
    /**
     *
     * @param parentShell parent shell
     * @param pManager preference manager
     * @param viewmodel ViewModel
     * @param alarms Alarms Profiles List
     */
    public ResourcePropertiesDialog(Shell parentShell,
            PreferenceManager pManager, EObject selObj, List resourceList, IProject project, ManageabilityEditor editor)
    {
        super(parentShell, pManager);
        _editor = editor;
        _preferenceManager = pManager;
        _project = project;
        ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(project);
        _alarmModel = pdm.getAlarmProfiles();
              
		_resourceList = resourceList;
        _viewModel = new Model(null, selObj).getViewModel();
        
        _classObject = _viewModel.getEObject();
        _contextHelpId     = EcoreUtils.getAnnotationVal(_classObject.eClass(), null, "Help");
        addPreferenceNodes();
    }
    
    /**
     * @param shell shell
     */
    protected void configureShell(Shell shell)
    {
        super.configureShell(shell);
        shell.setText(EcoreUtils.getLabel(_classObject.eClass()) + _dialogDescription);
        this.setMinimumPageSize(400, 185);
    }

    /**
	 * @param parent Composite
	 * @return Control
	 */
	protected Control createContents(Composite parent) {
		Control control = super.createContents(parent);
		PreferenceNode attrNode = (PreferenceNode) findNodeMatching("Attributes");
		if (attrNode != null) {
			if (attrNode.getPage() instanceof AttributesPage) {
				AttributesPage page = (AttributesPage) attrNode.getPage();
				page.getValidator().setOKButton(
						getButton(IDialogConstants.OK_ID));
			} else {
				GenericTablePage page = (GenericTablePage) attrNode.getPage();
				page.getValidator().setOKButton(
						getButton(IDialogConstants.OK_ID));
			}
		}
		PreferenceNode provAttrNode = (PreferenceNode) findNodeMatching("provAttributes");
		if (provAttrNode != null) {
			AttributesPage page = (AttributesPage) provAttrNode.getPage();
			page.getValidator().setOKButton(getButton(IDialogConstants.OK_ID));
		}
		PreferenceNode pmAttrNode = (PreferenceNode) findNodeMatching("pmAttributes");
		if (pmAttrNode != null) {
			AttributesPage page = (AttributesPage) provAttrNode.getPage();
			page.getValidator().setOKButton(getButton(IDialogConstants.OK_ID));
		}
		PreferenceNode operNode = (PreferenceNode) findNodeMatching("Operations");
		if (operNode != null) {
			GenericTablePage page = (GenericTablePage) operNode.getPage();
			page.getValidator().setOKButton(getButton(IDialogConstants.OK_ID));
		}
		PreferenceNode resourceNode = (PreferenceNode) findNodeMatching("resource");
		if (resourceNode != null) {
			ClassPropertiesPage page = (ClassPropertiesPage) resourceNode
					.getPage();
			page.getValidator().setShell(parent.getShell());
			page.getValidator().setOKButton(getButton(IDialogConstants.OK_ID));
		}
		PreferenceNode dsNode = (PreferenceNode) findNodeMatching("datastructure");
		if (dsNode != null) {
			DataStructurePropertiesPage page = (DataStructurePropertiesPage) dsNode
					.getPage();
			page.getValidator().setOKButton(getButton(IDialogConstants.OK_ID));
		}
		/*PreferenceNode alarmNode = (PreferenceNode) findNodeMatching(RESOURCE_ALARM);
		if (alarmNode != null) {
			ServicePropertiesPage page = (ServicePropertiesPage) alarmNode
					.getPage();
			page.getValidator().setOKButton(getButton(IDialogConstants.OK_ID));
		}*/
		if (_contextHelpId != null) {
			control.addHelpListener(new HelpListener() {
				public void helpRequested(HelpEvent e) {
					PlatformUI.getWorkbench().getHelpSystem().displayHelp(
							_contextHelpId);
				}
			});
		}
		return control;
	}

	/**
	 * add preference nodes in to preference manager
	 */
    private void addPreferenceNodes()
    {
        EClass eclass = _classObject.eClass();
        EReference atRef = (EReference) eclass
                .getEStructuralFeature(CLASS_ATTRIBUTES);

        if (eclass.getName().equals(DATA_STRUCTURE_NAME)) {
            _preferenceManager.addToRoot(new PreferenceNode("datastructure",
                new DataStructurePropertiesPage("Data Structure",
                            _classObject)));
            GenericTablePage attrPage = new GenericTablePage(
                    "Attributes", atRef.getEReferenceType(), (NotifyingList)
                    _classObject.eGet(atRef));
            _preferenceManager.addToRoot(new
                    PreferenceNode("Attributes", attrPage));

        } else {
            ClassPropertiesPage resourcePage = new
                    ClassPropertiesPage("Resource", _classObject);
            _preferenceManager.addToRoot(new PreferenceNode("resource",
                    resourcePage));
            resourcePage.setContainer(this);
            
            if (!eclass.getName().equals("MibResource")) {
				AttributesPage attrPage = new AttributesPage("Attributes",
						atRef.getEReferenceType(), (NotifyingList) _classObject
								.eGet(atRef));
				_preferenceManager.addToRoot(new PreferenceNode("Attributes",
						attrPage));
			}

            // Software and Chassis Resource will not
            //have associated Device Objects
            if (!(eclass.getName().equals("ChassisResource")
                    || eclass.getName().equals("SoftwareResource")
                    || eclass.getName().equals("MibResource"))) {

                EReference dosRef = (EReference)
                eclass.getEStructuralFeature(DEVICE_OBJECT);

                /*GenericTablePage doPage = new GenericTablePage(
                        "Device Objects", dosRef.getEReferenceType(),
                        (NotifyingList) _classObject.eGet(dosRef));
                _preferenceManager.addToRoot(new
                        PreferenceNode("Device Objects", doPage));*/
            }

            EReference serRef = (EReference)
                eclass.getEStructuralFeature(RESOURCE_PROVISIONING);
            addProvServicePage(serRef);

            serRef = (EReference) eclass.getEStructuralFeature("pm");
			addPMServicePage(serRef);

            serRef = (EReference) eclass.getEStructuralFeature(
                  RESOURCE_ALARM);
            addAlarmServicePage(serRef);
        }
    }
    /**
     * @param serRef EReference for service
     */
    private void addProvServicePage(EReference serRef)
    {
        EClass eclass = _classObject.eClass();
        EObject serObj = (EObject) _classObject.eGet(serRef);
        EClass serClass = serRef.getEReferenceType();
        if (serObj == null) {
            serObj = EcoreUtils.createEObject(serClass, true);
            _classObject.eSet(_classObject.eClass().getEStructuralFeature(
                    RESOURCE_PROVISIONING), serObj);
        }
        EReference atRef = (EReference) serClass
                .getEStructuralFeature(CLASS_ATTRIBUTES);
        //EReference opRef = (EReference) serClass
                //.getEStructuralFeature(CLASS_METHODS);
        /*EReference associatedDORef = (EReference) serClass
        .getEStructuralFeature(ASSOCIATED_DO);
*/


        PreferenceNode provNode = new PreferenceNode(RESOURCE_PROVISIONING,
                new ServicePropertiesPage(RESOURCE_PROVISIONING));
        _preferenceManager.addToRoot(provNode);
        provNode.add(new PreferenceNode("provAttributes",
            new AttributesPage("Attributes",
               atRef.getEReferenceType(), (NotifyingList) serObj.eGet(atRef))));
        
        /* commenting out Operations page. This is not in use 
        provNode.add(new PreferenceNode("Operations",
            new GenericTablePage("Operations",
               opRef.getEReferenceType(), (NotifyingList) serObj.eGet(opRef))));
        */

        //Software and Chassis Resource will not have associated Device Objects
        /*if (!(eclass.getName().equals("ChassisResource")
                || eclass.getName().equals("SoftwareResource")
                || eclass.getName().equals("MibResource"))) {
                 provNode.add(new PreferenceNode("Associate Device Objects",
                    new GenericTablePage("Associated Device Objects",
                    associatedDORef.getEReferenceType(), _classObject,
                    (NotifyingList) serObj.eGet(associatedDORef))));
        }*/
    }

    /**
	 * Adds pm service page.
	 * 
	 * @param serRef
	 *            EReference for service
	 */
	private void addPMServicePage(EReference serRef) {
		EClass eclass = _classObject.eClass();
		EObject serObj = (EObject) _classObject.eGet(serRef);
		EClass serClass = serRef.getEReferenceType();

		if (serObj == null) {
			serObj = EcoreUtils.createEObject(serClass, true);
			_classObject.eSet(
					_classObject.eClass().getEStructuralFeature("pm"), serObj);
		}
		EReference atRef = (EReference) serClass
				.getEStructuralFeature(CLASS_ATTRIBUTES);

		PreferenceNode pmNode = new PreferenceNode("pm",
				new ServicePropertiesPage("pm"));
		_preferenceManager.addToRoot(pmNode);

		pmNode.add(new PreferenceNode("pmAttributes", new AttributesPage(
				"Attributes", atRef.getEReferenceType(), (NotifyingList) serObj
						.eGet(atRef))));
	}

    /**
	 * Add alarm service page
	 * 
	 * @param serRef
	 *            EReference for alarm service
	 */
    private void addAlarmServicePage(EReference serRef)
    {
        EClass eclass = _classObject.eClass();
        EObject serObj = (EObject) _classObject.eGet(serRef);
        EClass serClass = serRef.getEReferenceType();
        /*EReference associatedDORef = (EReference) serClass
        .getEStructuralFeature(ASSOCIATED_DO);*/

        if (serObj == null) {
            serObj = EcoreUtils.createEObject(serClass, true);
            _classObject.eSet(_classObject.eClass().getEStructuralFeature(
                    RESOURCE_ALARM), serObj);
        }

        PreferenceNode alarmNode = new PreferenceNode(RESOURCE_ALARM,
                new ServicePropertiesPage(RESOURCE_ALARM));
        
        EObject mapObj = _editor.getLinkViewModel().getEObject();
        EObject linkObj = SubModelMapReader.getLinkObject(mapObj,
        		ClassEditorConstants.ASSOCIATED_ALARM_LINK);
        String rdn = (String) EcoreUtils.getValue(_classObject, ModelConstants.RDN_FEATURE_NAME);
        List associatedAlarmsList = (NotifyingList) SubModelMapReader.
    		getLinkTargetObjects(linkObj, rdn);
        if (associatedAlarmsList == null) {
        	SubModelMapReader.createLinkTargets(
    			linkObj, EcoreUtils.getName(_classObject), rdn);
        }
        
        EObject alarmRuleInfoObj = (EObject) _editor.getAlarmRuleViewModel().getEList().get(0);
        _alarmRuleViewModel = new Model(_editor.getAlarmRuleViewModel().getResource(), alarmRuleInfoObj).getViewModel();
        List alarmRulesResList = (EList) EcoreUtils.getValue(alarmRuleInfoObj, 
        		ClassEditorConstants.ALARM_RESOURCE);
        
        String resName	=	EcoreUtils.getName(_classObject);
        
                
        for(int i = 0; 	null != alarmRulesResList &&
        			   	i <  alarmRulesResList.size(); i++){
        	
        	_alarmRuleResObj = (EObject)alarmRulesResList.get(i);
        	
        	String resourceName = (String) EcoreUtils.getValue(_alarmRuleResObj,
        			ClassEditorConstants.ALARM_RESOURCE_NAME);
        	
        	if(resName.equals(resourceName)){
        		
        		break;
        	}
        		
        	_alarmRuleResObj = null;
        }
        
        if(null == _alarmRuleResObj){
  		   initAlarmRuleRes();
  	   }
                
        _alarmLinkViewModel = new Model(_editor.getLinkViewModel().getResource(), linkObj).getViewModel();
        
        AssociateAlarmsPage alarmsPage = new AssociateAlarmsPage(
                _alarmModel, _alarmRuleViewModel, _editor.getLinkViewModel(), _alarmRuleResObj, 
                _project, _classObject, _alarmLinkViewModel.getEObject(), _resourceList,
                "Associate Alarms");
        alarmNode.add(new PreferenceNode("attributes", alarmsPage));
        _preferenceManager.addToRoot(alarmNode);

        // Software and Chassis Resource will not have associated Device Objects
        /*if (!(eclass.getName().equals("ChassisResource")
                || eclass.getName().equals("SoftwareResource")
                || eclass.getName().equals("MibResource"))) {
            alarmNode.add(new PreferenceNode("Associate Device Objects",
                    new GenericTablePage("Associated Device Objects",
                       associatedDORef.getEReferenceType(), _classObject,
                       (NotifyingList) serObj.eGet(associatedDORef))));
        }*/

    }
    
    /**
     * Creates alarm objects for resource in alarm rule
     * for each alarm id associated.
     *
     */
    
    private void initAlarmRuleRes() {
		EObject alarmRuleInfoObj = (EObject) _alarmRuleViewModel.getEList().get(0);

		EReference resourceRef = (EReference) alarmRuleInfoObj.eClass()
				.getEStructuralFeature(ClassEditorConstants.ALARM_RESOURCE);

		java.util.List ruleResObjList = (java.util.List) alarmRuleInfoObj
					.eGet(resourceRef);

		_alarmRuleResObj = EcoreUtils.createEObject(resourceRef
					.getEReferenceType(), true);

		ruleResObjList.add(_alarmRuleResObj);

		EcoreUtils.setValue(_alarmRuleResObj, ClassEditorConstants.ALARM_RESOURCE_NAME,
				(String) EcoreUtils.getName(_classObject));
		
    }
    
    
    /**
     * ok pressed. so update the original EObject which model EObject.
     */
    protected void okPressed() {
    	enableMSOForResource();
    	_viewModel.save(false);
		_alarmLinkViewModel.save(false);
		if(null != _alarmRuleResObj){
		
			EReference alarmRef = (EReference) _alarmRuleResObj.eClass()
				.getEStructuralFeature(ClassEditorConstants.ALARM_ALARMOBJ);

			java.util.List alarmList = (java.util.List) _alarmRuleResObj
										.eGet(alarmRef);
			
			if(0 == alarmList.size()){
				
				EObject alarmRuleInfoObj = (EObject) _alarmRuleViewModel.getEList().get(0);
				
				List alarmRulesResList = (EList) EcoreUtils.getValue(alarmRuleInfoObj, 
		        		ClassEditorConstants.ALARM_RESOURCE);
				
				alarmRulesResList.remove(_alarmRuleResObj);
				
			}
		}
		
		_alarmRuleViewModel.save(false);
		super.okPressed();
	}
    /**
     * Enable/Disable Prov/Alarm MSOs if it required
     *
     */
    protected void enableMSOForResource() {
    	EObject provObj = (EObject) EcoreUtils.getValue(_classObject,
				ClassEditorConstants.RESOURCE_PROVISIONING);
    	EObject pmObj = (EObject) EcoreUtils.getValue(_classObject,
				"pm");
    	EObject alarmObj = (EObject) EcoreUtils.getValue(
				_classObject, ClassEditorConstants.RESOURCE_ALARM);
    	EcoreUtils.setValue(provObj, "isEnabled", "false");
    	EcoreUtils.setValue(pmObj, "isEnabled", "false");
    	EcoreUtils.setValue(alarmObj, "isEnabled", "false");
    	
    	EList attributes = (EList) EcoreUtils.getValue(provObj, ClassEditorConstants.CLASS_ATTRIBUTES);
    	if(attributes.size() > 0) {
    		EcoreUtils.setValue(provObj, "isEnabled", "true");
    	}
    	
    	attributes = (EList) EcoreUtils.getValue(pmObj, ClassEditorConstants.CLASS_ATTRIBUTES);
    	if(attributes.size() > 0) {
    		EcoreUtils.setValue(pmObj, "isEnabled", "true");
    	}
    	
    	String resourceName = EcoreUtils.getName(_classObject);
    	EObject mapObj = _alarmLinkViewModel.getEObject();
		List linkDetails = (List) EcoreUtils.getValue(mapObj, "linkDetail");
		if(linkDetails == null)
			return;
		Iterator iter = linkDetails.iterator();
		while (iter.hasNext()) {
			EObject linkDetailObj = (EObject) iter.next();
			String linkSrc = (String) EcoreUtils.getValue(
					linkDetailObj, "linkSource");
			if (linkSrc != null && linkSrc.equals(resourceName)) {
				if (EcoreUtils.getValue(linkDetailObj, "linkTarget") != null) {
					EList list = (EList) EcoreUtils.getValue(linkDetailObj,
							"linkTarget");
					if (list.size() > 0) {
						EcoreUtils.setValue(alarmObj, "isEnabled", "true");
					} 
				}
			}
		}
    }
	public IPreferenceNode findNodeMatching(String nodeId)
	{
		return super.findNodeMatching(nodeId);
	}
    /**
     * @return
     */
    public Model getViewModel() {
		return _viewModel;
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
}
