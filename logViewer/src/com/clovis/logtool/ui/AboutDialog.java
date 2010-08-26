/**
 * 
 */
package com.clovis.logtool.ui;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;

/**
 * About Dialog for the Log Tool.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class AboutDialog extends Dialog {

	/**
	 * Constructs the About Dialog.
	 * 
	 * @param shell
	 *            the parent shell
	 */
	public AboutDialog(Shell shell) {
		super(shell);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#createContents(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createContents(Composite parent) {
		GridLayout layout = new GridLayout();
		parent.setLayout(layout);

		Label label = new Label(parent, SWT.FLAT | SWT.BORDER);
		label
				.setImage(new Image(getShell().getDisplay(),
						"icons/logAbout.bmp"));

		GridData layoutData = new GridData(SWT.FILL, SWT.FILL, true, true);
		label.setLayoutData(layoutData);
		return label;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	protected void configureShell(Shell shell) {
		super.configureShell(shell);
		shell.setText("About Log Tool");
	}
}
