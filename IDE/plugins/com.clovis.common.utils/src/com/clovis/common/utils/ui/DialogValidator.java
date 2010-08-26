/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.common.utils/src/com/clovis/common/utils/ui/DialogValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.common.utils.ui;

import java.util.List;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.jface.dialogs.DialogPage;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.jface.wizard.WizardPage;
import org.eclipse.swt.widgets.Button;

import com.clovis.common.utils.constants.ModelConstants;
import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.common.utils.ecore.Model;

/**
 * @author shubhada Dialog Validator Class Which Does specific validation
 */
public class DialogValidator extends ModelValidator
{
    protected TitleAreaDialog  _tdialog    = null;

    protected PreferenceDialog _pdialog    = null;

    protected DialogPage       _dialogPage = null;
    
    protected Button           _okButton   = null;
    
    protected Button          _applyButton = null;
    
    /**
     * Empty Constructor
     */
    public DialogValidator() {
    }
    /**
     *
     * @param obj -
     *            Object
     * @param model Model
     *
     */
    public DialogValidator(Object obj, Model model)
    {
        this(obj, model, 2);
    }
    /**
    *
    * @param obj -
    *            Object
    * @param model Model
    * @param depth - Depth at which listener to be attached
    *
    */
   public DialogValidator(Object obj, Model model, int depth)
   {
       super(model, depth);
       if (obj instanceof TitleAreaDialog) {
           _tdialog = (TitleAreaDialog) obj;
       } else if (obj instanceof PreferenceDialog) {
           _pdialog = (PreferenceDialog) obj;
       } else if (obj instanceof DialogPage) {
           _dialogPage = (DialogPage) obj;
       }
   }
   /**
    * Sets the title back in place only in preference 
    * page.
    *
    */
   public void setTitle()
   {
       if (_dialogPage != null) {
            if (_dialogPage.getControl() != null
                && !_dialogPage.getControl().isDisposed()) {
                if (_dialogPage instanceof PreferencePage) {
                    PreferencePage page = (PreferencePage) _dialogPage;
                    page.setTitle(page.getTitle());
                }
            }
       }
   }
   /**
    * @param message -
    *            Message to set
    *
    */
   public void setMessage(String message)
   {
       if (message != null) {
           int type = isValid() ? IMessageProvider.NONE
                   : IMessageProvider.ERROR;
           if (_tdialog != null) {
               _tdialog.setMessage(message, type);
           } else if (_pdialog != null) {
               _pdialog.setMessage(message, type);
           } else if (_dialogPage != null) {
           	if (_dialogPage.getControl() != null
           		&& !_dialogPage.getControl().isDisposed()) {
           		if (_dialogPage instanceof PreferencePage) {
	           		if (type == IMessageProvider.ERROR) {
	           			((PreferencePage) _dialogPage).setValid(false);
	           		} else if (type == IMessageProvider.NONE) {
	           			((PreferencePage) _dialogPage).setValid(true);
	           		}
           		} else if (_dialogPage instanceof WizardPage) {
                 if (type == IMessageProvider.ERROR) {
                        ((WizardPage) _dialogPage).setPageComplete(false);
                    } else if (type == IMessageProvider.NONE) {
                        ((WizardPage) _dialogPage).setPageComplete(true);
                    }
                }
               _dialogPage.setMessage(message, type);
           	}
           }
       }
   }
    /**
     * @param valid - sets the model to be valid or not
     */
    public void setValid(boolean valid)
    {
        if (_okButton != null && !_okButton.isDisposed()) {
            if (valid) {
                _okButton.setEnabled(true);
            } else {
                _okButton.setEnabled(false);
            }
        }
        if (_applyButton != null && !_applyButton.isDisposed()) {
            if (valid) {
                _applyButton.setEnabled(true);
            } else {
                _applyButton.setEnabled(false);
            }
        }
        super.setValid(valid);
    }
    /**
     *
     * @param okButton OK Button instance
     */
    public void setOKButton(Button okButton)
    {
        _okButton = okButton;
    }
    /**
    *
    * @param applyButton  - Apply Button instance
    */
   public void setApplyButton(Button applyButton)
   {
       _applyButton = applyButton;
   }
    /**
     *
     * @return true if the model is valid else false
     */
    public boolean isModelValid()
    {
        for (int i = 0; i < _elist.size(); i++) {
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
     * @param eobj
     * @param n
     * @return
     */
    private boolean isObjectValid(EObject eobj, Notification n)
    {
        String message = isValid(eobj, n);
        if (message != null) {
            setValid(false);
            setMessage(message);
            return false;
        } 
        setValid(true);
        setMessage("");
        setTitle();
        return true;
    }
    
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

}
