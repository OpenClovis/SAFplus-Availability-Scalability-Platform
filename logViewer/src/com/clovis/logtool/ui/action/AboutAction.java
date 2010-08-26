/**
 * 
 */
package com.clovis.logtool.ui.action;

import java.net.URL;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;

import com.clovis.logtool.ui.AboutDialog;
import com.clovis.logtool.ui.LogDisplay;

/**
 * About Action for the log tool application.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class AboutAction extends Action {

	/**
	 * Constructs the About Action instance.
	 */
	public AboutAction() {
		setText("&About");
		setToolTipText("About LogTool");

		try {
			setImageDescriptor(ImageDescriptor.createFromURL(new URL(
					"file:icons/clovisLogo.gif")));
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
		AboutDialog aboutDialog = new AboutDialog(LogDisplay.getInstance()
				.getShell());
		aboutDialog.open();
	}
}
