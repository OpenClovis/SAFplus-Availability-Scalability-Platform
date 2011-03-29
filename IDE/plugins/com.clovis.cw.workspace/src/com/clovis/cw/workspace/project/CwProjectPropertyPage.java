/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/project/CwProjectPropertyPage.java $
 * $Author: srajyaguru $
 * $Date: 2007/04/30 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/
package com.clovis.cw.workspace.project;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Properties;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;

import com.clovis.common.utils.ClovisFileUtils;
import com.clovis.common.utils.UtilsPlugin;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.data.ICWProject;
import com.clovis.cw.workspace.WorkspacePlugin;
/**
 * This page is used to propmt user more information about the project.
 *
 * @author nadeem
 */
public class CwProjectPropertyPage extends PropertyPage
{
	/*
	 * NOTE : This class needs to be cleaned.
	 */
	private static final String VERSION_TITLE   = "Project Version:";
    private static final String VERSION_DEF_VALUE = "2.3.0.2";

    private Text _versionText;
    
    private static final String OWNER_TITLE   = "&SDK Location:";
    public  static final String SDK_LOCATION = "SDK_LOCATION";
    private Text _locationText;
    
    //-------- For Python location----- added by Abhay-----
    private static final String PYTHON_TITLE   = "&Python Location:";
    public  static final String PYTHON_LOCATION = "PYTHON_LOCATION";
    private static final String DEF_VALUE = "";

    private Text _pythonLocationText;
   
   
    private static final String CG_MODE_TITLE   = "&Code generation mode";
    public  static final String CODE_GEN_MODE = "CODE_GEN_MODE";
    private CCombo _codeGenMode;

    private static final String AUTOBACKUP_MODE_TITLE   = "Source &Backup Mode";
    public  static final String AUTOBACKUP_MODE = "AUTOBACKUP_MODE";
    public static final String AUTOBACKUP_VALUE_PROMPT = "prompt";
    public static final String AUTOBACKUP_VALUE_ALWAYS = "always";
    public static final String AUTOBACKUP_VALUE_NEVER = "never";
    private CCombo _autoBackupMode;
    
    private static final String MAXBACKUPS_MODE_TITLE = "&Number of Backups";
    public static final String MAXBACKUPS_MODE = "MAXBACKUPS_MODE";
    public static final int MAXBACKUPS_DEF_VALUE = 5;
    private Text _numBackups;
    
    /** Project Area Location **/
    private static final String PROJECT_AREA_LOCATION_TITLE = "Project &Area Location:";
    public static final String PROJECT_AREA_LOCATION_MODE = "PROJECT_AREA_LOCATION_MODE";
    private Text _projectAreaLocationText;
    
    private Button _mergeMode, _overrideMode;
    private static final String ALWAYS_MERGE_MODE = "ALWAYS_MERGE_MODE";
    private static final String ALWAYS_OVERRIDE_MODE = "ALWAYS_OVERRIDE_MODE";
    
    /**
     * Constructor for SamplePropertyPage.
     */
    public CwProjectPropertyPage()
    {
        super();
        noDefaultAndApplyButton();
    }
    /**
     * Get SDK Location for this Project.
     * @param res Resource
     * @return SDK Location for this Project.
     */
    public static String getSDKLocation(IResource res)
    {
    	String location  = null;
        try {
            location = res.getPersistentProperty(
                    new QualifiedName("", SDK_LOCATION));
        } catch (CoreException e) {
        }
        return location != null ? location : DEF_VALUE;
    }
   
//-------- For getting Python location----- added by Abhay-----
    /**
     * get the version for this Project.
     * @param res Resource
     * @return the version of the project
     */
    public static String getProjectVersion(IResource res)
    {
        String version  = null;
        try {
            IProjectDescription desc = ((IProject) res).getDescription();
            String comment = desc.getComment();
            int index = comment.indexOf(":");
            version = comment.substring(index + 1, comment.length());
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return version != null ? version : VERSION_DEF_VALUE;
    }
    /**
     * Get Project Area Location for this Project.
     * @param res Resource
     * @return Project Area Location for this Project.
     */
    public static String getProjectAreaLocation(IResource res)
    {
    	String location  = null;
        try {
            location = res.getPersistentProperty(
                    new QualifiedName("", PROJECT_AREA_LOCATION_MODE));
        } catch (CoreException e) {
        }
        
        if (location == null)
        {
        	location = DataPlugin.getProjectAreaLocation();
        }
        
        return location != null ? location : DEF_VALUE;
    }
    /**
     * Get PYTHON Location for this Project.
     * @param res Resource
     * @return PYTHON Location for this Project.
     */
    public static String getPythonLocation(IResource res)
    {
        String location  = null;
        try {
            location = res.getPersistentProperty(
                    new QualifiedName("", PYTHON_LOCATION));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return location != null ? location : DEF_VALUE;
    }
    
    /** Get Merge Mode
     * @param res Project
     * @return true or false
     */
    public static boolean getAlwaysMergeMode(IResource res) {
    	String mergeMode = null;
    	try {
			mergeMode = res.getPersistentProperty(new QualifiedName("", ALWAYS_MERGE_MODE));
		} catch (CoreException e) {
			e.printStackTrace();
		}
		return mergeMode != null ? Boolean.parseBoolean(mergeMode) : false;
    }
    /**
     * Set Merge Mode 
     * @param res Project
     * @param merge true or false
     */
    public static void setAlwaysMergeMode(IResource res, boolean merge) {
    	try {
			res.setPersistentProperty(new QualifiedName("", ALWAYS_MERGE_MODE), String.valueOf(merge));
		} catch (CoreException e) {
			e.printStackTrace();
		}
    }
    /** Get Override Mode
     * @param res Project
     * @return true or false
     */
    public static boolean getAlwaysOverrideMode(IResource res) {
    	String overrideMode = null;
    	try {
			overrideMode = res.getPersistentProperty(new QualifiedName("", ALWAYS_OVERRIDE_MODE));
		} catch (CoreException e) {
			e.printStackTrace();
		}
		return overrideMode != null ? Boolean.parseBoolean(overrideMode) : false;
    }
    /**
     * Set Override Mode
     * @param res Project
     * @param override true or false
     */
    public static void setAlwaysOverrideMode(IResource res, boolean override) {
    	try {
			res.setPersistentProperty(new QualifiedName("", ALWAYS_OVERRIDE_MODE), String.valueOf(override));
		} catch (CoreException e) {
			e.printStackTrace();
		}
    }
    
    /** Set the Code Generetaion Mode in properties File */
    public static void setCodeGenMode(IProject project, String val) {
		try {
			Properties properties = new Properties();
			properties.setProperty("codegenmode", val);
			properties.store(new FileOutputStream(project.getFile(
					ICWProject.CW_PROJECT_PROPERTIES_FILE_NAME).getLocation()
					.toOSString()),
					ICWProject.CW_PROJECT_PROPERTIES_FILE_DESCRIPTION);
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
    /**
     * Get Code Generation mode for this Project.
     * @param res Resource
     * @return Code Generation mode for this Project.
     */
    public static String getCodeGenMode(IResource res)
    {
        Properties properties = new Properties();
        try {
        	IFile propFile = ((IProject)res).getFile(ICWProject.CW_PROJECT_PROPERTIES_FILE_NAME);
        	if(!propFile.exists()) {
        		properties.store(new FileOutputStream(propFile.getLocation().toOSString()), ICWProject.CW_PROJECT_PROPERTIES_FILE_DESCRIPTION);
        	}
			properties.load(new FileInputStream(propFile.getLocation().toOSString()));
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		} 
		String val = properties.getProperty("codegenmode");
		return val != null ? val : "openclovis";
    }

    /**
     * Get Auto backup mode for this Project.
     * @param res Resource
     * @return Auto Backup mode for this Project.
     */
    public static String getAutoBackupMode(IResource res)
    {
        String mode  = null;
        try {
            mode = res.getPersistentProperty(
                    new QualifiedName("", AUTOBACKUP_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return mode != null ? mode : AUTOBACKUP_VALUE_PROMPT;
    }
    
    /**
     * Get Source code location for projects
     * @param res Resource
     * @return Location
     */
    public static String getSourceLocation(IResource res)
    {
    	String loc = null;
    	try {
            loc = res.getPersistentProperty(
                    new QualifiedName("",PROJECT_AREA_LOCATION_MODE));
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return (loc != null && !loc.equals("")) ? loc + File.separator + res.getName() +File.separator + ICWProject.CW_PROJECT_SRC_DIR_NAME:"";
    }
    
    /**
     * Set the configuration backup mode for this project.
     * @param res
     * @param mode
     */
    public static void setAutoBackupMode(IResource res, String numFiles)
    {
    	try
    	{
	    	res.setPersistentProperty(
	    		new QualifiedName("",AUTOBACKUP_MODE), String.valueOf(numFiles));
    	} catch (CoreException e) {
			e.printStackTrace();
    	}
    }

    /**
     * Get maximum number of backup files for the project
     * @param res Resource
     * @return Maximum number of backup files for this project.
     */
    public static int getNumBackups(IResource res)
    {
        int numBackups = MAXBACKUPS_DEF_VALUE;
        try {
        	numBackups = Integer.parseInt(res.getPersistentProperty(new QualifiedName("", MAXBACKUPS_MODE)));
        } catch (NumberFormatException nfe) {
        	//do nothing...we will use the default
        } catch (CoreException e) {
        	e.printStackTrace();
        }
        return numBackups;
    }
    
    /**
     * Set the maximum number of backup files for the project.
     * @param res
     * @param mode
     */
    public static void setNumBackups(IResource res, String numBackups)
    {
    	try
    	{
	    	res.setPersistentProperty(new QualifiedName("",MAXBACKUPS_MODE), String.valueOf(numBackups));
    	} catch (CoreException e) {
			e.printStackTrace();
    	}
    }
    
    /**
     * Create content.
     * @param parent Parent Composite
     * Adds control to get ASP Locaiton.
     * @return Control
     */
    protected Control createContents(Composite parent)
    {
        Composite composite = new Composite(parent, SWT.NONE | SWT.NO_RADIO_GROUP);
        composite.setLayoutData(new GridData(GridData.FILL_BOTH));

        GridLayout layout = new GridLayout();
        layout.numColumns = 3;        
        composite.setLayout(layout);

        // Label for owner field
        Label versionLabel = new Label(composite, SWT.NONE);
        versionLabel.setText(VERSION_TITLE);

        // Owner text field
        _versionText = new Text(composite, SWT.SINGLE | SWT.BORDER);
        _versionText.setEnabled(false);
        GridData gd = new GridData(GridData.FILL_HORIZONTAL);
        gd.horizontalSpan = 2;
        _versionText.setLayoutData(gd);
        _versionText.setText(getProjectVersion((IResource) getElement()));
        
        // Label for owner field
        Label ownerLabel = new Label(composite, SWT.NONE);
        ownerLabel.setText(OWNER_TITLE);

        // Owner text field
        _locationText = new Text(composite, SWT.SINGLE | SWT.BORDER);
        _locationText.setEditable(true);
        _locationText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        _locationText.setText(getSDKLocation((IResource) getElement()));
        _locationText.addListener(SWT.Modify, new Listener() {
			public void handleEvent(Event event) {
				boolean isValid = validatePage();
				if(isValid) {
					if(_pythonLocationText.getText().trim().equals("")) {
						String installLocation = new File(_locationText.getText().trim()).getParentFile().getAbsolutePath();
						if(new File(installLocation + File.separator + "buildtools" + File.separator + "local" + File.separator + "bin" + File.separator + "python").exists()) {
							_pythonLocationText.setText(installLocation + File.separator + "buildtools" + File.separator + "local" + File.separator + "bin");
						}
					}
					setErrorMessage(null);
					setValid(true);
				} else {
					setValid(false);
				}
			}
        });
        // Add Browse Button.
        Button button = new Button(composite, SWT.PUSH);
        button.setText("Browse...");
        button.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				DirectoryDialog dialog = new DirectoryDialog(getShell(),
						SWT.NONE);
				dialog.setFilterPath(UtilsPlugin.getDialogSettingsValue("SDKLOCATION"));
				String fileName = dialog.open();
				UtilsPlugin.saveDialogSettings("SDKLOCATION", fileName);
				if (fileName != null) {
					if (!new File(fileName).canExecute()) {
						MessageDialog.openError(getShell(), "Warning", fileName
								+ " does not have permission to access.");
					} else {
						File file1 = new File(fileName + File.separator + "IDE"
								+ File.separator + "ASP" + File.separator
								+ "static");
						File file2 = new File(fileName + File.separator + "IDE"
								+ File.separator + "ASP" + File.separator
								+ "templates");
						if (file1.exists() && file2.exists()) {
							_locationText.setText(fileName);
							if (_pythonLocationText.getText().trim().equals("")) {
								String installLocation = new File(fileName)
										.getParentFile().getAbsolutePath();
								if (new File(installLocation + File.separator
										+ "buildtools" + File.separator
										+ "local" + File.separator + "bin"
										+ File.separator + "python").exists()) {
									_pythonLocationText.setText(installLocation
											+ File.separator + "buildtools"
											+ File.separator + "local"
											+ File.separator + "bin");
								}
							}
						} else {
							MessageDialog.openError(getShell(), "Warning",
									fileName + " is not a valid SDK location.");
						}
					}
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
		});

       // --------------- For Project Area Location -----------------// 
        Label projectLabel = new Label(composite, SWT.NONE);
        projectLabel.setText(PROJECT_AREA_LOCATION_TITLE);
        _projectAreaLocationText = new Text(composite, SWT.SINGLE | SWT.BORDER);
        _projectAreaLocationText.setEditable(true);
        _projectAreaLocationText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        _projectAreaLocationText.setText(getProjectAreaLocation((IResource) getElement()));
        _projectAreaLocationText.addListener(SWT.Modify, new Listener() {
			public void handleEvent(Event event) {
				if(validatePage()) {
					setErrorMessage(null);
					setValid(true);
				} else {
					setValid(false);
				}
			}
        });
        Button button1 = new Button(composite, SWT.PUSH);
        button1.setText("Browse...");
        button1.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				DirectoryDialog dialog = new DirectoryDialog(getShell(),
						SWT.NONE);
				dialog.setFilterPath(UtilsPlugin.getDialogSettingsValue("PROJECTAREALOCATION"));
				String fileName = dialog.open();
				UtilsPlugin.saveDialogSettings("PROJECTAREALOCATION", fileName);
				if (fileName != null) {
					_projectAreaLocationText.setText(fileName);
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
		});
        //----------------------- ----------------------------------------//
        // -------- For Python location----- added by Abhay-----
        //      Label for Python field
        Label pythonLabel = new Label(composite, SWT.NONE);
        pythonLabel.setText(PYTHON_TITLE);

        // Owner text field
        _pythonLocationText = new Text(composite, SWT.SINGLE | SWT.BORDER);
        _pythonLocationText.setEditable(true);
        _pythonLocationText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
        _pythonLocationText.setText(getPythonLocation((IResource) getElement()));
        _pythonLocationText.addListener(SWT.Modify, new Listener() {
			public void handleEvent(Event event) {
				if(validatePage()) {
					setErrorMessage(null);
					setValid(true);
				} else {
					setValid(false);
				}
			}
        });
        // Add Browse Button.
        Button btnBrowse = new Button(composite, SWT.PUSH);
        btnBrowse.setText("Browse...");
        btnBrowse.addSelectionListener(new BrowseButtonListener(_pythonLocationText, "PYTHONLOCATION"));
             
        GridData twoColumnGd = new GridData(GridData.FILL_HORIZONTAL);
        twoColumnGd.horizontalSpan = 2;

        // Project setting which determines if the files in the ASP src directory
        // will be backed up or not on code generation. Options are Always, Never, or
        // Prompt (which will ask the user each time).
        Label autoBackupModeLabel = new Label(composite, SWT.NONE);
        autoBackupModeLabel.setText(AUTOBACKUP_MODE_TITLE);
        _autoBackupMode = new CCombo(composite, SWT.BORDER | SWT.READ_ONLY);
        _autoBackupMode.setLayoutData(twoColumnGd);
        _autoBackupMode.add(AUTOBACKUP_VALUE_PROMPT);
        _autoBackupMode.add(AUTOBACKUP_VALUE_ALWAYS);
        _autoBackupMode.add(AUTOBACKUP_VALUE_NEVER);
        _autoBackupMode.select(_autoBackupMode.indexOf(getAutoBackupMode((IResource) getElement())));

        // add control to capture number of src backups to keep 
        GridData backupTwoColumnGrid = new GridData(GridData.FILL_HORIZONTAL);
        backupTwoColumnGrid.horizontalSpan = 2;
        Label numBackupsLabel = new Label(composite, SWT.NONE);
        numBackupsLabel.setText(MAXBACKUPS_MODE_TITLE);
        _numBackups = new Text(composite, SWT.SINGLE | SWT.BORDER);
        _numBackups.setLayoutData(backupTwoColumnGrid);
        _numBackups.setText(String.valueOf(getNumBackups((IResource) getElement())));
        _numBackups.addListener(SWT.Modify, new Listener() {
			public void handleEvent(Event event) {
				if (validatePage())
				{
					setErrorMessage(null);
					setValid(true);
				} else {
					setValid(false);
				}
			}
        });

        Label cgModeLabel = new Label(composite, SWT.NONE);
        cgModeLabel.setText(CG_MODE_TITLE);
        _codeGenMode = new CCombo(composite, SWT.BORDER | SWT.READ_ONLY);
        //twoColumnGd = new GridData(GridData.FILL_HORIZONTAL);
        //twoColumnGd.horizontalSpan = 2;
        _codeGenMode.setLayoutData(twoColumnGd);
        String items[] = WorkspacePlugin.getCodeGenOptions();
        _codeGenMode.setItems(convertCodegenOptionsInUIForm(items));
        String codeGenOption = convertCodegenOptionInUIForm(getCodeGenMode((IResource) getElement()));
        if(_codeGenMode.indexOf(codeGenOption) != -1)
        	_codeGenMode.select(_codeGenMode.indexOf(codeGenOption));
        else
        	_codeGenMode.select(_codeGenMode.indexOf("openclovis"));
        _overrideMode = new Button(composite, SWT.RADIO);
		_overrideMode.setText("Always &override application code");
		GridData gridData = new GridData(GridData.FILL_HORIZONTAL);
		gridData.horizontalSpan = 2;
		_overrideMode.setLayoutData(gridData);
		_overrideMode.setSelection(getAlwaysOverrideMode((IResource)getElement()));
		_overrideMode.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {}
			public void widgetSelected(SelectionEvent e) {
				if(_overrideMode.getSelection()) {
					_mergeMode.setSelection(false);
				}
			}});
		_mergeMode = new Button(composite, SWT.RADIO);
		_mergeMode.setText("Always &merge application code");
		gridData = new GridData(GridData.FILL_HORIZONTAL);
		gridData.horizontalSpan = 2;
		_mergeMode.setLayoutData(gridData);
		_mergeMode.setSelection(getAlwaysMergeMode((IResource)getElement()));
		_mergeMode.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {}
			public void widgetSelected(SelectionEvent e) {
				if(_mergeMode.getSelection()) {
					_overrideMode.setSelection(false);
				}
			}});
		return composite;
    }
    /**
     * Returns whether this page's controls currently all contain valid 
     * values.
     *
     * @return <code>true</code> if all controls are valid, and
     *   <code>false</code> if at least one is invalid
     */
    protected boolean validatePage() {
		String fileName = _locationText.getText().trim();
		if (!fileName.equals("")) {
			if(!new File(fileName).isDirectory()) {
				setErrorMessage(fileName + " is not a valid SDK location.");
				return false;
			}
			if(!new File(fileName).canExecute()) {
				setErrorMessage(fileName + " does not have permission to access.");
				return false;
			}
			File file1 = new File(fileName + File.separator + "IDE" + File.separator + "ASP" + File.separator + "static");
			File file2 = new File(fileName + File.separator + "IDE" + File.separator + "ASP" + File.separator + "templates");
			if (!file1.exists() || !file2.exists()) {
				setErrorMessage(fileName + " is not a valid SDK location.");
				return false;
			} 
		}
		fileName = _projectAreaLocationText.getText().trim();
		if (!fileName.equals("")) {
			File file = new File(fileName);
			if (!file.exists()) {
				setErrorMessage(fileName + " is not a valid Project Area location.");
				return false;
			}
		}
		fileName = _pythonLocationText.getText().trim();
		if (!fileName.equals("")) {
			File file = new File(fileName + File.separator + "python");
			if (!file.exists()) {
				setErrorMessage(fileName + " is not a valid Python location.");
				return false;
			}
		}

		fileName = _numBackups.getText().trim();
		boolean valueOK = true;
		try {
			int numBackups = Integer.parseInt(fileName);
			if (numBackups < 1 || numBackups > 100) valueOK = false;
		} catch (NumberFormatException e) {
			valueOK = false;
		}
		if (!valueOK)
		{
			setErrorMessage("Number of backups must be a positive integer between 1 and 100.");
			return false;
		}

		return true;
	}
    public static String[] convertCodegenOptionsInUIForm(String items[]) {
    	String displayItems[] = new String [items.length];
        for(int i = 0; i < items.length; i++) {
        	displayItems[i] = convertCodegenOptionInUIForm(items[i]);
        }
        return displayItems;
    }
    public static String convertCodegenOptionInUIForm(String val) {
    	if(val.equals(ICWProject.CLOVIS_CODEGEN_OPTION)) {
    		return val;
    	}
    	return "Remote code generation (" + val + ")";
    }
    public static String convertCodegenOptionFromUIForm(String val) {
    	if(val.equals(ICWProject.CLOVIS_CODEGEN_OPTION)) {
    		return val;
    	}
    	return val.substring(val.indexOf('(') + 1, val.indexOf(')'));
    }
    /**
	 * Save the location.
	 * 
	 * @return true is saved.
	 */
    public boolean performOk()
    {
    	IResource resource = (IResource) getElement();
    	String oldSourceLocation = getSourceLocation(resource);

    	String sdkLocation = _locationText.getText().trim();
    	String projectAreaLocation = _projectAreaLocationText.getText().trim();
    	String pythonLocation = _pythonLocationText.getText().trim();
    	
    	String newSourceLocation = "";
    	
    	if(!projectAreaLocation.equals("") && null != projectAreaLocation){
    		newSourceLocation = projectAreaLocation + File.separator + resource.getName() + File.separator
					+ ICWProject.CW_PROJECT_SRC_DIR_NAME;
    	}
    	// store the value in the location text field
        try {
        	String codeGenMode = convertCodegenOptionFromUIForm(_codeGenMode.getText());
        	//checkCodeGenModeOption(newSourceLocation, codeGenMode);
        		
            resource.setPersistentProperty(
                new QualifiedName("", SDK_LOCATION), sdkLocation);
            
            resource.setPersistentProperty(
                    new QualifiedName("", PYTHON_LOCATION), pythonLocation);
            
            resource.setPersistentProperty(
                    new QualifiedName("", PROJECT_AREA_LOCATION_MODE),
                    projectAreaLocation);
            resource.setPersistentProperty(
                    new QualifiedName("true", CODE_GEN_MODE),
                    codeGenMode);
            
            setCodeGenMode((IProject)resource, codeGenMode);
            
            resource.setPersistentProperty(
                    new QualifiedName("", AUTOBACKUP_MODE),
                    String.valueOf(_autoBackupMode.getText()));
            
            setNumBackups(resource, String.valueOf(_numBackups.getText()));

          	resource.setPersistentProperty(new QualifiedName("",
					ALWAYS_MERGE_MODE), String.valueOf(_mergeMode
					.getSelection()));
          	resource.setPersistentProperty(new QualifiedName("",
					ALWAYS_OVERRIDE_MODE), String.valueOf(_overrideMode
					.getSelection()));	
        } catch (CoreException e) {
        	e.printStackTrace();
            return false;
        } catch (Exception e) {
			e.printStackTrace();
			return false;
		}
        File linkFile = ((IProject) getElement()).getLocation().append(
				ICWProject.CW_PROJECT_SRC_DIR_NAME).toFile();
        if(oldSourceLocation.equals(newSourceLocation))
        	return true;
        else {
        	try {
        		if (linkFile.exists() && oldSourceLocation.equals("")
    					&& linkFile.getCanonicalPath().equals(
    							linkFile.getAbsolutePath()) && !linkFile.getAbsolutePath().equals(newSourceLocation)) {
            		moveSourceFolder(linkFile.getAbsolutePath(), newSourceLocation);
    			} else if (linkFile.exists() && !oldSourceLocation.equals("") && ((IProject) getElement()).getParent().getLocation()
						.toOSString().equals(
								new File(oldSourceLocation).getParentFile()
										.getParent())) {
        			moveSourceFolder(linkFile.getAbsolutePath(), newSourceLocation);
				}
    		} catch (IOException e1) {
    			e1.printStackTrace();
    		} catch (InterruptedException e) {
				e.printStackTrace();
			}
        	try {
        		File srcFile = new File(newSourceLocation);
				if (linkFile.exists() && !linkFile.getCanonicalPath().equals(
								linkFile.getAbsolutePath())) {
					Process proc = Runtime.getRuntime().exec("rm " + linkFile);
					proc.waitFor();
				}
				if (!srcFile.exists()) {
					srcFile.mkdirs();
				}
				if (!srcFile.getAbsolutePath().equals(
						linkFile.getAbsolutePath())) {
					ClovisFileUtils.createRelativeSourceLink(newSourceLocation,
							linkFile.getAbsolutePath());
				}		
				resource.refreshLocal(IResource.DEPTH_INFINITE, null);
			} catch (IOException e) {
				e.printStackTrace();
			} catch (InterruptedException e) {
				e.printStackTrace();
			} catch (CoreException e) {
				e.printStackTrace();
			}
		}
        setValid(true);
		return true;
    }
    class BrowseButtonListener implements SelectionListener
    {
    	Text text;
    	String type;
    	public BrowseButtonListener(Text text, String type)
    	{
    		this.text = text;
    		this.type = type;
    	}
		public void widgetSelected(SelectionEvent e) {
			DirectoryDialog dialog =
                new DirectoryDialog(getShell(), SWT.NONE);
			dialog.setFilterPath(UtilsPlugin.getDialogSettingsValue(type));
            String fileName = dialog.open();
            UtilsPlugin.saveDialogSettings(type, fileName);
            if (fileName != null) {
                text.setText(fileName);
            }		
		}
		public void widgetDefaultSelected(SelectionEvent e) {}
    	
    }
    /**
     * move existing src folder if src is not a soft link
     * @param srcLocation
     * @param dstLocation
     * @throws IOException
     * @throws InterruptedException
     */
    private void moveSourceFolder(String srcLocation, String dstLocation)
			throws IOException, InterruptedException {
		if (new File(dstLocation).exists()) {
			dstLocation = dstLocation + "_" + System.nanoTime();
		}
		File parentFile = new File(dstLocation).getParentFile();
		if(!parentFile.exists()) {
			parentFile.mkdirs();
		}
		Process proc = Runtime.getRuntime().exec(
				"cp -r " + srcLocation + " " + dstLocation);
		proc.waitFor();
		proc = Runtime.getRuntime().exec("rm -fr " + srcLocation);
		proc.waitFor();
	}
    
    /**
     * Check whether code gen mode and existing code is in sync
     * @param sourceLocation src location
     * @param codeGenMode true/false for saf/nonsaf
     **/
    /*private void checkCodeGenModeOption(String sourceLocation, String codeGenMode) {
    	File safFile = new Path(sourceLocation + File.separator + "." + ICWProject.PROJECT_DEFAULT_SAF_TEMPLATE_GROUP_FOLDER).toFile();
		File nonSafFile = new Path(sourceLocation + File.separator + "." + ICWProject.PROJECT_DEFAULT_NONSAF_TEMPLATE_GROUP_FOLDER).toFile();
		if ((nonSafFile.exists() && codeGenMode.equals("true"))
				|| (new File(sourceLocation + File.separator + "app").exists() && !safFile.exists() && !nonSafFile.exists() && codeGenMode.equals("true"))) {
			MessageDialog
					.openWarning(
							getShell(),
							"Warning for Backup",
							"Existing code within this Project Area Location is not SAF compliant. So If you wants to preserve your changes please deselct 'Generate SAF compliant model' option or do the backup of existing code before generating new code.");
		} else if (safFile.exists() && codeGenMode.equals("false")) {
			MessageDialog
					.openWarning(
							getShell(),
							"Warning for Backup",
							"Existing code within this Project Area Location is SAF compliant. So If you wants to preserve your changes please select 'Generate SAF compliant model' option or do the backup of existing code before generating new code.");
		}
    }*/
}
