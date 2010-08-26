/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/CpmPushButtonCellEditor.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotifyingList;
import org.eclipse.emf.common.util.BasicEList;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.EStructuralFeature;
import org.eclipse.jface.dialogs.DialogPage;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.viewers.CellEditor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.menu.Environment;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.common.utils.ui.dialog.PushButtonDialog;
import com.clovis.common.utils.ui.factory.PushButtonCellEditor;
import com.clovis.cw.editor.ca.dialog.NodeProfileDialog;
import com.clovis.cw.editor.ca.manageability.ui.AssociateResourceUtils;
/**
 * @author Pushparaj
 *
 * PushButtonCellEditor for Associate Resources
 */
public class ResourcesPushButtonCellEditor extends PushButtonCellEditor
{
	private EReference _ref = null;
	private Environment _parentEnv = null;
	private List<String> _initializedResList = new ArrayList<String>();

    /**
     * @param parent - parent composite
     * @param ref -EReference
     * @param env - Parent Environment
     */
    public ResourcesPushButtonCellEditor(Composite parent, EReference ref,
            Environment env)
    {
        super(parent, ref, env);
        _ref = ref;
        _parentEnv = env;
    }
    /**
     * @param cellEditorWindow - Control
     * @return null
     */
    protected Object openDialogBox(Control cellEditorWindow) {
		EList list = (EList) getValue();
		if (NodeProfileDialog.getInstance() != null) {
			_initializedResList = AssociateResourceUtils
					.getInitializedArrayAttrResList(NodeProfileDialog.getInstance()
							.getResourceList());
		}
		new ResourcesPushButtonDialog(getControl().getShell(), _ref
				.getEReferenceType(), list, _parentEnv).open();
		return null;
	}
    /**
	 * 
	 * @param parent
	 *            Composite
	 * @param feature
	 *            EStructuralFeature
	 * @param env
	 *            Environment
	 * @return cell Editor
	 */
    public static CellEditor createEditor(Composite parent,
            EStructuralFeature feature, Environment env)
    {
        return new ResourcesPushButtonCellEditor(parent, (EReference) feature, env);
    }
    
    class ResourcesPushButtonDialog extends PushButtonDialog {

		public ResourcesPushButtonDialog(Shell shell, EClass eClass,
				Object value, Environment parentEnv) {
			super(shell);
			super.setShellStyle(SWT.CLOSE | SWT.RESIZE | SWT.APPLICATION_MODAL);
	        _value     = value;
	        _eClass    = eClass;
	        _parentEnv = parentEnv;
	        if (value instanceof List) {
	            NotifyingList origList = (NotifyingList) _value;
	            _model = new Model(null, origList, _eClass.getEPackage());
	            _viewModel  = _model.getViewModel();
	        } else if (value instanceof EObject) {
	            EObject eObject = (EObject) _value;
	            _model = new Model(null, eObject);
	            _viewModel  = _model.getViewModel();
	        }
	        _dialogValidator = new ResourcesValidator(this, _viewModel);
		}
    	
    }
    
    class ResourcesValidator extends DialogValidator
    {
    	private BasicEList<String> moIDs = new BasicEList<String>(); 
    	public ResourcesValidator(Object obj, Model model) {
			_model = model;
		    _elist = _model.getEList();
		    EcoreUtils.addListener((NotifyingList) _elist, this, 2);
		    initMap();
			if (obj instanceof TitleAreaDialog) {
				_tdialog = (TitleAreaDialog) obj;
			} else if (obj instanceof PreferenceDialog) {
				_pdialog = (PreferenceDialog) obj;
			} else if (obj instanceof DialogPage) {
				_dialogPage = (DialogPage) obj;
			}
			moIDs.grow(_elist.size());																																			
			for (int i = 0; i < _elist.size(); i++) {
				EObject eobj = (EObject) _elist.get(i);
				moIDs.addUnique((String)EcoreUtils.getValue(eobj, "moID"));
			}
		}
    	/**
		 * 
		 * @return true if the model is valid else false
		 */
	    public boolean isModelValid(List<EObject> objs)
	    {
	        for (int i = 0; i < objs.size(); i++) {
	            EObject eobj = (EObject) _elist.get(i);
	            String message = isValid(eobj);
	            if (message != null) {
	                setValid(false);
	                setMessage(message);
	                return false;
	            } 
	        }
	        setValid(true);
	        setMessage("");
	        setTitle();
	        return true;
	    }
	    /**
	     *
	     * @return true if the model is valid else false
	     */
	    public boolean isModelValid()
	    {
	        /*for (int i = 0; i < _elist.size(); i++) {
	            EObject eobj = (EObject) _elist.get(i);
	            String message = isValid(eobj);
	            if (message != null) {
	                setValid(false);
	                setMessage(message);
	                return false;
	            } 
	        }*/
	        setValid(true);
	        setMessage("");
	        setTitle();
	        return true;
	    }
		/**
	     * 
	     * @param eobj
	     * @param n
	     * @return
	     */
	    private boolean isObjectValid(EObject eobj, Notification n)
	    {
	        String message = ObjectValidator.checkPatternAndBlankValue(eobj);
	        if (message != null) {
	        	setValid(false);
				setMessage(message);
				return false;
				
	        }
	        /*message = isValidMOID(eobj);
			if (message != null) {
				setValid(false);
				setMessage(message);
				return false;
			}*/
	        setValid(true);
	        setMessage("");
	        setTitle();
	        return true;
	    }
	    
	    private String isValidMOID(EObject eobj) {
	    	String moID = (String)EcoreUtils.getValue(eobj, "moID");
	    	if(!moIDs.contains(moID)) {
	    		moIDs.grow(1);
	    		moIDs.addUnique(moID);
	    		return null;
	    	}
	    	return "Duplicate entry for MO Instance ID :" + moID;
	    }
	    //private boolean isValidOldObject(EObject eobj) {
	    	
	    //}
	    /**
	     * @param notification -
	     *            Notification
	     */
	    public void notifyChanged(Notification notification)
	    {
	        switch (notification.getEventType()) {
	        case Notification.REMOVING_ADAPTER:
	            break;
	        case Notification.SET:
	            Object object = notification.getNotifier();
	            if (!notification.isTouch() && object instanceof EObject) {
	            	if(((EStructuralFeature)notification.getFeature()).getName().equals("moID")) {
	            		String oldValue = notification.getOldStringValue();
	            		String newValue = notification.getNewStringValue();
	            		if (!newValue.equals(oldValue)) {
	            			  if (newValue.contains("*")
									|| _initializedResList
											.contains(AssociateResourceUtils
													.getResourceTypeFromInstanceID(newValue))) {
								EcoreUtils.setValue((EObject) object,
										"autoCreate", "false");
							}
						}
	            	} else if(((EStructuralFeature)notification.getFeature()).getName().equals("autoCreate")) {
	            		EObject modObject = (EObject) object;
	            		String moID = (String) EcoreUtils.getValue(modObject, "moID");
	            		 if (moID.contains("*")
								|| _initializedResList
										.contains(AssociateResourceUtils
												.getResourceTypeFromInstanceID(moID))) {
							EcoreUtils.setValue(modObject, "autoCreate",
									"false");
						}
					}
	                isObjectValid((EObject) object, notification);
	            }
	            break;
	        case Notification.ADD:
	            Object newVal = notification.getNewValue();
	            if (newVal instanceof EObject) {
	                EObject obj = (EObject) newVal;
	                String cwkey = (String) EcoreUtils.getValue(obj,
	                		ModelConstants.RDN_FEATURE_NAME);
	                _cwkeyObjectMap.put(cwkey, obj);
	                EcoreUtils.addListener(obj, this, 1);
	                isObjectValid(obj, notification);
	            }
	            break;

	        case Notification.ADD_MANY:
	            List objs = (List) notification.getNewValue();
	            for (int i = 0; i < objs.size(); i++) {
	                if (objs.get(i) instanceof EObject) {
	                    EObject eObj = (EObject) objs.get(i);
	                    String key = (String) EcoreUtils.getValue(eObj,
	                    		ModelConstants.RDN_FEATURE_NAME);
	                    _cwkeyObjectMap.put(key, eObj);
	                    EcoreUtils.addListener(eObj, this, 1);
	                }
	            }
	            //isModelValid(objs);
	            break;
	        case Notification.REMOVE:
	            Object obj = notification.getOldValue();
	            if (obj instanceof EObject) {
	                EObject ob = (EObject) obj;
	                String rkey = (String) EcoreUtils.getValue(ob,
	                		ModelConstants.RDN_FEATURE_NAME);
	                _cwkeyObjectMap.remove(rkey);
	                EcoreUtils.removeListener(ob, this, 1);
	            }
	            //isModelValid();
	            break;
	        case Notification.REMOVE_MANY:
	            objs = (List) notification.getOldValue();
	            for (int i = 0; i < objs.size(); i++) {
	                if (objs.get(i) instanceof EObject) {
	                    EObject o = (EObject) objs.get(i);
	                    String rk = (String) EcoreUtils.getValue(o,
	                    		ModelConstants.RDN_FEATURE_NAME);
	                    _cwkeyObjectMap.remove(rk);
	                    EcoreUtils.removeListener(o, this, 1);
	                }
	            }
	            //isModelValid();
	            break;
	        }
	    }
    }
}
