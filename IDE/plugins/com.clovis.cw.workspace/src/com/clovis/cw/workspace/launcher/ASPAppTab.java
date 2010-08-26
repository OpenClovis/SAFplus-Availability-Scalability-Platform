/**
 * 
 */
package com.clovis.cw.workspace.launcher;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.util.HashMap;
import java.util.Map;

import org.eclipse.cdt.debug.core.ICDTLaunchConfigurationConstants;
import org.eclipse.cdt.launch.ui.CLaunchConfigurationTab;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.debug.core.ILaunchConfiguration;
import org.eclipse.debug.core.ILaunchConfigurationWorkingCopy;
import org.eclipse.debug.core.ILaunchManager;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;

/**
 * Launch configuration tab for ASP app.
 * 
 * @author Suraj Rajyaguru
 * 
 */
public class ASPAppTab extends CLaunchConfigurationTab {

	private Text _aspDir;

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.debug.ui.ILaunchConfigurationTab#createControl(org.eclipse.swt.widgets.Composite)
	 */
	public void createControl(Composite parent) {
		Composite control = new Composite(parent, SWT.NONE);
		control.setLayout(new GridLayout(3, false));

		Label aspLocationLabel = new Label(control, SWT.NONE);
		aspLocationLabel.setText("ASP Directory");

		_aspDir = new Text(control, SWT.BORDER);
		_aspDir.setLayoutData(new GridData(SWT.FILL, 0, true, false));
		_aspDir.setEditable(false);

		Button button = new Button(control, SWT.PUSH);
		button.setText("Browse...");
		button.addSelectionListener(new SelectionAdapter() {
			/*
			 * (non-Javadoc)
			 * 
			 * @see org.eclipse.swt.events.SelectionAdapter#widgetSelected(org.eclipse.swt.events.SelectionEvent)
			 */
			@Override
			public void widgetSelected(SelectionEvent e) {
				DirectoryDialog dialog = new DirectoryDialog(getShell());
				String dirPath = dialog.open();

				if (dirPath != null) {
					_aspDir.setText(dirPath);
					getLaunchConfigurationDialog().updateButtons();
				}
			}
		});

		setControl(control);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.debug.ui.ILaunchConfigurationTab#getName()
	 */
	public String getName() {
		return "ASP Configuration";
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.debug.ui.ILaunchConfigurationTab#initializeFrom(org.eclipse.debug.core.ILaunchConfiguration)
	 */
	public void initializeFrom(ILaunchConfiguration configuration) {
		try {
			_aspDir.setText(configuration.getAttribute("ASP_DIR", ""));
		} catch (CoreException e) {
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.debug.ui.ILaunchConfigurationTab#performApply(org.eclipse.debug.core.ILaunchConfigurationWorkingCopy)
	 */
	public void performApply(ILaunchConfigurationWorkingCopy configuration) {
		try {
			HashMap<String, String> envMap = getEnvMap();
			if (envMap.size() == 0) {
				return;
			}

			String ASP_DIR = _aspDir.getText();
			envMap.put("ASP_COMPNAME", configuration.getAttribute(
					ICDTLaunchConfigurationConstants.ATTR_PROJECT_NAME, "")
					+ "_" + envMap.get("ASP_NODENAME"));
			envMap.put("LD_LIBRARY_PATH", ASP_DIR + File.separator + "lib"
					+ File.pathSeparator + ASP_DIR + File.separator + "lib"
					+ File.separator + "openhpi");
			envMap.put("PYTHON_PATH", ASP_DIR + File.separator + "lib");

			Map map = configuration.getAttribute(
					ILaunchManager.ATTR_ENVIRONMENT_VARIABLES, (Map) null);
			if (map != null) {
				map.putAll(envMap);
			} else {
				map = envMap;
			}

			if (map.get("ASP_WITHOUT_CPM") == null) {
				map.put("ASP_WITHOUT_CPM", "1");
			}

			configuration.setAttribute(
					ILaunchManager.ATTR_ENVIRONMENT_VARIABLES, map);
			configuration.setAttribute("ASP_DIR", ASP_DIR);

		} catch (CoreException e) {
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.debug.ui.ILaunchConfigurationTab#setDefaults(org.eclipse.debug.core.ILaunchConfigurationWorkingCopy)
	 */
	public void setDefaults(ILaunchConfigurationWorkingCopy configuration) {
	}

	/**
	 * Returns environment variable map.
	 * 
	 * @return
	 */
	private HashMap<String, String> getEnvMap() {
		HashMap<String, String> envMap = new HashMap<String, String>();

		if (!_aspDir.getText().equals("")) {
			String envFilePath = _aspDir.getText() + File.separator + "etc"
					+ File.separator + "asp_run.env";
			File envFile = new File(envFilePath);

			if (envFile.exists()) {
				try {
					BufferedReader reader = new BufferedReader(new FileReader(
							envFile));
					String line;
					String[] data;

					while ((line = reader.readLine()) != null) {
						data = line.split("=");
						if (data.length == 2) {
							envMap.put(data[0], data[1]);
						}
					}
				} catch (FileNotFoundException e) {
				} catch (Exception e) {
				}

			} else {
				MessageDialog.openInformation(getShell(), "ASP directory validation",
						"ASP is not running under '" + _aspDir.getText() + "'.");
			}
		}

		return envMap;
	}
}
