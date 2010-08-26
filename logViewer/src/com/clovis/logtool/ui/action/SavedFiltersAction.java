/**
 * 
 */
package com.clovis.logtool.ui.action;

import java.net.URL;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;

import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.ui.filter.SavedFiltersDialog;

/**
 * Saved Filters Action for the log tool application.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class SavedFiltersAction extends Action {

	/**
	 * Construts Filter Action instance.
	 */
	public SavedFiltersAction() {
		setText("&Saved Filters@Ctrl+S");
		setToolTipText("Saved Filters");

		try {
			setImageDescriptor(ImageDescriptor.createFromURL(new URL(
					"file:icons/logSavedFilters.png")));
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.action.Action#run()
	 */
	public void run() {
		SavedFiltersDialog dialog = new SavedFiltersDialog(LogDisplay
				.getInstance().getShell());
		dialog.open();
	}
}
