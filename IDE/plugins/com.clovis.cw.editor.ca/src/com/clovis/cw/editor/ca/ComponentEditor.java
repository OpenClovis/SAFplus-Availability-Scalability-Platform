/*
 * @(#) $RCSfile: ComponentEditor.java,v $
 * $Revision: #7 $ $Date: 2007/03/26 $
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
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/ComponentEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/03/26 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

/*
 * @(#) $RCSfile: ComponentEditor.java,v $
 * $Revision: #7 $ $Date: 2007/03/26 $
 *
 * Copyright (C) 2005 -- Clovis Solutions.
 * Proprietary and Confidential. All Rights Reserved.
 *
 * This software is the proprietary information of Clovis Solutions.
 * Use is subject to license terms.
 *
 */
package com.clovis.cw.editor.ca;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Vector;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.gef.editparts.ZoomManager;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.views.contentoutline.IContentOutlinePage;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.tree.TreeLabelProvider;
import com.clovis.cw.editor.ca.constants.ComponentEditorConstants;
import com.clovis.cw.editor.ca.handler.UpdateProject;
import com.clovis.cw.editor.ca.manageability.ui.ManageabilityEditorInput;
import com.clovis.cw.editor.ca.view.ComponentTreeContentProvider;
import com.clovis.cw.genericeditor.EditorModelValidator;
import com.clovis.cw.genericeditor.GEDataUtils;
import com.clovis.cw.genericeditor.GEUtils;
import com.clovis.cw.genericeditor.GenericEditor;
import com.clovis.cw.genericeditor.GenericEditorInput;
import com.clovis.cw.genericeditor.outline.OutlineViewPage;
import com.clovis.cw.project.data.ProjectDataModel;
import com.clovis.cw.project.data.SubModelMapReader;
import com.clovis.cw.project.utils.FormatConversionUtils;

/**
 * @author pushparaj
 *
 * Provides specific implementation of Component Editor
 */
public class ComponentEditor extends GenericEditor
{
    private static File    metaClassFile;

    private static boolean initialized;

    private OutlineViewPage _outlinePage =  null;

    private ComponentValidator _validator;

    private AmfDefinitionsReader _reader = null;
    
    private ComponentUtils _compUtils = null;
    
    private GEDataUtils _compDataUtils = null;
    
    private Model  _mapViewModel = null;
    
    private Model _pmConfigModel = null;
    
    private ModelMapListener _mapListener;
    

    //Initialize
    static {
        if (!initialized) {
            init();
        }
    }
    /**
     * Constructor
     * calls init()method
     */
    public ComponentEditor()
    {
        super();
        init();
    }
    /**
     * Initialize Meta-Class File for Component Editor
     */
    private static void init()
    {
        try {
            URL url = CaPlugin.getDefault().find(
                    new Path("xml" + File.separator + "component.xml"));
            metaClassFile = new Path(Platform.resolve(url).getPath())
            .toFile();
            initialized = true;
        } catch (IOException e) {
            CaPlugin.LOG.error("Unable to create meta-class file", e);
        }
    }
    /**
     * Returns MetaClass File for ComponentEditor
     * @return MetaClass File
     */
    protected File getMetaClassFile()
    {
        return metaClassFile;
    }
    /**
     * Returns Utils class for Component Editor
     * @param objs array of EObjects
     * @return Utils
     */
    protected GEUtils getUtils(Object[] objs)
    {
        _compUtils = new ComponentUtils(objs);
        return _compUtils;
       
    }
   /**
    * Create Utilities class for Generic Editor.
    * This is used for Data.
    * @param objs Objects to show.
    * @return Utility class for objects.
    */
   protected GEDataUtils getDataUtils(EList objs)
   {
       return new ComponentDataUtils(objs);
   }
    /**
     * @return Editor Model Validator
     */
    public EditorModelValidator getEditorModelValidator()
    {
    	return _validator;
    }
    /**
     * Returns Type of the Editor
     */
    public String getEditorType()
    {
    	return ComponentEditorConstants.EDITOR_TYPE;
    }
    /**
     * Returns true if Editor Model is valid. Child classes should implement
     * this method
     * @return true/false
     */
    protected boolean isValid()
    {
        return _validator.isEditorValid();
    }
    /**
     * @param progressMonitor - IProgressMonitor
     * Checking the validity of the model before saving it.
     */
    public void doSave(IProgressMonitor progressMonitor)
    {
        if (!_validator.isEditorValid()) {
            MessageDialog.openError(getSite().getShell(), getPartName(),
                    _validator.getMessage());
            return;
        } else {
        	
            IProject project = (IProject) getValue("project");
            ProjectDataModel pdm =  ProjectDataModel.getProjectDataModel(project);
            
            List resEditorObjects = pdm.getCAModel().getEList();
            List compList = getCompList();
            
            
            List assoConnList = _compDataUtils.getConnectionFrmType(
                    ComponentEditorConstants.ASSOCIATION_NAME,
                    ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME,
                    ComponentEditorConstants.NONSAFCOMPONENT_NAME);
            List list1 = _compDataUtils.getConnectionFrmType(
                    ComponentEditorConstants.ASSOCIATION_NAME,
                    ComponentEditorConstants.COMPONENTSERVICEINSTANCE_NAME,
                    ComponentEditorConstants.SAFCOMPONENT_NAME);    
            
            
            assoConnList.addAll(list1);
            setComponentCSIType(assoConnList);
            setComponentProxyCSIType();
            
            FormatConversionUtils.convertToResourceFormat((EObject) _model.getEList().
        			get(0), "Component Editor");
            _mapViewModel.save(true);
            setLibs(compList, resEditorObjects, project);
            super.doSave(progressMonitor);
            writeAmfDefinitions();
            _pmConfigModel.save(true);            
            if (progressMonitor != null) {
				if (pdm.getManageabilityEditorInput() != null) {
					ManageabilityEditorInput input = (ManageabilityEditorInput) pdm
							.getManageabilityEditorInput();
					if (input.getEditor() != null) {
						input.getEditor().refresh();
					}
				}
			}
        }
    }
    /**
	 * @param input -
	 *            IEditorInput create the validator after setting the Editor
	 *            Model.
	 */
    public void setInput(IEditorInput input)
    {
    	super.setInput(input);
        _validator = new ComponentValidator(_model, getSite().getShell());
        _compDataUtils = getDataUtils(_model.getEList());
        ProjectDataModel pdm = (ProjectDataModel) ProjectDataModel.
        	getProjectDataModel((IContainer)getValue("project"));
        Model mapModel = pdm.getComponentResourceMapModel();
        _mapViewModel = mapModel.getViewModel();
        EObject rootObj = (EObject) _model.getEList().get(0);
        NotifyingList safComponentList = (NotifyingList) EcoreUtils.getValue(rootObj,
        		ComponentEditorConstants.SAFCOMPONENT_REF_NAME);
        SubModelMapReader reader = SubModelMapReader.getSubModelMappingReader(
        		(IProject) getValue("project"), "component", "resource");
        reader.initializeLinkRDN(safComponentList);
        _pmConfigModel = pdm.getPMConfigModel().getViewModel();
        _mapListener = new ModelMapListener();
        EcoreUtils.addListener(safComponentList, _mapListener, -1);
    }
    /**
     * remove the listener on disposure
     */
    public void dispose()
    {
        _validator.removeListeners();
        EObject rootObj = (EObject) _model.getEList().get(0);
        NotifyingList safComponentList = (NotifyingList) EcoreUtils.getValue(rootObj,
        		ComponentEditorConstants.SAFCOMPONENT_REF_NAME);
        EcoreUtils.removeListener(safComponentList, _mapListener, -1);
        super.dispose();
    }
    /**
     * @param type -Class
     * @return the instance of outline page
     */
    public Object getAdapter(Class type)
    {
        if (type == ZoomManager.class) {
            return getGraphicalViewer().getProperty(
                    ZoomManager.class.toString());
        } else if (type == IContentOutlinePage.class) {
            if (_outlinePage == null) {
                _outlinePage = createOutlinePage();
            }
            return _outlinePage;
        }
        return super.getAdapter(type);
    }
    /**
     * @return ResourceOutlineViewPage
     */
    private OutlineViewPage createOutlinePage()
    {
        _outlinePage = new OutlineViewPage((EList) ((GenericEditorInput)
                getEditorInput()).getModel().getEList(), this, 
                new ComponentTreeContentProvider(), new TreeLabelProvider());
        return _outlinePage;
    }
    /**
     * Writes the AmfDefs file after initializing the Objects
     */
    private void writeAmfDefinitions()
    {
        _reader = new AmfDefinitionsReader(getEditorModel().getProject());
        _reader.writeAmfDefinitions(_model);
    }
    /**
     * 
     * @Method to return Component list.
     */
    private List getCompList()
    {
    	List compList = new Vector();
    	List modelList = (List) getValue("model");
    	EObject rootObject = (EObject) modelList.get(0);
    	EList list = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature(ComponentEditorConstants.SAFCOMPONENT_REF_NAME));
    	compList.addAll(list);
    	/*for (int i = 0; i < modelList.size(); i++) {
            EObject eobj = (EObject) modelList.get(i);
            if (eobj.eClass().getName().equals(
                    ComponentEditorConstants.SAFCOMPONENT_NAME)) {
                compList.add(eobj);
            }
       
    }*/
        return compList;
    }
    /**
     * 
     * @param assoConnList
     */
    private void setComponentCSIType(List assoConnList)
    {
        List compList = new Vector();
        EObject rootObject = (EObject) _model.getEList().get(0);
    	EList list = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature(ComponentEditorConstants.SAFCOMPONENT_REF_NAME));
    	compList.addAll(list);
    	list = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature(ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME));
    	compList.addAll(list);

    	for(int i=0 ; i<compList.size() ; i++) {
    		EObject csiTypesObj = ((EObject) EcoreUtils.getValue((EObject) compList.get(i),
					ComponentEditorConstants.COMPONENT_CSI_TYPES));
    		((List) EcoreUtils.getValue(csiTypesObj, ComponentEditorConstants.COMPONENT_CSI_TYPE)).clear();
    	}
        /*for (int i = 0; i < _model.getEList().size(); i++) {
            EObject eobj = (EObject) _model.getEList().get(i);
            if (eobj.eClass().getName().equals(
                    ComponentEditorConstants.SAFCOMPONENT_NAME)
                       || eobj.eClass().getName().equals(
                    ComponentEditorConstants.NONSAFCOMPONENT_NAME)) {
                compList.add(eobj);
            } 
        }*/
        for (int i = 0; i < assoConnList.size(); i++) {
            EObject connObj = (EObject) assoConnList.get(i);
            EObject srcObj = _compDataUtils.getSource(connObj);
            EObject targetObj = _compDataUtils.getTarget(connObj);
            String typeOfCSI = (String) EcoreUtils.getValue(srcObj, 
                    ComponentEditorConstants.NAME);
    		EObject csiTypesObj = ((EObject) EcoreUtils.getValue(targetObj,
					ComponentEditorConstants.COMPONENT_CSI_TYPES));
            List csiTypeList = (List) EcoreUtils.getValue(csiTypesObj, ComponentEditorConstants.
                    COMPONENT_CSI_TYPE);
            EObject csiTypeObj = EcoreUtils
					.createEObject((EClass) csiTypesObj.eClass().getEPackage()
							.getEClassifier("CompCSIType"), true);
			EcoreUtils.setValue(csiTypeObj, "name", typeOfCSI);
			csiTypeList.add(csiTypeObj);
            compList.remove(targetObj);
        }
    }
    private void setComponentProxyCSIType()
    {
        List nonSAFComponents = new Vector();
        EObject rootObject = (EObject) _model.getEList().get(0);
    	EList list = (EList) rootObject.eGet(rootObject.eClass().getEStructuralFeature(ComponentEditorConstants.NONSAFCOMPONENT_REF_NAME));
    	nonSAFComponents.addAll(list);
        /*for (int i = 0; i < _model.getEList().size(); i++) {
            EObject eobj = (EObject) _model.getEList().get(i);
            if (eobj.eClass().getName().equals(
                    ComponentEditorConstants.NONSAFCOMPONENT_NAME)) {
                nonSAFComponents.add(eobj);
            } 
        }*/
        List proxyProxiedConnList = _compDataUtils.getConnectionFrmType(
                ComponentEditorConstants.PROXY_PROXIED_NAME,
                ComponentEditorConstants.SAFCOMPONENT_NAME,
                ComponentEditorConstants.NONSAFCOMPONENT_NAME);
        for (int i = 0; i < proxyProxiedConnList.size(); i++) {
            EObject connObj = (EObject) proxyProxiedConnList.get(i);
            EObject srcObj = _compDataUtils.getSource(connObj);
            EObject targetObj = _compDataUtils.getTarget(connObj);
            nonSAFComponents.remove(targetObj);
    		EObject csiTypesObj = ((EObject) EcoreUtils.getValue(srcObj,
					ComponentEditorConstants.COMPONENT_CSI_TYPES));
            List csiTypeList = (List) EcoreUtils.getValue(csiTypesObj,
                    ComponentEditorConstants.COMPONENT_CSI_TYPE);
            String proxyCsiType = csiTypeList.size() > 0 ? (String) EcoreUtils
					.getName((EObject) csiTypeList.get(0)) : "";
            EcoreUtils.setValue(targetObj, ComponentEditorConstants.
                    COMPONENTPROXY_CSI_TYPE, proxyCsiType);
        }
        for (int i = 0; i < nonSAFComponents.size(); i++) {
            EObject nonSAFObj = (EObject) nonSAFComponents.get(i);
            EcoreUtils.setValue(nonSAFObj, ComponentEditorConstants.
                    COMPONENTPROXY_CSI_TYPE, "");
        }
    }
    /**
	 * Selects the libs.
	 *  
	 */
	private void setLibs(List compList, List resEditorObjects, IProject project) {
		for (int i = 0; i < compList.size(); i++) {
			EObject compObj = (EObject) compList.get(i);			
			UpdateProject.setProvLib(compObj, resEditorObjects, project);
			UpdateProject.setAlarmLib(compObj, resEditorObjects, project);
			UpdateProject.setHalLib(compObj, resEditorObjects, project);
			UpdateProject.setMandatoryLibs(compObj);
		}
	}
    /**
     * @see org.eclipse.ui.IWorkbenchPart#setFocus()
     */
    public void setFocus() {
		super.setFocus();
		try {
			ProjectDataModel pdm = (ProjectDataModel) ProjectDataModel.getProjectDataModel((IContainer)getValue("project"));
			if (pdm.getCAEditorInput() != null) {
				GenericEditorInput caInput = (GenericEditorInput) pdm
						.getCAEditorInput();

				if (caInput.isDirty()) {
					boolean status = MessageDialog.openQuestion(
							(Shell) getValue("shell"), "Save resource Model",
							"Resource Model has been modified. Save changes?");
					if (status) {
						if (caInput != null
								&& caInput.getEditor() != null
								&& caInput.getEditor().getEditorModel() != null
								&& caInput.getEditor().getEditorModel()
										.isDirty()) {
							caInput.getEditor().doSave(null);
						}
					}
				}
			}
			if (pdm.getManageabilityEditorInput() != null) {
				ManageabilityEditorInput maInput = (ManageabilityEditorInput) pdm
						.getManageabilityEditorInput();
				if (maInput.isDirty()) {
					boolean status = MessageDialog
							.openQuestion((Shell) getValue("shell"),
									"Save Manageability Model",
									"Manageability Model has been modified. Save changes?");
					if (status) {
						maInput.getEditor().doSave(null);
					}
				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}
    /**
     * 
     * @return the link view model
     */
    public Model getLinkViewModel()
    {
    	return _mapViewModel;
    }
    
    /**
     * 
     * @author shubhada
     * Model map listener class to listen for additions in the object list
     * and update the rdn
     */
    public class ModelMapListener extends AdapterImpl
    {
    	Map<String, EObject> _appMap = new HashMap<String, EObject>();
    	EList<EObject> _appIntervalList;
    	EList<EObject> _compIntervalList;
    	//Map <String, List> _appCompMap = new HashMap<String, List>();
    	EClass _configClass;
    	public ModelMapListener() {
    		populatePMConfig();
    	}
    	/**
    	 * Creates application config object map
    	 */
    	private void populatePMConfig() {
    		/*ProjectDataModel pdm = (ProjectDataModel) ProjectDataModel.
        	getProjectDataModel((IContainer)getValue("project"));
    		if(_pmConfigModel != null) {
    			_pmConfigModel.dispose();
    		}
    		_pmConfigModel = pdm.getPMConfigModel().getViewModel();*/
    		 _configClass = (EClass) _pmConfigModel.getEObject().eClass().getEPackage().getEClassifier("AppConfigIntervalType");
    		EObject pmObj = _pmConfigModel.getEObject();
   		 	_appIntervalList = (EList<EObject>)EcoreUtils.getValue(pmObj, "appConfigInterval");
   		 	_compIntervalList = (EList<EObject>)EcoreUtils.getValue(pmObj, "compConfigInterval");
    		for (int i = 0; i < _appIntervalList.size(); i++) {
    			EObject obj = _appIntervalList.get(i);
    			_appMap.put((String)EcoreUtils.getValue(obj, "applicationName"), obj);
    		}
    		/*for (int i = 0; i < _compIntervalList.size(); i++) {
    			EObject obj = _compIntervalList.get(i);
    			String type = (String)EcoreUtils.getValue(obj, "type");
    			List compList = _appCompMap.get(type);
    			if(compList == null) {
    				compList = new ArrayList();
    				_appCompMap.put(type, compList);
    			}
    			compList.add(obj);
    		}*/
    	}
    	/**
    	 * @param msg - Notification
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
                	if (feature.getName().equals(ModelConstants.NAME_FEATURE_NAME)) {
                		String rdn = EcoreUtils.getValue(eobj, ModelConstants.
            					RDN_FEATURE_NAME).toString();
            			EObject mapObj = _mapViewModel.getEObject();
            			EObject linkObj = SubModelMapReader.
            				getLinkObject(mapObj, ComponentEditorConstants.
            						ASSOCIATE_RESOURCES_NAME);
        				List linkDetails = (List) EcoreUtils.getValue(linkObj, "linkDetail");
        				Iterator iter = linkDetails.iterator();
        				while (iter.hasNext()) {
        					EObject linkDetailObj = (EObject) iter.next();
        					String linkSrc = (String) EcoreUtils.getValue(
        							linkDetailObj, "linkSourceRdn");
        					if (linkSrc != null && linkSrc.equals(rdn)) {
        						EcoreUtils.setValue(linkDetailObj, "linkSource",
        								notification.getNewStringValue());
        					}
        				}
                		if(eobj.eClass().getName().equals(ComponentEditorConstants.SAFCOMPONENT_NAME)) {
                			String oldName = notification.getOldStringValue();
                			String newName = notification.getNewStringValue();
                			EObject configObj = _appMap.get(oldName);
                			EcoreUtils.setValue(configObj, "applicationName", newName);
                			_appMap.remove(oldName);
                			_appMap.put(newName, configObj);
                			
                			for (int i = 0; i < _compIntervalList.size(); i++) {
                    			EObject compObj = _compIntervalList.get(i);
                    			String type = (String) EcoreUtils.getValue(compObj, "type");
                    			if(oldName.equals(type)){
                    				EcoreUtils.setValue(compObj, "type", newName);
                    			}
                    		}
                		}
                	}
                }
                break;
            case Notification.ADD:
            	 Object newVal = notification.getNewValue();
            	 if (newVal instanceof EObject) {
            		 EcoreUtils.addListener(newVal, _mapListener, -1);
            		 if(((EObject)newVal).eClass().getName().equals(ComponentEditorConstants.SAFCOMPONENT_NAME)) {
            			String compName = EcoreUtils.getName((EObject)newVal);
             			EObject configObj = EcoreUtils.createEObject(_configClass, true);
             			EcoreUtils.setValue(configObj, "applicationName", compName);
             			_appIntervalList.add(configObj);
             			_appMap.put(compName, configObj);
             		 }
            	 }
                 break;
            case Notification.ADD_MANY:
	           	List newVals = (List) notification.getNewValue();
	            for (int i = 0; i < newVals.size(); i++) {
	           		 newVal = newVals.get(i);
		           	 if (newVal instanceof EObject) {
		           		 EcoreUtils.addListener(newVal, _mapListener, -1);
		           		 if(((EObject)newVal).eClass().getName().equals(ComponentEditorConstants.SAFCOMPONENT_NAME)) {
	             			String compName = EcoreUtils.getName((EObject)newVal);
	             			EObject configObj = EcoreUtils.createEObject(_configClass, true);
	             			EcoreUtils.setValue(configObj, "applicationName", compName);
	             			_appIntervalList.add(configObj);
	             			_appMap.put(compName, configObj);
		           		 }
		           	 }
	           	 }
                break;
            case Notification.REMOVE:
            	Object oldVal = notification.getOldValue();
                if (oldVal instanceof EObject) {
                    EObject eobj = (EObject) oldVal;
                    Object rdnObj = EcoreUtils.getValue(eobj, ModelConstants.
        					RDN_FEATURE_NAME);
                    if(rdnObj != null) {
                        String rdn = rdnObj.toString();
                        EObject mapObj = _mapViewModel.getEObject();
            			EObject linkObj = SubModelMapReader.
            				getLinkObject(mapObj, ComponentEditorConstants.
            						ASSOCIATE_RESOURCES_NAME);
        				List linkDetails = (List) EcoreUtils.getValue(linkObj, "linkDetail");
        				Iterator iter = linkDetails.iterator();
        				while (iter.hasNext()) {
        					EObject linkDetailObj = (EObject) iter.next();
        					String linkSrc = (String) EcoreUtils.getValue(
        							linkDetailObj, "linkSourceRdn");
        					if (linkSrc != null && linkSrc.equals(rdn)) {
        						iter.remove();
        					}
        				}
                    }
        			EcoreUtils.removeListener(eobj, _mapListener, -1);
        			if(((EObject)eobj).eClass().getName().equals(ComponentEditorConstants.SAFCOMPONENT_NAME)) {
        				String compName = EcoreUtils.getName((EObject)eobj);
             			EObject configObj = _appMap.get(compName);
             			_appIntervalList.remove(configObj);
             			_appMap.remove(compName);
             			
             			/*List compList = _appCompMap.get(compName);
             			if(compList != null) {
             				_compIntervalList.removeAll(compList);
             			}*/
             		}
        		}
                break;

            case Notification.REMOVE_MANY:
                List objs = (List) notification.getOldValue();
                for (int i = 0; i < objs.size(); i++) {
                    if (objs.get(i) instanceof EObject) {
                        EObject eobj = (EObject) objs.get(i);
                        Object rdnObj = EcoreUtils.getValue(eobj, ModelConstants.
            					RDN_FEATURE_NAME);
                        if(rdnObj != null) {
                            String rdn = rdnObj.toString();
                            EObject mapObj = _mapViewModel.getEObject();
                			EObject linkObj = SubModelMapReader.
                				getLinkObject(mapObj, ComponentEditorConstants.
                						ASSOCIATE_RESOURCES_NAME);
    	    				List linkDetails = (List) EcoreUtils.getValue(linkObj, "linkDetail");
    	    				Iterator iter = linkDetails.iterator();
    	    				while (iter.hasNext()) {
    	    					EObject linkDetailObj = (EObject) iter.next();
    	    					String linkSrc = (String) EcoreUtils.getValue(
    	    							linkDetailObj, "linkSourceRdn");
    	    					if (linkSrc.equals(rdn)) {
    	    						iter.remove();
    	    					}
    	    				}
                        }
	    				EcoreUtils.removeListener(eobj, _mapListener, -1);
	    				if(((EObject)eobj).eClass().getName().equals(ComponentEditorConstants.SAFCOMPONENT_NAME)) {
	             			String compName = EcoreUtils.getName((EObject)eobj);
	             			EObject configObj = _appMap.get(compName);
	             			_appIntervalList.remove(configObj);
	             			_appMap.remove(compName);
	             			
	             			/*List compList = _appCompMap.get(compName);
	             			if(compList != null) {
	             				_compIntervalList.removeAll(compList);
	             			}*/
	             		}
                    }
                }
                break;
           }
    		
    	}
       	
    }

}
