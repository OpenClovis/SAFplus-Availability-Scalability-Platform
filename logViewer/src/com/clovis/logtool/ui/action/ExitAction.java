/**
 * 
 */
package com.clovis.logtool.ui.action;

import java.net.URL;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.resource.ImageDescriptor;

import com.clovis.logtool.ui.LogDisplay;

/**
 * Exit Action for the log tool application.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ExitAction extends Action {

	/**
	 * Constructs Exit Action instance.
	 */
	public ExitAction() {
		setText("E&xit@Ctrl+X");
		setToolTipText("Exit LogTool");

		try {
			setImageDescriptor(ImageDescriptor.createFromURL(new URL(
					"file:icons/logExit.png")));
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.action.IAction#run()
	 */
	public void run() {
		LogDisplay.getInstance().close();
	}
}
