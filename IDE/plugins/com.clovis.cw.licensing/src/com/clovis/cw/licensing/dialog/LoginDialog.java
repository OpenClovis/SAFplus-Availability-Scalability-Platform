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
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

import com.clovis.cw.licensing.Activator;
import com.clovis.cw.licensing.UserInfo;
import com.clovis.cw.licensing.server.LicenseServerCommunicator;

/**
 * 
 * @author Pushparaj
 * Dialog to capture user's log-in information
 */
public class LoginDialog extends Dialog {
	private Text _loginText, _pwdText;
	private String _loginName, _password;
	private Label _errorDispLabel;
	/**
	 * Creates LoginDialog instance 
	 * @param shell parent Shell
	 */
	public LoginDialog(Shell shell) {
		super(shell);
		// TODO Auto-generated constructor stub
	}
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createDialogArea(Composite parent)
    {
        Composite container = new Composite(parent, SWT.NONE);
        GridLayout containerLayout = new GridLayout();
        containerLayout.numColumns = 1;
        container.setLayout(containerLayout);
        container.setLayoutData(new GridData(GridData.FILL_BOTH));
        
        Label loginLabel = new Label(container, SWT.NONE);
        loginLabel.setText("E-mail:");
        _loginText = new Text(container, SWT.BORDER);
        _loginText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        new Label(container, SWT.NONE).setText("(e.g. rhyme@gmail.com)");
        new Label(container, SWT.NONE);
        
        Label pwdLabel = new Label(container, SWT.NONE);
        pwdLabel.setText("Password:");
        _pwdText = new Text(container, SWT.BORDER |SWT.PASSWORD);
        _pwdText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        new Label(container, SWT.NONE);
        
        _errorDispLabel = new Label(container, SWT.NONE);
        _errorDispLabel.setForeground(Display.getDefault().getSystemColor(SWT.COLOR_RED));
        GridData data = new GridData(GridData.FILL_BOTH);
        data.horizontalSpan = 2;
        _errorDispLabel.setLayoutData(data);
        
        getShell().setText("Sign in to License Server");
        
        return container;
    }
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
	 */
	protected void createButtonsForButtonBar(Composite parent)
	{
		Button loginButton = createButton(parent, IDialogConstants.YES_ID, "Sign In",
				true);
		loginButton.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {
			}
			public void widgetSelected(SelectionEvent e) {
				_loginName = _loginText.getText().trim();
				_password = _pwdText.getText().trim();
				if(_loginName.equals("") || _password.equals("")) {
					_errorDispLabel.setText("Invalid Login Name or Password. Please try again.");
					return;
				}
				LicenseServerCommunicator servCom = new LicenseServerCommunicator();
				int errCode = servCom.login(_loginName, _password); 
				if(errCode == 1){
					_errorDispLabel.setText("");
					UserInfo info = Activator.getUserInfo();
					info.setLoginName(_loginName);
					info.setPassword(_password);
					info.setUserVerification(true);
					LicenseWarningDialog dialog = new LicenseWarningDialog(getShell());
					if(LicenseWarningDialog.getRetVal() != LicenseWarningDialog.RET_ALWAYS_OK) {
						dialog.open();
					}
					okPressed();
				} else {
					if(errCode == -2) {
						_errorDispLabel.setText("Login information incorrect, or unverified.");
					} else if(errCode == -3) {
						_errorDispLabel.setText("Your trial has expired.  Please contact Openclovis.");
					} else {
						_errorDispLabel.setText("UnKnown Error.");
					}
					UserInfo info = Activator.getUserInfo();
					info.setLoginName("");
					info.setPassword("");
					info.setUserVerification(false);
					//cancelPressed();
				}
			}
        });

		Button cancelButton = createButton(parent, IDialogConstants.CANCEL_ID, IDialogConstants.CANCEL_LABEL,
				false);
		cancelButton.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {
			}
			public void widgetSelected(SelectionEvent e) {
				cancelPressed();
			}
        });
	}
	/**
	 * Returns log-in name
	 * @return _loginName
	 */
	public String getLoginName() {
		return _loginName;
	}
	/**
	 * Returns password
	 * @return _password
	 */
	public String getPassword() {
		return _password;
	}
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	protected void okPressed() {
		super.okPressed();
	}
	/**
	 * (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#getInitialSize()
	 */
	protected Point getInitialSize()
    {
        Point shellSize = super.getInitialSize();
        return new Point(Math.max(convertHorizontalDLUsToPixels(200), shellSize.x),
                Math.max(convertVerticalDLUsToPixels(80), shellSize.y));
    }
}
