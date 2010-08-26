/**
 * 
 */
package com.clovis.logtool.ui.action;

import java.net.URL;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;

import com.clovis.logtool.ui.LogDisplay;
import com.clovis.logtool.ui.filter.FilterDialog;

/**
 * Filter Action for the log tool application.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class FilterAction extends Action {

	/**
	 * Construts Filter Action instance.
	 */
	public FilterAction() {
		setText("Advanced &Filter@Ctrl+F");
		setToolTipText("Create Advanced Filter");

		try {
			setImageDescriptor(ImageDescriptor.createFromURL(new URL(
					"file:icons/logFilter.png")));
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
		FilterDialog filterDialog = new FilterDialog(LogDisplay.getInstance()
				.getShell(), false);
		filterDialog.open();
	}
}
