package com.clovis.cw.licensing.dialog;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;

/**
 * 
 * @author Pushparaj
 * Dialog for display license information
 */
public class LicenseWarningDialog extends Dialog {

	public static final int RET_OK    = 0;
	public static final int RET_ALWAYS_OK     = 1;
	public static final int RET_CANCEL = 2;
	
	private static int _retVal = RET_OK;
	
	/**
	 * Creates Warning Dialog instance
	 * @param parentShell Shell obj
	 */
	public LicenseWarningDialog(Shell parentShell) {
		super(parentShell);
		// TODO Auto-generated constructor stub
	}
	
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createDialogArea(Composite parent)
	{
		Composite composite = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
        layout.marginHeight = 10;
        layout.marginWidth = 10;
		
		composite.setLayout(layout);
		composite.setLayoutData(new GridData(GridData.FILL_BOTH));
		
		Label question = new Label(composite, SWT.WRAP);
		
		question.setText("By using this 'preview' you agrees that the model and all code shall\n" 
				+ "be LGPL-licensed and understands that since the model is being sent to\n" 
				+ "OpenClovis for transformation, this constitutes a 'distribution'. ");
		GridData data = new GridData(GridData.FILL_HORIZONTAL);
		question.setLayoutData(data);
		
		getShell().setText("License Warning");

		return composite;
	}
	
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
	 */
	protected void createButtonsForButtonBar(Composite parent)
	{
		Button yesButton = createButton(parent, IDialogConstants.OK_ID, IDialogConstants.OK_LABEL,
				true);
		yesButton.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {
			}
			public void widgetSelected(SelectionEvent e) {
				_retVal = RET_OK;
				okPressed();
			}
        });
		
		Button alwaysOKButton = createButton(parent, IDialogConstants.YES_TO_ALL_ID, "Always OK",
				false);
		alwaysOKButton.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {
			}
			public void widgetSelected(SelectionEvent e) {
				_retVal = RET_ALWAYS_OK;
				okPressed();
			}
        });
		
		Button cancelButton = createButton(parent, IDialogConstants.CANCEL_ID, IDialogConstants.CANCEL_LABEL,
				false);
		cancelButton.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {
			}
			public void widgetSelected(SelectionEvent e) {
				_retVal = RET_CANCEL;
				okPressed();
			}
        });
	}
	/**
	 * Return a string representing the button which was clicked by the user.
	 */
	public static int getRetVal()
	{
		return _retVal;
	}

    /**
     * Set an initial size for the dialog that is reasonable.
     * 
     * @see org.eclipse.jface.dialogs.Dialog#getInitialSize()
     */
	protected Point getInitialSize()
    {
        Point shellSize = super.getInitialSize();
        return new Point(Math.max(convertHorizontalDLUsToPixels(320), shellSize.x),
                Math.max(convertVerticalDLUsToPixels(75), shellSize.y));
    }
}
