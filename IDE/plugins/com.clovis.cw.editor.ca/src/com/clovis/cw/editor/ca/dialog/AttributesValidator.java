/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/AttributesValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.DialogValidator;
import com.clovis.common.utils.ui.ObjectValidator;
import com.clovis.common.utils.ui.factory.ValidatorFactory;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
/**
 *
 * @author shubhada
 *    Specific Validator Class to validate the uniqueness of Attributes across
 *  provisioning and Attributes directly under the node.
 */
public class AttributesValidator extends DialogValidator
{
    private Shell _shell = null;
    private Notification _isProcessed = null;
    
    /**
     *
     * @param obj - Object
     * @param model - Model
     */
    public AttributesValidator(Object obj, Model model)
    {
        super(obj, model, -1);
    }
    /**
     *
     * @param shell Shell
     * Sets the Shell
     */
    public void setShell(Shell shell)
    {
        _shell = shell;
    }
    /**
     * @param notification -
     *            Notification
     */
    public void notifyChanged(Notification notification)
    {
    	if(notification == _isProcessed)
    		return;
    	else
    		_isProcessed = notification;
    	switch (notification.getEventType()) {
        case Notification.REMOVING_ADAPTER:
            break;
        case Notification.SET:
        	if (!notification.isTouch()) {
                boolean valid = true;
                EObject eobj = (EObject) notification.getNotifier();
                if (eobj.eClass().getName().equals(
						ClassEditorConstants.ATTRIBUTE_CLASS_NAME)
						|| eobj.eClass().getName().equals(
								ClassEditorConstants.PROV_ATTRIBUTE_CLASS_NAME)
						|| eobj.eClass().getName().equals(
								ClassEditorConstants.PM_ATTRIBUTE_CLASS_NAME)) {
                    valid = isAttributeNameUnique(eobj, notification);
                }
                if (valid
                    && !(notification.getNewValue() instanceof EObject)) {
                    valid = isModelValid();
                }

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
                isAttributeNameUnique((EObject) newVal, notification);
            }
            isModelValid();
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
                    isAttributeNameUnique(eObj, notification);
                }
            }
            isModelValid();
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
            isModelValid();
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
            isModelValid();
            break;
        }
    }
    /**
     *
     * @param eobj EObject
     * @return true if Attribute name is unique else false
     */
    private boolean isAttributeNameUnique(EObject eobj, Notification n)
    {
    	String message = null;
        EObject classObj = (EObject) _elist.get(0);
        List attrList = (List) EcoreUtils.getValue(classObj,
                ClassEditorConstants.CLASS_ATTRIBUTES);

        EObject provObj = (EObject) EcoreUtils.getValue(classObj,
				ClassEditorConstants.RESOURCE_PROVISIONING);
		EObject pmObj = (EObject) EcoreUtils.getValue(classObj,
				ClassEditorConstants.RESOURCE_PM);

		ObjectValidator validator = ValidatorFactory.getValidatorFactory()
				.getValidator(eobj.eClass());

		if (validator != null) {

			if (eobj.eClass().getName().equals(
					ClassEditorConstants.ATTRIBUTE_CLASS_NAME)) {

				if (provObj != null) {
					List provAttrList = (List) EcoreUtils.getValue(provObj,
							ClassEditorConstants.CLASS_ATTRIBUTES);
					message = validator.isValid(eobj, provAttrList, n);
				}
				if (pmObj != null) {
					List pmAttrList = (List) EcoreUtils.getValue(pmObj,
							ClassEditorConstants.CLASS_ATTRIBUTES);
					message = validator.isValid(eobj, pmAttrList, n);
				}

			} else if (eobj.eClass().getName().equals(
					ClassEditorConstants.PROV_ATTRIBUTE_CLASS_NAME)) {

				message = validator.isValid(eobj, attrList, n);
				if (pmObj != null) {
					List pmAttrList = (List) EcoreUtils.getValue(pmObj,
							ClassEditorConstants.CLASS_ATTRIBUTES);
					message = validator.isValid(eobj, pmAttrList, n);
				}

			} else if (eobj.eClass().getName().equals(
					ClassEditorConstants.PM_ATTRIBUTE_CLASS_NAME)) {

				message = validator.isValid(eobj, attrList, n);
				if (provObj != null) {
					List provAttrList = (List) EcoreUtils.getValue(provObj,
							ClassEditorConstants.CLASS_ATTRIBUTES);
					message = validator.isValid(eobj, provAttrList, n);
				}
			}
			//System.out.println(message);
			if (message != null && _shell != null
					&& message.indexOf("Duplicate") != -1) {
				MessageDialog
						.openError(
								_shell,
								"Attribute Validations",
								"Attribute names should be unique across the "
								+ "'Provisioning Attributes', 'PM Attributes'"
								+ " and 'Attributes' of the resource");

			} else {
				setValid(true);
				setTitle();
				setMessage("");
			}
		}

		return message == null ? true : false;
    }
    /**
     * @param valid - boolean
     */
	public void setValid(boolean valid)
	{
		if (_dialogPage instanceof PreferencePage) {
			PreferencePage page = (PreferencePage) _dialogPage;
			PreferencePage oldPage = (PreferencePage) ((PreferenceDialog) page.
					getContainer()).getSelectedPage();
			if( oldPage != null )
			{
				Control pageControl = oldPage.getControl();
				if (pageControl != null
                    && !pageControl.isDisposed())
					oldPage.setValid(valid);
			}
			super.setValid(valid);
		}
		
	}
}
