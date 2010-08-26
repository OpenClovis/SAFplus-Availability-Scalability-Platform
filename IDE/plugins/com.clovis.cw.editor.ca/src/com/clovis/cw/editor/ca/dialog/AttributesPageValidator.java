/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.editor.ca/src/com/clovis/cw/editor/ca/dialog/AttributesPageValidator.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.editor.ca.dialog;

import java.util.Iterator;
import java.util.List;

import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.preference.IPreferenceNode;
import org.eclipse.jface.preference.PreferenceDialog;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferencePage;

import com.clovis.common.utils.ecore.Model;
import com.clovis.common.utils.ui.DialogValidator;

public class AttributesPageValidator extends DialogValidator
{

	public AttributesPageValidator(Object obj, Model model)
	{
		super(obj, model);
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
            	if (_dialogPage instanceof PreferencePage
            		&& type == IMessageProvider.ERROR) {
            		((PreferencePage) _dialogPage).setValid(false);
            	} else if (_dialogPage instanceof PreferencePage
            		&& type == IMessageProvider.NONE) {
            		PreferenceDialog dialog = (PreferenceDialog)
            		((PreferencePage) _dialogPage).getContainer();
            		if (dialog != null) {
            			IPreferenceNode resourceNode = null;
        				List nodes = dialog.getPreferenceManager().getElements(PreferenceManager.POST_ORDER);
        				for (Iterator i = nodes.iterator(); i.hasNext();) {
        					IPreferenceNode node = (IPreferenceNode) i.next();
        					if (node.getId().equals("resource")) {
        						resourceNode = node;
        					}
        				}
		        	    if (resourceNode != null) {
		        	        ClassPropertiesPage page = (ClassPropertiesPage) resourceNode.
		        	            getPage();
		        	        if (page.getValidator().isValid()) {
		        	        	((PreferencePage) _dialogPage).setValid(true);
		        	        }
		        	    }
            		}	
            	}
                _dialogPage.setMessage(message, type);
            }
        }
    }
    public void setValid(boolean valid)
	{
		if (_dialogPage instanceof PreferencePage) {
			PreferenceDialog dialog = (PreferenceDialog)
    		((PreferencePage) _dialogPage).getContainer();
			if (dialog != null) {
				IPreferenceNode resourceNode = null;
				List nodes = dialog.getPreferenceManager().getElements(PreferenceManager.POST_ORDER);
				for (Iterator i = nodes.iterator(); i.hasNext();) {
					IPreferenceNode node = (IPreferenceNode) i.next();
					if (node.getId().equals("resource")) {
						resourceNode = node;
					}
				}
				if (resourceNode != null) {
		        ClassPropertiesPage page = (ClassPropertiesPage)
		        resourceNode.getPage();
		        	if (page.getValidator().isValid()) {
		        	super.setValid(valid);
		        	}
			}
			}
		}	
	}
}
