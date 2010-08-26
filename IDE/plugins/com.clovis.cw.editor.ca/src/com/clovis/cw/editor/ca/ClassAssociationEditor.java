/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/ClassAssociationEditor.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.Iterator;
import java.util.List;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Platform;
import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.impl.AdapterImpl;
import org.eclipse.emf.common.util.EList;
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
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.manageability.ui.ManageabilityEditorInput;
import com.clovis.cw.editor.ca.view.MOTreeContentProvider;
import com.clovis.cw.editor.ca.view.MOTreeLabelProvider;
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
 * @author Ashish
 * ClassAssociation Editor.
 * Provides specific implementation of Clss Association.
 */
public class ClassAssociationEditor extends GenericEditor
{
    private static File    metaClassFile;

    private static boolean initialized;

    private OutlineViewPage _outlinePage =  null;
    private ResourceValidator  _validator    = null;
    private Model _mapViewModel = null;
    private Model _alarmRuleViewModel = null;
    private ModelMapListener _mapListener;
    //Initialize
    static {
        if (!initialized) {
            init();
        }
    }
    /**
     * Reads Ecore and Meta Class file in first call. Subsequent calls does not
     * do anything.
     */
    private static void init()
    {
        try {
            URL url = CaPlugin.getDefault().find(
                    new Path("xml" + File.separator + "resource.xml"));
            metaClassFile = new Path(Platform.resolve(url).getPath()).toFile();
            initialized = true;
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    /**
     * Constructor. Reads
     */
    public ClassAssociationEditor()
    {
        super();
    }
    /**
     * MetaClassFile as needed by Generic Editor.
     * @return meta class file
     */
    protected File getMetaClassFile()
    {
        return metaClassFile;
    }
    /**
     * Create Utilities class for Generic Editor.
     * This is used for UI data.
     * @param objs Objects to show.
     * @return Utility class for objects.
     */
    protected GEUtils getUtils(Object[] objs)
    {
        return new CAUtils(objs);
    }
    /**
     * Create Utilities class for Generic Editor.
     * This is used for Data.
     * @param objs Objects to show.
     * @return Utility class for objects.
     */
    protected GEDataUtils getDataUtils(EList objs)
    {
        return new ResourceDataUtils(objs);
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
    	return ClassEditorConstants.EDITOR_TYPE;
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
                new MOTreeContentProvider(), new MOTreeLabelProvider());
        return _outlinePage;
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
        	FormatConversionUtils.convertToResourceFormat((EObject) _model.getEList().
        			get(0), "Resource Editor");
            super.doSave(progressMonitor);
            _mapViewModel.save(true);
            _alarmRuleViewModel.save(true);
            if(progressMonitor != null) {
            	ProjectDataModel pdm = (ProjectDataModel) ProjectDataModel.
        		getProjectDataModel((IContainer)getValue("project"));
                if(pdm.getManageabilityEditorInput() != null) {
                	ManageabilityEditorInput input = (ManageabilityEditorInput)pdm.getManageabilityEditorInput();
                	if(input.getEditor() != null) {
                		input.getEditor().refresh();
                	}
                }
            }
        }
    }
    /**
     * @param input - IEditorInput
     * create the validator after setting the Editor Model.
     */
    public void setInput(IEditorInput input)
    {
    	super.setInput(input);
        _validator = new ResourceValidator(_model, getSite().getShell(),
    	        (IProject)((GenericEditorInput) input).getResource());
        SubModelMapReader reader = SubModelMapReader.getSubModelMappingReader(
        		(IProject) getValue("project"),"resource", "alarm");
        ProjectDataModel pdm = (ProjectDataModel) ProjectDataModel.
    		getProjectDataModel((IContainer)getValue("project"));
        Model mapModel = pdm.getResourceAlarmMapModel();
        _mapViewModel = mapModel.getViewModel();
        
        Model alarmRuleModel = pdm.getAlarmRules();
        _alarmRuleViewModel	= alarmRuleModel.getViewModel();
        
        List moList = ResourceDataUtils.getMoList(_model.getEList());
        reader.initializeLinkRDN(moList);
        EObject rootObj = (EObject) _model.getEList().get(0);
        _mapListener = new ModelMapListener();
        EcoreUtils.addListener(rootObj.eGet(rootObj.eClass().getEStructuralFeature(
						ClassEditorConstants.HARDWARE_RESOURCE_REF_NAME)), _mapListener, -1);
        EcoreUtils.addListener(rootObj.eGet(rootObj.eClass().getEStructuralFeature(
				ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME)), _mapListener, -1);
        EcoreUtils.addListener(rootObj.eGet(rootObj.eClass().getEStructuralFeature(
				ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME)), _mapListener, -1);
        EcoreUtils.addListener(rootObj.eGet(rootObj.eClass().getEStructuralFeature(
				ClassEditorConstants.MIB_RESOURCE_REF_NAME)), _mapListener, -1);
        EcoreUtils.addListener(rootObj.eGet(rootObj.eClass().getEStructuralFeature(
				ClassEditorConstants.SYSTEM_CONTROLLER_REF_NAME)), _mapListener, -1);
    }
    /**
     * remove the listener on disposure
     */
    public void dispose()
    {
        _validator.removeListeners();
        EObject rootObj = (EObject) _model.getEList().get(0);
        EcoreUtils.removeListener(rootObj.eGet(rootObj.eClass().getEStructuralFeature(
						ClassEditorConstants.HARDWARE_RESOURCE_REF_NAME)), _mapListener, -1);
        EcoreUtils.removeListener(rootObj.eGet(rootObj.eClass().getEStructuralFeature(
				ClassEditorConstants.NODE_HARDWARE_RESOURCE_REF_NAME)), _mapListener, -1);
        EcoreUtils.removeListener(rootObj.eGet(rootObj.eClass().getEStructuralFeature(
				ClassEditorConstants.SOFTWARE_RESOURCE_REF_NAME)), _mapListener, -1);
        EcoreUtils.removeListener(rootObj.eGet(rootObj.eClass().getEStructuralFeature(
				ClassEditorConstants.MIB_RESOURCE_REF_NAME)), _mapListener, -1);
        EcoreUtils.removeListener(rootObj.eGet(rootObj.eClass().getEStructuralFeature(
				ClassEditorConstants.SYSTEM_CONTROLLER_REF_NAME)), _mapListener, -1);
        if(_mapViewModel != null) {
        	_mapViewModel.dispose();
        	_mapViewModel = null;
        }
        if(_alarmRuleViewModel != null) {
        	_alarmRuleViewModel.dispose();
        	_alarmRuleViewModel = null;
        }
        super.dispose();
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
     * @return the alarm rule view model
     */
    public Model getAlarmRuleViewModel()
    {
    	return _alarmRuleViewModel;
    }
    
    /**
     * @see org.eclipse.ui.IWorkbenchPart#setFocus()
     */
    public void setFocus() {
		super.setFocus();
		try {
			ProjectDataModel pdm = (ProjectDataModel) ProjectDataModel.getProjectDataModel((IContainer)getValue("project"));
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
     * @author shubhada
     * Model map listener class to listen for additions in the object list
     * and update the rdn
     */
    public class ModelMapListener extends AdapterImpl
    {
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
            				getLinkObject(mapObj, ClassEditorConstants.
            						ASSOCIATED_ALARM_LINK);
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
                		
                	}
                }
                break;
            case Notification.ADD:
            	 Object newVal = notification.getNewValue();
            	 if (newVal instanceof EObject) {
            		 EcoreUtils.addListener(newVal, _mapListener, -1);
            	 }
                 break;
            case Notification.ADD_MANY:
	           	 List newVals = (List) notification.getNewValue();
	           	 for (int i = 0; i < newVals.size(); i++) {
	           		 newVal = newVals.get(i);
		           	 if (newVal instanceof EObject) {
		           		 EcoreUtils.addListener(newVal, _mapListener, -1);
		           	 }
	           	 }
                break;
            case Notification.REMOVE:
                Object oldVal = notification.getOldValue();
                if (oldVal instanceof EObject) {
                    EObject eobj = (EObject) oldVal;
                    String rdn = EcoreUtils.getValue(eobj, ModelConstants.
        					RDN_FEATURE_NAME).toString();
                    EObject mapObj = _mapViewModel.getEObject();
        			EObject linkObj = SubModelMapReader.
        				getLinkObject(mapObj, ClassEditorConstants.
        						ASSOCIATED_ALARM_LINK);
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
    	    			
        			EcoreUtils.removeListener(eobj, _mapListener, -1);
                }
                break;

            case Notification.REMOVE_MANY:
                List objs = (List) notification.getOldValue();
                for (int i = 0; i < objs.size(); i++) {
                    if (objs.get(i) instanceof EObject) {
                        EObject eobj = (EObject) objs.get(i);
                        String rdn = EcoreUtils.getValue(eobj, ModelConstants.
    	    					RDN_FEATURE_NAME).toString();
                        EObject mapObj = _mapViewModel.getEObject();
            			EObject linkObj = SubModelMapReader.
            				getLinkObject(mapObj, ClassEditorConstants.
            						ASSOCIATED_ALARM_LINK);
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
	    				EcoreUtils.removeListener(eobj, _mapListener, -1);
                    }
                }
                break;
           }
    		
    	}
    	
    }

}
