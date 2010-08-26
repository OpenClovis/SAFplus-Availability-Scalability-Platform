/**
 * 
 */
package com.clovis.cw.editor.ca.dialog;

import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.preference.PreferencePage;

/**
 * Generic Page for the Preference View.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public abstract class GenericPreferencePage extends PreferencePage {

	private boolean isErrorFree = true;

	/**
	 * Constructor.
	 * 
	 * @param title
	 */
	public GenericPreferencePage(String title) {
		super(title);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#isValid()
	 */
	@Override
	public boolean isValid() {
		return true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#setValid(boolean)
	 */
	@Override
	public void setValid(boolean valid) {
		isErrorFree = valid;
		GenericPreferenceDialog dialog = (GenericPreferenceDialog) getShell()
				.getData();

		dialog.getButton(IDialogConstants.OK_ID).setEnabled(valid);
		if (dialog.getButton(-1) != null) {
			dialog.getButton(-1).setEnabled(valid);
		}
	}

	/**
	 * @return the isErrorFree
	 */
	public boolean isErrorFree() {
		return isErrorFree;
	}
}
