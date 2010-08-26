/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/DeploymentPreferencePage.java $
 * $Author: bkpavan $
 * $Date: 2007/01/03 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.io.File;
import java.util.HashMap;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;

import com.clovis.cw.workspace.builders.DeployImages;
import com.clovis.cw.workspace.builders.MakeImages;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * @author Pushparaj
 * 
 * Dialog for Deployment
 */
public class DeploymentPreferencePage extends PreferencePage {

	private Text _addressText, _userText, _passwordText, _locationText;
	private String _nodeInstanceName = "";
	private String _remoteAddress, _userName, _password, _remoteLocation = null;
	private IProject _project;
	public static final String TARGET_ADDRESS = "&Target Address";
	public static final String USERNAME = "&User Name";
	public static final String PASSWORD = "&Password";
	public static final String TARGET_LOCATION ="&Target Location";
	
	public DeploymentPreferencePage(IProject project, String name) {
		super(name);
		_nodeInstanceName = name;
		_project = project;
	}
	
	private void setDefaultValues() {
		if(_addressText.getText().trim().equals("")) {
		_remoteAddress = getTargetAddress();
		_addressText.setText(_remoteAddress);
		}
		if(_userText.getText().trim().equals("")) {
		_userName = getUserName();
		_userText.setText(_userName);
		}
		if(_passwordText.getText().trim().equals("")) {
		_password = getPassword();
		_passwordText.setText(_password);
		}
		if(_locationText.getText().trim().equals("")) {
		_remoteLocation = getLocation();
		_locationText.setText(_remoteLocation);
		}
	}
	
	private void setValues() {
		_remoteAddress = getTargetAddress();
		_addressText.setText(_remoteAddress);
		_userName = getUserName();
		_userText.setText(_userName);
		_password = getPassword();
		_passwordText.setText(_password);
		_remoteLocation = getLocation();
		_locationText.setText(_remoteLocation);
	}
	
	private String getTargetAddress() {
		try {
            _remoteAddress = _project.getPersistentProperty(
                    new QualifiedName(_nodeInstanceName, TARGET_ADDRESS));
            if(_remoteAddress == null || _remoteAddress.equals("")) {
            	_remoteAddress = _project.getPersistentProperty(
                        new QualifiedName(_project.getName(), TARGET_ADDRESS));
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return _remoteAddress != null ? _remoteAddress : "";
	}
	private String getUserName() {
		try {
            _userName = _project.getPersistentProperty(
                    new QualifiedName(_nodeInstanceName, USERNAME));
            if(_userName == null || _userName.equals("")) {
            	_userName = _project.getPersistentProperty(
                        new QualifiedName(_project.getName(), USERNAME));
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return _userName != null ? _userName : "";
	}
	private String getPassword() {
		try {
            _password = _project.getPersistentProperty(
                    new QualifiedName(_nodeInstanceName, PASSWORD));
            if(_password == null || _password.equals("")) {
            	_password = _project.getPersistentProperty(
                        new QualifiedName(_project.getName(), PASSWORD));
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return _password != null ? _password : "";
	}
	private String getLocation() {
		try {
            _remoteLocation = _project.getPersistentProperty(
                    new QualifiedName(_nodeInstanceName, TARGET_LOCATION));
            if(_remoteLocation == null || _remoteLocation.equals("")) {
            	_remoteLocation = _project.getPersistentProperty(
                        new QualifiedName(_project.getName(), TARGET_LOCATION));
            }
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return _remoteLocation != null ? _remoteLocation : "";
	}
	
	protected Control createContents(Composite parent) {
		Composite container = new Composite(parent, SWT.NONE);
		GridLayout containerLayout = new GridLayout();
		containerLayout.numColumns = 2;
		container.setLayout(containerLayout);
		GridData containerData = new GridData(GridData.FILL_BOTH);
		container.setLayoutData(containerData);
		Label addressLabel = new Label(container, SWT.NONE);
		addressLabel.setLayoutData(new GridData(GridData.BEGINNING));
		addressLabel.setText("Target address:");
		_addressText = new Text(container, SWT.BORDER);
		_addressText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_addressText.addModifyListener(new ModifyListener(){
			public void modifyText(ModifyEvent e) {
				_remoteAddress = _addressText.getText().trim();
				try {
					_project.setPersistentProperty(new QualifiedName(_nodeInstanceName,
							TARGET_ADDRESS), _remoteAddress);
					_project.setPersistentProperty(new QualifiedName(_project.getName(),
								TARGET_ADDRESS), _remoteAddress);
				} catch (CoreException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
			}});
		Label userLabel = new Label(container, SWT.NONE);
		userLabel.setLayoutData(new GridData(GridData.BEGINNING));
		userLabel.setText("User name:");
		_userText = new Text(container, SWT.BORDER);
		_userText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_userText.addModifyListener(new ModifyListener(){;
			public void modifyText(ModifyEvent e) {
				_userName = _userText.getText().trim();
				try {
					_project.setPersistentProperty(new QualifiedName(_nodeInstanceName,
							USERNAME), _userName);
					_project.setPersistentProperty(new QualifiedName(_project.getName(),
								USERNAME), _userName);
				} catch (CoreException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
			}});
		Label passwordLabel = new Label(container, SWT.NONE);
		passwordLabel.setLayoutData(new GridData(GridData.BEGINNING));
		passwordLabel.setText("Password:");
		_passwordText = new Text(container, SWT.BORDER | SWT.PASSWORD);
		_passwordText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_passwordText.addModifyListener(new ModifyListener(){
			public void modifyText(ModifyEvent e) {
				_password = _passwordText.getText().trim();
				try {
					_project.setPersistentProperty(new QualifiedName(_nodeInstanceName,
							PASSWORD), _password);
					_project.setPersistentProperty(new QualifiedName(_project.getName(),
								PASSWORD), _password);
				} catch (CoreException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
			}});
		Label locationLabel = new Label(container, SWT.NONE);
		locationLabel.setLayoutData(new GridData(GridData.BEGINNING));
		locationLabel.setText("Target location:");
		_locationText = new Text(container, SWT.BORDER);
		_locationText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_locationText.addModifyListener(new ModifyListener(){
			public void modifyText(ModifyEvent e) {
				_remoteLocation = _locationText.getText().trim();
				try {
					_project.setPersistentProperty(new QualifiedName(_nodeInstanceName,
							TARGET_LOCATION), _remoteLocation);
					_project.setPersistentProperty(new QualifiedName(_project.getName(),
								TARGET_LOCATION), _remoteLocation);
				} catch (CoreException e1) {
					// TODO Auto-generated catch block
					e1.printStackTrace();
				}
			}});
		setValues();
		setTitle("Please provide valid information to deploy image");
		return container;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#createControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createControl(Composite parent) {
		// TODO Auto-generated method stub
		super.createControl(parent);
		getApplyButton().setText("De&ploy");
	}

	/*
	 * (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	protected void performApply() {
		performOk();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	protected void performDefaults() {
		_addressText.setText("");
		_userText.setText("");
		_passwordText.setText("");
		_locationText.setText("");
    }
	/*
	 * (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#isValid()
	 */
	public boolean isValid(){
		/*if ((_remoteAddress != null && _remoteAddress.equals(""))
				|| (_userName != null && _userName.equals(""))
				|| (_password != null && _password.equals(""))
				|| (_remoteLocation != null && _remoteLocation.equals(""))) {
			return false;
		}*/
		return true;
	}
	/*
	 * (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	public boolean performOk() {
		try {
			if(!getTargetAddress().equals("")) {
				HashMap makeSettings = MakeImages.readTargetConf(_project);
				boolean tarballSetting = true;
				boolean specificImages = false;
		    	if (makeSettings.get(MakeImages.CREATE_TARBALLS_KEY) != null)
		    	{
		    		if (((String)makeSettings.get(MakeImages.CREATE_TARBALLS_KEY)).toLowerCase().equals("no"))
		    		{
		    	    	tarballSetting = false;
		    		}
		    	}
		    	if (makeSettings.get(MakeImages.INSTANTIATE_IMAGES_KEY) != null)
		    	{
		    		if (((String)makeSettings.get(MakeImages.INSTANTIATE_IMAGES_KEY)).toLowerCase().equals("yes"))
		    		{
		    			specificImages = true;
		    		}
		    	}
				String imageName = _nodeInstanceName;
				if(!specificImages) {
					String arch = (String) makeSettings.get(MakeImages.ARCH_KEY);
					if(tarballSetting) {
						String values[]= arch.split("/");
						if(values.length == 2) {
							imageName = values[0] + "-" + values[1];
						}
					} else {
						imageName = arch;
					}
				}
				String imagePath = CwProjectPropertyPage
							.getProjectAreaLocation(_project)
							+ File.separator
							+ "target"
							+ File.separator
							+ _project.getName()
							+ File.separator
							+ "images";
							
				new DeployImages().upLoadFile(_nodeInstanceName, imageName, imagePath, getUserName(), getPassword(),
						getTargetAddress(), getLocation(), _project, tarballSetting, specificImages);
			}
		} catch (Exception e) {
			return false;
		}
		return true;
	}
	/*
	 * (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.DialogPage#setVisible(boolean)
	 */
	public void setVisible(boolean visible) {
		if(visible)
			setDefaultValues();
        super.setVisible(visible);
    }
}
