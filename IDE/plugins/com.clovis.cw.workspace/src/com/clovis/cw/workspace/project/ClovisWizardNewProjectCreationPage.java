/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/ClovisWizardNewProjectCreationPage.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.project;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.regex.Pattern;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.WizardNewProjectCreationPage;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.workspace.WorkspacePlugin;

/**
 * Standard main page for a wizard that is creates a project resource.
 * <p>
 * This page may be used by clients as-is; it may be also be subclassed to suit.
 * </p>
 * <p>
 * Example useage:
 * <pre>
 * mainPage = new WizardNewProjectCreationPage("basicNewProjectPage");
 * mainPage.setTitle("Project");
 * mainPage.setDescription("Create a new project resource.");
 * </pre>
 * </p>
 */
public class ClovisWizardNewProjectCreationPage extends WizardNewProjectCreationPage  
{

	private static final String OWNER_TITLE   = "&SDK Location:";
    private Text _locationText;
    private static final String PROJECT_AREA_LOCATION_TITLE = "Project &Area Location:";
    private Text _projectAreaLocationText;
    private static final String PYTHON_TITLE   = "&Python Location:";
    private Text _pythonLocationText;
    private static final String CG_MODE_TITLE = "&Code generation mode";
	private CCombo _codeGenModeCombo;
	private String _locationSDK = "", _pythonLocation = "",
			_projectAreaLocation = "", _codeGenMode = "openclovis";
    /**
     * Creates a new project creation wizard page.
     *
     * @param pageName the name of this page
     */
    public ClovisWizardNewProjectCreationPage(String pageName) 
    {
        super(pageName);
    }
    /**
     * @see org.eclipse.jface.dialogs.IDialogPage#createControl(org.eclipse.swt.widgets.Composite)
     */
    public void createControl(Composite parent) {
		super.createControl(parent);
		Composite control = (Composite) getControl();
		Group composite = new Group(control, SWT.BORDER);
		composite.setText("Project properties");
		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		composite.setLayout(layout);
		composite.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));

		Label ownerLabel = new Label(composite, SWT.NONE);
		ownerLabel.setText(OWNER_TITLE);
		_locationText = new Text(composite, SWT.SINGLE | SWT.BORDER);
		_locationText.setEditable(true);
		_locationText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_locationSDK = DataPlugin.getDefaultSDKLocation();
		_locationText.setText(_locationSDK);
		_locationText.addListener(SWT.Modify, new Listener() {
			public void handleEvent(Event e) {
				if (_pythonLocationText.getText().trim().equals("")
						&& isValidSDKLocation(_locationText.getText().trim())) {
					String installLocation = new File(_locationText.getText()
							.trim()).getParentFile().getAbsolutePath();
					if (new File(installLocation + File.separator
							+ "buildtools" + File.separator + "local"
							+ File.separator + "bin" + File.separator
							+ "python").exists()) {
						_pythonLocation = installLocation + File.separator
								+ "buildtools" + File.separator + "local"
								+ File.separator + "bin";
						_pythonLocationText.setText(_pythonLocation);
					}
				}
				setPageComplete(validatePage());
			}
		});
		Button button = new Button(composite, SWT.PUSH);
		button.setText("Browse...");
		button.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				DirectoryDialog dialog = new DirectoryDialog(getShell(),
						SWT.NONE);
				dialog.setFilterPath(UtilsPlugin
						.getDialogSettingsValue("SDKLOCATION1"));
				String fileName = dialog.open();
				UtilsPlugin.saveDialogSettings("SDKLOCATION1", fileName);
				if (fileName != null) {
					if (isValidSDKLocation(fileName)) {
						_locationText.setText(fileName);
						_locationSDK = fileName;
						if (_pythonLocationText.getText().trim().equals("")) {
							String installLocation = new File(_locationSDK)
									.getParentFile().getAbsolutePath();
							if (new File(installLocation + File.separator
									+ "buildtools" + File.separator + "local"
									+ File.separator + "bin" + File.separator
									+ "python").exists()) {
								_pythonLocation = installLocation
										+ File.separator + "buildtools"
										+ File.separator + "local"
										+ File.separator + "bin";
								_pythonLocationText.setText(_pythonLocation);
							}
						}
					} else {
						MessageDialog.openError(getShell(), "Warning", fileName
								+ " is not a valid SDK location.");
					}
				}
			}

			public void widgetDefaultSelected(SelectionEvent e) {
			}
		});
		Label projectLabel = new Label(composite, SWT.NONE);
		projectLabel.setText(PROJECT_AREA_LOCATION_TITLE);
		_projectAreaLocationText = new Text(composite, SWT.SINGLE | SWT.BORDER);
		_projectAreaLocationText.setEditable(true);
		_projectAreaLocationText
				.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_projectAreaLocation = DataPlugin.getProjectAreaLocation();
		_projectAreaLocationText.setText(_projectAreaLocation);
		_projectAreaLocationText.addListener(SWT.Modify, new Listener() {
			public void handleEvent(Event e) {
				setPageComplete(validatePage());
			}
		});
		Button btnBr = new Button(composite, SWT.PUSH);
		btnBr.setText("Browse...");
		btnBr.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				DirectoryDialog dialog = new DirectoryDialog(getShell(),
						SWT.NONE);
				dialog.setFilterPath(UtilsPlugin
						.getDialogSettingsValue("PROJECTAREALOCATION1"));
				String fileName = dialog.open();
				UtilsPlugin.saveDialogSettings("PROJECTAREALOCATION1", fileName);
				if (fileName != null) {
					_projectAreaLocationText.setText(fileName);
					_projectAreaLocation = fileName;
				}
			}

			public void widgetDefaultSelected(SelectionEvent e) {
			}
		});

		Label pythonLabel = new Label(composite, SWT.NONE);
		pythonLabel.setText(PYTHON_TITLE);
		_pythonLocationText = new Text(composite, SWT.SINGLE | SWT.BORDER);
		_pythonLocationText.setEditable(true);
		_pythonLocationText
				.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		_pythonLocation = DataPlugin.getPythonLocation();

		if (_pythonLocation.equals("")) {
			Runtime runtime = Runtime.getRuntime();
			try {
				Process process = runtime.exec("which python");
				BufferedReader inputReader = new BufferedReader(
						new InputStreamReader(process.getInputStream()));
				String path = inputReader.readLine();

				if (path != null && !path.equals("")) {
					_pythonLocation = path.substring(0, path
							.lastIndexOf(File.separator));
				}
			} catch (IOException e1) {
				e1.printStackTrace();
			}
		}

		_pythonLocationText.setText(_pythonLocation);
		_pythonLocationText.addListener(SWT.Modify, new Listener() {
			public void handleEvent(Event e) {
				setPageComplete(validatePage());
			}
		});
		Button btnBrowse = new Button(composite, SWT.PUSH);
		btnBrowse.setText("Browse...");
		btnBrowse.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				DirectoryDialog dialog = new DirectoryDialog(getShell(),
						SWT.NONE);
				dialog.setFilterPath(UtilsPlugin
						.getDialogSettingsValue("PYTHONLOCATION1"));
				String fileName = dialog.open();
				UtilsPlugin.saveDialogSettings("PYTHONLOCATION1", fileName);
				if (fileName != null) {
					_pythonLocationText.setText(fileName);
					_pythonLocation = fileName;
				}
			}

			public void widgetDefaultSelected(SelectionEvent e) {
			}
		});

		Label cgModeLabel = new Label(composite, SWT.NONE);
		cgModeLabel.setText(CG_MODE_TITLE);
		_codeGenModeCombo = new CCombo(composite, SWT.BORDER | SWT.READ_ONLY);
		_codeGenModeCombo.setLayoutData(new GridData(SWT.FILL, SWT.CENTER,
				true, false, 2, 0));
		String items[] = WorkspacePlugin.getCodeGenOptions();
        _codeGenModeCombo.setItems(CwProjectPropertyPage.convertCodegenOptionsInUIForm(items));
		_codeGenModeCombo.select(_codeGenModeCombo.indexOf(_codeGenMode));

		_codeGenModeCombo.addModifyListener(new ModifyListener() {
			public void modifyText(ModifyEvent e) {
				_codeGenMode = CwProjectPropertyPage.convertCodegenOptionFromUIForm(_codeGenModeCombo.getText());
			}
		});

		Composite bottomControl = new Composite(control, SWT.NONE);
		GridData data = new GridData(GridData.FILL_BOTH);
		bottomControl.setLayoutData(data);
		RowLayout rowlayout = new RowLayout();
		rowlayout.marginTop = 44;
		rowlayout.marginBottom = 0;
		rowlayout.pack = true;
		bottomControl.setLayout(rowlayout);
		Group descGroup = new Group(bottomControl, SWT.BORDER);
		descGroup.setText("Note:");
		RowLayout descLayout = new RowLayout(SWT.VERTICAL);
		descLayout.marginHeight = 4;
		descLayout.marginWidth = 14;
		descGroup.setLayout(descLayout);
		Label descLabel = new Label(descGroup, SWT.WRAP);
		descLabel
				.setText("You can use the project creation wizard to fill out some of the basic details of your new project, or you can" 
						+" start with a blank project. Click on 'Next' to start the Wizard, or click on 'Finish' to start with a blank project.");
		RowData rowData = new RowData();
		rowData.width = 550;
		descLabel.setLayoutData(rowData);
    }
    /**
	 * Returns whether this page's controls currently all contain valid values.
	 * 
	 * @return <code>true</code> if all controls are valid, and
	 *         <code>false</code> if at least one is invalid
	 */
    protected boolean validatePage() {
		String projectName = super.getProjectName();
		// validate the project name. It should be equivalent to a C identifier.
		if (projectName.length() != 0
				&& !Pattern.compile("^[a-zA-Z][a-zA-Z0-9_]*$").matcher(
						projectName).matches()) {
			setErrorMessage("Project name contains invalid characters");
			return false;
		}
		if(!getLocationPath().toFile().exists()) {
			setErrorMessage("Please specify valid Project contents directory");
			return false;
		}
		if(!getLocationPath().toFile().canWrite()) {
			setErrorMessage("Project contents directory should have write permissions");
			return false;
		}
		if (_locationText != null) {
			String fileName = _locationText.getText().trim();
			if (!fileName.equals("")) {
				File file1 = new File(fileName + File.separator + "IDE" + File.separator + "ASP" + File.separator + "static");
				File file2 = new File(fileName + File.separator + "IDE" + File.separator + "ASP" + File.separator + "templates");
				if (file1.exists() && file2.exists()) {
					_locationSDK = fileName;
				} else {
					setErrorMessage(fileName + " is not a valid SDK location.");
					return false;
				}
			}
		}
		if (_projectAreaLocationText != null) {
			String fileName = _projectAreaLocationText.getText().trim();
			if (!fileName.equals("")) {
				File file = new File(fileName);
				if (file.exists()) {
					_projectAreaLocation = fileName;
				} else {
					setErrorMessage(fileName + " is not a valid Project Area location.");
					return false;
				}
			}
		}
		if (_pythonLocationText != null) {
			String fileName = _pythonLocationText.getText().trim();
			if (!fileName.equals("")) {
				File file = new File(fileName + File.separator + "python");
				if (file.exists()) {
					_pythonLocation = fileName;
				} else {
					setErrorMessage(fileName
							+ " is not a valid Python location.");
					return false;
				}
			}
		}
		return super.validatePage();
	}
    /**
     * Returns SDK location
     * @return _locationSDK
     */
    public String getSDKLocation()
    {
    	return _locationSDK;
    }
    /**
     * Returns Project Area location
     * @return _projectAreaLocation
     */
    public String getProjectAreaLocation()
    {
    	return _projectAreaLocation;
    }
    /**
     * Returns Python Location
     * @return _pythonLocation
     */
    public String getPythonLocation()
    {
    	return _pythonLocation;
    }
    /** Check the SDK Location 
     * 
     * @param fileName FileName
     * @return boolean
     */
    private boolean isValidSDKLocation(String fileName) {
    	File file1 = new File(fileName + File.separator + "IDE" + File.separator + "ASP" + File.separator + "static");
		File file2 = new File(fileName + File.separator + "IDE" + File.separator + "ASP" + File.separator + "templates");
		if (file1.exists() && file2.exists()) {
			return true;
		}
		return false;
    }
	/**
	 * Returns code generation mode.
	 * 
	 * @return
	 */
	public String getCodeGenMode() {
		return _codeGenMode;
	}
}
