package com.clovis.cw.workspace.project.app;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

/**
 * Dialog to capture Prebuild ASP lib location
 * @author Pushparaj
 *
 */

public class PrebuildASPLibDialog extends Dialog{

	private String _aspLibLoc;
	private Text _libText;
	protected PrebuildASPLibDialog(Shell parentShell) {
		super(parentShell);
	}
	
	protected Control createDialogArea(Composite parent)
	{
		Composite composite = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
        layout.marginHeight = 10;
		
		composite.setLayout(layout);
		composite.setLayoutData(new GridData(GridData.FILL_BOTH));
		
		Label info = new Label(composite, SWT.WRAP);
		
		info.setText("Unable to find the prebuild ASP location. Please specify " 
				   + "the ASP lib location if prebuild ASP is already installed.\n");
		GridData data = new GridData(GridData.FILL_HORIZONTAL);
		data.horizontalSpan = 3;
		info.setLayoutData(data);
		
		Label libLabel = new Label(composite, SWT.NONE);
		libLabel.setText("Specify prebuild asp lib location:");
			
		_libText = new Text(composite, SWT.BORDER);
		GridData data1 = new GridData(GridData.FILL_HORIZONTAL);
		data1.widthHint = convertHorizontalDLUsToPixels(20);
		_libText.setLayoutData(data1);
		_libText.setEditable(false);
		
		Button libButton = new Button(composite, SWT.NONE);
		libButton.setText("Browse...");
		libButton.addSelectionListener(new SelectionListener() {
            public void widgetSelected(SelectionEvent e)
            {
                DirectoryDialog dialog =
                    new DirectoryDialog(getShell(), SWT.NONE);
                String fileName = dialog.open();
                if(fileName != null) {
                	_libText.setText(fileName);
                }
            }

			public void widgetDefaultSelected(SelectionEvent e) {}
		});
		getShell().setText("Prebuild ASP Lib Location");
		return composite;
	}
	@Override
	public void okPressed() {
		_aspLibLoc = _libText.getText(); 
		super.okPressed();
	}
	/**
	 * Returrns Prebuild ASP lib location
	 * @return String
	 */
	public String getASPLibLocation() {
		return _aspLibLoc;
	}
	
	/*
     * Set an initial size for the dialog that is reasonable.
     * 
     * @see org.eclipse.jface.dialogs.Dialog#getInitialSize()
     */
	protected Point getInitialSize()
    {
        Point shellSize = super.getInitialSize();
        return new Point(Math.max(convertHorizontalDLUsToPixels(400), shellSize.x),
                Math.max(convertVerticalDLUsToPixels(60), shellSize.y));
    }

}
