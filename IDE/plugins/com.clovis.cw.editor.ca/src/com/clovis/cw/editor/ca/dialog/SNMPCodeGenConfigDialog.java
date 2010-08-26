package com.clovis.cw.editor.ca.dialog;


import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.QualifiedName;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.jface.dialogs.IMessageProvider;
import org.eclipse.jface.dialogs.TitleAreaDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.DirectoryDialog;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

import com.clovis.common.utils.ecore.EcoreUtils;
import com.clovis.cw.data.DataPlugin;
import com.clovis.cw.editor.ca.constants.ClassEditorConstants;
import com.clovis.cw.editor.ca.snmp.ClovisMibUtils;
import com.clovis.cw.project.data.ProjectDataModel;
import com.ireasoning.protocol.snmp.MibUtil;
import com.ireasoning.util.MibParseException;
import com.ireasoning.util.MibTreeNode;


/**
 * Dialog for capturing properties for SNMP code generation 
 * @author Pushparaj
 *
 */
public class SNMPCodeGenConfigDialog extends TitleAreaDialog {
	private Text _moduleNamesText, _dirsLocationText;
	private EObject _mapObject;
	private IProject _project;
	public SNMPCodeGenConfigDialog(Shell parentShell, EObject mapObj, IProject project) {
		super(parentShell);
		_mapObject = mapObj;
		_project = project;
	}
	/**
	 * @see org.eclipse.jface.dialogs.Dialog#createContents(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createContents(Composite parent) {
		return super.createContents(parent);
	}
	/**
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	protected Control createDialogArea(Composite parent) {
		Composite composite = new Composite(parent, SWT.NONE);
		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		composite.setLayout(layout);
		composite.setLayoutData(new GridData(GridData.FILL_BOTH));
		
		String moduleName = "";
		String dirsLocation = "";
		
		moduleName = (String) EcoreUtils.getValue(_mapObject, "moduleName");
		if(moduleName == null || moduleName.equals("")) {
			moduleName = findMiboduleName();
		}
		if(EcoreUtils.getValue(_mapObject, "mibPath") != null) {
			dirsLocation = (String) EcoreUtils.getValue(_mapObject, "mibPath");
		}
		Label dirLocationLabel = new Label(composite, SWT.NONE);
		dirLocationLabel.setText("Enter MIB directory Location(s) [<mibdir1>:<mibdir2>]:");
		GridData data = new GridData();
		data.horizontalSpan = 3;
		dirLocationLabel.setLayoutData(data);
		_dirsLocationText = new Text(composite, SWT.BORDER);
		if(dirsLocation == null || dirsLocation.equals("")) {
			dirsLocation = getMibPath();
		}
		_dirsLocationText.setText(dirsLocation);
		data = new GridData();
		data.horizontalSpan = 2;
		data.widthHint = convertWidthInCharsToPixels(75);
		_dirsLocationText.setLayoutData(data);
		Button brsButton1 = new Button(composite, SWT.BORDER);
		brsButton1.setText("Browse...");
		brsButton1.addSelectionListener(new SelectionListener() {
			public void widgetDefaultSelected(SelectionEvent e) {}
			public void widgetSelected(SelectionEvent e) {
				DirectoryDialog dialog = new DirectoryDialog(getShell(),
						SWT.NONE);
				String fileName = dialog.open();
				if(fileName != null) {
					String loc = _dirsLocationText.getText().trim();
					if(!loc.equals("")) {
						loc = loc + ":";
					}
					_dirsLocationText.setText(loc + fileName);
				}
			}
		});
		
		Label moduleLabel = new Label(composite, SWT.NONE);
		moduleLabel.setText("Enter MIB Module Name:");
		data = new GridData();
		data.horizontalSpan = 3;
		moduleLabel.setLayoutData(data);
		_moduleNamesText = new Text(composite, SWT.BORDER);
        if(moduleName != null)
			_moduleNamesText.setText(moduleName);
		data = new GridData();
		data.horizontalSpan = 3;
		data.widthHint = convertWidthInCharsToPixels(75);
		_moduleNamesText.setLayoutData(data);
		setTitle("SNMP sub-agent Properties");
		setMessage("Enter values for SNMP sub-agent code generation", IMessageProvider.INFORMATION);
		return composite;
	}
	/**
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	protected void configureShell(Shell shell)
	{
		shell.setText("SNMP sub-agent Properties");
		super.configureShell(shell);
	}
	/**
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	protected void okPressed() {
		String dirsLocation = _dirsLocationText.getText().trim();
		String moduleName = _moduleNamesText.getText().trim();
		EcoreUtils.setValue(_mapObject, "moduleName", moduleName);
		EcoreUtils.setValue(_mapObject, "mibPath", dirsLocation);
		super.okPressed();
	}
	/**
	 * Finds and returns the moduleName(s)
	 * @return
	 */
	private String findMiboduleName() {
		StringBuffer buffer = new StringBuffer("");
		ProjectDataModel pdm = ProjectDataModel.getProjectDataModel(_project);
		List resObjects = pdm.getCAModel().getEList();
		EObject rootObject = (EObject)resObjects.get(0);
		EReference ref = (EReference) rootObject.eClass()
		.getEStructuralFeature(ClassEditorConstants.MIB_RESOURCE_REF_NAME);
		EList list = (EList) rootObject.eGet(ref);
		List<String> mibNames = new ArrayList<String>();
		for (int i = 0; i < list.size(); i++) {
			String mibName = (String)EcoreUtils.getValue((EObject)list.get(i), "mibName");
			if(!mibNames.contains(mibName)) {
				mibNames.add(mibName);
			}
		}
		List loadedMibs = pdm.getLoadedMibs();
		if (loadedMibs.size() > 0) {
			MibUtil.unloadAllMibs();
			ClovisMibUtils.loadSystemMibs(_project);
			MibUtil.setResolveSyntax(true);
		    for (int i = 0; i < loadedMibs.size(); i++) {
				String mibFile = (String) loadedMibs.get(i);
				try {
					if(mibNames.contains(new File(mibFile).getName())) {
						MibTreeNode node = MibUtil.parseMib(mibFile,false);
						String moduleName = node.getModuleIdentity();
						if(buffer.length() > 0) {
							buffer.append("," + moduleName);
						} else {
							buffer.append(moduleName);
						}
					}
				} catch (MibParseException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
		return buffer.toString();
	}
	/**
	 * Returns standard and imported Mib Paths.
	 * @return
	 */
	private String getMibPath() {
		String mibPath = "";
		String installedLoc = DataPlugin.getInstalledLocation();
		try {
			String sdkLoc = _project.getPersistentProperty(new QualifiedName(
					"", "SDK_LOCATION"));
			if (sdkLoc != null && !sdkLoc.equals("")) {
				String installLoc = new File(sdkLoc).getParentFile()
						.getAbsolutePath();
				mibPath = installLoc + File.separator + "buildtools"
						+ File.separator + "local" + File.separator + "share"
						+ File.separator + "snmp" + File.separator + "mibs";
			}

			if (!new File(mibPath).exists()) {
				mibPath = "";
				if (installedLoc != null && new File(installedLoc).exists()) {
					mibPath = installedLoc + File.separator + "buildtools"
							+ File.separator + "local" + File.separator
							+ "share" + File.separator + "snmp"
							+ File.separator + "mibs";
					if (new File(mibPath).exists()) {
						return mibPath;
					} else {
						return "";
					}
				}
			}
			
			//Finds the satandard mib paths if net-snmp is installed by the user
			String command = "net-snmp-config";
			List<String> commandList = new ArrayList<String>();
			commandList.add(command);
			commandList.add("--mibdirs");
			ProcessBuilder processBuilder = new ProcessBuilder(commandList);
			Process process = processBuilder.start();
			InputStream is = process.getInputStream();
			InputStreamReader isr = new InputStreamReader(is);
			BufferedReader br = new BufferedReader(isr);
			String systemPaths = br.readLine();
			if (systemPaths != null && !systemPaths.contains("command not found")) {
				StringTokenizer tokenizer = new StringTokenizer(systemPaths, ":");
				while (tokenizer.hasMoreTokens()) {
					String path = tokenizer.nextToken();
					if (new File(path).isDirectory() && !mibPath.contains(path)) {
						if (!mibPath.equals("")) {
							mibPath += ":";
						}
						mibPath += path;
					}
				}
			}
			
			//Finds the imported mib paths
			ProjectDataModel pdm = ProjectDataModel
					.getProjectDataModel(_project);
			List loadedMibs = pdm.getLoadedMibs();
			for (int i = 0; i < loadedMibs.size(); i++) {
				String mibFile = (String) loadedMibs.get(i);
				String path = new File(mibFile).getParent();
				if (!mibPath.contains(path)) {
					mibPath += ":" + path;
				}
			}
		} catch (CoreException e) {
			return mibPath;
		} catch (IOException e) {
			return mibPath;
		}
		return mibPath;
	}
}
