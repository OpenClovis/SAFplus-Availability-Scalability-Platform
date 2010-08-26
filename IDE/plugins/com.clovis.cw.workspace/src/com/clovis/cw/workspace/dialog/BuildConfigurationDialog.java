/*******************************************************************************
 * ModuleName  : com
 * $File: //depot/dev/main/Andromeda/Ganga/IDE/plugins/com.clovis.cw.workspace/src/com/clovis/cw/workspace/dialog/BuildConfigurationDialog.java $
 * $Author: pushparaj $
 * $Date: 2007/04/17 $
 *******************************************************************************/

/*******************************************************************************
 * Description :
 *******************************************************************************/

package com.clovis.cw.workspace.dialog;

import java.io.File;

import org.eclipse.core.resources.IResource;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CCombo;
import org.eclipse.swt.events.HelpEvent;
import org.eclipse.swt.events.HelpListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.PlatformUI;

import com.clovis.common.utils.UtilsPlugin;
import com.clovis.cw.workspace.builders.ClovisConfigurator;
import com.clovis.cw.workspace.project.CwProjectPropertyPage;

/**
 * 
 * @author pushparaj
 * Class to capture build settings
 */
public class BuildConfigurationDialog extends TitleAreaDialog
{
	private static final String BUILD_WITH_CROSS = "With cross build";
    private Button _crossbuildButton;
    private CCombo _crossbuildText;
    
    private static final String BUILD_WITH_SIMULATION = "With ASP simulation";
    private Button _aspSimulationButton;
        
    private static final String BUILD_WITH_KERNEL = "With kernel build";
    private Button _kernelButton;
    
    private static final String BUILD_WITH_PRE_ASP = "Use pre built ASP";
    private Button _aspPreBuildButton;
    private Text _preASPLocationText;
    Button _aspPreBrowseButton;
    
    private static final String BUILD_WITH_SNMP = "Include SNMP for North Bound Access";
    private Button _snmpButton;
    private Text _kernelText;
    
    private static final String BUILD_WITH_CM = "Include Chassis Manager for HPI Access";
    private Button _cmButton;
    private CCombo _cmText;
    
    private static final String FORCE_CONFIGURE = "Force Configure";
    private Button _forceConfigureButton;
    
    private IResource _project;
    
    /**
     * 
     * @param shell
     * @param resource
     */
	public BuildConfigurationDialog(Shell shell, IResource resource)
	{
		super(shell);
		_project = resource;
	}
	/**
	 *@see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createDialogArea(Composite composite)
    {
		/* Create container for dialog controls */		
		Composite container = new Composite(composite, org.eclipse.swt.SWT.NONE);
        GridLayout glayout = new GridLayout();
        glayout.numColumns = 2;
        container.setLayout(glayout);
    	GridData data1 = new GridData(GridData.FILL_HORIZONTAL);
        container.setLayoutData(data1);
        
    	/******************************************************/
    	/* Create pre-build ASP controls                      */
    	/******************************************************/
    	_aspPreBuildButton = new Button(container, SWT.CHECK);
    	_aspPreBuildButton.setText(BUILD_WITH_PRE_ASP);
    	_aspPreBuildButton.setAlignment(SWT.LEFT);
    	_aspPreBuildButton.setLayoutData(data1);
    	
		// create composite with grid to hold ASP location selector in single column
    	Composite preBuildComp = new Composite(container, org.eclipse.swt.SWT.NONE);
    	GridData preBuildGrid = new GridData(GridData.FILL_HORIZONTAL);
    	preBuildGrid.horizontalSpan = 1;
    	preBuildGrid.minimumWidth = 250;
    	preBuildComp.setLayoutData(preBuildGrid);
    	GridLayout preBuildLayout = new GridLayout();
    	preBuildLayout.numColumns = 2;
    	preBuildLayout.marginHeight = 0;
    	preBuildLayout.marginWidth = 0;
    	preBuildComp.setLayout(preBuildLayout);
    	
    	boolean aspPreBuildMode = new Boolean(
    			ClovisConfigurator.getASPPreBuildMode(_project)).booleanValue();
    	_aspPreBuildButton.setSelection(aspPreBuildMode);
    	_aspPreBuildButton.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				if(_aspPreBuildButton.getSelection()) {
					_preASPLocationText.setEnabled(true);
					_aspPreBrowseButton.setEnabled(true);
				} else {
					_preASPLocationText.setEnabled(false);
					_aspPreBrowseButton.setEnabled(false);
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
    	});

    	_preASPLocationText = new Text(preBuildComp, SWT.BORDER);
    	_preASPLocationText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	_preASPLocationText.setText(ClovisConfigurator.getPreBuildASPLocation(_project));
    	_aspPreBrowseButton = new Button(preBuildComp, SWT.PUSH);
    	_aspPreBrowseButton.setText("Browse...");
    	_aspPreBrowseButton.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				DirectoryDialog dialog =
	                new DirectoryDialog(getShell(), SWT.NONE);
				dialog.setFilterPath(UtilsPlugin.getDialogSettingsValue("PREASPLOCATION"));
	            String fileName = dialog.open();
	            UtilsPlugin.saveDialogSettings("PREASPLOCATION", fileName);
	            if (fileName != null) {
	            	_preASPLocationText.setText(fileName);
	            } 
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
		});

    	// enable/disable ASP selector based on the mode
    	if(aspPreBuildMode) {
    		_preASPLocationText.setEnabled(true);
			_aspPreBrowseButton.setEnabled(true);
    	} else {
    		_preASPLocationText.setEnabled(false);
			_aspPreBrowseButton.setEnabled(false);
    	}
    	
    	/******************************************************/
    	/* Create ASP simulation controls                         */
    	/******************************************************/
    	_aspSimulationButton = new Button(container, SWT.CHECK);
    	_aspSimulationButton.setText(BUILD_WITH_SIMULATION);
    	_aspSimulationButton.setAlignment(SWT.LEFT);
    	_aspSimulationButton.setLayoutData(data1);
    	GridData data3 = new GridData(GridData.FILL_HORIZONTAL);
    	data3.horizontalSpan = 2;
    	_aspSimulationButton.setLayoutData(data3);
    	_aspSimulationButton.setSelection(new Boolean(
    			ClovisConfigurator.getASPSimulationMode(_project)).booleanValue());
    	
    	/******************************************************/
    	/* Create cross-build controls                        */
    	/******************************************************/
    	_crossbuildButton = new Button(container, SWT.CHECK);
    	_crossbuildButton.setText(BUILD_WITH_CROSS);
    	_crossbuildButton.setAlignment(SWT.LEFT);
    	_crossbuildButton.setLayoutData(data1);
    	_crossbuildText = new CCombo(container, SWT.BORDER | SWT.READ_ONLY);
    	_crossbuildText.setLayoutData(new GridData(SWT.FILL, SWT.TOP, false, false));
    	addToolChains();
    	_crossbuildText.select(_crossbuildText.indexOf(ClovisConfigurator.getToolChainMode(_project)));
    	_crossbuildButton.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				if(_crossbuildButton.getSelection()) {
					_crossbuildText.setEnabled(true);
					_crossbuildText.select(0);
				} else {
					_crossbuildText.select(-1);
					_crossbuildText.setEnabled(false);
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
    	});
    	boolean buildMode = new Boolean(
    			ClovisConfigurator.getCrossBuildMode(_project)).booleanValue(); 
    	if(_crossbuildText.getItemCount() == 0) {
    		_crossbuildButton.setSelection(false);
    		_crossbuildButton.setEnabled(false);
    		_crossbuildText.setEnabled(false);
    	} else {
    		_crossbuildButton.setEnabled(true);
    		_crossbuildButton.setSelection(buildMode);
    		_crossbuildText.setEnabled(buildMode);
    	}
    	   	    	
    	/******************************************************/
    	/* Create with kernel build controls                  */
    	/******************************************************/
    	_kernelButton = new Button(container, SWT.CHECK);
    	_kernelButton.setText(BUILD_WITH_KERNEL);
    	_kernelButton.setAlignment(SWT.LEFT);
    	_kernelButton.setLayoutData(data1);
    	_kernelButton.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				if(_kernelButton.getSelection()) {
					_kernelText.setEnabled(true);
				} else {
					_kernelText.setEnabled(false);
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
    	});
    	boolean kernelMode = new Boolean(
    			ClovisConfigurator.getKernelBuildMode(_project)).booleanValue();
    	_kernelButton.setSelection(kernelMode);
    	_kernelText = new Text(container, SWT.BORDER);
    	_kernelText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	_kernelText.setText(ClovisConfigurator.getKernelName(_project));
    	_kernelText.setEnabled(kernelMode);
    	
    	/******************************************************/
    	/* Create SNMP build controls                         */
    	/******************************************************/
    	_snmpButton = new Button(container, SWT.CHECK);
    	_snmpButton.setText(BUILD_WITH_SNMP);
    	_snmpButton.setAlignment(SWT.LEFT);
    	_snmpButton.setLayoutData(data1);
    	data3 = new GridData(GridData.FILL_HORIZONTAL);
    	data3.horizontalSpan = 2;
    	_snmpButton.setLayoutData(data3);
    	_snmpButton.setSelection(new Boolean(
    			ClovisConfigurator.getSNMPBuildMode(_project)).booleanValue());
    	
    	/******************************************************/
    	/* Create chassis manager controls                    */
    	/******************************************************/
    	_cmButton = new Button(container, SWT.CHECK);
    	_cmButton.setText(BUILD_WITH_CM);
    	_cmButton.setAlignment(SWT.LEFT);
    	_cmButton.setLayoutData(data1);
    	boolean cmMode = new Boolean(
    			ClovisConfigurator.getCMBuildMode(_project)).booleanValue();
    	_cmButton.setSelection(cmMode);
    	_cmText = new CCombo(container, SWT.BORDER | SWT.READ_ONLY);
    	_cmText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
    	_cmText.add("radisys");
    	_cmText.add("openhpi");
    	if(cmMode)
    		_cmText.select(_cmText.indexOf(ClovisConfigurator.getCMValue(_project)));
    	else {
    		_cmText.select(-1);
    		_cmText.setEnabled(false);
    	}
    	_cmButton.addSelectionListener(new SelectionListener() {
			public void widgetSelected(SelectionEvent e) {
				if(_cmButton.getSelection()) {
					_cmText.setEnabled(true);
					_cmText.select(0);
				} else {
					_cmText.select(-1);
					_cmText.setEnabled(false);
				}
			}
			public void widgetDefaultSelected(SelectionEvent e) {
			}
    	});
    	
    	// disable chassis management controls if no Platform Support Package is installed
    	if (!isPSPInstalled())
    	{
    		_cmButton.setSelection(false);
    		_cmButton.setEnabled(false);
    		_cmText.select(-1);
    		_cmText.setEnabled(false);
    	}
    	
    	/******************************************************/
    	/* Create Force Configure controls                         */
    	/******************************************************/
    	_forceConfigureButton = new Button(container, SWT.CHECK);
    	_forceConfigureButton.setText(FORCE_CONFIGURE);
    	_forceConfigureButton.setAlignment(SWT.LEFT);
    	GridData forceConfigureData = new GridData(GridData.FILL_HORIZONTAL);
    	forceConfigureData.horizontalSpan = 2;
    	_forceConfigureButton.setLayoutData(forceConfigureData);
    	_forceConfigureButton.setSelection(new Boolean(
    			ClovisConfigurator.getForceConfigureMode(_project)).booleanValue());
    	
    	setTitle("Build Configuration");
    	setMessage("Configure project build settings", IMessageProvider.INFORMATION);
    	composite.addHelpListener(new HelpListener() {
			public void helpRequested(HelpEvent e) {
				PlatformUI.getWorkbench().getHelpSystem()
						.displayHelp("com.clovis.cw.help.configure");
			}
		});	
        return container;
    }

	/**
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	protected void configureShell(Shell shell)
	{
		shell.setText("Configure");
		super.configureShell(shell);
	}
	/**
	 *@see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	protected void okPressed()
	{
		String preASPLocation = _preASPLocationText.getText().trim();

		boolean aspPreBuildMode = _aspPreBuildButton.getSelection();

		if(aspPreBuildMode && preASPLocation.equals("")) {
			MessageDialog.openError(getShell(), "Error in Build settings",
			"Please select the location of the pre-build ASP libraries when using pre-built ASP");
			return;
		} else if(aspPreBuildMode && !isValidPrebuiltASPLocation(preASPLocation)) {
			MessageDialog.openError(getShell(), "Error in Build settings",
			"Please select a valid location for pre-build ASP libraries");
			return;
		}

		try {
			boolean buildMode = _crossbuildButton.getSelection();
			ClovisConfigurator.setCrossBuildMode(_project, String.valueOf(buildMode));

            int index = _crossbuildText.getSelectionIndex();
            String toolChain = null;
			if (index == -1) {
				ClovisConfigurator.setToolChainMode(_project, null);
			} else {
				toolChain = _crossbuildText.getItem(index);
				ClovisConfigurator.setToolChainMode(_project, toolChain);
			}
			ClovisConfigurator.setASPLocation(_project, CwProjectPropertyPage.getSDKLocation(_project) + File.separator + "src" + File.separator + "ASP");
			
			ClovisConfigurator.setASPBuildMode(_project, String.valueOf(!aspPreBuildMode));

			ClovisConfigurator.setASPPreBuildMode(_project, String.valueOf(aspPreBuildMode));

			ClovisConfigurator.setPreBuildASPLocation(_project, preASPLocation);

			boolean kernelMode = _kernelButton.getSelection();
			ClovisConfigurator.setKernelBuildMode(_project, String.valueOf(kernelMode));

			String kernelName = _kernelText.getText();
			ClovisConfigurator.setKernelName(_project, kernelName);

            boolean snmpMode = _snmpButton.getSelection();
			ClovisConfigurator.setSNMPBuildMode(_project, String.valueOf(snmpMode));
			
			boolean simulationMode = _aspSimulationButton.getSelection();
			ClovisConfigurator.setASPSimulationMode(_project, String.valueOf(simulationMode));
			
            boolean cmMode = _cmButton.getSelection();
			ClovisConfigurator.setCMBuildMode(_project, String.valueOf(cmMode));
			
            boolean forceConfigureMode = _forceConfigureButton.getSelection();
			ClovisConfigurator.setForceConfigureMode(_project, String.valueOf(forceConfigureMode));

            String cmValue = _cmText.getText();
			ClovisConfigurator.setCMValue(_project, cmValue);

            boolean ipcMode = true;
			ClovisConfigurator.setIPCBuildMode(_project, String.valueOf(ipcMode));

            String ipcValue = "tipc";
			ClovisConfigurator.setIPCValue(_project, ipcValue);

			String projectAreaLocation = CwProjectPropertyPage.getProjectAreaLocation(_project);
			
			
			if (projectAreaLocation.equals("")) {
				String message = 	"Project Area location is not set on Project [" + _project.getName()
								+ "]. Use Project->Right Click->properties->Clovis System Project to set its value.\n";
				MessageDialog.openError(new Shell(), "Project settings errors for " + _project.getName(), message);
				return;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		super.okPressed();
	}
	/**
     * Add ToolChains
     *
     */
    private void addToolChains() {
		_crossbuildText.removeAll();
		String buildToolsLoc = ClovisConfigurator.getBuildToolsLocation(_project);
		if (!buildToolsLoc.equals("")) {
			File toolsFolder = new File(buildToolsLoc);
			File files[] = toolsFolder.listFiles();
			for (int i = 0; i < files.length; i++) {
				File file = files[i];
				if (!file.getName().equals("local") && new File(file.getAbsolutePath() + File.separator + "config.mk").exists()) {
					_crossbuildText.add(files[i].getName());
				}
			}
		}
	}
    
    /**
     * Check to see if a Platform Support Package is installed under
     *  the SDK location. This is required if a project is to be configured
     *  for chassis management.
     * @return
     */
    private boolean isPSPInstalled()
    {
    	String sdkLocation = CwProjectPropertyPage.getSDKLocation(_project);
    	if (sdkLocation != null && sdkLocation.length() > 0)
    	{
        	File pspDir = new File(sdkLocation
        			+ File.separator + "src"
        			+ File.separator + "ASP"
        			+ File.separator + "components"
        			+ File.separator + "cm"
        			+ File.separator + "server");
        	if (pspDir.exists()) return true;
    	}
    	return false;
    }
    
    /**
     * Routine to see if the given string is a valid location for pre-build ASP
     *  libraries. This is a very rudimentary check right now. It basically looks
     *  for a lib directory 4 levels below the given directory.
     *  i.e. <location>/target/<any-dir>/<any-dir>/lib
     *  Eventually this can be improved to use the target platform and target OS
     *  for the two intermediate directories.
     * @param location
     * @return
     */
    private boolean isValidPrebuiltASPLocation(String location)
    {
    	boolean valid = false;
    	
    	File preBuiltLoc = new File(location + File.separator + "target");
    	if (!preBuiltLoc.exists()) return false;
    	
    	File[] firstLevel = preBuiltLoc.listFiles();
    	
    	for (int i=0; i<firstLevel.length; i++)
    	{
    		if (firstLevel[i].isDirectory())
    		{
    	    	File[] secondLevel = firstLevel[i].listFiles();
    	    	for (int j=0; j<secondLevel.length; j++)
    	    	{
    	    		if (secondLevel[j].isDirectory())
    	    		{
    	    	    	File[] thirdLevel = secondLevel[j].listFiles();
    	    	    	for (int k=0; k<thirdLevel.length; k++)
    	    	    	{
    	    	    		if (thirdLevel[k].isDirectory() && thirdLevel[k].getName().equals("lib"))
    	    	    		{
    	    	    	    	valid = true;
    	    	    	    	i = firstLevel.length;
    	    	    	    	j = secondLevel.length;
    	    	    	    	k = thirdLevel.length;
    	    	    		}
    	    	    	}
    	    			
    	    		}
    	    	}
    		}
    	}
    	
    	return valid;
    }
}
